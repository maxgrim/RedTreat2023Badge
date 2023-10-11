#include "leds.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ws2812.h"

struct led_state state;

void leds_init(void) {
    ws2812_control_init();
}

void leds_blink(int color) {
    for (int i = 0; i < 3; i++) {
        leds_fill(color);
        vTaskDelay(250 / portTICK_PERIOD_MS);

        leds_fill(0);
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void leds_fill(int color) {
    for (int i = 0; i < sizeof(state) / sizeof(uint32_t); i++) {
        state.leds[i] = ((color & 0xFF) | (color >> 16 & 0xFF) << 8 | (color >> 8 & 0xFF) << 16);
    }

    ws2812_write_leds(state);
}
