#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "MicroUSC/system/memory_pool.h"
#include "MicroUSC/internal/USC_driver_config.h"
#include "MicroUSC/internal/genList.h"
#include "MicroUSC/internal/uscdef.h"
#include "esp_err.h"

#ifdef LIST_HEAD
#undef LIST_HEAD
#endif

struct usc_driverList {
    struct usc_driver_t driver;
    struct list_head list;
};
 
 struct usc_driversHandler {
     struct usc_driverList driver_list;
     size_t size;
     size_t max; // need to initial with size already, do not change size
     SemaphoreHandle_t lock;
};

extern struct usc_driversHandler driver_system;

extern memory_block_handle_t mem_block_driver_nodes;

void addSingleDriver(const struct usc_driver_t *driver, const UBaseType_t priority);

void removeSingleDriver(struct usc_driverList *item);

void freeDriverList(void);

bool getTask_status(const struct usc_task_manager_t *task);

void setTask_status(struct usc_task_manager_t *task, bool active);

void setTaskHandlersNULL(struct usc_task_manager_t *task);

void setTaskDefault(struct usc_task_manager_t *task);

#define WAIT_FOR_RESPONSE            ( pdMS_TO_TICKS( 1000 ) )

esp_err_t init_driver_list_memory_pool(void);

esp_err_t init_hidden_driver_lists(void);

/**
 * @brief Set the driver settings to default values, only used for initializing
 * @param driver Pointer to driver that will be initialized
 * @return ESP_OK if everything went ok
 * 
**/
esp_err_t set_driver_default(struct usc_driver_t *driver);

/**
 *
**/
esp_err_t set_driver_default_task(struct usc_driver_t *driver);

/**
 * @brief Deactivate a driver
 * @param driver Pointer to driver that will be deactivated completelty as
 * its functions connected to it
**/
void set_driver_inactive(struct usc_driver_t *driver);

#ifdef __cplusplus
}
#endif