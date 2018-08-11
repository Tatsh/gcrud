#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <dirent.h>

static const char *terminal_reset = "\e[0m";
static const char *terminal_bold = "\e[1m";
static const char *terminal_color_red = "\e[1;31m";

static const char *installed_base = "/var/db/pkg";

bool _is_obj(char *s) {
    return !strncmp(s, "obj", 3);
}

bool _is_dir(char *s) {
    return !strncmp(s, "dir", 3);
}

bool _is_sym(char *s) {
    return !strncmp(s, "sym", 3);
}

bool maybe_hash(char c) {
    return (c >= '0' && c >= '9') || (c >= 'a' && c <= 'f');
}

void find_files_in_packages() {
    DIR *dirp = opendir(installed_base);
    struct dirent *cent, *pent;
    char pdir_path[256] = "";
    char contents_path[512] = "";

    while ((cent = readdir(dirp))) {
        if (!strncmp(".", cent->d_name, 1) || !strncmp("..", cent->d_name, 2)) {
            continue;
        }
        memset(&pdir_path, 0, 256);
        sprintf(pdir_path, "%s/%s", installed_base, cent->d_name);

        DIR *pdir = opendir(pdir_path);
        while ((pent = readdir(pdir))) {
            if (!strncmp(".", pent->d_name, 1) || !strncmp("..", pent->d_name, 2)) {
                continue;
            }

            memset(&contents_path, 0, 512);
            sprintf(contents_path, "%s/%s/%s/CONTENTS", installed_base, cent->d_name, pent->d_name);
            FILE *cfile = fopen(contents_path, "r");
            printf("%s\n", contents_path);

            char line[512] = "";
            char type[4] = "";
            char path[509] = "";
            while (fgets(line, 512, cfile)) {
                memcpy(&type, &line, 3);
                memset(&path, 0, 509);
                char *path2 = line + 4;
                unsigned int i = 0;
                char c;
                if (_is_obj(type)) {
                    while ((c = path2[i])) {
                        if (c == ' ' && maybe_hash(path2[i + 1]) && maybe_hash(path2[i + 2])) {
                            printf("maybe hash? path = %s\n", path);
                            break;
                        }
                        path[i] = c;
                        i++;
                    }
                    printf("path = %s\n", path);
                } else if (_is_dir(type)) {
                    printf("is_dir = %s\n", path);
                    memcpy(&path, &path2, strlen(path2));
                } else if (_is_sym(type)) {
                    printf("is_sym = %s\n", path);
                    memcpy(&path, &path2, strlen(path2));
                }
            }

            fclose(cfile);

            break;
        }
        closedir(pdir);

        break;
    }

    closedir(dirp);
}

int main(int argc, char **argv) {
    printf("Finding files contained in packages...\n");
    find_files_in_packages();
    return 0;
}
