all: prog16a
prog16a: prog16a.c
	gcc -Wall -fsanitize=address,undefined -o prog16a prog16a.c
.PHONY: clean all
clean:
	rm prog16a