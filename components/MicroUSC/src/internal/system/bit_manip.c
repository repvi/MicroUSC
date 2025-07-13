#include "MicroUSC/internal/system/bit_manip.h"
#include "MicroUSC/internal/USC_driver_config.h"
#include "esp_log.h"

#define TAG "[INTERNAL_BIT_MANIP]"

struct usc_bit_manip {
    UBaseType_t active_driver_bits;
    portMUX_TYPE critical_lock;
};

struct usc_bit_manip priority_storage;

/**
 * @brief Initializes the bit manipulation structure for driver priorities.
 *
 * Sets all driver bits to 0 (no drivers active) and initializes the critical section lock.
 *
 * @param bit_manip Pointer to the usc_bit_manip structure to initialize.
 * @return ESP_OK on success.
 */
static esp_err_t init_usc_bit_manip(struct usc_bit_manip *bit_manip)
{
    /* Set all driver bits to 0 (no drivers active) */
    bit_manip->active_driver_bits = 0;
    /* Initialize the critical section lock to unlocked */
    bit_manip->critical_lock = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;
    return ESP_OK;
}

esp_err_t init_configuration_storage(void)
{
    /* Initialize USC bit manipulation priority storage system */
    if (init_usc_bit_manip(&priority_storage) != ESP_OK) {
        /* Priority storage initialization failed */
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

//returns the first bit that is 0
UBaseType_t getCurrentEmptyDriverIndex(void)
{
    UBaseType_t driver_bits;
    /* Enter critical section to safely read active_driver_bits */
    portENTER_CRITICAL(&priority_storage.critical_lock);
    {
        driver_bits = priority_storage.active_driver_bits;
    }
    portEXIT_CRITICAL(&priority_storage.critical_lock);

    /* Scan each bit to find the first unset (available) bit */
    UBaseType_t v;
    for (UBaseType_t i = 0; i < DRIVER_MAX; i++) {
        v = BIT(i);
        if ((v & driver_bits) == 0) {
            /* Return the index of the first available bit */
            return i;
        }
    }
    return NOT_FOUND;
}

UBaseType_t getCurrentEmptyDriverIndexAndOccupy(void)
{
    /* Find the first available driver bit */
    const UBaseType_t v = getCurrentEmptyDriverIndex();
    if (v != NOT_FOUND) {
        UBaseType_t occupied_bits;
        /* Enter critical section to set the bit as occupied */
        portENTER_CRITICAL(&priority_storage.critical_lock);
        {
            priority_storage.active_driver_bits |= BIT(v); /* Mark bit as occupied */
            occupied_bits = priority_storage.active_driver_bits;
        }
        portEXIT_CRITICAL(&priority_storage.critical_lock);
        /* Log the new bitmask */
        ESP_LOGI(TAG, "Bit is now: %u", occupied_bits);
    }
    return v;
}