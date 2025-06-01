#pragma once 

#include "freertos/FreeRTOS.h"
#include "esp_system.h"

#ifdef __cplusplus
extern {
#endif

/**
 * @brief Initialize configuration storage system and allocate UART buffer
 * 
 * This function performs two critical initialization tasks:
 * 1. Initializes the USC bit manipulation priority storage system
 * 2. Allocates internal memory buffer for future UART operations
 * 
 * The function follows ESP-IDF error handling conventions, returning specific
 * error codes to indicate the type of failure that occurred.
 * 
 * @return esp_err_t Status of initialization
 * @retval ESP_OK               Success - all components initialized
 * @retval ESP_ERR_NO_MEM       Memory allocation failure (either priority storage or UART buffer)
 * 
 * @note UART buffer is allocated from internal SRAM for optimal performance
 * @note Buffer size is defined by buf_SIZE constant
 * @note Caller must ensure proper cleanup if function fails
 * 
 * @warning If this function fails, the system may be in an inconsistent state
 *          where priority storage is initialized but UART buffer is not allocated
 */
esp_err_t init_configuration_storage(void);

UBaseType_t getCurrentEmptyDriverIndex(void);

UBaseType_t getCurrentEmptyDriverIndexAndOccupy(void);

#ifdef __cplusplus
}
#endif