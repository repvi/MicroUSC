#pragma once 

#ifdef __cplusplus
extern {
#endif

esp_err_t init_configuration_storage(void);

UBaseType_t getCurrentEmptyDriverIndex(void);

UBaseType_t getCurrentEmptyDriverIndexAndOccupy(void);

#ifdef __cplusplus
}
#endif