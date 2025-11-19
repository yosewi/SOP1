all: l1-3
l1-3: l1-3.c
	gcc -Wall -fsanitize=address,undefined -o l1-3 l1-3.c
.PHONY: clean all
clean:
	rm l1-3