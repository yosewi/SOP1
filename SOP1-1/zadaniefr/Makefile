all: sop-venv4 zadanie3
sop-venv4: sop-venv4.c
	gcc -Wall -fsanitize=address,undefined -o sop-venv4 sop-venv4.c
zadanie3: zadanie3.c
	gcc -Wall -fsanitize=address,undefined -o zadanie3 zadanie3.c
.PHONY: clean all
clean:
	rm sop-venv4
	rm zadanie3