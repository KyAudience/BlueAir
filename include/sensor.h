#ifndef SENSOR_H
#define SENSOR_H

#include <stddef.h>

#include "sensor_filter.h"
#include "sensor_platform.h"

typedef enum {
    SENSOR_EVENT_SAMPLE_READY = 0
} sensor_event_type;

typedef struct sensor sensor_t;

/*
 * 使用方法：
 * 1. 先实现底层 platform.read_raw()
 * 2. 构造 sensor_config_t，设置 scale/offset/filter_config
 * 3. 调用 sensor_init() 初始化 sensor
 * 4. 用 sensor_register_callback() 注册 UI / MQTT / 业务逻辑回调
 * 5. 调用 sensor_sample() 进行一次采样
 */
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

/*
 * 传感器初始化函数。
 * 用法：
 * sensor_config_t cfg = {
 *     .platform = platform,
 *     .filter_config = { .type = SENSOR_FILTER_MOVING_AVG, .window_size = 5, .alpha = 0.5f },
 *     .scale = 0.05f,
 *     .offset = -20.0f,
 * };
 * sensor_init(&sensor, "TEMP_1", &cfg);
 */
int sensor_init(sensor_t *sensor, const char *name, const sensor_config_t *config);

/*
 * 读取一次原始值并执行：转换 + 滤波 + 回调通知。
 * 调用方式：
 * float value = 0.0f;
 * sensor_sample(&sensor, &value);
 */
int sensor_sample(sensor_t *sensor, float *value_out);

/*
 * 注册一个回调，用于接收新的采样值。
 * 典型用途：
 * - GUI 显示
 * - MQTT 推送
 * - 报警 / 控制判断
 */
void sensor_register_callback(sensor_t *sensor, sensor_value_cb cb, void *user_data);

/*
 * 内部函数：向所有已注册回调发送当前值。
 * 一般由 sensor_sample() 自动调用，不建议业务代码直接调用。
 */
void sensor_notify_callbacks(sensor_t *sensor, float value);

#endif
