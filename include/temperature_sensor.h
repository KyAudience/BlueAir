#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include "sensor.h"

typedef struct {
    sensor_t base;
    float conversion_scale;
    float conversion_offset;
} temperature_sensor_t;

/*
 * 温度传感器初始化函数。
 * 用法示例：
 * temperature_sensor_t temp_sensor;
 * sensor_platform platform = { .ctx = &ctx, .ops = &platform_ops };
 * sensor_filter_config cfg = {
 *     .type = SENSOR_FILTER_MOVING_AVG,
 *     .window_size = 5,
 *     .alpha = 0.5f
 * };
 * temperature_sensor_init(&temp_sensor, "TEMP_1", platform, SENSOR_FILTER_MOVING_AVG, &cfg);
 */
int temperature_sensor_init(temperature_sensor_t *sensor,
                           const char *name,
                           sensor_platform platform,
                           sensor_filter_type filter_type,
                           const sensor_filter_config *filter_config);

/*
 * 采样一次温度并返回摄氏度值。
 * 用法：
 * float temp_c = 0.0f;
 * temperature_sensor_sample(&temp_sensor, &temp_c);
 */
int temperature_sensor_sample(temperature_sensor_t *sensor, float *temperature_c_out);

#endif
