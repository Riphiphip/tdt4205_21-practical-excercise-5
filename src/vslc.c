#include <stdio.h>
#include <stdlib.h>
#include <vslc.h>


node_t *root;               // Syntax tree                  
tlhash_t *global_names;     // Symbol table        
char **string_list;         // List of strings in the source
size_t n_string_list = 8;   // Initial string list capacity (grow on demand)                                            
size_t stringc = 0;         // Initial string count



int
main ( int argc, char **argv )
{
    string_list = malloc(n_string_list * sizeof(char*));
    yyparse();
<<<<<<< HEAD
    simplify_tree(&root, root);
    node_print(root, 0);
=======
    simplify_tree ( &root, root );
    // node_print ( root, 0 );
>>>>>>> 5a7fbb42f594d3880ca137c1143a74a919343536

    create_symbol_table();
	print_symbol_table();

    destroy_subtree(root);
    destroy_symbol_table();

}
