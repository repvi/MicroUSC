#include "logger.h"
#include "genList.h"
#include "stdint.h"
#include "esp_heap_caps.h"


#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define ALIGNOF(type) _Alignof(type)
#else
    #define ALIGNOF(type) __alignof__(type)
#endif

typedef struct {
    int total_occupied;
    char *buffer;
} logger_page_t;

typedef struct {
    logger_page_t page;
    struct list_head list;
} page_list;

struct logger_t {
    int total_pages;
    page_list pages;
};

const int LOGGER_SIZE_BASE = sizeof(struct logger_t); // Default size for the logger buffer

static logger_page_add(page_list *main, page_list *page) 
{
    list_add_tail(&page->list, &pages->list);
}

static void page_init(page_list *pages, uint8_t *memory, int page_amount, int page_size)
{
    /* align the base of our pages on a page_list boundary */
    const size_t alignment = ALIGNOF(page_list);
    memory = (uint8_t *)ALIGN_PTR(memory, alignment);

    INIT_LIST_HEAD(&pages->list);
    pages->page.total_occupied = 0;
    pages->page.buffer = (char *)memory;
    memory += page_size;

    for (int i = 0; i < page_amount; i++) {
        page_list *new_page = (page_list *)memory;
        memory += sizeof(page_list);
        new_page->page.buffer = (char *)memory;
        new_page->page.total_occupied = 0;
        memory += page_size; // Move memory pointer to the next page buffer
        logger_page_add(&pages->list, new_page);
    }
}

LoggerHandler logger_create(int page_amount, int page_size)
{
    const int alloc_size = LOGGER_SIZE_BASE 
    + ( ( page_size + sizeof( page_list ) - 1 ) * page_amount ) 
    + ( ALIGNOF( page_list ) - 1 ) + 20; // 20 bytes for safety margin

    void *memory = (LoggerHandler)heap_caps_malloc(alloc_size, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if (memory != NULL) {
        LoggerHandler logger = (LoggerHandler)memory;
        logger->total_pages = page_amount;
        uintptr_t ptr =  ((unsigned char *)logger + LOGGER_SIZE_BASE);
        page_init(&logger->pages, (uint8_t *)ptr, page_amount, page_size);
        return logger;
    } else {
        return NULL; // Memory allocation failed
    }
}

void logger_destroy(LoggerHandler logger)
{
    heap_caps_free(logger);
}

void 