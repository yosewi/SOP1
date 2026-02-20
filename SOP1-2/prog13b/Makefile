all: prog13b
prog13b: prog13b.c
	gcc -Wall -fsanitize=address,undefined -o prog13b prog13b.c
.PHONY: clean all
clean:
	rm prog13b