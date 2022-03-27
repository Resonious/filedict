#include <stdio.h>

#include "filedict.h"

#define error_check() do { if (filedict.error) { printf("Error: %s\n", filedict.error); filedict_deinit(&filedict); return 1; } } while (0)

int main() {
    filedict_t filedict;
    filedict_init(&filedict);
    error_check();

    filedict_open_new(&filedict, "test.data");
    error_check();
    filedict_insert(&filedict, "mykey", "myvalue");
    error_check();

    filedict_insert(&filedict, "key2", "key2value1");
    error_check();
    filedict_insert(&filedict, "key2", "key2value2");
    error_check();
    filedict_insert(&filedict, "key2", "key2value3");
    error_check();
    filedict_insert(&filedict, "key2", "key2value4");
    error_check();
    filedict_insert(&filedict, "key2", "key2value5");
    error_check();

    filedict_insert(&filedict, "mykey", "myvalue2");
    error_check();

    filedict_read_t read = filedict_get(&filedict, "key2");
    int success = 1;

    while (success) {
        printf("Read %s\n", read.value);
        success = filedict_get_next(&read);
    }

    filedict_deinit(&filedict);

    printf("\nEverything went well?\n");
    return 0;
}
