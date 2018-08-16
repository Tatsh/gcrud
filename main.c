#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "colors.h"
#include "util.h"

// TODO Move to configuration
static const char *installed_base = "/var/db/pkg";
static const char *libmap = "lib64";

int main() {
#ifdef NDEBUG
    if (geteuid() != 0) {
        fprintf(stderr, "This must be run as root.\n");
        return 1;
    }
#endif

    fprintf(stderr, "Finding files contained in packages...\n");
    GHashTable *package_files = find_files_in_packages(installed_base);

    // TODO Support this option in config file like /etc/gcrud
    if (strcmp(libmap, "lib") != 0) {
        apply_lib_mapping(package_files, libmap);
    }

    fprintf(stderr, "Finding files on system...\n");

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

    for (unsigned int i = 0; i < sizeof(check_dirs) / sizeof(const char *);
         i++) {
        const char *dir = check_dirs[i];

        struct stat s;
        g_assert(stat(dir, &s) == 0);
        if (!S_ISDIR(s.st_mode) || S_ISLNK(s.st_mode)) {
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
