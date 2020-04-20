#include <limits.h>
#include <stdlib.h>

#include <glib/gprintf.h>
#include <libmount/libmount.h>

#include "util.h"
#include "whitelist.h"

static struct libmnt_table *tb = NULL;
G_LOCK_DEFINE_STATIC(tb);

GHashTable *find_files_in_packages(const char *base) {
    const gchar *cent, *pent;

    GDir *dirp = g_dir_open(base, 0, NULL);
    GHashTable *set = g_hash_table_new_full(
        (GHashFunc)g_str_hash, (GEqualFunc)g_str_equal, NULL, g_free);
    GRegex *sym_re = g_regex_new(
        "sym (/.+)(?= \\->)", G_REGEX_ANCHORED, G_REGEX_MATCH_ANCHORED, NULL);
    g_assert_nonnull(dirp);
    g_assert_nonnull(set);
    g_assert_nonnull(sym_re);

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
                gchar *path = NULL;
                gchar *rpath = NULL;

                if (g_str_equal(type, "dir")) {
                    path = g_strstrip(g_strdup(line + 4));
                } else if (g_str_equal(type, "sym")) {
                    GMatchInfo *match_info;
                    g_regex_match(sym_re, line, 0, &match_info);
                    g_assert(g_match_info_matches(match_info));
                    path = g_match_info_fetch(match_info, 1);
                    g_assert_nonnull(path);
                    g_match_info_free(match_info);
                    rpath = realpath(path, NULL);
                    if (!rpath) {
                        g_free(path);
                        goto cleanup;
                    }
                } else {
                    size_t end_index = strlen(line);
                    for (size_t space_count = 0;
                         end_index >= 0 && space_count < 2;
                         end_index--) {
                        if (line[end_index] == ' ') {
                            space_count++;
                        }
                    }
                    path = g_strndup(line + 4, end_index - 3);
                }
                g_assert(path[0] == '/');

                g_hash_table_add(set, path);
                if (rpath) {
                    g_hash_table_add(set, rpath);
                }

                if ((g_str_equal("obj", type) || rpath) &&
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
            }

            g_io_channel_unref(cfile);
            g_free(cpath);
        }

        g_dir_close(pdir);
        g_free(pdir_path);
    }

    g_regex_unref(sym_re);
    g_dir_close(dirp);

    return set;
}

static void g_hash_table_add_all(GHashTable *target, GHashTable *src) {
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, src);
    gpointer file;
    while (g_hash_table_iter_next(&iter, &file, NULL)) {
        g_hash_table_add(target, file);
    }
}

static GHashTable *g_hash_table_clone(GHashTable *src,
                                      GHashFunc hash_func,
                                      GEqualFunc key_equal_func) {
    GHashTable *ret =
        g_hash_table_new_full(hash_func, key_equal_func, NULL, NULL);
    g_hash_table_add_all(ret, src);
    return ret;
}

void apply_lib_mapping(GHashTable *package_files, const char *libmap) {
    GHashTable *snapshot = g_hash_table_clone(
        package_files, (GHashFunc)g_str_hash, (GEqualFunc)g_str_equal);
    GHashTableIter it;
    gpointer file;
    g_hash_table_iter_init(&it, snapshot);
    while (g_hash_table_iter_next(&it, &file, NULL)) {
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

        g_hash_table_iter_remove(&it);
        g_hash_table_add(package_files, file_);

        g_free(prefix);
        g_free(rest);
    }

    g_hash_table_destroy(snapshot);
}

static inline void initialize_tb() {
    if (!tb) {
        G_LOCK(tb);
        tb = mnt_new_table_from_file("/proc/self/mountinfo");
        g_assert_nonnull(tb);
        struct libmnt_cache *cache = mnt_new_cache();
        g_assert_nonnull(cache);
        mnt_table_set_cache(tb, cache);
        mnt_unref_cache(cache);
        G_UNLOCK(tb);
    }
}

static gboolean is_mountpoint_pseudo(const char *path) {
    initialize_tb();
    struct libmnt_fs *fs =
        mnt_table_find_mountpoint(tb, path, MNT_ITER_BACKWARD);
    return fs && mnt_fs_get_target(fs) && mnt_fs_is_pseudofs(fs);
}

static inline gboolean should_recurse(const char *path) {
    return !g_file_test(path, G_FILE_TEST_IS_SYMLINK) &&
           (g_file_test(path, G_FILE_TEST_IS_DIR) &&
            !is_mountpoint_pseudo(path));
}

GHashTable *findwalk(const char *path,
                     const GHashTable *package_files,
                     GDestroyNotify value_destroy_func) {
    GHashTable *candidates = g_hash_table_new_full((GHashFunc)g_str_hash,
                                                   (GEqualFunc)g_str_equal,
                                                   NULL,
                                                   value_destroy_func);
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

        gboolean clean_up_ce = FALSE;

        // Whitelist and package_files check
        if (!g_hash_table_contains((GHashTable *)package_files, ce) &&
            !whitelist_check(ce)) {
            if (g_file_test(ce, G_FILE_TEST_IS_SYMLINK)) {
                char *resolved_path = realpath(ce, NULL);
                if (resolved_path == NULL) {
                    // dead link
                    g_hash_table_add(candidates, ce);
                } else {
                    if (!g_hash_table_contains((GHashTable *)package_files,
                                               resolved_path) &&
                        !whitelist_check(resolved_path)) {
                        g_hash_table_add(candidates, ce);
                    }
                    free(resolved_path);
                }
            } else {
                g_hash_table_add(candidates, ce);
            }
        } else {
            clean_up_ce = TRUE;
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

void findwalk_cleanup() {
    if (tb) {
        mnt_unref_table(tb);
        tb = NULL;
    }
}
