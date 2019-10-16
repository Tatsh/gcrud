#include <stdbool.h>
#include <sys/stat.h>

#include <glib-2.0/glib.h>
#include <glib/gprintf.h>

#include "array_size.h"
#include "whitelist.h"

static GHashTable *package_installed_cache = NULL;
static GRegex *prefix_re = NULL;
static GRegex *filenames_re = NULL;
static GRegex *ssh_host_re = NULL;
static GRegex *lib_debug_re = NULL;
// TODO Move these to a configuration file
static const char *prefixes[] = {
    "/etc/ld.so.conf.d/",
    "/lib/ld-linux",
    "/lib32/ld-linux",
    "/lib64/ld-linux",
    "/usr/portage",
    "/var/db/pkg/",
    "/var/lib/gentoo/news/",
    "/var/lib/portage/",
};
static const char *filenames[] = {
    "/bin/awk",
    "/bin/sh",
    "/etc/adjtime",
    "/etc/fstab",
    "/etc/group",
    "/etc/group-",
    "/etc/gshadow",
    "/etc/gshadow-",
    "/etc/hostname",
    "/etc/ld.so.cache",
    "/etc/ld.so.conf",
    "/etc/ld.so.conf.d",
    "/etc/locale.conf",
    "/etc/localtime",
    "/etc/mtab",
    "/etc/ntp.conf",
    "/etc/password",
    "/etc/password-",
    "/etc/prelink.conf.d/portage.conf",
    "/etc/shadow",
    "/etc/shadow-",
    "/etc/timezone",
    "/etc/udev/hwdb.bin",
    "/lib/gentoo/functions.sh",
    "/lib/ld-2.27.so",
    "/lib/systemd/resolv.conf",
    "/lib32/gentoo/functions.sh",
    "/lib32/ld-2.27.so",
    "/lib32/systemd/resolv.conf",
    "/lib64/gentoo/functions.sh",
    "/lib64/ld-2.27.so",
    "/lib64/systemd/resolv.conf",
    "/usr/lib/debug",
    "/usr/lib32/debug",
    "/usr/lib64/debug",
    "/usr/portage",
    "/var/db/pkg",
    "/var/lib/ip6tables/rules-save",
    "/var/lib/iptables/rules-save",
    "/var/lib/ntp",
    "/var/lib/ntp/ntp.drift",
    "/var/lib/portage",
    "/var/tmp/portage/._unmerge_",
};
static const char *package_checks[] = {
    "/lib64/firmware/intel-ucode:/lib32/firmware/"
    "intel-ucode:/lib/firmware/intel-ucode|sys-firmware/intel-microcode",
    "/var/cache/eix|app-portage/eix",
    "/var/cache/fontconfig|media-libs/fontconfig",
    "/var/lib/docker|app-emulation/docker",
    "/var/lib/gitolite|dev-vcs/gitolite|dev-vcs/gitolite-gentoo",
    "/var/cache/genkernel|sys-kernel/genkernel",
};

static gboolean has_package_installed(const char *atom) {
    if (package_installed_cache == NULL) {
        package_installed_cache = g_hash_table_new_full(
            (GHashFunc)g_str_hash, (GEqualFunc)g_str_equal, g_free, NULL);
        g_assert_nonnull(package_installed_cache);
    }

    gpointer value;
    if (g_hash_table_lookup_extended(package_installed_cache, atom, NULL, &value)) {
      
    }

    if (g_hash_table_contains(package_installed_cache,
                              g_strdup_printf("%s:::1", atom))) {
        return true;
    }
    if (g_hash_table_contains(package_installed_cache,
                              g_strdup_printf("%s:::0", atom))) {
        return false;
    }

    bool ret = false;
    GDir *dirp = NULL;
    gchar *path_with_category = NULL;
    gchar **spl = g_strsplit(atom, "/", 2);
    path_with_category = g_strdup_printf("/var/db/pkg/%s", spl[0]);
    dirp = g_dir_open(path_with_category, 0, NULL);
    if (!dirp) {
        goto cleanup;
    }
    gchar *name = spl[1];
    const char *cent;

    while ((cent = g_dir_read_name(dirp))) {
        if (g_str_has_prefix(cent, name)) {
            ret = true;
            break;
        }
    }

cleanup:
    g_strfreev(spl);
    if (path_with_category) {
        g_free(path_with_category);
    }
    if (dirp) {
        g_dir_close(dirp);
    }

    g_hash_table_add(
        package_installed_cache,
        (gpointer)g_strdup_printf("%s%s", atom, ret ? ":::1" : ":::0"));

    return ret;
}

static gboolean whitelist_package_check(const char *ce) {
    gboolean found = false;
    for (size_t i = 0, l = ARRAY_SIZE(package_checks); i < l && !found; i++) {
        gchar **package_checks_spl = g_strsplit(package_checks[i], "|", 2);
        gchar **paths = g_strsplit(package_checks_spl[0], ":", 10);
        for (size_t j = 0; paths[j] != NULL; j++) {
            if (g_str_has_prefix(ce, paths[j])) {
                gchar **packages = g_strsplit(package_checks_spl[1], "|", 2);
                for (size_t k = 0; packages[k] != NULL; k++) {
                    if (has_package_installed(packages[k])) {
                        found = true;
                        break;
                    }
                }
                g_strfreev(packages);
            }
        }
        g_strfreev(package_checks_spl);
    }
    return found;
}

static gboolean whitelist_re_check(const char *ce) {
    if (prefix_re == NULL) {
        GString *pattern = g_string_new("");
        for (size_t i = 0, l = ARRAY_SIZE(prefixes); i < l; i++) {
            g_string_append(
                pattern,
                g_regex_escape_string(prefixes[i], (gint)strlen(prefixes[i])));
            if (i < (l - 1)) {
                g_string_append(pattern, "|");
            }
        }
        gchar *s = g_string_free(pattern, false);
        prefix_re = g_regex_new(s, G_REGEX_ANCHORED, 0, NULL);
        g_free(s);
    }
    if (filenames_re == NULL) {
        GString *pattern = g_string_new("");
        for (size_t i = 0, l = ARRAY_SIZE(filenames); i < l; i++) {
            g_string_append(pattern,
                            g_regex_escape_string(filenames[i],
                                                  (gint)strlen(filenames[i])));
            if (i < (l - 1)) {
                g_string_append(pattern, "|");
            }
        }
        gchar *s = g_string_free(pattern, false);
        filenames_re = g_regex_new(s, G_REGEX_ANCHORED, 0, NULL);
        g_free(s);
    }
    if (ssh_host_re == NULL) {
        GString *pattern =
            g_string_new("/etc/ssh/ssh_host_(?:[^_]+_)?key(?:\\.pub)?");
        gchar *s = g_string_free(pattern, false);
        ssh_host_re = g_regex_new(s, G_REGEX_ANCHORED, 0, NULL);
        g_free(s);
    }
    if (lib_debug_re == NULL) {
        GString *pattern = g_string_new("/usr/lib(?:32|64)?/debug.*\\.debug$");
        gchar *s = g_string_free(pattern, false);
        lib_debug_re = g_regex_new(s, G_REGEX_ANCHORED, 0, NULL);
        g_free(s);
    }

    return g_regex_match(prefix_re, ce, 0, NULL) ||
           g_regex_match(filenames_re, ce, 0, NULL) ||
           g_regex_match(ssh_host_re, ce, 0, NULL) ||
           g_regex_match(lib_debug_re, ce, 0, NULL);
}

/**
 * TODO Candidates:
 * * Any directory with at least one .keep-* file inside if the package
 * mentioned is installed (a `qatom` implementation is required to parse this)
 * * /lib64/modules/<active kernel>/
 * * /lib64/systemd/system/ - files that derive from systemd main package
 */
gboolean whitelist_check(const char *ce) {
    return g_str_has_prefix(ce, "/var/tmp/systemd-private-") ||
           whitelist_package_check(ce) || whitelist_re_check(ce);
}
