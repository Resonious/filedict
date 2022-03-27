# Filedict

This is a single-header C library that provides very simple file-backed key-value store.

Filedict does not allocate memory using `malloc` or similar; it only uses `open` and `mmap`. All values are `const char *` and live inside the file-backed memory.

## Key features

* Simple file-backed key-value store. (hashmap style. not a btree)
* Great for when you need to save data but redis or sqlite is too much.
* Doesn't allocate memory.
* No dependencies other than the C standard library.
* All in a single header - no need to mess with your linker or package manager.

# What data does this store, exactly?

String keys, each associated with one or more string values.

All keys and all values are null-terminated C strings. Works fine with UTF-8 because it doesn't actually parse anything.

There is a size limit! By default, keys and values have a maximum of 256 bytes. You can change this with `#define FILEDICT_KEY_SIZE 1024` and/or `#define FILEDICT_VALUE_SIZE 2048` if you'd like. But do note that this will make your data files much larger and more spacious unless you actually use most of the bytes for each key.

Despite the size limit on individual keys and values, there is actually no limit on _how many_ values you can have under one key, or how many keys you can have. The store can grow indefinitely without any re-hashing. Also, storing many small values under the same key will stuff all of the values into the same entry until that entry runs out of space.

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

    filedict_read_t read = filedict_get(&filedict, "my key");
    assert(strcmp(read.value, "my key") == 0);

    /* Because filedict lets you store multiple values under the same key, you
     * can use this "filedict_read_t" to get the rest of the values by calling
     * `filedict_get_next(&read)` until it returns 0.
     */

    /* This will unmap the memory and close the file. */
    filedict_deinit(&filedict);

    return 0;
}
```
