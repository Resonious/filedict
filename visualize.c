#include <stdio.h>
#include <signal.h>

#include "filedict.h"

#define error_check() do { if (filedict.error) { printf("[%i] error: %s\n", __LINE__, filedict.error); filedict_deinit(&filedict); return 2; } } while (0)

int main(int argc, const char **argv) {
    size_t file_i;
    int success = 1;
    filedict_t filedict;
    filedict_read_t read;
    filedict_bucket_entry_t *last_entry = NULL;
    filedict_bucket_t *last_bucket = NULL;
    const char *last_key = "";

    if (argc == 1) {
        printf("Usage: ./visualize dict-file-1.fdict dict-file-2.fdict ...\n");
        return 1;
    }

    for (file_i = 1; file_i < argc; ++file_i, filedict_deinit(&filedict)) {
        filedict_init(&filedict);
        filedict_open_readonly(&filedict, argv[file_i]);

        error_check();

        /*
         * 1234 zeros...
         * ------------------ BUCKET 4423 --------------------
         *  "key1":
         *      "value1"
         *      "value2"
         *      "value3"
         * ---------------------------------------------------
         */

        read = filedict_get(&filedict, NULL);

        success = 1;
        while (success && read.value) {
            if (last_bucket != read.bucket) {
                if (last_bucket != NULL) printf("\n");
                printf("================= BUCKET ================\n");
                last_bucket = read.bucket;
            }
            if (last_entry != read.entry) {
                printf("---------------- ENTRY ----------------\n");
                last_entry = read.entry;
                last_key = "";
            }
            if (strcmp(last_key, read.entry->bytes) != 0) {
                printf("\"%s\":\n", read.entry->bytes);
                last_key = read.entry->bytes;
            }
            printf("    %s\n", read.value);

            success = filedict_get_next(&read);
        }
        error_check();
    }
}
