#include <stdio.h>
#include <sys/stat.h>
#ifdef NDEBUG
// For geteuid()
#include <sys/types.h>
#include <unistd.h>
#endif

#include <libmount/libmount.h>

#include "array_size.h"
#include "colors.h"
#include "util.h"
#include "whitelist.h"

// TODO Move to configuration
static const char *installed_base = "/var/db/pkg";
static const char *libmap = "lib64";
static const char *version = "v0.1.1";
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

static inline struct libmnt_table *get_mountinfo_table() {
    struct libmnt_table *tb = mnt_new_table_from_file("/proc/self/mountinfo");
    g_assert_nonnull(tb);
    struct libmnt_cache *cache = mnt_new_cache();
    g_assert_nonnull(cache);
    mnt_table_set_cache(tb, cache);
    mnt_unref_cache(cache);
    return tb;
}

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
    struct libmnt_table *tb = get_mountinfo_table();
    fprintf(stderr, "Finding files on system...\n");

    for (size_t i = 0; i < ARRAY_SIZE(check_dirs); i++) {
        const char *dir = check_dirs[i];

        struct stat s;
        int ret = stat(dir, &s);
        if (ret != 0 || (!S_ISDIR(s.st_mode) || S_ISLNK(s.st_mode))) {
            continue;
        }

        struct findwalk_data fw = {dir, package_files, g_free, tb};
        GHashTable *candidates = findwalk(&fw);
        GHashTableIter iter;
        gpointer file;

        g_hash_table_iter_init(&iter, candidates);
        while (g_hash_table_iter_next(&iter, &file, NULL)) {
            g_assert_nonnull(&file);
            printf("%s\n", (gchar *)file);
        }

        g_hash_table_destroy(candidates);
    }

    g_hash_table_destroy(package_files);
    whitelist_cleanup();

    return 0;
}
