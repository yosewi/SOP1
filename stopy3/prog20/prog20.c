#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAXLINE 4096
#define DEFAULT_STUDENT_COUNT 100
/* Makro ELAPSED: Oblicza różnicę czasu w sekundach między dwoma strukturami timespec. */
#define ELAPSED(start, end) ((end).tv_sec - (start).tv_sec) + (((end).tv_nsec - (start).tv_nsec) * 1.0e-9)
/* Makro ERR: Wypisuje błąd i kończy program. */
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef unsigned int UINT;
typedef struct timespec timespec_t;

/* Struktura przechowująca stan wszystkich studentów */
typedef struct studentList
{
    bool *removed;          // Tablica flag: true jeśli student został usunięty (kicked)
    pthread_t *thStudents;  // Tablica identyfikatorów wątków
    int count;              // Całkowita liczba studentów
    int present;            // Liczba obecnie aktywnych studentów
} studentsList_t;

/* Struktura liczników studentów na poszczególnych latach */
typedef struct yearCounters
{
    int values[4];               // Liczba studentów na roku 1, 2, 3 oraz inżynierów (indeks 3)
    pthread_mutex_t mxCounters[4]; // Osobny mutex dla każdego licznika (dla wydajności)
} yearCounters_t;

/* Argumenty przekazywane do funkcji pomocniczych (inkrementacja/dekrementacja) */
typedef struct argsModify
{
    yearCounters_t *pYearCounters; // Wskaźnik na strukturę liczników
    int year;                      // Indeks roku, którego dotyczy operacja
} argsModify_t;

void ReadArguments(int argc, char **argv, int *studentsCount);
void *student_life(void *);
void increment_counter(argsModify_t *args);
void decrement_counter(void *_args);
void msleep(UINT milisec);
void kick_student(studentsList_t *studentsList);

/**
 * Funkcja main:
 * Co robi: Zarządza cyklem życia symulacji (tworzenie wątków, pętla "dziekana", sprzątanie).
 * W jaki sposób:
 * 1. Inicjalizuje struktury danych (liczniki, mutexy, listy studentów).
 * 2. Tworzy wątki studentów funkcją pthread_create.
 * 3. W pętli głównej (trwającej 4 sekundy):
 * - Czeka losowy czas (100-300ms).
 * - Usuwa losowego studenta funkcją kick_student.
 * 4. Czeka na zakończenie wszystkich wątków (pthread_join).
 * 5. Wypisuje statystyki i zwalnia pamięć.
 */
int main(int argc, char **argv)
{
    int studentsCount;
    ReadArguments(argc, argv, &studentsCount);
    yearCounters_t counters = {.values = {0, 0, 0, 0},
                               .mxCounters = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
                                              PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER}};
    studentsList_t studentsList;
    studentsList.count = studentsCount;
    studentsList.present = studentsCount;
    studentsList.thStudents = (pthread_t *)malloc(sizeof(pthread_t) * studentsCount);
    studentsList.removed = (bool *)malloc(sizeof(bool) * studentsCount);
    if (studentsList.thStudents == NULL || studentsList.removed == NULL)
        ERR("Failed to allocate memory for 'students list'!");
    for (int i = 0; i < studentsCount; i++)
        studentsList.removed[i] = false;
    for (int i = 0; i < studentsCount; i++)
        if (pthread_create(&studentsList.thStudents[i], NULL, student_life, &counters))
            ERR("Failed to create student thread!");
    srand(time(NULL));
    timespec_t start, current;
    if (clock_gettime(CLOCK_REALTIME, &start))
        ERR("Failed to retrieve time!");
    do
    {
        msleep(rand() % 201 + 100);
        if (clock_gettime(CLOCK_REALTIME, &current))
            ERR("Failed to retrieve time!");
        kick_student(&studentsList);
    } while (ELAPSED(start, current) < 4.0);
    for (int i = 0; i < studentsCount; i++)
        if (pthread_join(studentsList.thStudents[i], NULL))
            ERR("Failed to join with a student thread!");
    printf(" First year: %d\n", counters.values[0]);
    printf("Second year: %d\n", counters.values[1]);
    printf(" Third year: %d\n", counters.values[2]);
    printf("  Engineers: %d\n", counters.values[3]);
    free(studentsList.removed);
    free(studentsList.thStudents);
    exit(EXIT_SUCCESS);
}

/**
 * Funkcja ReadArguments:
 * Co robi: Parsuje argumenty wiersza poleceń.
 * W jaki sposób:
 * 1. Ustawia wartość domyślną.
 * 2. Sprawdza, czy podano argument (argc >= 2).
 * 3. Konwertuje argument na int i waliduje (musi być > 0).
 */
void ReadArguments(int argc, char **argv, int *studentsCount)
{
    *studentsCount = DEFAULT_STUDENT_COUNT;
    if (argc >= 2)
    {
        *studentsCount = atoi(argv[1]);
        if (*studentsCount <= 0)
        {
            printf("Invalid value for 'studentsCount'");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * Funkcja student_life:
 * Co robi: Symuluje życie studenta (przechodzenie przez kolejne lata studiów).
 * W jaki sposób:
 * 1. Ustawia typ anulowania na odroczony (DEFERRED).
 * 2. W pętli (dla lat 0, 1, 2):
 * - Inkrementuje licznik danego roku (wszedł na rok).
 * - Rejestruje procedurę czyszczącą `decrement_counter` (na stosie cleanup).
 * - Śpi 1 sekundę (studiuje).
 * - Zdejmuje procedurę ze stosu z flagą execute=1. To powoduje wywołanie `decrement_counter`
 * (student kończy rok, więc znika z licznika tego roku).
 * 3. Jeśli przetrwał 3 lata, inkrementuje licznik inżynierów (ostatnie wywołanie increment_counter).
 */
void *student_life(void *voidArgs)
{
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    argsModify_t args;
    args.pYearCounters = voidArgs;
    for (args.year = 0; args.year < 3; args.year++)
    {
        increment_counter(&args);
        // Push: Jeśli wątek zostanie anulowany podczas msleep, 
        // automatycznie wykona się decrement_counter, naprawiając licznik.
        pthread_cleanup_push(decrement_counter, &args);
        msleep(1000); // Punkt anulowania
        // Pop(1): Zdejmuje handler i GO WYKONUJE. 
        // Oznacza to: koniec roku, wypisz się z tego roku (zmniejsz licznik), idź do następnego.
        pthread_cleanup_pop(1); 
    }
    increment_counter(&args); // Został inżynierem
    return NULL;
}

/**
 * Funkcja increment_counter:
 * Co robi: Zwiększa licznik studentów na danym roku o 1.
 * W jaki sposób:
 * 1. Blokuje mutex przypisany do konkretnego roku (`args->year`).
 * 2. Inkrementuje wartość w tablicy.
 * 3. Odblokowuje mutex.
 */
void increment_counter(argsModify_t *args)
{
    pthread_mutex_lock(&(args->pYearCounters->mxCounters[args->year]));
    args->pYearCounters->values[args->year] += 1;
    pthread_mutex_unlock(&(args->pYearCounters->mxCounters[args->year]));
}

/**
 * Funkcja decrement_counter:
 * Co robi: Zmniejsza licznik studentów na danym roku o 1. Służy też jako handler czyszczący.
 * W jaki sposób:
 * 1. Rzutuje void* na właściwy typ argumentów.
 * 2. Blokuje mutex odpowiedniego roku.
 * 3. Dekrementuje wartość.
 * 4. Odblokowuje mutex.
 */
void decrement_counter(void *_args)
{
    argsModify_t *args = _args;
    pthread_mutex_lock(&(args->pYearCounters->mxCounters[args->year]));
    args->pYearCounters->values[args->year] -= 1;
    pthread_mutex_unlock(&(args->pYearCounters->mxCounters[args->year]));
}

/**
 * Funkcja msleep:
 * Co robi: Usypia wątek na zadaną liczbę milisekund.
 * W jaki sposób:
 * 1. Przelicza milisekundy na sekundy i nanosekundy.
 * 2. Używa `nanosleep`, który jest punktem anulowania (cancellation point).
 */
void msleep(UINT milisec)
{
    time_t sec = (int)(milisec / 1000);
    milisec = milisec - (sec * 1000);
    timespec_t req = {0};
    req.tv_sec = sec;
    req.tv_nsec = milisec * 1000000L;
    if (nanosleep(&req, &req))
        ERR("nanosleep");
}

/**
 * Funkcja kick_student:
 * Co robi: Usuwa (anuluje) losowego, jeszcze aktywnego studenta.
 * W jaki sposób:
 * 1. Jeśli nie ma już kogo wyrzucać (`present == 0`), kończy.
 * 2. Losuje indeks tak długo, aż trafi na studenta, który nie ma flagi `removed`.
 * 3. Wysyła sygnał anulowania do wątku (`pthread_cancel`).
 * 4. Ustawia flagę `removed` na true i zmniejsza licznik obecnych.
 */
void kick_student(studentsList_t *studentsList)
{
    int idx;
    if (0 == studentsList->present)
        return;
    do
    {
        idx = rand() % studentsList->count;
    } while (studentsList->removed[idx] == true);
    pthread_cancel(studentsList->thStudents[idx]);
    studentsList->removed[idx] = true;
    studentsList->present--;
}