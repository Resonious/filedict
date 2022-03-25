test: filedict.h filedict.c test.c
	gcc -ggdb filedict.c test.c -o test
