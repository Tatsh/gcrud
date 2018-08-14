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
GHashTable *findwalk(const char *, const GHashTable *, GDestroyNotify)
    __attribute__((returns_nonnull, warn_unused_result));

bool is_current_or_parent(const char *name) {
    return name[0] == '.' || (name[0] == '.' && name[1] == '.');
}

GHashTable *find_files_in_packages(const char *base) {
    DIR *dirp = opendir(base);
    assert(dirp);
    struct dirent *cent, *pent;
    GHashTable *set = g_hash_table_new_full(
        (GHashFunc)g_str_hash, (GEqualFunc)g_str_equal, g_free, NULL);
    assert(set);

    while ((cent = readdir(dirp))) {
        if (is_current_or_parent(cent->d_name)) {
            continue;
        }

        gchar *pdir_path =
            g_strdup_printf("%s/%s", installed_base, cent->d_name);
        DIR *pdir = opendir(pdir_path);
        assert(pdir);

        while ((pent = readdir(pdir))) {
            if (is_current_or_parent(pent->d_name)) {
                continue;
            }

            gchar *cpath = g_strdup_printf("%s/%s/%s/CONTENTS",
                                           installed_base,
                                           cent->d_name,
                                           pent->d_name);

            gchar *line;
            GError *error = NULL;
            GIOChannel *cfile = g_io_channel_new_file(cpath, "r", &error);
            GIOStatus status;

            while ((status = g_io_channel_read_line(
                        cfile, &line, NULL, NULL, &error)) !=
                   G_IO_STATUS_EOF) {
                assert(status == G_IO_STATUS_NORMAL);
                gchar *type = g_strndup(line, 3);
                assert(type[0] != '/');

                gchar **sp = g_strsplit(line + 4, " ", 2);
                gchar *path = g_strdup(sp[0]);
                path = g_strstrip(path);
                assert(path[0] == '/');

                g_hash_table_add(set, path);

                if (g_str_equal("obj", type) || g_str_equal("sym", type)) {
                    gchar *py_compiled_type = g_strdup_printf("%sc", path);
                    g_hash_table_add(set, py_compiled_type);
                    py_compiled_type = g_strdup_printf("%so", path);
                    g_hash_table_add(set, py_compiled_type);
                }

                g_strfreev(sp);
            }

            g_free(cpath);
        }

        closedir(pdir);
        g_free(pdir_path);
    }

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
    assert(candidates != NULL);

    DIR *dir = opendir(path);
    assert(dir != NULL);
    struct dirent *cdir;
    while ((cdir = readdir(dir))) {
        if (is_current_or_parent(cdir->d_name)) {
            continue;
        }

        gchar *ce = g_strdup_printf("%s/%s", path, cdir->d_name);
        gboolean clean_up_ce = FALSE;
        assert(ce != NULL);

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
            assert(next != NULL);
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

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("Finding files contained in packages...\n");
    GHashTable *package_files = find_files_in_packages(installed_base);
    assert(package_files);

    // TODO Support this option in /etc/gcruft, etc
    if (strneq(libmap, "lib64")) {
        g_hash_table_destroy(package_files);

        fprintf(stderr,
                "libmap option with value \"lib\" not yet supported.\n");

        return 1;
    }

    // TODO
    // printf("Applying general exceptions...\n");

    printf("Finding files on system...\n");

    const char *check_dirs[CHECK_DIRS_SIZE] = {
        "/bin",
        "/etc",
        "/lib",
        "/lib32",
        "/lib64",
        "/opt",
        "/sbin",
        "/usr",
        "/var",
    };

    for (unsigned int i = 0; i < CHECK_DIRS_SIZE; i++) {
        const char *dir = check_dirs[i];

        struct stat s;
        stat(dir, &s);
        if (!S_ISDIR(s.st_mode) || S_ISLNK(s.st_mode)) {
            continue;
        }

        GHashTable *candidates = findwalk(dir, package_files, g_free);
        assert(candidates);

        GHashTableIter iter;
        g_hash_table_iter_init(&iter, candidates);
        gpointer file, value;
        while (g_hash_table_iter_next(&iter, &file, &value)) {
            printf("%s\n", (gchar *)file);
        }

        g_hash_table_destroy(candidates);
    }

    g_hash_table_destroy(package_files);

    return 0;
}
