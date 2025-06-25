/**
 * @file binary_tree.c
 * @brief Binary Search Tree implementation for string data with memory-efficient storage
 *
 * This module implements a binary search tree (BST) optimized for string storage. It uses
 * ESP32's heap capabilities for DMA-compatible memory allocation and flexible array members
 * for efficient string storage. The implementation provides creation, insertion, and cleanup
 * operations while maintaining BST properties.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct binary_tree *BinaryTreeString;

/**
 * @brief Creates a new binary search tree
 *
 * Allocates and initializes a new BST structure with DMA-capable memory.
 *
 * @return Pointer to new tree structure, or NULL if allocation fails
 */
struct binary_tree *binary_tree_create(void);

/**
 * @brief Destroys a binary tree and all its nodes
 *
 * Recursively frees all nodes and their associated string data,
 * then frees the tree structure itself.
 *
 * @param tree Pointer to tree to destroy
 */
void binary_tree_destroy(struct binary_tree *tree);

/**
 * @brief Inserts a string into the binary search tree
 *
 * Creates a new node containing a copy of the input string and inserts it
 * into the tree while maintaining BST properties. The string is stored in
 * memory directly following the node structure.
 *
 * @param tree Pointer to the binary tree
 * @param data String to insert (must be null-terminated)
 *
 * Memory layout per node:
 * [node_struct][string_data\0]
 *
 * @note Handles NULL inputs and empty strings gracefully
 * @note Uses DMA-capable memory for allocation
 */
void binary_tree_insert(struct binary_tree *tree, char *data);

#ifdef __cplusplus
}
#endif