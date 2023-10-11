#include <esp_log.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "uuid.h"

static const char *TAG = "redtreat_config";

void delete_uuid() {
    struct stat uuid_stat;

    if (stat("/storage/uuid", &uuid_stat) == 0) {
        unlink("/storage/uuid");
    }
}

void get_uuid(char *uuid) {
    struct stat uuid_stat;
    FILE *f;

    if (stat("/storage/uuid", &uuid_stat) != 0) {
        // UUID does not exist yet..
        uuid_t new_uuid;
        uuid_generate(new_uuid);
        ESP_LOGI(TAG, "Generating new UUID");

        char uu_str[UUID_STR_LEN];
        uuid_unparse(new_uuid, uu_str);

        ESP_LOGV(TAG, "Opening UUID file");
        f = fopen("/storage/uuid", "w");

        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for writing");
            return;
        }

        fprintf(f, uu_str);
        fclose(f);
        ESP_LOGV(TAG, "UUID file written");
    }

    ESP_LOGV(TAG, "Opening UUID file");
    f = fopen("/storage/uuid", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }

    fgets(uuid, UUID_STR_LEN, f);
    fclose(f);

    ESP_LOGI(TAG, "UUID: %s", uuid);
}
