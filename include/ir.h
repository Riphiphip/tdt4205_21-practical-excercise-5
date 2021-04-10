#ifndef IR_H
#define IR_H

#define LOCALS_BUCKET_COUNT 64
/* This is the tree node structure */
typedef struct n
{
    node_index_t type;
    void *data;
    struct s *entry;
    uint64_t n_children;
    struct n **children;
} node_t;

// Export the initializer function, it is needed by the parser
void node_init(
    node_t *nd, node_index_t type, void *data, uint64_t n_children, ...);

typedef enum
{
    SYM_GLOBAL_VAR,
    SYM_FUNCTION,
    SYM_PARAMETER,
    SYM_LOCAL_VAR
} symtype_t;

typedef struct s
{
    char *name;
    symtype_t type;
    node_t *node;
    size_t seq;
    size_t nparms;
    tlhash_t *locals;
} symbol_t;

typedef struct ssi
{
    struct ssi *enclosing;
    uint64_t value;
    uint64_t depth;
} scope_frame;

int bind_declarations(symbol_t *function, node_t *root, size_t *seq_num, scope_frame* scope_stack);
void create_symbol_table(void);
void print_symbol_table(void);
void print_symbols(void);
void print_bindings(node_t *root);
void destroy_symbol_table(void);
void find_globals(void);
void bind_names(symbol_t *function, node_t *root);
void destroy_symtab(tlhash_t *symtab);
void *get_id_key(scope_frame *scope, char *id);
uint64_t get_key_length(scope_frame *scope, char *id);
#endif
