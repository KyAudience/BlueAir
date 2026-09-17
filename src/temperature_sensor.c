#include "temperature_sensor.h"

#include <string.h>

int temperature_sensor_init(temperature_sensor_t *sensor,
                           const char *name,
                           sensor_platform platform,
                           sensor_filter_type filter_type,
                           const sensor_filter_config *filter_config)
{
    if (sensor == NULL) {
        return -1;
    }

    memset(sensor, 0, sizeof(*sensor));

    sensor_filter_config cfg = {0};
    if (filter_config != NULL) {
        cfg = *filter_config;
    }
    cfg.type = filter_type;

    if (cfg.type == SENSOR_FILTER_MOVING_AVG && cfg.window_size <= 0) {
        cfg.window_size = 5;
    }
    if (cfg.type == SENSOR_FILTER_IIR && cfg.alpha <= 0.0f) {
        cfg.alpha = 0.5f;
    }
    if (cfg.type == SENSOR_FILTER_MEDIAN && cfg.window_size <= 0) {
        cfg.window_size = 5;
    }

    sensor_config_t base_config = {
        .platform = platform,
        .filter_config = cfg,
        .scale = 0.05f,
        .offset = -20.0f
    };

    sensor->conversion_scale = 0.05f;
    sensor->conversion_offset = -20.0f;

    return sensor_init(&sensor->base, name, &base_config);
}

int temperature_sensor_sample(temperature_sensor_t *sensor, float *temperature_c_out)
{
    if (sensor == NULL) {
        return -1;
    }

    return sensor_sample(&sensor->base, temperature_c_out);
}
