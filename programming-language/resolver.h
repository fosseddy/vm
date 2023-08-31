//#include "lib/sstring.h:
//#include "vmc.h:

struct scope;

typedef struct {
    struct scope *scope;
} Resolver;

void resolver_resolve(Resolver *r, Stmts *stmts);
void resolver_make(Resolver *r);
