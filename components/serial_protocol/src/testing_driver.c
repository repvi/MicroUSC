#include "testing_driver.h"
#include "string.h"
// pvParameter doesn't need to be used.

int num = 20;

void test_driver_action_function(int index) {
    // index of the driver (0 - 1)

    // make sure this points to a function that will handle
    // the actions of the serial code that is recieved.
    while (1) {
        // check for data or something
        char *data = "iw4";
        if (strcmp(data, "iw4") == 0) { // code to do something
            // blink LED for exammple
        }
        else if (strcmp(data, "er3") == 0)  {
            // turn off LED
        }
    }
}






















