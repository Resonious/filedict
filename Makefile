all: test analyze analyze-dbg

test: filedict.h test.c
	gcc -Wall -ggdb test.c -o test

analyze: filedict.h analyze.c
	gcc -Wall -O3 analyze.c -o analyze

analyze-dbg: filedict.h analyze.c
	gcc -Wall -ggdb analyze.c -o analyze-dbg
