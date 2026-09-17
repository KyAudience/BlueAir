#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include "sensor.h"

typedef struct {
    sensor_t base;
    float conversion_scale;
    float conversion_offset;
} temperature_sensor_t;

int temperature_sensor_init(temperature_sensor_t *sensor,
                           const char *name,
                           sensor_platform platform,
                           sensor_filter_type filter_type,
                           const sensor_filter_config *filter_config);
int temperature_sensor_sample(temperature_sensor_t *sensor, float *temperature_c_out);

#endif
