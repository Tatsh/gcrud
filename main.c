#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <pbl.h>
#include <ustr.h>

#define CHECK_DIRS_SIZE 9

static const char *terminal_reset = "\e[0m";
static const char *terminal_bold = "\e[1m";
static const char *terminal_color_red = "\e[1;31m";

static const char *installed_base = "/var/db/pkg";
static const char *libmap = "lib64";

bool is_current_or_parent(const char *name) {
    return name[0] == '.' || (name[0] == '.' && name[1] == '.');
}

int pbl_set_compare_strings(const void *l, const void *r) {
    char *left = *(char **)l;
    char *right = *(char **)r;
    return strcmp(left, right);
}

PblSet *find_files_in_packages() {
    DIR *dirp = opendir(installed_base);
    struct dirent *cent, *pent;
    Ustr *pdir_path = USTR("");
    Ustr *contents_path = USTR("");
    PblSet *set = pblSetNewHashSet();
    pblSetSetHashValueFunction(set, pblSetStringHashValue);
    pblSetSetCompareFunction(set, &pbl_set_compare_strings);

    while ((cent = readdir(dirp))) {
        if (is_current_or_parent(cent->d_name)) {
            continue;
        }

        ustr_set_fmt(&pdir_path, "%s/%s", installed_base, cent->d_name);

        DIR *pdir = opendir(ustr_cstr(pdir_path));
        while ((pent = readdir(pdir))) {
            if (is_current_or_parent(pent->d_name)) {
                continue;
            }

            ustr_set_fmt(&contents_path,
                         "%s/%s/%s/CONTENTS",
                         installed_base,
                         cent->d_name,
                         pent->d_name);
            FILE *cfile = fopen(ustr_cstr(contents_path), "r");
            char buf_line[USTR_SIZE_FIXED(512)];
            Ustr *line = USTR_SC_INIT_AUTO(buf_line, USTR_FALSE, 0);

            while (ustr_io_getline(&line, cfile)) {
                size_t off = 0;
                Ustr *type = NULL;
                Ustr *path;
                Ustr *result;

                while ((result = ustr_split_spn_chrs(
                            line, &off, " ", 1, NULL, 0))) {
                    if (ustr_len(result) == 3) {
                        type = result;
                    } else if (ustr_cmp_prefix_cstr_eq(result, "/")) {
                        assert(type != NULL);
                        size_t off2 = 0;
                        path = ustr_split_spn_chrs(
                            result, &off2, "\n", 1, NULL, 0);

                        int ret = pblSetAdd(set, (void *)ustr_cstr(path));
                        assert(ret >= 0);
                        printf("add '%s' from line: '%s'\n",
                               ustr_cstr(path),
                               ustr_cstr(line));

                        if (ustr_cmp_cstr_eq(type, "obj") ||
                            ustr_cmp_cstr_eq(type, "sym")) {
                            if (ustr_cmp_suffix_cstr_eq(path, ".py")) {
                                Ustr *py_compiled_type = ustr_dup(path);
                                ustr_add_cstr(&py_compiled_type, "c");
                                pblSetAdd(set,
                                          (void *)ustr_cstr(py_compiled_type));
                                py_compiled_type = ustr_dup(path);
                                ustr_add_cstr(&py_compiled_type, "o");
                                pblSetAdd(set,
                                          (void *)ustr_cstr(py_compiled_type));
                            }
                        }

                        break;
                    }
                }

                // Required for the loop to continue
                if (line != USTR(buf_line)) {
                    ustr_sc_free2(&line,
                                  USTR_SC_INIT_AUTO(buf_line, USTR_FALSE, 0));
                }
            }

            fclose(cfile);
        }

        closedir(pdir);
    }

    closedir(dirp);

    return set;
}

static inline int strneq(const char *t, const char *u) {
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

PblSet *findwalk(const char *path, const PblSet *package_files) {
    PblSet *candidates = pblSetNewHashSet();
    pblSetSetHashValueFunction(candidates, pblSetStringHashValue);
    pblSetSetCompareFunction(candidates, &pbl_set_compare_strings);

    DIR *dir = opendir(path);
    struct dirent *cdir;
    while ((cdir = readdir(dir))) {
        if (is_current_or_parent(cdir->d_name)) {
            continue;
        }

        Ustr *ce = ustr_dup_cstr(path);
        ustr_add_fmt(&ce, "/%s", cdir->d_name);
        const char *cen = ustr_cstr(ce);

        // Whitelist check

        // package_files check
        if (!pblSetContains((PblSet *)package_files, (char *)cen)) {
            pblSetAdd(candidates, (void *)cen);
        }

        // Continue if the entry is a directory
        struct stat s;
        lstat(cen, &s);
        if (S_ISDIR(s.st_mode) && !S_ISLNK(s.st_mode)) {
            PblSet *next = findwalk(cen, package_files);
            pblSetAddAll(candidates, next);
            pblSetFree(next);
        }
    }

    closedir(dir);

    return candidates;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("Finding files contained in packages...\n");
    PblSet *package_files = find_files_in_packages();

    if (strneq(libmap, "lib64")) {
        pblSetFree(package_files);

        fprintf(stderr, "libmap option with value \"lib\" not supported.\n");

        return 1;
    }

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
    PblSet *candidates;

    for (unsigned int i = 0; i < CHECK_DIRS_SIZE; i++) {
        const char *dir = check_dirs[i];
        struct stat s;
        stat(dir, &s);
        if (!S_ISDIR(s.st_mode) || S_ISLNK(s.st_mode)) {
            continue;
        }
        candidates = findwalk(dir, package_files);

        PblIterator *it = pblSetIterator(candidates);
        char *file;
        while ((file = pblIteratorNext(it))) {
            // printf("%s\n", file);
            if (!pblIteratorHasNext(it)) {
                break;
            }
        }

        pblSetFree(candidates);
    }

    pblSetFree(package_files);

    return 0;
}
