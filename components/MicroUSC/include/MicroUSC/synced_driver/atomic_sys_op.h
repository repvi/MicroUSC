#pragma once

#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DataStorageQueue DataStorageQueue;

typedef DataStorageQueue *SerialDataQueueHandler;

SerialDataQueueHandler createDataStorageQueue(const size_t len);

void destroyDataStorageQueue(SerialDataQueueHandler queue);

void dataStorageQueue_add(SerialDataQueueHandler queue, const uint32_t data);

uint32_t dataStorageQueue_top(SerialDataQueueHandler queue);

void dataStorageQueue_clean(SerialDataQueueHandler queue);

#ifdef __cplusplus
}
#endif