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

void list_add(struct list_head *new, struct list_head *head); // helper function

void list_add_tail(struct list_head *new, struct list_head *head);

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