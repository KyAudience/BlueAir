#ifndef SENSOR_H
#define SENSOR_H

#include <stddef.h>

#include "sensor_filter.h"
#include "sensor_platform.h"

typedef enum {
    SENSOR_EVENT_SAMPLE_READY = 0
} sensor_event_type;

typedef struct sensor sensor_t;

typedef void (*sensor_value_cb)(const sensor_t *sensor, float value, void *user_data);

struct sensor {
    char name[16];
    sensor_platform platform;
    sensor_filter_t *filter;
    float scale;
    float offset;
    float last_value;
    int initialized;
    sensor_value_cb callbacks[4];
    void *callback_user_data[4];
    size_t callback_count;
};

typedef struct {
    sensor_platform platform;
    sensor_filter_config filter_config;
    float scale;
    float offset;
} sensor_config_t;

int sensor_init(sensor_t *sensor, const char *name, const sensor_config_t *config);
int sensor_sample(sensor_t *sensor, float *value_out);
void sensor_register_callback(sensor_t *sensor, sensor_value_cb cb, void *user_data);
void sensor_notify_callbacks(sensor_t *sensor, float value);

#endif
