#include <stdbool.h>
#include <sys/stat.h>

#include <glib-2.0/glib.h>
#include <glib/gprintf.h>

#include "whitelist.h"

bool has_package_installed(const char *atom) {
    bool ret = false;
    GDir *dirp = NULL;
    gchar *path_with_category = NULL;
    struct stat s;
    gchar **spl = g_strsplit(atom, "/", 2);
    gchar *full_path = g_strdup_printf("/var/db/pkg/%s", atom);
    if (stat(full_path, &s) == 0) {
        ret = true;
        goto cleanup;
    }

    path_with_category = g_strdup_printf("/var/db/pkg/%s", atom);
    dirp = g_dir_open(path_with_category, 0, NULL);
    gchar *name = spl[1];
    const char *cent;

    while ((cent = g_dir_read_name(dirp))) {
        if (g_str_has_prefix(name, cent)) {
            ret = true;
            goto cleanup;
        }
    }

cleanup:
    g_strfreev(spl);
    g_free(full_path);
    if (path_with_category) {
        g_free(path_with_category);
        g_dir_close(dirp);
    }

    return ret;
}

static GRegex *prefix_re = NULL;
static GRegex *filenames_re = NULL;
static GRegex *ssh_host_re = NULL;
static GRegex *lib_debug_re = NULL;

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
    "/lib/ld-2.27.so",
    "/lib/systemd/resolv.conf",
    "/lib32/ld-2.27.so",
    "/lib32/systemd/resolv.conf",
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
};

// TODO Move these to a configuration file
/*
# Candidates

* Any directory with at least one .keep-* file inside if the package mentioned
is installed
* /lib64/firmware/intel-ucode if sys-firmware/intel-microcode is installed
* /lib64/gentoo/functions.sh ?
* /lib64/modules/<active kernel>/
* /lib64/systemd/system/ - files that derive from systemd main package
* /var/tmp/portage/._unmerge_
* /var/tmp/systemd-private-*
* /var/lib/geoclue if geoclue is installed
* /var/lib/gitolite/ if gitolite is installed
* /var/lib/docker/ if Docker is installed
* /var/cache/edb/ ?
* /var/cache/fontconfig/ if fontconfig is installed
* /var/cache/eix/ if eix is installed
* /var/cache/genkernel if genkernel is installed
*/
int whitelist_check(const char *ce) {
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
