#include <stdio.h>
#include <sys/stat.h>

#ifdef NDEBUG
// For geteuid()
#include <sys/types.h>
#include <unistd.h>
#endif

#include "colors.h"
#include "util.h"

// TODO Move to configuration
static const char *installed_base = "/var/db/pkg";
static const char *libmap = "lib64";
static const char *version = "v0.1.0";
static const char *check_dirs[] = {
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

int main(int argc, char *argv[]) {
    if (argc == 2 &&
        (g_str_equal(argv[1], "-v") || g_str_equal(argv[1], "--version"))) {
        printf("%s\n", version);
        return 0;
    }

#ifdef NDEBUG
    if (geteuid() != 0) {
        fprintf(stderr, "This must be run as root.\n");
        return 1;
    }
#endif

    fprintf(stderr, "Finding files contained in packages...\n");
    GHashTable *package_files = find_files_in_packages(installed_base);

    // TODO Support this option in config file like /etc/gcrud
    if (!g_str_equal(libmap, "lib")) {
        apply_lib_mapping(package_files, libmap);
    }

    fprintf(stderr, "Finding files on system...\n");

    for (unsigned int i = 0; i < sizeof(check_dirs) / sizeof(const char *);
         i++) {
        const char *dir = check_dirs[i];

        struct stat s;
        int ret = stat(dir, &s);
        if (ret != 0 || (!S_ISDIR(s.st_mode) || S_ISLNK(s.st_mode))) {
            continue;
        }

        GHashTable *candidates = findwalk(dir, package_files, g_free);
        GHashTableIter iter;
        gpointer file, _;

        g_hash_table_iter_init(&iter, candidates);
        while (g_hash_table_iter_next(&iter, &file, &_)) {
            g_assert_nonnull(&file);
            printf("%s\n", (gchar *)file);
        }

        g_hash_table_destroy(candidates);
    }

    g_hash_table_destroy(package_files);

    return 0;
}
