#include <stdio.h>
#include <signal.h>

#include "filedict.h"

#define breakpoint() raise(SIGTRAP)

#define error_check() do { if (filedict.error) { printf("Line %i error: %s\n", __LINE__, filedict.error); filedict_deinit(&filedict); return 1; } } while (0)
#define error_check2() do { if (filedict2.error) { printf("Line %i error: %s\n", __LINE__, filedict2.error); filedict_deinit(&filedict2); return 1; } } while (0)

int main() {
    filedict_t filedict, filedict2;
    filedict_init(&filedict);
    filedict_init(&filedict2);
    error_check();
    error_check2();

    filedict_open_new(&filedict, "test.data");
    error_check();
    filedict_insert(&filedict, "mykey", "myvalue");
    error_check();
    filedict_open(&filedict2, "test.data");
    error_check2();

    printf("--------- INSERTING MANY KEYS --------\n");
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
    filedict_insert(&filedict, "key2", "here is a really long one. lets get crazy. push it to the limit. pump it pump it pump it pump it pump it pump it pump it pump it pump it pump it pump it 1.");
    error_check();
    filedict_insert(&filedict, "key2", "here is a really long one. lets get crazy. push it to the limit. pump it pump it pump it pump it pump it pump it pump it pump it pump it pump it pump it 2.");
    filedict_insert(&filedict, "key2", "here is a really long one. lets get crazy. push it to the limit. pump it pump it pump it pump it pump it pump it pump it pump it pump it pump it pump it 3.");
    filedict_insert(&filedict, "key2", "this is it, right? pushes you over the limit. the LIMIT. 限界オーバー！ IT IS OVER FOR YOU!!!!!.");
    error_check();

    filedict_insert_unique(&filedict, "key2", "key2value15");
    filedict_insert_unique(&filedict, "key2", "key2value15");
    filedict_insert_unique(&filedict, "key2", "key2value16");
    error_check();

    filedict_insert(&filedict, "mykey", "myvalue2");
    error_check();

    printf("------------- READING \"key2\" ---------------\n");
    filedict_read_t read = filedict_get(&filedict, "key2");
    int success = 1;

    while (success) {
        printf("Read %s\n", read.value);
        success = filedict_get_next(&read);
    }

    filedict_deinit(&filedict);

    printf("-------- reading from filedict2 \"key2\" ---------\n");
    read = filedict_get(&filedict2, "key2");
    success = 1;

    while (success) {
        printf("Read %s\n", read.value);
        success = filedict_get_next(&read);
    }

    filedict_deinit(&filedict);
    filedict_deinit(&filedict2);

    printf("\nEverything went well?\n");
    return 0;
}
