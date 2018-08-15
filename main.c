#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "colors.h"
#include "util.h"

static const char *installed_base = "/var/db/pkg";
static const char *libmap = "lib64";

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

#ifdef NDEBUG
    if (geteuid() != 0) {
        fprintf(stderr, "This must be run as root.\n");
        return 1;
    }
#endif

    fprintf(stderr, "Finding files contained in packages...\n");
    GHashTable *package_files = find_files_in_packages(installed_base);
    g_assert_nonnull(package_files);

    // TODO Support this option in /etc/gcruft, etc
    if (strcmp(libmap, "lib64") != 0) {
        g_hash_table_destroy(package_files);

        fprintf(stderr,
                "libmap option with value \"lib\" not yet supported.\n");

        return 1;
    }

    // TODO
    // fprintf(stderr, "Applying general exceptions...\n");

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
        stat(dir, &s);
        if (!S_ISDIR(s.st_mode) || S_ISLNK(s.st_mode)) {
            continue;
        }

        GHashTable *candidates = findwalk(dir, package_files, g_free);
        g_assert_nonnull(candidates);

        GHashTableIter iter;
        gpointer file, value;

        g_hash_table_iter_init(&iter, candidates);
        while (g_hash_table_iter_next(&iter, &file, &value)) {
            g_assert_nonnull(&file);
            printf("%s\n", (gchar *)file);
        }

        g_hash_table_destroy(candidates);
    }

    g_hash_table_destroy(package_files);

    return 0;
}
