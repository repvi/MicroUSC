#include "MicroUSC/internal/system/wireless_service.h"
#include "MicroUSC/internal/system/service_def.h"
#include "MicroUSC/internal/wireless/mqtt.h"
#include "MicroUSC/system/manager.h"

void send_microusc_system_mqtt_data(char *const data)
{
    if (eTaskGetState(microusc_system.task.main_task) != eSuspended) {
        union MicrouscSystemData_t sys_data;
        sys_data.data = data;

        if (uxQueueSpacesAvailable(microusc_system.queue_system.queue_handler) != 0) {
            taskENTER_CRITICAL(&microusc_system.critical_lock);
            {
                microusc_system.queue_system.count = 0;
            }
            taskEXIT_CRITICAL(&microusc_system.critical_lock);
            xQueueSend(microusc_system.queue_system.queue_handler, &sys_data.data, 0);
        }
        else {
            ESP_LOGW(TAG, "MicroUSC system queuehandler has overflowed");
            taskENTER_CRITICAL(&microusc_system.critical_lock);
            {
                microusc_system.queue_system.count++;
                if (microusc_system.queue_system.count == 3) {
                    microusc_queue_flush();
                }
            }
            taskEXIT_CRITICAL(&microusc_system.critical_lock);
        }
    }
}
