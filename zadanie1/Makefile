all: etap1 etap2 etap3 etap4 etap5 etap6 zadanie1np zadanie1
etap1: etap1.c
	gcc -Wall -fsanitize=address,undefined -o etap1 etap1.c
etap2: etap2.c
	gcc -Wall -fsanitize=address,undefined -o etap2 etap2.c
etap3: etap3.c
	gcc -Wall -fsanitize=address,undefined -o etap3 etap3.c
etap4: etap4.c
	gcc -Wall -fsanitize=address,undefined -o etap4 etap4.c
etap5: etap5.c
	gcc -Wall -fsanitize=address,undefined -o etap5 etap5.c
etap6: etap6.c
	gcc -Wall -fsanitize=address,undefined -o etap6 etap6.c
zadanie1np: zadanie1np.c
	gcc -Wall -fsanitize=address,undefined -o zadanie1np zadanie1np.c
zadanie1: zadanie1.c
	gcc -Wall -fsanitize=address,undefined -o zadanie1 zadanie1.c
.PHONY: clean all
clean:
	rm etap1
	rm etap2
	rm etap3
	rm etap4
	rm etap5
	rm etap6
	rm zadanie1np
	rm zadanie1