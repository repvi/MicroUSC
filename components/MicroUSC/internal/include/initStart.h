#ifndef __INITSTART_MICRO_USC_H
#define __INITSTART_MICRO_USC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "USC_driver_config.h" // could be renamed in the future
#include "uscdef.h"
#include "memory_pool.h"
#include "esp_err.h"    

typedef struct usc_driverNode usc_driverNode; // maybe not needed

struct usc_driverList {
    struct usc_driverNode *head;
    struct usc_driverNode *tail;
    size_t size;
    size_t max; // need to initial with size already, do not change size
    SemaphoreHandle_t lock;
    // maybe add status enum
};

bool getTask_status(const struct usc_task_manager_t *task);

void setTask_status(struct usc_task_manager_t *task, bool active);

void setTaskHandlersNULL(struct usc_task_manager_t *task);

void setTaskDefault(struct usc_task_manager_t *task);

struct usc_driver_t *getDriverListDriverAtTail(void);

struct usc_driver_t *getOverdriverListOverdriverAtTail(void);

UBaseType_t getNodePriority(usc_driverNode *node);

struct usc_driverNode *driverListNext(struct usc_driverNode *node);

struct usc_driver_t *getDriverfromNode(struct usc_driverNode *node);

#define WAIT_FOR_RESPONSE            ( pdMS_TO_TICKS( 1000 ) )

#define DEFINE_DRIVERLIST_CYCLE(x, name) \
    bool hasSemaphore = false; /* defined inside the macro */ \
    for(struct usc_driverNode *current = x; current != NULL; current = driverListNext(current)) \
        for (struct usc_driver_t *name = getDriverfromNode(current); (hasSemaphore = xSemaphoreTake(name->sync_signal, SEMAPHORE_WAIT_TIME)) == pdTRUE; ) \


extern struct usc_driverList driver_list;
extern struct usc_driverList overdriver_list;


#define cycle_drivers() DEFINE_DRIVERLIST_CYCLE(driver_list.head, driver)

#define cycle_overdrivers() DEFINE_DRIVERLIST_CYCLE(overdriver_list.head, overdriver)

esp_err_t init_driver_list(void);

esp_err_t init_overdriver_list(void);

esp_err_t init_hidden_driver_lists(void);

esp_err_t addDriverNode(const struct usc_driver_t *driver);

esp_err_t addOverdriverNode(const struct usc_driver_t *overdriver);

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
esp_err_t set_driver_inactive(struct usc_driver_t *driver);

#define DEFINE_USC_OVERDRIVER_INIT(driver_type) DEFINE_USC_DRIVER_INIT(driver_type)

#ifdef __cplusplus
}
#endif

#endif