#include "testing_driver.h"
#include "string.h"
#include "stdint.h"

/*
    0 -> default value
    1 -> turn off esp32
    2 -> sleep mode
    3 -> pause port data
    4 -> connect to wifi
    5 -> connect to bluetooth

*/

uint32_t data_returner(void){
    return 1;
}

void get_data(void *p){
    uint32_t data = 0;

    while(1){
        data = data_returner();
        switch(data){
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
