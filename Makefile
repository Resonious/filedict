all: test analyze analyze-dbg visualize

test: filedict.h test.c
	gcc -Wall -ggdb test.c -o test

analyze: filedict.h analyze.c
	gcc -Wall -O3 analyze.c -o analyze

visualize: filedict.h visualize.c
	gcc -Wall -ggdb visualize.c -o visualize

analyze-dbg: filedict.h analyze.c
	gcc -Wall -ggdb analyze.c -o analyze-dbg
