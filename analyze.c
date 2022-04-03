#include <stdio.h>
#include <signal.h>

#include "filedict.h"

#define error_check() do { if (filedict.error) { printf("[%i] error: %s\n", __LINE__, filedict.error); filedict_deinit(&filedict); return 2; } } while (0)

int main(int argc, const char **argv) {
    int i;
    filedict_t filedict;
    filedict_init(&filedict);

    if (argc == 1) {
        printf("Usage: ./analyze dict-file-1.fdict dict-file-2.fdict ...\n");
        return 1;
    }

    for (i = 1; i < argc; ++i, filedict_deinit(&filedict)) {
        long long zeros = 0;
        long long nonzeros = 0;
        size_t b;
        filedict_header_t *header;

        filedict_open_readonly(&filedict, argv[i]);
        error_check();

        if (i > 1) printf("\n\n");

        /*
         * First, print the filename
         */
        printf("--- %s ---\n", argv[i]);

        /*
         * Simple analysis of the number of zeros in the file
         */
        printf("\n");
        for (b = 0; b < filedict.data_len; ++b) {
            zeros += (((char *)filedict.data)[b] == 0);
            nonzeros += (((char *)filedict.data)[b] != 0);
        }

        printf("zeros:    %lli\n", zeros);
        printf("nonzeros: %lli\n", nonzeros);
        printf("zero %%:   %f%%\n", (double)zeros / (double)(zeros + nonzeros) * 100.0);

        /*
         * Now let's look at actual hashmap info
         */
        header = (filedict_header_t *)filedict.data;

        printf("\n");
        printf("hashmap count: %i\n", header->hashmap_count);
    }

    filedict_deinit(&filedict);
    return 0;
}
