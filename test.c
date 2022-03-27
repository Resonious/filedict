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
    filedict_insert(&filedict, "key2", "key2value2");
    filedict_insert(&filedict, "key2", "key2value3");
    filedict_insert(&filedict, "key2", "key2value4");
    filedict_insert(&filedict, "key2", "key2value5");
    filedict_insert(&filedict, "key2", "key2value6");
    filedict_insert(&filedict, "key2", "key2value7");
    filedict_insert(&filedict, "key2", "key2value8");
    filedict_insert(&filedict, "key2", "key2value9");
    filedict_insert(&filedict, "key2", "key2value10");
    filedict_insert(&filedict, "key2", "key2value11");
    filedict_insert(&filedict, "key2", "key2value12");
    filedict_insert(&filedict, "key2", "key2value13");
    filedict_insert(&filedict, "key2", "key2value14");
    filedict_insert(&filedict, "key2", "key2value15");
    filedict_insert(&filedict, "key2", "here is a really long one. lets get crazy. push it to the limit. pump it pump it pump it pump it pump it pump it pump it pump it pump it pump it pump it");
    error_check();
    filedict_insert(&filedict, "key2", "here is a really long one. lets get crazy. push it to the limit. pump it pump it pump it pump it pump it pump it pump it pump it pump it pump it pump it");
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
