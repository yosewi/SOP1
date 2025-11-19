all: prog14
prog14: prog14.c
	gcc -Wall -fsanitize=address,undefined -o prog14 prog14.c
.PHONY: clean all
clean:
	rm prog14