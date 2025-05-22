#ifndef __KERNEL_STATUS_H
#define __KERNEL_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Prints the configurations of all initialized drivers.
 * Iterates through each driver and logs its configuration details if it is initialized.
 */
void usc_print_driver_configurations(void);

#ifdef __cplusplus
}
#endif

#endif // __KERNEL_STATUS_H