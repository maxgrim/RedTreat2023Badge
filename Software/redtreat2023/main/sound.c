#include <driver/i2s_std.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>

#include "config.h"

#define I2S_BLK_PIN GPIO_NUM_36
#define I2S_WS_PIN GPIO_NUM_37
#define I2S_DATA_OUT_PIN GPIO_NUM_35
#define I2S_SCLK_PIN I2S_GPIO_UNUSED
#define I2S_DATA_IN_PIN I2S_GPIO_UNUSED

#define AUDIO_BUFFER 2048  // buffer size for reading the wav file and sending to i2s

static const char *TAG = "redtreat_i2s";

static i2s_chan_handle_t tx_handle;

static void sound_task();

esp_err_t sound_init(void) {
    ESP_LOGI(TAG, "Setting up i2s");

    // setup a standard config and the channel
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

    // setup the i2s config
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),                                                   // the wav file sample rate
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),  // the wav faile bit and channel config
        .gpio_cfg = {
            // refer to configuration.h for pin setup
            .mclk = I2S_SCLK_PIN,
            .bclk = I2S_BLK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_DATA_OUT_PIN,
            .din = I2S_DATA_IN_PIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    return i2s_channel_init_std_mode(tx_handle, &std_cfg);
}

void sound_play(char *file_path) {
    ESP_LOGI(TAG, "Playing WAV file");

    FILE *fh = fopen(file_path, "rb");
    if (fh == NULL) {
        ESP_LOGE(TAG, "Failed to open file");
        return;
    }

    // Skip the WAV header
    fseek(fh, 44, SEEK_SET);

    // Create a writer buffer
    int16_t *buf = calloc(AUDIO_BUFFER, sizeof(int16_t));
    size_t bytes_read = 0;
    size_t bytes_written = 0;

    bytes_read = fread(buf, sizeof(int16_t), AUDIO_BUFFER, fh);

    i2s_channel_enable(tx_handle);

    while (bytes_read > 0) {
        // Write the buffer to the i2s
        i2s_channel_write(tx_handle, buf, bytes_read * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        bytes_read = fread(buf, sizeof(int16_t), AUDIO_BUFFER, fh);
        ESP_LOGV(TAG, "Bytes read: %d", bytes_read);
    }

    i2s_channel_disable(tx_handle);

    // Clear the buffer
    free(buf);
}
