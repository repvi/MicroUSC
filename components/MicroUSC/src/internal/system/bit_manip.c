#include "MicroUSC/internal/system/bit_manip.h"
#include "USC_driver_config.h"

struct usc_bit_manip {
    UBaseType_t active_driver_bits;
    portMUX_TYPE critical_lock;
};

struct usc_bit_manip priority_storage;


static esp_err_t init_usc_bit_manip(struct usc_bit_manip *bit_manip)
{
    bit_manip->active_driver_bits = 0;
    bit_manip->critical_lock = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;
    return ESP_OK;
}

esp_err_t init_configuration_storage(void)
{
    // Initialize USC bit manipulation priority storage system
    // This must succeed before proceeding with buffer allocation
    if (init_usc_bit_manip(&priority_storage) != ESP_OK) {
        return ESP_ERR_NO_MEM;  // Priority storage initialization failed
    }

    return ESP_OK;
}

//returns the first bit that is 0
UBaseType_t getCurrentEmptyDriverIndex(void)
{
    UBaseType_t driver_bits;
    portENTER_CRITICAL(&priority_storage.critical_lock);
    {
        driver_bits = priority_storage.active_driver_bits;
    }
    portEXIT_CRITICAL(&priority_storage.critical_lock);
    UBaseType_t v;
    for (UBaseType_t i = 0; i < DRIVER_MAX; i++) { // there can be alternate code for this function, could have performance difference between the two possibly
        v = BIT(i);
        if ((v & driver_bits) == 0) {
            return i; // returns empty bit
        }
    }
    return NOT_FOUND;
}

UBaseType_t getCurrentEmptyDriverIndexAndOccupy(void)
{
    const UBaseType_t v = getCurrentEmptyDriverIndex();
    if (v != NOT_FOUND) {
        UBaseType_t occupied_bits;
        portENTER_CRITICAL(&priority_storage.critical_lock);
        {
            priority_storage.active_driver_bits |= BIT(v); // bit is now occupied
            occupied_bits = priority_storage.active_driver_bits;
        }
        portEXIT_CRITICAL(&priority_storage.critical_lock);
        ESP_LOGI(TAG, "Bit is now: %u", occupied_bits);
    }
    return v;
}