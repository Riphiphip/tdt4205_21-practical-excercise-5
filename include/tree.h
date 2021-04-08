#pragma once
#include "vslc.h"

void node_print(node_t *root, int nesting);
void node_init(node_t *nd, node_index_t type, void *data, uint64_t n_children, ...);
void node_finalize(node_t *discard);
void destroy_subtree(node_t *discard);
void simplify_tree(node_t **simplified, node_t *root);
