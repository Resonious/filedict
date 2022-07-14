#include <stdio.h>
#include <signal.h>

#include "filedict.h"

#define error_check(filedict) do { if (filedict.error) { printf("[%i] error: %s\n", __LINE__, filedict.error); filedict_deinit(&filedict); return 2; } } while (0)

int main(int argc, const char **argv) {
    size_t file_i;
    int success = 1;
    filedict_t dest, src;
    filedict_read_t read;

    if (argc < 2) {
        printf("Usage: ./merge dest-file.fdict src-file.fdict ...\n");
        return 1;
    }

    filedict_init(&dest);
    filedict_open(&dest, argv[1]);
    error_check(dest);

    for (file_i = 2; file_i < argc; ++file_i, filedict_deinit(&src)) {
        filedict_init(&src);
        filedict_open_readonly(&src, argv[file_i]);
        error_check(src);

        read = filedict_get(&src, NULL);

        success = 1;
        while (success && read.value) {
            filedict_insert_unique(&dest, read.entry->bytes, read.value);
            success = filedict_get_next(&read);
        }
        error_check(src);
        error_check(dest);
    }
}
