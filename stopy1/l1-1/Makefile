all: l1-1
l1-1: l1-1.c
	gcc -Wall -fsanitize=address,undefined -o l1-1 l1-1.c
.PHONY: clean all
clean:
	rm l1-1