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
        size_t j, k, bucket_count;
        filedict_header_t *header;
        filedict_bucket_t *hashmap;
        const char *last_entry_key;

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
        for (j = 0; j < filedict.data_len; ++j) {
            zeros += (((char *)filedict.data)[j] == 0);
            nonzeros += (((char *)filedict.data)[j] != 0);
        }

        printf("zeros:    %lli\n", zeros);
        printf("nonzeros: %lli\n", nonzeros);
        printf("zero %%:   %f%%\n", (double)zeros / (double)(zeros + nonzeros) * 100.0);

        /*
         * Now let's look at actual hashmap info
         */
        header = (filedict_header_t *)filedict.data;
        hashmap = (filedict_bucket_t*)(filedict.data + sizeof(filedict_header_t));

        printf("\n");
        printf("hashmap count: %i\n", header->hashmap_count);
        printf("initial bucket count: %i\n", header->initial_bucket_count);

        bucket_count = header->initial_bucket_count;

        for (j = 0; j < header->hashmap_count; ++j) {
            size_t used_buckets = 0;
            size_t unused_buckets = 0;

            for (k = 0; k < bucket_count; ++k) {
                filedict_bucket_t *bucket = &hashmap[k];

                used_buckets += (bucket->entries[0].bytes[0] != 0);
                unused_buckets += (bucket->entries[0].bytes[0] == 0);

                if (bucket->entries[0].bytes[0] != 0) {
                    last_entry_key = bucket->entries[0].bytes;
                }
            }
            printf("\n");
            printf("hashmap %li used buckets:   %li\n", j+1, used_buckets);
            printf("hashmap %li unused buckets: %li\n", j+1, unused_buckets);
            printf("hashmap %li last key:       %s\n", j+1, last_entry_key);

            hashmap += bucket_count;
        }
    }

    filedict_deinit(&filedict);
    return 0;
}
