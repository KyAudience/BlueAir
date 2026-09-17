#include "sensor.h"

#include <stdio.h>
#include <string.h>

int sensor_init(sensor_t *sensor, const char *name, const sensor_config_t *config)
{
    if (sensor == NULL || config == NULL) {
        return -1;
    }

    memset(sensor, 0, sizeof(*sensor));
    snprintf(sensor->name, sizeof(sensor->name), "%s", name ? name : "sensor");
    sensor->platform = config->platform;
    sensor->scale = config->scale;
    sensor->offset = config->offset;
    sensor->last_value = 0.0f;
    sensor->initialized = 1;
    sensor->filter = sensor_filter_create(&config->filter_config);

    return sensor->filter == NULL ? -1 : 0;
}

int sensor_sample(sensor_t *sensor, float *value_out)
{
    if (sensor == NULL || sensor->platform.ops == NULL || sensor->platform.ops->read_raw == NULL) {
        return -1;
    }

    sensor_raw_t raw_value = 0;
    if (sensor->platform.ops->read_raw(sensor->platform.ctx, &raw_value) != 0) {
        return -1;
    }

    float converted = ((float)raw_value) * sensor->scale + sensor->offset;
    float filtered = sensor_filter_update(sensor->filter, converted);
    sensor->last_value = filtered;

    if (value_out != NULL) {
        *value_out = filtered;
    }

    sensor_notify_callbacks(sensor, filtered);
    return 0;
}

void sensor_register_callback(sensor_t *sensor, sensor_value_cb cb, void *user_data)
{
    if (sensor == NULL || cb == NULL || sensor->callback_count >= 4) {
        return;
    }

    sensor->callbacks[sensor->callback_count] = cb;
    sensor->callback_user_data[sensor->callback_count] = user_data;
    sensor->callback_count++;
}

void sensor_notify_callbacks(sensor_t *sensor, float value)
{
    if (sensor == NULL) {
        return;
    }

    for (size_t i = 0; i < sensor->callback_count; ++i) {
        if (sensor->callbacks[i] != NULL) {
            sensor->callbacks[i](sensor, value, sensor->callback_user_data[i]);
        }
    }
}
