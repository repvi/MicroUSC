#include "MicroUSC/chip_specific/system_attr.h"
#include "led_strip.h"
#include "esp_system.h"
#include "esp_log.h"

#define LED_STRIP_GPIO_PIN 48  // or 38, depending on your board
#define LED_STRIP_LED_COUNT 1  // Number of LEDs (1 for onboard)

#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000) // 10 MH


#define TAG "[LED]"

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGBcolor;

RGBcolor colorBase = {0};

led_strip_handle_t led_strip;

static void set_off(led_strip_handle_t led_s) 
{
    ESP_ERROR_CHECK(led_strip_clear(led_s));
    ESP_ERROR_CHECK(led_strip_refresh(led_s));
}

void init_builtin_led(void) 
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN,
        .max_leds = LED_STRIP_LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
        .flags.with_dma = false, // DMA not recommended for ESP32-S3 RMT[5]
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    
    set_off(led_strip);
    ESP_LOGI(TAG, "Initialized");
}

void set_led_color(RGBcolor colors)
{
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, colors.red, colors.green, colors.blue));
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

void builtin_led_set(bool state) 
{
    if (state) {
        ESP_LOGI(TAG, "Led is on");
        set_led_color(colorBase);
    }
    else {
        ESP_LOGI(TAG, "Led is off");
        set_off(led_strip);
    }
}

void setRGBcolors(RGBcolor *colors, uint8_t r, uint8_t g, uint8_t b)
{
    colors->blue = b;
    colors->green = g;
    colors->red = r;
}

#define setRGBbase(r, g, b) setRGBcolors(&colorBase, r, g, b)

void builtin_led_system(microusc_status status)
{
    bool builtin_led = true;
    switch (status) {
        case USC_SYSTEM_DEFAULT:
            builtin_led = false;
            break;
        case USC_SYSTEM_SLEEP:
            setRGBbase(0, 255, 255);
            break;
        case USC_SYSTEM_PAUSE:
            setRGBbase(255, 255, 0);
            break;
        case USC_SYSTEM_WIFI_CONNECT:
            setRGBbase(0, 255, 0);
            break;
        case USC_SYSTEM_BLUETOOTH_CONNECT:
            setRGBbase(255, 255, 0);
            break;
        case USC_SYSTEM_LED_ON:
            // do nothing
            break;
        case USC_SYSTEM_LED_OFF:
            builtin_led = false;
            break;
         case USC_SYSTEM_ERROR:
            setRGBbase(255, 0, 0);
            break;
        default:
            setRGBbase(255, 128,  0);
            break;
    }

    builtin_led_set(builtin_led);
}