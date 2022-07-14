all: test analyze analyze-dbg visualize merge merge-dbg

test: filedict.h test.c merge
	gcc -Wall -ggdb test.c -o test

analyze: filedict.h analyze.c
	gcc -Wall -O3 analyze.c -o analyze

visualize: filedict.h visualize.c
	gcc -Wall -ggdb visualize.c -o visualize

analyze-dbg: filedict.h analyze.c
	gcc -Wall -ggdb analyze.c -o analyze-dbg

merge: filedict.h merge.c
	gcc -Wall -O3 merge.c -o merge

merge-dbg: filedict.h merge.c
	gcc -Wall -ggdb merge.c -o merge-dbg
