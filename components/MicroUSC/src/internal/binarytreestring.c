#include "MicroUSC/internal/binarytreestring.h"
#include "esp_heap_caps.h"
#include "string.h"

/**
 * @brief Node structure for the binary search tree
 *
 * Each node contains a string pointer, size information, and left/right child pointers.
 * The actual string data is stored immediately after the node structure in memory.
 */
struct binary_tree_node {
    char *data;
    unsigned int size;
    struct binary_tree_node *left;
    struct binary_tree_node *right;
};

/**
 * @brief Binary tree structure containing root and count information
 */
struct binary_tree {
    size_t count;
    struct binary_tree_node *root;
};

struct binary_tree *binary_tree_create(void) {
    struct binary_tree *tree = heap_caps_malloc(sizeof(struct binary_tree), MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if (tree) {
        tree->count = 0;
        tree->root = NULL;
    }
    return tree;
}

/**
 * @brief Recursively destroys nodes in a binary tree
 *
 * @param node Root node of subtree to destroy
 */
static void binary_tree_destroy_node(struct binary_tree_node *node) {
    if (node) {
        binary_tree_destroy_node(node->left);
        binary_tree_destroy_node(node->right);
        heap_caps_free(node);
    }
}

void binary_tree_destroy(struct binary_tree *tree) {
    if (tree) {
        binary_tree_destroy_node(tree->root);
        heap_caps_free(tree);
    }
}

void binary_tree_insert(struct binary_tree *tree, char *data) {
    if (tree == NULL || data == NULL) {
        return; // Handle null tree or data
    }

    int size = strlen(data);
    // Allocate memory for the new node
    if (size == 0) {
        return; // Handle empty string
    }

    /* Allocate memory for node and string data */
    struct binary_tree_node *new_node = heap_caps_malloc(
        sizeof(struct binary_tree_node) + size + 1, // +1 for the null terminator
        MALLOC_CAP_8BIT | MALLOC_CAP_DMA
    );
    
    if (!new_node) {
        return; // Handle memory allocation failure
    }

    /* Initialize new node */
    new_node->data = (char *)(new_node + 1);
    memcpy(new_node->data, data, size + 1); // Copy the string including the null terminator
    new_node->size = size + 1; // +1 for the null terminator
    new_node->left = NULL;
    new_node->right = NULL;

    if (tree->root != NULL) {
        /* Find insertion point */
        struct binary_tree_node *current = tree->root;
        struct binary_tree_node *previous = NULL;
        while (current) {
            previous = current;
            if (strcmp(data, current->data) < 0) {
                current = current->left;
            } else {
                current = current->right;
            }
        }


        /* Link new node to parent */
        if (strcmp(data, previous->data) < 0) {
            previous->left = new_node;
        } else {
            previous->right = new_node;
        }
    }
    else {
        tree->root = new_node;
    }
    tree->count++;
}