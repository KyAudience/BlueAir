#include <stdio.h>
#include <stdint.h>

#include "temperature_sensor.h"

#define DEMO_SAMPLE_COUNT 8

typedef struct {
    int raw_index;
} fake_platform_ctx_t;

static int fake_platform_read_raw(void *ctx, sensor_raw_t *value)
{
    fake_platform_ctx_t *platform_ctx = (fake_platform_ctx_t *)ctx;
    static const int32_t sequence[] = {420, 430, 438, 445, 442, 448, 455, 462, 460, 470};

    if (platform_ctx == NULL || value == NULL) {
        return -1;
    }

    *value = sequence[platform_ctx->raw_index % (int)(sizeof(sequence) / sizeof(sequence[0]))];
    platform_ctx->raw_index++;
    return 0;
}

static void gui_update_cb(const sensor_t *sensor, float value, void *user_data)
{
    (void)user_data;
    printf("[GUI] %s = %.2f C\n", sensor->name, value);
}

static void mqtt_publish_cb(const sensor_t *sensor, float value, void *user_data)
{
    (void)user_data;
    printf("[MQTT] topic=sensor/%s payload=%.2f\n", sensor->name, value);
}

static void business_check_cb(const sensor_t *sensor, float value, void *user_data)
{
    (void)user_data;
    if (value > 25.0f) {
        printf("[BUSINESS] %s -> high temperature alert\n", sensor->name);
    }
}

int main(void)
{
    temperature_sensor_t temp_sensor;
    fake_platform_ctx_t ctx = {0};
    sensor_platform platform = {
        .ctx = &ctx,
        .ops = &(sensor_platform_ops){
            .ctx = NULL,
            .read_raw = fake_platform_read_raw
        }
    };

    sensor_filter_config cfg = {
        .type = SENSOR_FILTER_MOVING_AVG,
        .window_size = 5,
        .alpha = 0.5f
    };

    if (temperature_sensor_init(&temp_sensor, "TEMP_1", platform, SENSOR_FILTER_MOVING_AVG, &cfg) != 0) {
        printf("temperature sensor init failed\n");
        return 1;
    }

    sensor_register_callback(&temp_sensor.base, gui_update_cb, NULL);
    sensor_register_callback(&temp_sensor.base, mqtt_publish_cb, NULL);
    sensor_register_callback(&temp_sensor.base, business_check_cb, NULL);

    for (int i = 0; i < DEMO_SAMPLE_COUNT; ++i) {
        float temp_c = 0.0f;
        if (temperature_sensor_sample(&temp_sensor, &temp_c) != 0) {
            printf("sample failed\n");
            return 1;
        }
        printf("[SAMPLE] cycle=%d value=%.2f C\n", i + 1, temp_c);
    }

    return 0;
}
