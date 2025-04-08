#include "testing_driver.h"
#include "string.h"
#include "stddef.h"
#include "stdint.h"

/* n = 0 -> default value 
   n = 1 -> turn off esp32
   n = 2 -> sleep mode
   n = 3 -> pause port data
   n = 4 -> connect to wifi
   n = 5 -> connect to bluetooth
   n = 6 -> turn on built-in LED on esp32
   n = 7 -> memory usage 
   n = 8 -> system specs
   n = 9 -> driver status
   n = 10 -> overdriver status
*/

uint32_t getData(void) {
    return 1;
}

void function(void *p) {
    uint32_t data = 0;

    while (1) {
        data = getData();
        
        switch(data) {
            case 1: 
                
                break;
            case 2:
                break;
            case 3: 
                break;
            case 4:
                break;
            case 5:
                break;
            case 6:
                break;
            case 7:
                break;
            case 8:
                break;
            case 9:
                break;
            case 10:
                break;
        }
    }
}






















