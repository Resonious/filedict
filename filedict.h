#ifndef FILEDICT_H
#define FILEDICT_H 1

#include <stddef.h>

#ifndef FILEDICT_KEY_SIZE
#define FILEDICT_KEY_SIZE 256
#endif

#ifndef FILEDICT_VALUE_SIZE
#define FILEDICT_VALUE_SIZE 256
#endif

typedef struct filedict_bucket_entry_t {
    char key[FILEDICT_KEY_SIZE];
    char value[FILEDICT_VALUE_SIZE];
} filedict_bucket_entry_t;

#ifndef FILEDICT_BUCKET_ENTRY_COUNT
#define FILEDICT_BUCKET_ENTRY_COUNT 4
#endif

typedef struct filedict_bucket_t {
    filedict_bucket_entry_t entries[FILEDICT_BUCKET_ENTRY_COUNT];
} filedict_bucket_t;

typedef size_t (*filedict_hash_function_t)(const char *);

typedef struct filedict_t {
    const char *error;
    int fd;
    int flags;
    void *data;
    size_t data_len;
    filedict_hash_function_t hash_function;
} filedict_t;

typedef struct filedict_header_t {
    unsigned long long initial_bucket_count : 32;
    unsigned long long hashmap_count : 32;
} __attribute__ ((__packed__)) filedict_header_t;

typedef struct filedict_read_t {
    filedict_t *filedict;
    const char *key;
    const char *value;
    filedict_bucket_t *bucket;
    filedict_bucket_entry_t *entry;
    size_t entry_i;
    size_t hashmap_i;
    size_t bucket_count;
    size_t key_hash;
} filedict_read_t;

#endif

/*
 * Above is the header, blow is the implementation
 */

#ifndef FILEDICT_IMPL
#define FILEDICT_IMPL
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>

/* This is "djb2" from http://www.cse.yorku.ca/~oz/hash.html */
static size_t filedict_default_hash_function(const char *input) {
    unsigned long hash = 5381;
    int c;

    while ((c = *input++) != 0) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

/*
 * Writes at most max_len chars from src into dest.
 * Returns the total number of bytes in src.
 */
static size_t filedict_copy_string(char *dest, const char *src, size_t max_len) {
    size_t src_len = 0;
    char c;

    while (1) {
        c = *src++;
        if (src_len < max_len) { *dest++ = c; }
        if (c == 0) return src_len;
        src_len += 1;
    }

    return src_len;
}

static void filedict_init(filedict_t *filedict) {
    filedict->error = NULL;
    filedict->fd = 0;
    filedict->flags = 0;
    filedict->data_len = 0;
    filedict->data = NULL;
    filedict->hash_function = filedict_default_hash_function;
}

static void filedict_deinit(filedict_t *filedict) {
    if (filedict->data) {
        munmap(filedict->data, filedict->data_len);
        filedict->data = NULL;
        filedict->data_len = 0;
    }
    if (filedict->fd) {
        close(filedict->fd);
        filedict->fd = 0;
        filedict->flags = 0;
    }
}

/*
 * This computes the size of the entire filedict file given an initial bucket count and hashmap count.
 */
static size_t filedict_file_size(size_t initial_bucket_count, size_t hashmap_count) {
    /*
     * We used to size each additional hashmap at 2x the previous, but realistically it seems that
     * most resizes are triggered by keys that are ridiculously large, not by mass collision.
     *
     * A more proper fix might be to re-structure the whole filedict. We could keep the existing
     * hashmap structure, but with buckets that expand dynamically. This would require each bucket
     * to contain a "pointer" to the next bucket object if present.
     *
     * For now, it's easiser to just keep the hashmap duplication without the size doubling.
     */
    return sizeof(filedict_header_t) + initial_bucket_count * hashmap_count * sizeof(filedict_bucket_t);
}

/*
 * Resizes the filedict based on the header hashmap count and initial bucket count.
 * Naturally, your pointers into the map will become invalid after calling this.
 */
static void filedict_resize(filedict_t *filedict) {
    filedict_header_t *header = (filedict_header_t*)filedict->data;
    size_t computed_size = filedict_file_size(header->initial_bucket_count, header->hashmap_count);
    if (computed_size <= filedict->data_len) return;

    munmap(filedict->data, filedict->data_len);
    filedict->data = mmap(
        filedict->data,
        computed_size,
        PROT_READ | ((filedict->flags & O_RDWR) ? PROT_WRITE : 0),
        MAP_SHARED,
        filedict->fd,
        0
    );
    if (filedict->data == MAP_FAILED) { filedict->error = strerror(errno); return; }
    filedict->data_len = computed_size;
}

/*
 * This opens a new file for reading and writing, optionally letting you specify the initial bucket count.
 */
#define filedict_open_new(filedict, filename) \
    filedict_open_f(filedict, filename, O_CREAT | O_TRUNC | O_RDWR, 4096)

#define filedict_open_readonly(filedict, filename) \
    filedict_open_f(filedict, filename, O_RDONLY, 4096)

#define filedict_open(filedict, filename) \
    filedict_open_f(filedict, filename, O_CREAT | O_RDWR, 4096)

static void filedict_open_f(
    filedict_t *filedict,
    const char *filename,
    int flags,
    unsigned int initial_bucket_count
) {
    struct stat info;

    filedict->flags = flags;
    filedict->fd = open(filename, flags, 0666);
    if (filedict->fd == -1) { filedict->error = strerror(errno); return; }
    if (fstat(filedict->fd, &info) != 0) { filedict->error = strerror(errno); return; }

    if (info.st_size == 0 && (flags & O_RDWR)) {
        filedict->data_len = filedict_file_size(initial_bucket_count, 1);
        ftruncate(filedict->fd, filedict->data_len);
    } else {
        filedict->data_len = info.st_size;
    }

    filedict->data = mmap(
        NULL,
        filedict->data_len,
        PROT_READ | ((flags & O_RDWR) ? PROT_WRITE : 0),
        MAP_SHARED,
        filedict->fd,
        0
    );
    if (filedict->data == MAP_FAILED) { filedict->error = strerror(errno); return; }

    filedict_header_t *data = (filedict_header_t *)filedict->data;
    assert(initial_bucket_count <= UINT_MAX);

    if (data->initial_bucket_count == 0) {
        data->initial_bucket_count = initial_bucket_count;
        data->hashmap_count = 1;
    }
}

/*
 * Inserts a new value under "key". Filedict keys have multiple values, so this will "append" a new
 * value onto the end of the entry.
 */
#define filedict_insert(filedict, key, value) filedict_insert_f(filedict, key, value, 0)
#define filedict_insert_unique(filedict, key, value) filedict_insert_f(filedict, key, value, 1)

static void filedict_insert_f(filedict_t *filedict, const char *key, const char *value, int unique) {
    assert(filedict->fd != 0);
    assert(filedict->data != NULL);

    size_t i, hashmap_i = 0, bucket_count, key_hash;
    filedict_header_t *header = (filedict_header_t *)filedict->data;
    filedict_bucket_t *hashmap = filedict->data + sizeof(filedict_header_t);
    filedict_bucket_t *bucket;

    bucket_count = header->initial_bucket_count;

    key_hash = filedict->hash_function(key);

    /*
     * Here we loop through each hashmap.
     */
    while (hashmap_i < header->hashmap_count) {
try_again:
        /* TODO: can we truncate instead of modulo, like in Ruby? */
        bucket = &hashmap[key_hash % bucket_count];

        for (i = 0; i < FILEDICT_BUCKET_ENTRY_COUNT; ++i) {
            filedict_bucket_entry_t *entry = &bucket->entries[i];

            /* Easy case: fresh entry. We can just insert here and call it quits. */
            if (entry->key[0] == 0) {
                strncpy(entry->key, key, FILEDICT_KEY_SIZE);
                size_t value_len = filedict_copy_string(entry->value, value, FILEDICT_VALUE_SIZE);

                if (value_len > FILEDICT_VALUE_SIZE) {
                    filedict->error = "Value too big";
                }
                return;
            }
            /*
             * We need to check for room in the value, then append value.
             * This is also where we might run into a duplicate and duck out.existing
             */
            else if (strncmp(entry->key, key, FILEDICT_KEY_SIZE) == 0) {
                long long first_nonzero = -1;
                char *candidate = NULL;
                size_t value_i, candidate_len;

                for (value_i = 0; value_i < FILEDICT_VALUE_SIZE - 1; ++value_i) {
                    if (unique) {
                        if (first_nonzero == -1 && entry->value[value_i] != 0) {
                            first_nonzero = value_i;
                        }

                        if (entry->value[value_i] == 0) {
                            int cmp = strncmp(
                                &entry->value[first_nonzero],
                                value,
                                FILEDICT_VALUE_SIZE - first_nonzero
                            );
                            if (cmp == 0) {
                                /* Looks like this value already exists! */
                                return;
                            }
                            first_nonzero = -1;
                        }
                    }

                    if (entry->value[value_i] == 0 && entry->value[value_i + 1] == 0) {
                        candidate = &entry->value[value_i + 1];
                        candidate_len = FILEDICT_VALUE_SIZE - value_i - 1;

                        if (strlen(value) >= candidate_len) break;

                        strncpy(candidate, value, candidate_len);
                        return;
                    }
                }
            }
        }

        ++hashmap_i;
        hashmap += bucket_count;
    }

    /*
     * If we fell through to here, that means we need to allocate a new hashmap.
     */
    size_t new_hashmap_count = header->hashmap_count + 1;
    size_t old_data_len = filedict->data_len;
    size_t new_data_len = filedict_file_size(header->initial_bucket_count, new_hashmap_count);

    assert(new_data_len > old_data_len);
    assert((new_data_len - old_data_len) % header->initial_bucket_count == 0);

    munmap(filedict->data, filedict->data_len);
    int truncate_result = ftruncate(filedict->fd, new_data_len);
    if (truncate_result != 0) { filedict->error = strerror(errno); return; }

    filedict->data = mmap(
        filedict->data,
        new_data_len,
        PROT_READ | ((filedict->flags & O_RDWR) ? PROT_WRITE : 0),
        MAP_SHARED,
        filedict->fd,
        0
    );
    if (filedict->data == MAP_FAILED) { filedict->error = strerror(errno); return; }
    header = (filedict_header_t *)filedict->data;
    hashmap = filedict->data + old_data_len;

    filedict->data_len = new_data_len;
    header->hashmap_count = new_hashmap_count;
    goto try_again;
}

/*
 * There are 3 "levels" to a filedict. From top to bottom:
 * 1. Hashmap - which hashmap are we looking at? We create additional hashmaps to handle overflow.
 * 2. Entry   - which entry in our hashmap bucket are we looking at?
 * 3. Value   - where in the value buffer are we looking? There's 256 bytes, so can be many strings.
 */

/* #define log_return(val) do { printf("%s -> %i\n", __func__, (val)); return (val); } while(0) */
#define log_return(val) return val

/*
 * Returns 1 when we successfully advanced to the next value
 * Returns 0 when there is no next value
 */
static int filedict_read_advance_value(filedict_read_t *read) {
    assert(read->entry != NULL);

    const char *buffer_begin = read->entry->value;
    const char *buffer_end = buffer_begin + FILEDICT_VALUE_SIZE;

    const char *c;
    for (c = read->value; c < buffer_end; ++c) {
        if (*c == 0) {
            c += 1;
            break;
        }
    }

    if (c >= buffer_end) log_return(0);
    if (*c == 0) log_return(0);

    read->value = c;
    log_return(1);
}

/*
 * Returns 1 when we successfully find a new entry that matches read->key.
 *           advances read->entry_i and read->entry to the new entry.
 *
 * Returns 0 when we exhausted all remaining entries and didn't find a match.
 */
static int filedict_read_advance_entry(filedict_read_t *read) {
    assert(read->key != NULL);
    assert(strlen(read->key) > 0);
    assert(read->bucket != NULL);

    while (1) {
        if (read->entry_i >= FILEDICT_BUCKET_ENTRY_COUNT) log_return(0);

        read->entry = &read->bucket->entries[read->entry_i];

        if (strncmp(read->entry->key, read->key, FILEDICT_KEY_SIZE) == 0) {
            read->value = read->entry->value;
            log_return(1);
        }

        read->entry_i += 1;
    }
}

/*
 * Returns 1 when we successfully advanced to the next hashmap.
 *           read->bucket, read->entry, and read->value will be populated.
 *
 * Returns 0 when there are no more hashmaps, or the latest hashmap has no matching entries.
 */
static int filedict_read_advance_hashmap(filedict_read_t *read) {
    filedict_t *filedict = read->filedict;

    assert(filedict);
    assert(filedict->data);

    filedict_header_t *header = (filedict_header_t*)filedict->data;

    if (read->hashmap_i >= header->hashmap_count) log_return(0);

    size_t offset = filedict_file_size(header->initial_bucket_count, read->hashmap_i);

    if (offset >= filedict->data_len) {
        filedict_resize(filedict);
        if (filedict->error) log_return(0);
        header = (filedict_header_t*)filedict->data;
    }

    filedict_bucket_t *hashmap = filedict->data + offset;

    read->bucket_count = (size_t)header->initial_bucket_count;
    read->bucket = &hashmap[read->key_hash % read->bucket_count];
    read->entry = &read->bucket->entries[0];

    read->entry_i = 0;

    log_return(filedict_read_advance_entry(read));
}

/*
 * Returns a "read" at the given key. If there's a hit, <return>.value will have the value.
 */
static filedict_read_t filedict_get(filedict_t *filedict, const char *key) {
    filedict_read_t read;
    read.filedict = filedict;
    read.key = key;
    read.value = NULL;
    read.bucket = NULL;
    read.entry = NULL;
    read.entry_i = 0;
    read.hashmap_i = 0;
    read.bucket_count = 0;
    read.key_hash = filedict->hash_function(key);

    filedict_read_advance_hashmap(&read);
    return read;
}

/*
 * Lets you find the next value. Pass the return value of filedict_get.
 *
 * Returns 1 when a next value was found, 0 otherwise.
 *
 * If this returns 0, your filedict_read_t is defunct and shouldn't be used anymore.
 */
static int filedict_get_next(filedict_read_t *read) {
    int found = -1;

    found = filedict_read_advance_value(read);
    if (found == 1) return found;

    read->entry_i += 1;
    found = filedict_read_advance_entry(read);
    if (found == 1) return found;

    read->hashmap_i += 1;
    return filedict_read_advance_hashmap(read);
}

#endif
