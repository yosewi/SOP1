all: prog15
prog15: prog15.c
	gcc -Wall -fsanitize=address,undefined -o prog15 prog15.c
.PHONY: clean all
clean:
	rm prog15