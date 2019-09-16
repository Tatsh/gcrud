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

// TODO Move these to a configuration file
/*
# Candidates

* Any directory with at least one .keep-* file inside if the package mentioned is installed
* /lib64/firmware/intel-ucode if sys-firmware/intel-microcode is installed
* /lib64/gentoo/functions.sh ?
* /lib64/modules/<active kernel>/
* /lib64/systemd/system/ - files that derive from systemd main package
* /var/tmp/portage/._unmerge_
* /var/tmp/systemd-private-*
* /var/lib/geoclue if geoclue is installed
* /var/lib/gitolite/ if gitolite is installed
* /var/lib/ip6tables/rules-save
* /var/lib/iptables/rules-save
* /var/lib/docker/ if Docker is installed
* /var/cache/edb/ ?
* /var/cache/fontconfig/ if fontconfig is installed
* /var/cache/eix/ if eix is installed
* /var/cache/genkernel if genkernel is installed
*/
int whitelist_check(const char *ce) {
    return g_str_has_prefix(ce, "/usr/portage/") ||
           g_str_equal("/usr/portage", ce) || g_str_equal("/bin/awk", ce) ||
           g_str_equal("/bin/sh", ce) || g_str_equal("/etc/adjtime", ce) ||
           g_str_equal("/etc/fstab", ce) || g_str_equal("/etc/group", ce) ||
           g_str_equal("/etc/group-", ce) || g_str_equal("/etc/gshadow", ce) ||
           g_str_equal("/etc/gshadow-", ce) ||
           g_str_equal("/etc/password", ce) ||
           g_str_equal("/etc/password-", ce) ||
           g_str_equal("/etc/shadow", ce) || g_str_equal("/etc/shadow-", ce) ||
           g_str_equal("/etc/mtab", ce) || g_str_equal("/etc/ntp.conf", ce) ||
           g_str_equal("/etc/hostname", ce) ||
           g_str_equal("/etc/ld.so.cache", ce) ||
           g_str_equal("/etc/ld.so.conf", ce) ||
           g_str_equal("/etc/ld.so.conf.d", ce) ||
           g_str_has_prefix(ce, "/etc/ld.so.conf.d/") ||
           g_str_equal("/etc/localtime", ce) ||
           g_str_equal("/etc/timezone", ce) ||
           g_str_equal("/etc/udev/hwdb.bin", ce) ||
           g_str_equal("/etc/locale.conf", ce) ||
           g_str_equal("/etc/prelink.conf.d/portage.conf", ce) ||
           (g_str_has_prefix(ce, "/etc/ssh/ssh_host_") &&
            (g_str_has_suffix(ce, "_key") ||
             g_str_has_suffix(ce, "_key.pub"))) ||
           g_str_equal("/lib/systemd/resolv.conf", ce) ||
           g_str_equal("/lib32/systemd/resolv.conf", ce) ||
           g_str_equal("/lib64/systemd/resolv.conf", ce) ||
           g_str_equal("/lib/ld-2.27.so", ce) ||
           g_str_equal("/lib32/ld-2.27.so", ce) ||
           g_str_equal("/lib64/ld-2.27.so", ce) ||
           g_str_has_prefix(ce, "/lib/ld-linux") ||
           g_str_has_prefix(ce, "/lib32/ld-linux") ||
           g_str_has_prefix(ce, "/lib64/ld-linux") ||
           g_str_equal("/usr/lib/debug", ce) ||
           g_str_equal("/usr/lib32/debug", ce) ||
           g_str_equal("/usr/lib64/debug", ce) ||
           (g_str_has_prefix(ce, "/usr/lib/debug") &&
            g_str_has_suffix(ce, ".debug")) ||
           (g_str_has_prefix(ce, "/usr/lib32/debug") &&
            g_str_has_suffix(ce, ".debug")) ||
           (g_str_has_prefix(ce, "/usr/lib64/debug") &&
            g_str_has_suffix(ce, ".debug")) ||
           g_str_has_prefix(ce, "/var/db/pkg/") ||
           g_str_equal("/var/db/pkg", ce) ||
           g_str_equal("/var/lib/portage", ce) ||
           g_str_has_prefix(ce, "/var/lib/portage/") ||
           g_str_equal("/var/lib/ntp", ce) ||
           g_str_equal("/var/lib/ntp/ntp.drift", ce) ||
           g_str_has_prefix(ce, "/var/lib/gentoo/news/");
}
