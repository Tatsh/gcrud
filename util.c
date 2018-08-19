#include "util.h"
#include "whitelist.h"

#include <stdlib.h>

#include <glib/gprintf.h>

GHashTable *find_files_in_packages(const char *base) {
    GDir *dirp = g_dir_open(base, 0, NULL);
    g_assert_nonnull(dirp);
    const gchar *cent, *pent;
    GHashTable *set = g_hash_table_new_full(
        (GHashFunc)g_str_hash, (GEqualFunc)g_str_equal, g_free, NULL);
    g_assert_nonnull(set);

    while ((cent = g_dir_read_name(dirp))) {
        gchar *pdir_path = g_strdup_printf("%s/%s", base, cent);
        GDir *pdir = g_dir_open(pdir_path, 0, NULL);
        g_assert_nonnull(pdir);

        while ((pent = g_dir_read_name(pdir))) {
            gchar *cpath =
                g_strdup_printf("%s/%s/%s/CONTENTS", base, cent, pent);

            gchar *line;
            GIOChannel *cfile = g_io_channel_new_file(cpath, "r", NULL);
            g_assert_nonnull(cfile);
            GIOStatus status;

            while ((status = g_io_channel_read_line(
                        cfile, &line, NULL, NULL, NULL)) != G_IO_STATUS_EOF) {
                g_assert(status == G_IO_STATUS_NORMAL);
                gchar *type = g_strndup(line, 3);
                g_assert(type[0] != '/');

                gchar **sp = g_strsplit(line + 4, " ", 2);
                gchar *path = g_strdup(sp[0]);
                path = g_strstrip(path);
                g_assert(path[0] == '/');

                const gboolean is_sym = g_str_equal("sym", type);
                char *resolved_path = NULL;

                if (is_sym) {
                    resolved_path = realpath(path, NULL);
                    if (!resolved_path) {
                        goto cleanup;
                    }
                }

                g_hash_table_add(set, path);
                gchar *rpath = NULL;
                if (is_sym && resolved_path) {
                    rpath = g_strdup(resolved_path);
                    free(resolved_path);
                    g_hash_table_add(set, rpath);
                }

                if ((g_str_equal("obj", type) || is_sym) &&
                    g_str_has_suffix(path, ".py")) {
                    gchar *py_compiled_type = g_strdup_printf("%sc", path);
                    g_hash_table_add(set, py_compiled_type);
                    py_compiled_type = g_strdup_printf("%so", path);
                    g_hash_table_add(set, py_compiled_type);

                    if (rpath) {
                        py_compiled_type = g_strdup_printf("%sc", rpath);
                        g_hash_table_add(set, py_compiled_type);
                        py_compiled_type = g_strdup_printf("%so", rpath);
                        g_hash_table_add(set, py_compiled_type);
                    }
                }

            cleanup:
                g_free(type);
                g_free(line);
                g_strfreev(sp);
            }

            g_io_channel_unref(cfile);
            g_free(cpath);
        }

        g_dir_close(pdir);
        g_free(pdir_path);
    }

    g_dir_close(dirp);

    return set;
}

void apply_lib_mapping(GHashTable *package_files, const char *libmap) {
    GHashTableIter it;
    gpointer file, _;
    g_hash_table_iter_init(&it, package_files);
    while (g_hash_table_iter_next(&it, &file, &_)) {
        gboolean starts_with_usr_lib = g_str_has_prefix(file, "/usr/lib/");
        gboolean starts_with_lib = g_str_has_prefix(file, "/lib/");
        if (!starts_with_lib && !starts_with_usr_lib) {
            continue;
        }

        size_t off = starts_with_lib ? 0 : 5;
        gchar *prefix = g_strndup(file, off);
        g_assert_nonnull(prefix);
        gchar *rest = g_strdup((gchar *)file + off + 4);
        g_assert_nonnull(rest);
        gchar *file_ = g_strdup_printf("%s%s/%s", prefix, libmap, rest);
        g_assert_nonnull(file_);

        g_hash_table_iter_replace(&it, file_);

        g_free(prefix);
        g_free(rest);
    }
}

static void g_hash_table_add_all(GHashTable *target, GHashTable *src) {
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, src);
    gpointer file, _;
    while (g_hash_table_iter_next(&iter, &file, &_)) {
        g_hash_table_add(target, file);
    }
}

static inline gboolean should_recurse(const char *path) {
    return !g_file_test(path, G_FILE_TEST_IS_SYMLINK) &&
           g_file_test(path, G_FILE_TEST_IS_DIR);
}

GHashTable *findwalk(const char *path,
                     const GHashTable *package_files,
                     GDestroyNotify key_destroy_func) {
    GHashTable *candidates = g_hash_table_new_full((GHashFunc)g_str_hash,
                                                   (GEqualFunc)g_str_equal,
                                                   key_destroy_func,
                                                   NULL);
    g_assert_nonnull(candidates);

    GDir *dir = g_dir_open(path, 0, NULL);
#ifdef NDEBUG
    g_assert_nonnull(dir);
#else
    if (!dir) {
        // If not root it's a permission issue
        return candidates;
    }
#endif
    const char *cdir;
    while ((cdir = g_dir_read_name(dir))) {
        gchar *ce = g_strdup_printf("%s/%s", path, cdir);
        if (!ce) {
            g_fprintf(stderr,
                      "Unexpected return value from g_strdup_printf(). "
                      "Stopping here.");
            break;
        }
        gchar *rce = NULL;
        const gboolean is_sym = g_file_test(ce, G_FILE_TEST_IS_SYMLINK);
        if (is_sym) {
            rce = realpath(ce, NULL);
        }

        gboolean clean_up_ce = FALSE;

        // Whitelist and package_files check
        if (!g_hash_table_contains((GHashTable *)package_files, ce) &&
            !whitelist_check(ce)) {
            g_hash_table_add(candidates, ce);
        } else {
            clean_up_ce = TRUE;
        }

        if (is_sym && rce && !g_str_equal(ce, rce) &&
            !g_hash_table_contains((GHashTable *)package_files, rce) &&
            !whitelist_check(rce)) {
            g_hash_table_add(candidates, rce);
        } else if (rce) {
            g_free(rce);
        }

        // Recurse if the entry is a directory
        if (should_recurse(ce)) {
            // On recursion do not set a key destroy function because the
            // top hash table already has one
            GHashTable *next = findwalk(ce, package_files, NULL);
            g_hash_table_add_all(candidates, next);
            g_hash_table_destroy(next);
        }

        if (clean_up_ce) {
            g_free(ce);
        }
    }

    g_dir_close(dir);

    return candidates;
}
