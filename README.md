# Filedict

This is a single-header C library that provides very simple file-backed key-value store.

Filedict does not allocate memory using `malloc` or similar; it only uses `open` and `mmap`. All values are `const char *` and live inside the file-backed memory.

# What data does this store, exactly?

String keys, and string values.

Keys can have multiple values.

More specifically, all keys and all values are null-terminated C strings. Works fine with UTF-8.

There is a size limit! By default, keys and values have a maximum of 256 bytes. You can change this with `#define FILEDICT_KEY_SIZE 1024` or `#define FILEDICT_VALUE_SIZE 2048` if you'd like. But do note that this will make your files much larger and more spacious unless you actually use all of those bytes for most of your entries.

# How to use

```c
#include "../path/to/filedict.h"

int main() {
    /* Init a new filedict */
    filedict_t filedict;
    filedict_init(&filedict);

    /* You need to open a file before using it! */
    filedict_open_new(&filedict, "my-data-store.filedict");

    /* Instead of error return-values, most functions will set filedict.error
     * when something goes wrong.
     */
    if (filedict.error) {
        printf("Something went wrong: %s\n", filedict.error);
        return 1;
    }

    filedict_insert(&filedict, "my key", "my value");

    /* Because filedict lets you store multiple values under the same key, you
     * can use this "filedict_read_t" to get the rest of the values by calling
     * `filedict_get_next(&read)` until it returns 0.
     */
    filedict_read_t read = filedict_get(&filedict, "my key");
    assert(strcmp(read.value, "my key") == 0);

    /* This will unmap the memory and close the file. */
    filedict_deinit(&filedict);

    return 0;
}
```
