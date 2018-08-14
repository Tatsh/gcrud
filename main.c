#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib-2.0/glib.h>

#define CHECK_DIRS_SIZE 9

static const char *terminal_reset = "\e[0m";
static const char *terminal_bold = "\e[1m";
static const char *terminal_color_red = "\e[1;31m";

static const char *installed_base = "/var/db/pkg";
static const char *libmap = "lib64";

bool is_current_or_parent(const char *)
    __attribute__((nonnull, warn_unused_result));
int pbl_set_compare_strings(const void *, const void *)
    __attribute__((nonnull, warn_unused_result));
int strneq(const char *, const char *)
    __attribute__((nonnull, warn_unused_result));
GHashTable *find_files_in_packages(const char *)
    __attribute__((nonnull, returns_nonnull, warn_unused_result));
GHashTable *findwalk(const char *, const GHashTable *)
    __attribute__((nonnull, returns_nonnull, warn_unused_result));

bool is_current_or_parent(const char *name) {
    return name[0] == '.' || (name[0] == '.' && name[1] == '.');
}

GHashTable *find_files_in_packages(const char *base) {
    DIR *dirp = opendir(base);
    assert(dirp);
    struct dirent *cent, *pent;
    GString *pdir_path = g_string_new(NULL);
    GString *contents_path = g_string_new(NULL);
    GHashTable *set =
        g_hash_table_new((GHashFunc)g_string_hash, (GEqualFunc)g_string_equal);
    assert(set);

    GString *s_obj = g_string_new("obj");
    GString *s_sym = g_string_new("sym");

    while ((cent = readdir(dirp))) {
        if (is_current_or_parent(cent->d_name)) {
            continue;
        }

        g_string_printf(pdir_path, "%s/%s", installed_base, cent->d_name);
        DIR *pdir = opendir(pdir_path->str);
        assert(pdir);
        while ((pent = readdir(pdir))) {
            if (is_current_or_parent(pent->d_name)) {
                continue;
            }

            g_string_printf(contents_path,
                            "%s/%s/%s/CONTENTS",
                            installed_base,
                            cent->d_name,
                            pent->d_name);
            GString *line = g_string_new(NULL);
            GError *error = NULL;
            GIOChannel *cfile =
                g_io_channel_new_file(contents_path->str, "r", &error);
            GIOStatus status;

            while ((status = g_io_channel_read_line_string(
                        cfile, line, NULL, &error)) != G_IO_STATUS_EOF) {
                assert(status == G_IO_STATUS_NORMAL);
                GString *type = g_string_new(g_strndup(line->str, 3));
                assert(type->str[0] != '/');

                gchar **sp = g_strsplit(line->str + 4, " ", 2);
                GString *path = g_string_new(g_strstrip(sp[0]));
                g_strfreev(sp);
                assert(path->len > 0);
                assert(path->str[0] == '/');

                g_hash_table_add(set, path);

                if (g_string_equal(s_obj, type) ||
                    g_string_equal(s_sym, type)) {
                    GString *py_compiled_type = g_string_new(path->str);
                    g_string_append_c(py_compiled_type, 'c');
                    g_hash_table_add(set, py_compiled_type);
                    g_string_assign(py_compiled_type, path->str);
                    g_string_append_c(py_compiled_type, 'o');
                    g_hash_table_add(set, py_compiled_type);
                    g_string_free(py_compiled_type, TRUE);
                }
            }
        }

        closedir(pdir);
    }

    g_string_free(s_obj, TRUE);
    g_string_free(s_sym, TRUE);
    g_string_free(contents_path, TRUE);
    g_string_free(pdir_path, TRUE);

    closedir(dirp);

    return set;
}

inline int strneq(const char *t, const char *u) {
    return strcmp(t, u) != 0;
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

void g_hash_table_add_all(GHashTable *target, GHashTable *src) {
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, src);
    gpointer file, _;
    while (g_hash_table_iter_next(&iter, &file, &_)) {
        printf("%d: %s\n", __LINE__, ((GString *)file)->str);
        g_hash_table_add(target, file);
    }
}

GHashTable *findwalk(const char *path, const GHashTable *package_files) {
    GHashTable *candidates =
        g_hash_table_new((GHashFunc)g_string_hash, (GEqualFunc)g_string_equal);
    assert(candidates);

    DIR *dir = opendir(path);
    assert(dir);
    struct dirent *cdir;
    while ((cdir = readdir(dir))) {
        if (is_current_or_parent(cdir->d_name)) {
            continue;
        }

        GString *ce = g_string_new(path);
        g_string_append_printf(ce, "/%s", cdir->d_name);

        // Whitelist check

        // package_files check
        if (!g_hash_table_contains((GHashTable *)package_files, ce)) {
            g_hash_table_add(candidates, ce);
        }

        // Continue if the entry is a directory
        struct stat s;
        lstat(ce->str, &s);
        if (S_ISDIR(s.st_mode) && !S_ISLNK(s.st_mode)) {
            GHashTable *next = findwalk(ce->str, package_files);
            assert(next);
            g_hash_table_add_all(candidates, next);
            g_hash_table_destroy(next);
        }

        g_string_free(ce, TRUE);
    }

    closedir(dir);

    return candidates;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("Finding files contained in packages...\n");
    GHashTable *package_files = find_files_in_packages(installed_base);
    assert(package_files);

    if (strneq(libmap, "lib64")) {
        g_hash_table_destroy(package_files);

        fprintf(stderr,
                "libmap option with value \"lib\" not yet supported.\n");

        return 1;
    }

    GHashTableIter iter;
    g_hash_table_iter_init(&iter, package_files);
    gpointer file, value;
    while (g_hash_table_iter_next(&iter, &file, &value)) {
        printf("%d: '%s'\n", __LINE__, ((GString *)value)->str);
    }

    // printf("Applying general exceptions...\n");

    //     printf("Finding files on system...\n");
    //
    //     const char *check_dirs[CHECK_DIRS_SIZE] = {
    //         "/bin",
    //         "/etc",
    //         "/lib",
    //         "/lib32",
    //         "/lib64",
    //         "/opt",
    //         "/sbin",
    //         "/usr",
    //         "/var",
    //     };

    //     for (unsigned int i = 0; i < CHECK_DIRS_SIZE; i++) {
    //         const char *dir = check_dirs[i];
    //
    //         struct stat s;
    //         stat(dir, &s);
    //         if (!S_ISDIR(s.st_mode) || S_ISLNK(s.st_mode)) {
    //             continue;
    //         }
    //
    //         GHashTable *candidates = findwalk(dir, package_files);
    //         assert(candidates);
    //
    //         GHashTableIter iter;
    //         g_hash_table_iter_init(&iter, candidates);
    //         gpointer file, value;
    //         while (g_hash_table_iter_next(&iter, &file, &value)) {
    //             printf("%d: %s\n", __LINE__, ((GString *)file)->str);
    //         }
    //
    //         g_hash_table_destroy(candidates);
    //     }

    g_hash_table_destroy(package_files);

    return 0;
}
