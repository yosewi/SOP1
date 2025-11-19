all: prog13a
prog13a: prog13a.c
	gcc -Wall -fsanitize=address,undefined -o prog13a prog13a.c
.PHONY: clean all
clean:
	rm prog13a