#ifndef GCRUD_UTIL_H
#define GCRUD_UTIL_H

#include <glib-2.0/glib.h>

typedef struct findwalk_data {
    const char *path;
    GHashTable *package_files;
    GDestroyNotify value_destroy_func;
} findwalk_data_t;

extern void apply_lib_mapping(GHashTable *, const char *)
    __attribute__((nonnull));
extern GHashTable *find_files_in_packages(const char *)
    __attribute__((nonnull, returns_nonnull, warn_unused_result));
extern GHashTable *findwalk(const findwalk_data_t *fw)
    __attribute__((nonnull, returns_nonnull, warn_unused_result));
extern void findwalk_cleanup();

#endif // GCRUD_UTIL_H
