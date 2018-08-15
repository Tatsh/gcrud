#include "util.h"

#include <sys/stat.h>

static inline gboolean is_current_or_parent(const char *name) {
    return name[0] == '.' || (name[0] == '.' && name[1] == '.');
}

GHashTable *find_files_in_packages(const char *base) {
    DIR *dirp = opendir(base);
    g_assert_nonnull(dirp);
    struct dirent *cent, *pent;
    GHashTable *set = g_hash_table_new_full(
        (GHashFunc)g_str_hash, (GEqualFunc)g_str_equal, g_free, NULL);
    g_assert_nonnull(set);

    while ((cent = readdir(dirp))) {
        if (is_current_or_parent(cent->d_name)) {
            continue;
        }

        gchar *pdir_path = g_strdup_printf("%s/%s", base, cent->d_name);
        DIR *pdir = opendir(pdir_path);
        g_assert_nonnull(pdir);

        while ((pent = readdir(pdir))) {
            if (is_current_or_parent(pent->d_name)) {
                continue;
            }

            gchar *cpath = g_strdup_printf(
                "%s/%s/%s/CONTENTS", base, cent->d_name, pent->d_name);

            gchar *line;
            GIOChannel *cfile = g_io_channel_new_file(cpath, "r", NULL);
            g_assert_nonnull(cfile);
            GIOStatus status;

            while ((status = g_io_channel_read_line(
                        cfile, &line, NULL, NULL, NULL)) !=
                   G_IO_STATUS_EOF) {
                g_assert(status == G_IO_STATUS_NORMAL);
                gchar *type = g_strndup(line, 3);
                g_assert(type[0] != '/');

                gchar **sp = g_strsplit(line + 4, " ", 2);
                gchar *path = g_strdup(sp[0]);
                path = g_strstrip(path);
                g_assert(path[0] == '/');

                g_hash_table_add(set, path);

                if (g_str_equal("obj", type) || g_str_equal("sym", type)) {
                    gchar *py_compiled_type = g_strdup_printf("%sc", path);
                    g_hash_table_add(set, py_compiled_type);
                    py_compiled_type = g_strdup_printf("%so", path);
                    g_hash_table_add(set, py_compiled_type);
                }

                g_free(type);
                g_free(line);
                g_strfreev(sp);
            }

            g_io_channel_unref(cfile);
            g_free(cpath);
        }

        closedir(pdir);
        g_free(pdir_path);
    }

    closedir(dirp);

    return set;
}

// void apply_lib_mapping(const PblSet *package_files) {
//     PblIterator *it = pblSetIterator(package_files);
//     char *file;
//     while ((file = pblIteratorNext(it))) {
//
//         if (!pblIteratorHasNext(it)) {
//             break;
//         }
//     }
// }

static void g_hash_table_add_all(GHashTable *target, GHashTable *src) {
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, src);
    gpointer file, _;
    while (g_hash_table_iter_next(&iter, &file, &_)) {
        g_hash_table_add(target, file);
    }
}

GHashTable *findwalk(const char *path,
                     const GHashTable *package_files,
                     GDestroyNotify key_destroy_func) {
    GHashTable *candidates = g_hash_table_new_full((GHashFunc)g_str_hash,
                                                   (GEqualFunc)g_str_equal,
                                                   key_destroy_func,
                                                   NULL);
    g_assert_nonnull(candidates);

    DIR *dir = opendir(path);
#ifndef DEBUG
    g_assert_nonnull(dir);
#else
        if (!dir) {
            // If not root it's a permission issue
            return candidates;
        }
#endif
    struct dirent *cdir;
    while ((cdir = readdir(dir))) {
        if (is_current_or_parent(cdir->d_name)) {
            continue;
        }

        gchar *ce = g_strdup_printf("%s/%s", path, cdir->d_name);
        gboolean clean_up_ce = FALSE;
        g_assert_nonnull(ce);

        // Whitelist check

        // package_files check
        if (!g_hash_table_contains((GHashTable *)package_files, ce)) {
            g_hash_table_add(candidates, ce);
        } else {
            clean_up_ce = TRUE;
        }

        // Recurse if the entry is a directory
        struct stat s;
        lstat(ce, &s);
        if (S_ISDIR(s.st_mode) && !S_ISLNK(s.st_mode)) {
            // On recursion do not set a key destroy function because the
            // top hash table already has one
            GHashTable *next = findwalk(ce, package_files, NULL);
            g_assert_nonnull(next);
            g_hash_table_add_all(candidates, next);
            g_hash_table_destroy(next);
        }

        if (clean_up_ce) {
            g_free(ce);
        }
    }

    closedir(dir);

    return candidates;
}
