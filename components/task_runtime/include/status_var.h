#ifndef __KERNEL_STATUS_H
#define __KERNEL_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Prints the configurations of all initialized drivers.
 * Iterates through each driver and logs its configuration details if it is initialized.
 */
extern void usc_print_driver_configurations(void);

/**
 * @brief Prints the configurations of all overdrivers.
 * Iterates through each overdriver and logs its configuration details if it is initialized.
 */
extern void usc_print_overdriver_configurations(void);

#ifdef __cplusplus
}
#endif

#endif // __KERNEL_STATUS_H