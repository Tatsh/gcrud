#ifndef GRUD_UTIL_H
#define GRUD_UTIL_H

#include <glib-2.0/glib.h>

extern void apply_lib_mapping(GHashTable *, const char *)
    __attribute__((nonnull));
extern GHashTable *find_files_in_packages(const char *)
    __attribute__((nonnull, returns_nonnull, warn_unused_result));
extern GHashTable *findwalk(const char *, const GHashTable *, GDestroyNotify)
    __attribute__((nonnull(1, 2), returns_nonnull, warn_unused_result));

#endif // GCRUD_UTIL_H
