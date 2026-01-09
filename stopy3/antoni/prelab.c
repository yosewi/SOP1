/**
 * Makro _GNU_SOURCE:
 * Co robi: Włącza rozszerzenia specyficzne dla systemu GNU/Linux.
 * W jaki sposób: Musi być zdefiniowane przed dołączeniem jakichkolwiek nagłówków.
 * Tutaj jest potrzebne głównie dla makra TEMP_FAILURE_RETRY oraz funkcji nanostleep.
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CIRC_BUF_SIZE 1024
#define DEFAULT_N_THREADS 3
#define ALPHABET_SIZE 128

/* Makro do obsługi błędów krytycznych: wypisuje błąd, zabija proces grupą sygnałów i wychodzi. */
#define ERR(source)                                                            \
  (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),             \
   kill(0, SIGKILL), exit(EXIT_FAILURE))
   
/* Makro do generowania losowej liczby double w zakresie [0, 1] w sposób bezpieczny wątkowo. */
#define NEXT_DOUBLE(seedptr) ((double)rand_r(seedptr) / (double)RAND_MAX)

/* Struktura bufora cyklicznego przechowującego ścieżki do plików */
typedef struct circular_buffer {
  char *buf[CIRC_BUF_SIZE]; // Tablica wskaźników na napisy (ścieżki)
  int head;                 // Indeks zapisu (gdzie wstawić nowy element)
  int tail;                 // Indeks odczytu (skąd pobrać element)
  int len;                  // Aktualna liczba elementów w buforze
  pthread_mutex_t mx;       // Mutex do synchronizacji dostępu do bufora
} circ_buf;

/* Struktura argumentów przekazywanych do wątków */
typedef struct thread_args {
  pthread_t tid;            // ID wątku
  unsigned int seed;        // Ziarno dla generatora liczb losowych (unikalne dla wątku)
  circ_buf *cb;             // Wskaźnik na wspólny bufor cykliczny
  int *freq;                // Tablica częstości znaków (wspólna)
  pthread_mutex_t *mx_freq; // Mutex chroniący tablicę częstości
  sigset_t *mask;           // Maska sygnałów (które sygnały ignorować/blokować)
  int *quit;                // Flaga nakazująca zakończenie pracy
  pthread_mutex_t *mx_quit; // Mutex chroniący flagę quit
} thread_args_t;

/**
 * Funkcja msleep:
 * Co robi: Usypia wątek na zadaną liczbę milisekund.
 * W jaki sposób:
 * 1. Przelicza milisekundy na sekundy i nanosekundy (format wymagany przez nanosleep).
 * 2. Wywołuje nanosleep w pętli makra TEMP_FAILURE_RETRY, aby wznowić czekanie,
 * jeśli funkcja zostanie przerwana przez sygnał.
 */
void msleep(unsigned msec) {
  time_t sec = (int)(msec / 1000);
  msec = msec - (sec * 1000);
  struct timespec req = {0};
  req.tv_sec = sec;
  req.tv_nsec = msec * 1000000L;
  if (TEMP_FAILURE_RETRY(nanosleep(&req, &req)))
    ERR("nanosleep");
}

/**
 * Funkcja circ_buf_create:
 * Co robi: Tworzy i inicjalizuje strukturę bufora cyklicznego.
 * W jaki sposób:
 * 1. Alokuje pamięć na strukturę `circ_buf`.
 * 2. Inicjalizuje wskaźniki w tablicy `buf` na NULL.
 * 3. Ustawia liczniki (head, tail, len) na 0.
 * 4. Inicjalizuje mutex chroniący bufor.
 * Zwraca wskaźnik na utworzony bufor.
 */
circ_buf *circ_buf_create() {
  circ_buf *cb = malloc(sizeof(circ_buf));
  if (cb == NULL)
    ERR("malloc");
  for (int i = 0; i < CIRC_BUF_SIZE; i++) {
    cb->buf[i] = NULL;
  }
  cb->head = 0;
  cb->tail = 0;
  cb->len = 0;
  if (pthread_mutex_init(&cb->mx, NULL)) {
    free(cb);
    ERR("pthread_mutex_init");
  }

  return cb;
}

/**
 * Funkcja circ_buf_destroy:
 * Co robi: Usuwa bufor cykliczny i zwalnia całą powiązaną pamięć.
 * W jaki sposób:
 * 1. Niszczy mutex.
 * 2. Przechodzi przez tablicę i zwalnia każdy zaalokowany string (ścieżkę), który mógł zostać w buforze.
 * 3. Zwalnia pamięć samej struktury bufora.
 */
void circ_buf_destroy(circ_buf *cb) {
  if (cb == NULL)
    return;
  pthread_mutex_destroy(&cb->mx);
  for (int i = 0; i < CIRC_BUF_SIZE; i++) {
    free(cb->buf[i]);
  }
  free(cb);
}

/**
 * Funkcja circ_buf_push:
 * Co robi: Dodaje nową ścieżkę do bufora (producent).
 * W jaki sposób:
 * 1. Sprawdza, czy bufor jest pełny (len == CIRC_BUF_SIZE). Jeśli tak, czeka aktywnie (sleep),
 * aż zwolni się miejsce.
 * 2. Alokuje pamięć na kopię łańcucha `path` (strdup ręcznie).
 * 3. Blokuje mutex bufora.
 * 4. Zwalnia stary element pod indeksem `head` (dla bezpieczeństwa) i wpisuje nowy.
 * 5. Inkrementuje `head` (modulo rozmiar bufora) i `len`.
 * 6. Odblokowuje mutex.
 */
void circ_buf_push(circ_buf *cb, char *path) {
  if (cb == NULL)
    return;
  while (cb->len == CIRC_BUF_SIZE) {
    msleep(5);
  }
  char *s = malloc((strlen(path) + 1) * sizeof(char));
  if (s == NULL)
    ERR("malloc");
  strcpy(s, path);
  pthread_mutex_lock(&cb->mx);
  free(cb->buf[cb->head]);
  cb->buf[cb->head] = s;
  cb->len++;
  cb->head++;
  if (cb->head == CIRC_BUF_SIZE)
    cb->head = 0;
  pthread_mutex_unlock(&cb->mx);
}

/**
 * Funkcja circ_buf_pop:
 * Co robi: Pobiera ścieżkę z bufora (konsument).
 * W jaki sposób:
 * 1. Wchodzi w nieskończoną pętlę sprawdzającą dostępność danych.
 * 2. Blokuje mutex i sprawdza `len > 0`.
 * 3. Jeśli są dane: pobiera wskaźnik spod indeksu `tail`, aktualizuje `tail` i `len`,
 * odblokowuje mutex i zwraca wskaźnik.
 * 4. Jeśli brak danych: odblokowuje mutex i usypia wątek na 5ms (aktywne oczekiwanie/polling),
 * po czym ponawia próbę.
 */
char *circ_buf_pop(circ_buf *cb) {
  if (cb == NULL)
    ERR("circ_buf is NULL");
  for (;;) {
    pthread_mutex_lock(&cb->mx);
    if (cb->len > 0) {
      char *s = cb->buf[cb->tail];
      cb->len--;
      cb->tail++;
      if (cb->tail == CIRC_BUF_SIZE)
        cb->tail = 0;
      pthread_mutex_unlock(&cb->mx);
      return s;
    }
    pthread_mutex_unlock(&cb->mx);
    msleep(5);
  }
}

/**
 * Funkcja read_args:
 * Co robi: Parsuje argumenty wiersza poleceń.
 * W jaki sposób:
 * 1. Ustawia domyślną liczbę wątków.
 * 2. Jeśli podano argument (argc >= 2), konwertuje go na int (atoi).
 * 3. Waliduje, czy liczba wątków jest dodatnia; jeśli nie, kończy program.
 */
void read_args(int argc, char *argv[], int *n_threads) {
  *n_threads = DEFAULT_N_THREADS;
  if (argc >= 2) {
    *n_threads = atoi(argv[1]);
    if (*n_threads <= 0) {
      printf("n_threads must be positive\n");
      exit(EXIT_FAILURE);
    }
  }
}

/**
 * Funkcja has_ext:
 * Co robi: Sprawdza, czy plik ma określone rozszerzenie.
 * W jaki sposób:
 * 1. Znajduje ostatnie wystąpienie kropki w ścieżce (`strrchr`).
 * 2. Porównuje tekst po kropce z oczekiwanym rozszerzeniem (`strcmp`).
 * Zwraca 1 (prawda) lub 0 (fałsz).
 */
int has_ext(char *path, char *ext) {
  char *ext_pos = strrchr(path, '.');
  return ext_pos && strcmp(ext_pos, ext) == 0;
}

/**
 * Funkcja walk_dir:
 * Co robi: Rekurencyjnie przeszukuje katalog i dodaje pliki .txt do bufora.
 * W jaki sposób:
 * 1. Otwiera katalog (`opendir`).
 * 2. Iteruje po zawartości (`readdir`). Pomija "." i "..".
 * 3. Tworzy pełną ścieżkę do pliku (dokleja nazwę do bieżącej ścieżki).
 * 4. Pobiera metadane pliku (`stat`).
 * 5. Jeśli to katalog: wywołuje samą siebie rekurencyjnie (`walk_dir`).
 * 6. Jeśli to plik regularny (`S_ISREG`) i ma rozszerzenie .txt: wrzuca ścieżkę do bufora (`circ_buf_push`).
 * 7. Zamyka katalog.
 */
void walk_dir(char *path, circ_buf *cb) {
  DIR *dir = opendir(path);
  if (dir == NULL)
    ERR("opendir");
  struct dirent *entry;
  struct stat statbuf;
  while ((entry = readdir(dir)) != NULL) {
    char *entry_name = entry->d_name;
    if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0)
      continue;
    int path_len = strlen(path);
    int new_path_len = path_len + strlen(entry_name) + 2;
    char *new_path = malloc(new_path_len * sizeof(char));
    if (new_path == NULL)
      ERR("malloc");
    strcpy(new_path, path);
    new_path[path_len] = '/';
    strcpy(new_path + path_len + 1, entry_name);

    if (stat(new_path, &statbuf) < 0)
      ERR("stat");
    if (S_ISDIR(statbuf.st_mode)) {
      walk_dir(new_path, cb);
    } else if (S_ISREG(statbuf.st_mode)) {
      if (has_ext(entry_name, ".txt"))
        circ_buf_push(cb, new_path);
    }
    free(new_path);
  }
  if (closedir(dir))
    ERR("closedir");
}

/**
 * Funkcja bulk_read:
 * Co robi: Czyta z deskryptora pliku zadaną liczbę bajtów, odporna na przerwania.
 * W jaki sposób:
 * 1. Wywołuje `read` w pętli.
 * 2. Używa `TEMP_FAILURE_RETRY` by obsłużyć przerwanie sygnałem (EINTR).
 * 3. Sumuje odczytane bajty i przesuwa wskaźnik bufora, aż przeczyta `count` bajtów lub napotka EOF (0).
 */
ssize_t bulk_read(int fd, char *buf, size_t count) {
  ssize_t c;
  ssize_t len = 0;
  do {
    c = TEMP_FAILURE_RETRY(read(fd, buf, count));
    if (c < 0)
      return c;
    if (c == 0)
      return len; // EOF
    buf += c;
    len += c;
    count -= c;
  } while (count > 0);
  return len;
}

/**
 * Funkcja thread_work:
 * Co robi: Główna funkcja wykonywana przez wątki robocze (konsumentów).
 * W jaki sposób:
 * 1. Blokuje sygnały (maska przekazana w args), aby nie przerywały pracy.
 * 2. W pętli pobiera ścieżki z bufora (`circ_buf_pop`).
 * 3. Otwiera plik i czyta go znak po znaku (`bulk_read` po 1 bajcie).
 * 4. W pętli czytania:
 * - Sprawdza flagę `quit` (chronioną mutexem). Jeśli ustawiona, zwalnia zasoby i kończy.
 * - Zlicza wystąpienie znaku, aktualizując globalną tablicę `freq` (chronioną mutexem).
 * - Dodaje sztuczne opóźnienie (`msleep(2)`), aby symulować długą pracę.
 * 5. Zamyka plik po przeczytaniu.
 */
void *thread_work(void *_args) {
  thread_args_t *args = _args;
  sigset_t *mask = args->mask;
  if (pthread_sigmask(SIG_BLOCK, mask, NULL))
    ERR("pthread_sigmask");
  circ_buf *cb = args->cb;
  unsigned int seed = args->seed;
  int *freq = args->freq;
  pthread_mutex_t *mx_freq = args->mx_freq;
  int *quit = args->quit;
  pthread_mutex_t *mx_quit = args->mx_quit;

  char *c = malloc(sizeof(char));
  if (c == NULL)
    ERR("malloc");
  while (cb->len > 0) {
    char *path = circ_buf_pop(cb);
    printf("thread %u, processing %s\n", seed, path);
    int fd = TEMP_FAILURE_RETRY(open(path, O_RDONLY));
    if (fd < 0)
      ERR("open");
    while (bulk_read(fd, c, 1) > 0) {
      msleep(2); // so that the files aren't read "instanly"
      pthread_mutex_lock(mx_quit);
      if (*quit == 1) {
        pthread_mutex_unlock(mx_quit);
        free(c);
        if (TEMP_FAILURE_RETRY(close(fd)) < 0)
          ERR("close");
        printf("thread %u ending\n", seed);
        return NULL;
      }
      pthread_mutex_unlock(mx_quit);

      // printf("thread %u, read %c\n", seed, *c);
      pthread_mutex_lock(mx_freq);
      freq[(int)*c]++;
      pthread_mutex_unlock(mx_freq);
    }
    if (TEMP_FAILURE_RETRY(close(fd)) < 0)
      ERR("close");
  }
  free(c);
  printf("thread %u ending\n", seed);
  return NULL;
}

/**
 * Funkcja signal_handling:
 * Co robi: Dedykowany wątek do obsługi sygnałów.
 * W jaki sposób:
 * 1. Czeka na sygnały z maski (`sigwait`), które są zablokowane w innych wątkach.
 * 2. SIGUSR1: Wypisuje aktualne statystyki (tablicę `freq`), blokując mutex na czas odczytu.
 * 3. SIGINT (Ctrl+C): Ustawia flagę `quit` na 1 (chronioną mutexem) i kończy swoje działanie,
 * co spowoduje zakończenie pętli w wątkach roboczych.
 */
void *signal_handling(void *_args) {
  thread_args_t *args = _args;
  int *freq = args->freq;
  pthread_mutex_t *mx_freq = args->mx_freq;
  sigset_t *mask = args->mask;
  int *quit = args->quit;
  pthread_mutex_t *mx_quit = args->mx_quit;

  int signo;
  for (;;) {
    if (sigwait(mask, &signo))
      ERR("sigwait");
    if (signo == SIGUSR1) {
      printf("freqs: ");
      pthread_mutex_lock(mx_freq);
      for (char c = 'a'; c <= 'z'; c++) {
        int f = freq[(int)c];
        if (f > 0)
          printf("%c: %d, ", c, f);
      }
      pthread_mutex_unlock(mx_freq);
      printf("\n");
    } else if (signo == SIGINT) {
      pthread_mutex_lock(mx_quit);
      *quit = 1;
      pthread_mutex_unlock(mx_quit);
      return NULL;
    }
  }
  return NULL;
}

/**
 * Funkcja main:
 * Co robi: Główny punkt wejścia, inicjalizacja i koordynacja.
 * W jaki sposób:
 * 1. Inicjalizuje struktury (argumenty wątków, bufor, mutexy).
 * 2. Wypełnia bufor plikami (`walk_dir`) - uwaga: tutaj dzieje się to przed startem wątków.
 * 3. Blokuje sygnały SIGUSR1 i SIGINT w wątku głównym (dziedziczone przez potomne).
 * 4. Uruchamia wątek obsługi sygnałów oraz wątki robocze.
 * 5. Wchodzi w pętlę, w której co 100ms wysyła do procesu sygnał SIGUSR1 (by wypisać statystyki)
 * i sprawdza flagę `quit`.
 * 6. Po wyjściu z pętli (gdy quit=1), czeka na zakończenie wątków (`pthread_join`) i zwalnia zasoby.
 */
int main(int argc, char *argv[]) {
  int n_threads;
  read_args(argc, argv, &n_threads);
  // +1 for the additional signal-handling thread
  thread_args_t *thread_args = malloc((n_threads + 1) * sizeof(thread_args_t));
  if (thread_args == NULL)
    ERR("malloc");

  circ_buf *cb = circ_buf_create();
  walk_dir("data1", cb);
  srand(time(NULL));
  int *freq = malloc(ALPHABET_SIZE * sizeof(int));
  if (freq == NULL)
    ERR("malloc");
  memset(freq, 0, ALPHABET_SIZE * sizeof(int));
  pthread_mutex_t mx_freq = PTHREAD_MUTEX_INITIALIZER;
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigaddset(&mask, SIGINT);
  if (pthread_sigmask(SIG_BLOCK, &mask, NULL))
    ERR("pthread_sigmask");
  int quit = 0;
  pthread_mutex_t mx_quit = PTHREAD_MUTEX_INITIALIZER;
  for (int i = 0; i < n_threads + 1; i++) {
    thread_args[i].cb = cb;
    thread_args[i].seed = rand();
    thread_args[i].freq = freq;
    thread_args[i].mx_freq = &mx_freq;
    thread_args[i].mask = &mask;
    thread_args[i].quit = &quit;
    thread_args[i].mx_quit = &mx_quit;
  }
  if (pthread_create(&(thread_args[n_threads].tid), NULL, signal_handling,
                     &thread_args[n_threads]))
    ERR("pthread_create");
  for (int i = 0; i < n_threads; i++) {
    if (pthread_create(&(thread_args[i].tid), NULL, thread_work,
                       &thread_args[i]))
      ERR("pthread_create");
  }
  for (;;) {
    msleep(100);
    pthread_mutex_lock(&mx_quit);
    if (quit == 1) {
      pthread_mutex_unlock(&mx_quit);
      break;
    }
    pthread_mutex_unlock(&mx_quit);
    kill(0, SIGUSR1);
  }
  for (int i = 0; i < n_threads + 1; i++) {
    if (pthread_join(thread_args[i].tid, NULL))
      ERR("pthread_join");
  }
  free(freq);
  free(thread_args);
  circ_buf_destroy(cb);

  exit(EXIT_SUCCESS);
}