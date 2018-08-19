#include "whitelist.h"

#include <glib-2.0/glib.h>

// TODO Move these to a configuration file
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
