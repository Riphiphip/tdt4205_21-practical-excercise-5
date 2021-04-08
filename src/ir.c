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
    destroy_symtab();
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

                    // Insert the symbol into the globals symbol table
                    tlhash_insert(global_names, symbol->name, strlen(symbol->name), symbol);

                    // Update the node to have a pointer to its symbol table entry
                    identifier->entry = symbol;
                }
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
                    tlhash_insert(global_names, func_symbol->name, strlen(func_symbol->name), func_symbol);

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
            tlhash_insert(function->locals, param->name, strlen(param->name), param);
        }
    }
    size_t *seq_number = calloc(1,sizeof(size_t));
    int result = bind_declarations(function, root, seq_number);
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
int bind_declarations(symbol_t *function, node_t *root, size_t* seq_num )
{
    if (root == NULL)
    {
        return 0;
    }
    if (root->type != DECLARATION && root->type != STRING_DATA)
    {
        for (int i = 0; i < root->n_children; i++)
        {
            bind_declarations(function, root->children[i], seq_num);
        }
        return 0;
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
            tlhash_insert(function->locals, seq_num, sizeof(size_t), var);
            *seq_num += 1;
        }
        break;
    }
    case STRING_DATA:
    {
        if (stringc >= n_string_list)
        {
            string_list = realloc(string_list, n_string_list * 2 * sizeof(char*));
            if (string_list == NULL)
            {
                return -1;
            }
            n_string_list *= 2;
        }
        string_list[stringc] = root->data;
        root->data = malloc(sizeof(stringc));
        *(size_t *)root->data = stringc;
        stringc += 1;
        break;
    }
    }
    return 0;
}

void destroy_symtab(void)
{
}
