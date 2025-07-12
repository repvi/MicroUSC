/**
 * @file genList.h
 * @brief Generic doubly-linked list manipulation routines for MicroUSC.
 *
 * This header defines a set of functions and structures for managing generic doubly-linked lists,
 * modeled after the Linux kernel’s list implementation. It enables efficient insertion and removal
 * of elements at both the head and tail of the list, as well as deletion of arbitrary entries.
 * These utilities are used internally by the MicroUSC system for managing dynamic data structures,
 * such as driver configurations and runtime lists.
 *
 * Usage:
 *   - Include this header in modules that require dynamic list management.
 *   - Use list_add() and list_add_tail() to insert elements at the head or tail.
 *   - Use list_del() to remove elements from the list.
 *
 * @note All list operations assume the list_head structures are properly initialized before use.
 *       These routines are designed for modular, component-based embedded C projects and support
 *       manual file management and CMake-based dependency organization.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

// Declare and initialize a list head
#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list) 
{
    list->next = list;
    list->prev = list;
}

/**
 * @brief Add a new entry after the specified head (at the beginning of the list).
 * 
 * @param new  Pointer to the new list_head to insert.
 * @param head Pointer to the list head after which the new entry will be inserted.
 */
void list_add(struct list_head *new, struct list_head *head);

/**
 * @brief Add a new entry before the specified head (at the end of the list).
 * 
 * @param new  Pointer to the new list_head to insert.
 * @param head Pointer to the list head before which the new entry will be inserted.
 */
void list_add_tail(struct list_head *new, struct list_head *head);

/**
 * @brief Remove an entry from the list and reset its pointers.
 * 
 * @param entry Pointer to the list_head to remove.
 *
 * After removal, entry->next and entry->prev are set to NULL.
 */
void list_del(struct list_head *entry);

// --- Iteration ---

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

// --- Accessing Container Structures ---

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ({ \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); })

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member), \
         n = list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_last_entry(head, type, member) \
    ((head)->prev != (head) ? list_entry((head)->prev, type, member) : NULL)


#ifdef __cplusplus
}
#endif