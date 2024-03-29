// #include "lib/sstring.h"

// #include "vm.h"
// #include "linker/ln.h"

// #include "scanner.h"

struct parser {
    struct scanner s;
    struct tokens toks;
    struct token *tok;
    int cur;
};

struct symbol {
    enum ln_symkind kind;
    enum vm_section sec;
    string label;
    int addr;
    int is_resolved;
    int idx;
};

struct symtab {
    int len;
    int cap;
    int data_size;
    struct symbol *buf;
};

struct relocations {
    int len;
    int cap;
    int data_size;
    struct ln_relocation *buf;
};

struct operand {
    enum { OPERAND_NUM, OPERAND_SYM } kind;
    int size;
    union {
        int num;
        struct symbol *sym;
    } as;
};

struct mnemonic {
    enum vm_opcode opcode;
    struct operand operand;
};

struct text_label {
    int len;
    int cap;
    int data_size;
    struct mnemonic *buf;
};

struct data_label {
    int value_size;
    struct {
        int len;
        int cap;
        int data_size;
        int *buf;
    } values;
};

struct parsed_value {
    enum {
        PARSVAL_TEXT_LABEL = 0,
        PARSVAL_DATA_LABEL
    } kind;
    void *value;
};

struct parsed_values {
    int len;
    int cap;
    int data_size;
    struct parsed_value *buf;
};

void parse(struct parser *p, struct symtab *st, struct relocations *rels, struct parsed_values *values);
void make_parser(struct parser *p, char *filepath);
int with_operand(enum vm_opcode op);
