#include <vslc.h>
#include <ir.h>
#include <tlhash.h>

// Externally visible, for the generator
extern tlhash_t *global_names;
extern char **string_list;
extern size_t n_string_list, stringc;

uint64_t func_count = 0;
uint64_t scope_id = 0;
/* External interface */

void create_symbol_table(void)
{
    find_globals();
    size_t n_globals = tlhash_size(global_names);
    symbol_t *global_list[n_globals];
    tlhash_values(global_names, (void **)&global_list);
    for (size_t i = 0; i < n_globals; i++)
        if (global_list[i]->type == SYM_FUNCTION)
            bind_names(global_list[i], global_list[i]->node);
}

void print_symbol_table(void)
{
    print_symbols();
    print_bindings(root);
}

void print_symbols(void)
{
    printf("String table:\n");
    for (size_t s = 0; s < stringc; s++)
        printf("%zu: %s\n", s, string_list[s]);
    printf("-- \n");

    printf("Globals:\n");
    size_t n_globals = tlhash_size(global_names);
    symbol_t *global_list[n_globals];
    tlhash_values(global_names, (void **)&global_list);
    for (size_t g = 0; g < n_globals; g++)
    {
        switch (global_list[g]->type)
        {
        case SYM_FUNCTION:
            printf(
                "%s: function %zu:\n",
                global_list[g]->name, global_list[g]->seq);
            if (global_list[g]->locals != NULL)
            {
                size_t localsize = tlhash_size(global_list[g]->locals);
                printf(
                    "\t%zu local variables, %zu are parameters:\n",
                    localsize, global_list[g]->nparms);
                symbol_t *locals[localsize];
                tlhash_values(global_list[g]->locals, (void **)locals);
                for (size_t i = 0; i < localsize; i++)
                {
                    printf("\t%s: ", locals[i]->name);
                    switch (locals[i]->type)
                    {
                    case SYM_PARAMETER:
                        printf("parameter %zu\n", locals[i]->seq);
                        break;
                    case SYM_LOCAL_VAR:
                        printf("local var %zu\n", locals[i]->seq);
                        break;
                    }
                }
            }
            break;
        case SYM_GLOBAL_VAR:
            printf("%s: global variable\n", global_list[g]->name);
            break;
        }
    }
    printf("-- \n");
}

void print_bindings(node_t *root)
{
    if (root == NULL)
        return;
    else if (root->entry != NULL)
    {
        switch (root->entry->type)
        {
        case SYM_GLOBAL_VAR:
            printf("Linked global var '%s'\n", root->entry->name);
            break;
        case SYM_FUNCTION:
            printf("Linked function %zu ('%s')\n",
                   root->entry->seq, root->entry->name);
            break;
        case SYM_PARAMETER:
            printf("Linked parameter %zu ('%s')\n",
                   root->entry->seq, root->entry->name);
            break;
        case SYM_LOCAL_VAR:
            printf("Linked local var %zu ('%s')\n",
                   root->entry->seq, root->entry->name);
            break;
        }
    }
    else if (root->type == STRING_DATA)
    {
        size_t string_index = *((size_t *)root->data);
        if (string_index < stringc)
            printf("Linked string %zu\n", *((size_t *)root->data));
        else
            printf("(Not an indexed string)\n");
    }
    for (size_t c = 0; c < root->n_children; c++)
        print_bindings(root->children[c]);
}

void destroy_symbol_table(void)
{
    destroy_symtab(global_names);

    // Now clean up the global list of strings
    for (int i = 0; i < stringc; i++)
        free(string_list[i]);
    free(string_list);
}

void find_globals(void)
{
    // Initialize the global symbol table because apparently that wasn't done in the skeleton code 😡
    global_names = (tlhash_t *)malloc(sizeof(tlhash_t));
    // Not expecting a massive amount of globals
    tlhash_init(global_names, 32);

    node_t *node = root;

    // The root node should point to the global list pretty much immediately
    while (node->type != GLOBAL_LIST)
    {
        node = root->children[0];
    }

    // Globals are the children of the GLOBAL_LIST
    for (int i = 0; i < node->n_children; i++)
    {
        node_t *global_node = node->children[i];
        switch (global_node->type)
        {
        case DECLARATION:
        {
            // Look for variable lists inside global variable declarations
            for (int j = 0; j < global_node->n_children; j++)
            {
                // Look for variable lists inside global variable declarations
                for (int j = 0; j < global_node->n_children; j++)
                {
                    node_t *global_child = global_node->children[j];
                    if (global_child->type != VARIABLE_LIST)
                        continue;

                    // Add all global identifiers to the symbol table
                    for (int k = 0; k < global_child->n_children; k++)
                    {
                        node_t *identifier = global_child->children[k];
                        symbol_t *symbol = (symbol_t *)malloc(sizeof(symbol_t));
                        symbol->name = strdup(identifier->data);
                        symbol->type = SYM_GLOBAL_VAR;
                        symbol->node = identifier;
                        symbol->locals = NULL;

                        // Insert the symbol into the globals symbol table
                        tlhash_insert(global_names, symbol->name, strlen(symbol->name), symbol);

                        // Update the node to have a pointer to its symbol table entry
                        identifier->entry = symbol;
                    }
                }
                break;
            }
            break;
        }

        case FUNCTION:
        {
            // Function declarations should look for a function name identifier
            // Finding parameter symbols is left for bind_names, although we *could* do it here
            for (int j = 0; j < global_node->n_children; j++)
            {
                node_t *global_child = global_node->children[j];
                // Function name
                if (global_child->type == IDENTIFIER_DATA)
                {
                    symbol_t *func_symbol = (symbol_t *)malloc(sizeof(symbol_t));
                    func_symbol->name = strdup(global_child->data);
                    func_symbol->type = SYM_FUNCTION;
                    func_symbol->seq = func_count++;
                    func_symbol->node = global_node;
                    func_symbol->nparms = 0;
                    // Alloc and init a new hashtable for function locals
                    func_symbol->locals = (tlhash_t *)malloc(sizeof(tlhash_t));
                    tlhash_init(func_symbol->locals, 64);

                    // Insert into the globals table
                    tlhash_insert(global_names, func_symbol->name, strlen(func_symbol->name) + 1, func_symbol);

                    global_child->entry = func_symbol;
                    break;
                }
            }
            break;
        }
        }
    }
}

void bind_names(symbol_t *function, node_t *root)
{
    node_t *param_list = root->children[1];
    int n_params = 0;
    if (param_list != NULL)
    {
        n_params = param_list->n_children;
        function->nparms = n_params;
        for (int i = 0; i < n_params; i++)
        {
            node_t *param_node = param_list->children[i];
            symbol_t *param = malloc(sizeof(symbol_t));
            param->name = strdup(param_node->data);
            param->type = SYM_PARAMETER;
            param->seq = i;
            param->nparms = 0;
            param->locals = NULL;
            param->node = param_node;
            param_node->entry = param;
            tlhash_insert(function->locals, param->name, strlen(param->name) + 1, param);
        }
    }
    size_t *seq_number = calloc(1, sizeof(size_t));
    scope_id++;
    scope_frame scope;
    scope.depth = 1;
    scope.enclosing = NULL;
    scope.value = scope_id;
    scope_id++;
    int result = bind_declarations(function, root, seq_number, &scope);
    free(seq_number);
    if (result != 0)
    {
        printf("Couldn't bind local variables\n");
        return;
    }
}

/**
 * Traverses syntax tree to find all local variables. Also updates string table.
 **/
int bind_declarations(symbol_t *function, node_t *root, size_t *seq_num, scope_frame *scope_stack)
{
    if (root == NULL)
    {
        return 0;
    }
    if (root->type == BLOCK)
    {
        scope_frame new_scope;
        new_scope.enclosing = scope_stack;
        new_scope.value = scope_id;
        new_scope.depth = scope_stack->depth + 1;
        scope_id++;
        scope_stack = &new_scope;
    }

    for (int i = 0; i < root->n_children; i++)
    {
        if (bind_declarations(function, root->children[i], seq_num, scope_stack))
        {
            return -1;
        };
    }

    switch (root->type)
    {
    case DECLARATION:
    {
        node_t *var_list = root->children[0];
        for (int i = 0; i < var_list->n_children; i++)
        {
            node_t *id_data = var_list->children[i];
            symbol_t *var = malloc(sizeof(symbol_t));
            var->name = strdup(id_data->data);
            var->type = SYM_LOCAL_VAR;
            var->seq = *seq_num;
            var->nparms = 0;
            var->locals = NULL;
            var->node = id_data;
            id_data->entry = var;
            void *key = get_id_key(scope_stack, var->name);
            uint64_t key_len = get_key_length(scope_stack, var->name);
            tlhash_insert(function->locals, key, key_len, var);
            free(key);
            *seq_num += 1;
        }
        break;
    }
    case STRING_DATA:
    {
        if (stringc >= n_string_list)
        {
            if (string_list == NULL)
            {
                return -1;
            }

            n_string_list *= 2;
            string_list = realloc(string_list, n_string_list * sizeof(char *));
        }
        string_list[stringc] = root->data;
        root->data = malloc(sizeof(size_t));
        *(size_t *)root->data = stringc;
        stringc += 1;
        break;
    }
    case IDENTIFIER_DATA:
    {
        void *key = get_id_key(scope_stack, root->data);
        uint64_t key_len = get_key_length(scope_stack, root->data);
        void **symbol_ptr = malloc(sizeof(symbol_t **));
        int result = tlhash_lookup(function->locals, key, key_len, symbol_ptr);
        if (result == TLHASH_ENOENT)
        {
            result = tlhash_lookup(global_names, root->data, strlen(root->data), symbol_ptr);
        }
        free(key);
        root->entry = *symbol_ptr;
        free(symbol_ptr);
        break;
    }
    }
    return 0;
}

void *get_id_key(scope_frame *scope, char *id)
{
    void *key = malloc(get_key_length(scope, id));
    scope_frame *s = scope;
    for (int i = 0; s != NULL; i++)
    {
        *(uint64_t *)((size_t)key + i * sizeof(uint64_t)) = s->value;
        s = s->enclosing;
    }
    strcpy((char*)((size_t)key + scope->depth * sizeof(uint64_t)), id);
    return key;
}

uint64_t get_key_length(scope_frame *scope, char *id)
{
    return sizeof(uint64_t) * scope->depth + strlen(id) + 1;
}

void destroy_symtab(tlhash_t *symtab)
{
    // tlhash_size returns the amount of elements in the hash table, each of which has an associated (string) key
    size_t symtable_size = tlhash_size(symtab);
    symbol_t **symbols = (symbol_t **)calloc(symtable_size, sizeof(symbol_t *));
    tlhash_values(symtab, (void **)symbols);

    for (int k = 0; k < symtable_size; k++)
    {
        symbol_t *symbol = symbols[k];

        // Remove the reference to the symbol from the corresponding node
        symbol->node->entry = NULL;
        // Free all allocated data from the symbol
        free(symbol->name);
        // Recursively destroy local symtabs
        if (symbol->locals != NULL)
        {
            destroy_symtab(symbol->locals);
        }

        // Free the symbol itself
        free(symbol);
    }

    free(symbols);
    // Finally, finalize and free the symtable itself
    tlhash_finalize(symtab);
    free(symtab);
}
