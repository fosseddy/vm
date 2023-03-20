#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "vm.h"
#include "mem.h"

struct scanner {
    size_t cur;
    size_t start;
    char *src;
    size_t srclen;
};

enum token_kind {
    TOKEN_OPCODE,
    TOKEN_IMMEDIATE,
    TOKEN_LABEL
};

struct token {
    enum vm_opcode opcode;
    enum token_kind kind;
    uint16_t value;
    char *lexeme;
    size_t lexemelen;
};

struct tokens {
    size_t size;
    size_t cap;
    struct token *buf;
};

struct label {
    size_t addr;
    char *start;
    size_t len;
};

struct labels {
    size_t size;
    size_t cap;
    struct label *buf;
};

char *read_file(char *pathname)
{
    int fd;
    struct stat statbuf;
    char *src;

    fd = open(pathname, O_RDONLY);
    if (fd < 0) {
        perror("failed to open file");
        exit(1);
    }

    if (fstat(fd, &statbuf) < 0) {
        perror("failed to get file info");
        exit(1);
    }

    src = malloc(statbuf.st_size + 1);
    if (src == NULL) {
        perror("failed to allocate memory for source");
        exit(1);
    }

    if (read(fd, src, statbuf.st_size) < 0) {
        perror("failed read file content into source");
        exit(1);
    }

    src[statbuf.st_size] = '\0';

    close(fd);
    return src;
}

int has_src(struct scanner *s)
{
    return s->cur < s->srclen;
}

char peek(struct scanner *s)
{
    if (has_src(s) == 0) {
        return '\0';
    }

    return s->src[s->cur];
}

char advance(struct scanner *s)
{
    char c = peek(s);

    if (c != '\0') {
        s->cur++;
    }

    return c;
}

int next(struct scanner *s, char c)
{
    return peek(s) == c;
}

size_t lexeme_len(struct scanner *s)
{
    return s->cur - s->start;
}

char *lexeme_start(struct scanner *s)
{
    return s->src + s->start;
}

int compare_lexeme(struct scanner *s, char *word)
{
    size_t len = lexeme_len(s);

    if (strlen(word) != len) {
        return 0;
    }

    if (strncmp(lexeme_start(s), word, len) == 0) {
        return 1;
    }

    return 0;
}

void scan(struct scanner *s, struct tokens *toks, struct labels *ls)
{
    for (;;) {
        char c;

        if (has_src(s) == 0) {
            return;
        }

        s->start = s->cur;
        c = advance(s);

        switch (c) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            break;

        case ';':
            while (next(s, '\n') == 0 && has_src(s) == 1) {
                advance(s);
            }
            break;

        case '"':
            while (next(s, '"') == 0 && has_src(s) == 1) {
                advance(s);
            }

            if (has_src(s) == 0) {
                printf("unterminated string\n");
                exit(1);
            }

            for (size_t i = 1; i < lexeme_len(s); ++i) {
                struct token *t;
                char val = s->src[s->start + i];

                memgrow(toks, sizeof(struct token));
                t = toks->buf + toks->size;

                t->kind = TOKEN_IMMEDIATE;
                t->value = val;
                toks->size++;
            }

            advance(s);
            break;


        default:
            if (isalpha(c) != 0 || c == '_') {
                struct token *t;

                while(isalnum(peek(s)) != 0 || peek(s) == '_') {
                    advance(s);
                }

                if (peek(s) == ':') {
                    struct label *l;

                    memgrow(ls, sizeof(struct label));
                    l = ls->buf + ls->size;
                    l->addr = toks->size;
                    l->start = lexeme_start(s);
                    l->len = lexeme_len(s);
                    ls->size++;
                    advance(s);
                    break;
                }

                memgrow(toks, sizeof(struct token));
                t = toks->buf + toks->size;
                t->kind = TOKEN_OPCODE;

                if (compare_lexeme(s, "st") == 1) {
                    t->opcode = OP_ST;
                } else if (compare_lexeme(s, "ld") == 1) {
                    t->opcode = OP_LD;
                } else if (compare_lexeme(s, "push") == 1) {
                    t->opcode = OP_PUSH;
                } else if (compare_lexeme(s, "add") == 1) {
                    t->opcode = OP_ADD;
                } else if (compare_lexeme(s, "sub") == 1) {
                    t->opcode = OP_SUB;
                } else if (compare_lexeme(s, "mul") == 1) {
                    t->opcode = OP_MUL;
                } else if (compare_lexeme(s, "div") == 1) {
                    t->opcode = OP_DIV;
                } else if (compare_lexeme(s, "mod") == 1) {
                    t->opcode = OP_MOD;
                } else if (compare_lexeme(s, "inc") == 1) {
                    t->opcode = OP_INC;
                } else if (compare_lexeme(s, "dec") == 1) {
                    t->opcode = OP_DEC;
                } else if (compare_lexeme(s, "dup") == 1) {
                    t->opcode = OP_DUP;
                } else if (compare_lexeme(s, "over") == 1) {
                    t->opcode = OP_OVER;
                } else if (compare_lexeme(s, "swap") == 1) {
                    t->opcode = OP_SWAP;
                } else if (compare_lexeme(s, "drop") == 1) {
                    t->opcode = OP_DROP;
                } else if (compare_lexeme(s, "retpush") == 1) {
                    t->opcode = OP_RETPUSH;
                } else if (compare_lexeme(s, "retpop") == 1) {
                    t->opcode = OP_RETPOP;
                } else if (compare_lexeme(s, "retcopy") == 1) {
                    t->opcode = OP_RETCOPY;
                } else if (compare_lexeme(s, "retdrop") == 1) {
                    t->opcode = OP_RETDROP;
                } else if (compare_lexeme(s, "eq") == 1) {
                    t->opcode = OP_EQ;
                } else if (compare_lexeme(s, "gt") == 1) {
                    t->opcode = OP_GT;
                } else if (compare_lexeme(s, "lt") == 1) {
                    t->opcode = OP_LT;
                } else if (compare_lexeme(s, "or") == 1) {
                    t->opcode = OP_OR;
                } else if (compare_lexeme(s, "and") == 1) {
                    t->opcode = OP_AND;
                } else if (compare_lexeme(s, "not") == 1) {
                    t->opcode = OP_NOT;
                } else if (compare_lexeme(s, "jmp") == 1) {
                    t->opcode = OP_JMP;
                } else if (compare_lexeme(s, "ifjmp") == 1) {
                    t->opcode = OP_IFJMP;
                } else if (compare_lexeme(s, "halt") == 1) {
                    t->opcode = OP_HALT;
                } else if (compare_lexeme(s, "syscall") == 1) {
                    t->opcode = OP_SYSCALL;
                } else if (compare_lexeme(s, "call") == 1) {
                    t->opcode = OP_CALL;
                } else if (compare_lexeme(s, "ret") == 1) {
                    t->opcode = OP_RET;
                } else {
                    t->kind = TOKEN_LABEL;
                    t->lexeme = lexeme_start(s);
                    t->lexemelen = lexeme_len(s);
                }

                toks->size++;
                break;
            }

            if (isdigit(c) != 0) {
                struct token *t;
                char val[6];

                memgrow(toks, sizeof(struct token));
                t = toks->buf + toks->size;

                while(isdigit(peek(s)) != 0) {
                    advance(s);
                }

                if (lexeme_len(s) >= sizeof(val))  {
                    printf("value(%.*s) is greater than 16bit\n",
                           (int) lexeme_len(s),
                           lexeme_start(s));
                    exit(1);
                }

                memset(val, 0, sizeof(val));
                memcpy(val, lexeme_start(s), lexeme_len(s));
                t->kind = TOKEN_IMMEDIATE;
                t->value = atoi(val);
                toks->size++;
                break;
            }

            printf("ERROR: unknown character `%c`\n", c);
            exit(1);
        }
    }
}

int main(int argc, char **argv)
{
    struct scanner s;
    struct tokens toks;
    struct labels labels;
    FILE *out;

    argc--;
    argv++;

    if (argc < 1) {
        printf("assembly file is required\n");
        return 1;
    }

    // TODO: unhardcode outfile name
    out = fopen("a.out", "w");
    if (out == NULL) {
        printf("failed to open out file\n");
        return 1;
    }

    s.src = read_file(*argv);
    s.srclen = strlen(s.src);
    s.cur = 0;
    s.start = 0;

    meminit(&toks, sizeof(struct token), 64);
    meminit(&labels, sizeof(struct label), 32);

    scan(&s, &toks, &labels);

    {
        int start_found = 0;
        char *start = "_start";
        size_t len = strlen(start);

        for (size_t i = 0; i < labels.size; ++i) {
            struct label *l = labels.buf + i;

            if (l->len != len) {
                continue;
            }

            if (memcmp(l->start, start, len) == 0) {
                start_found = 1;
                fwrite(&l->addr, sizeof(uint16_t), 1, out);
                break;
            }
        }

        if (start_found == 0) {
            printf("_start entry point is missing\n");
            return 1;
        }
    }

    for (size_t i = 0; i < toks.size; ++i) {
        int label_found = 0;
        struct token *t = toks.buf + i;

        if (t->kind != TOKEN_LABEL) {
            continue;
        }

        for (size_t j = 0; j < labels.size; ++j) {
            struct label *l = labels.buf + j;

            if (t->lexemelen != l->len) {
                continue;
            }

            if (memcmp(t->lexeme, l->start, l->len) == 0) {
                label_found = 1;
                t->value = l->addr;
                break;
            }
        }

        if (label_found == 0) {
            printf("Undefined label %.*s\n", (int) t->lexemelen, t->lexeme);
            return 1;
        }
    }

    for (size_t i = 0; i < toks.size; ++i) {
        struct token *t = toks.buf + i;
        uint16_t val = t->opcode;

        if (t->kind != TOKEN_OPCODE) {
            val = t->value;
        }

        fwrite(&val, sizeof(uint16_t), 1, out);
    }

    return 0;
}
