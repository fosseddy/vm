#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "util.h"
#include "mem.h" // lib

struct directive {
    char *start;
    size_t len;
    char *name;
    size_t namelen;
    char *value;
    size_t valuelen;
};

struct directive_array {
    size_t size;
    size_t cap;
    struct directive *buf;
};

struct scanner {
    char *src;
    char *cur;
};

void scan(struct scanner *s, struct directive_array *dirs)
{
    while (*s->cur != '\0') {
        char *start = s->cur;

        if (*s->cur != '#') {
            s->cur++;
            continue;
        }

        s->cur++;
        while (isalpha(*s->cur) != 0) {
            s->cur++;
        }

        if (s->cur - start-1 == 6 && memcmp(start+1, "define", 6) == 0) {
            struct directive *d;

            memgrow(dirs, sizeof(struct directive));
            d = dirs->buf + dirs->size;
            dirs->size++;

            d->start = start;

            s->cur++;
            start = s->cur;
            while (isspace(*s->cur) == 0 && *s->cur != '\0') {
                s->cur++;
            }

            d->name = start;
            d->namelen = s->cur - start;

            s->cur++;
            start = s->cur;
            while (isspace(*s->cur) == 0 && *s->cur != '\0') {
                s->cur++;
            }

            d->value = start;
            d->valuelen = s->cur - start;
            d->len = s->cur - d->start;
            s->cur++;
            continue;
        }

        printf("Unknown directive %.*s\n", (int) (s->cur - start), start);
        exit(1);
    }
}

void process_directives(char *pathname, struct scanner *s,
                        struct directive_array *dirs)
{
    FILE *f;
    char *last_dir = NULL;

    f = fopen(pathname, "w");
    if (f == NULL) {
        perror("failed to open file");
        exit(1);
    }

    for (size_t i = 0; i < dirs->size; ++i) {
        struct directive *d = dirs->buf + i;
        char *end = d->start + d->len;

        if (last_dir == NULL) {
            last_dir = end;
            continue;
        }

        if (end > last_dir) {
            last_dir = end;
        }
    }

    while (*s->cur != '\0') {
        if (s->cur < last_dir) {
            // remove declaration
            for (size_t i = 0; i < dirs->size; ++i) {
                struct directive *d = dirs->buf + i;

                if (s->cur == d->start) {
                    s->cur += d->len;
                    break;
                }
            }
        }

        for (size_t i = 0; i < dirs->size; ++i) {
            struct directive *d = dirs->buf + i;

            if (*s->cur == *d->name) {
                char *start = s->cur, *ch = s->cur;

                while (isspace(*ch) == 0 && *ch != '\0') {
                    ch++;
                }

                if ((size_t) (ch - start) == d->namelen &&
                        memcmp(start, d->name, d->namelen) == 0) {
                    fwrite(d->value, d->valuelen, 1, f);
                    s->cur = ch;
                    break;
                }
            }
        }

        fwrite(s->cur, 1, 1, f);
        s->cur++;
    }

    fclose(f);
}

int main(int argc, char **argv)
{
    struct scanner s;
    struct directive_array dirs;
    char *outpath;

    argc--;
    argv++;

    if (argc < 1) {
        printf("assembly file is required\n");
        return 1;
    }

    outpath = create_outpath(*argv, "i"); // LEAK: os is freeing
    s.src = read_file(*argv); // LEAK: os is freeing
    s.cur = s.src;

    meminit(&dirs, sizeof(struct directive), 0); // LEAK: os is freeing

    scan(&s, &dirs);
    s.cur = s.src;
    process_directives(outpath, &s, &dirs);

    return 0;
}