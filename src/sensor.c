#include "sensor.h"

#include <stdio.h>
#include <string.h>

/*
 * 传感器初始化函数。
 *
 * 作用：
 *  - 清空对象
 *  - 保存平台接口和换算参数
 *  - 创建滤波器对象
 *  - 标记 sensor 已初始化
 *
 * 使用方法：
 * sensor_config_t cfg = {
 *    .platform = platform,
 *    .filter_config = { .type = SENSOR_FILTER_MOVING_AVG, .window_size = 5, .alpha = 0.5f },
 *    .scale = 0.05f,
 *    .offset = -20.0f,
 * };
 * sensor_init(&temp_sensor, "TEMP_1", &cfg);
 */
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

/*
 * 执行一次传感器采样。
 *
 * 处理流程：
 *  1. 调用底层 read_raw() 读取原始值
 *  2. 经过 scale + offset 换算成为工程值
 *  3. 经过滤波器处理得到 filtered
 *  4. 更新 last_value
 *  5. 通知所有已注册的回调
 *
 * 用法：
 * float temp_c = 0.0f;
 * sensor_sample(&sensor, &temp_c);
 */
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

/*
 * 注册回调的接口。
 *
 * 典型用途：
 * - UI 显示回调：更新页面或屏幕
 * - MQTT 回调：封装并发送 JSON / payload
 * - 业务回调：判断温度超阈值、控制风扇等
 *
 * 用法：
 * sensor_register_callback(&sensor, gui_update_cb, NULL);
 * sensor_register_callback(&sensor, mqtt_publish_cb, &mqtt_ctx);
 */
void sensor_register_callback(sensor_t *sensor, sensor_value_cb cb, void *user_data)
{
    if (sensor == NULL || cb == NULL || sensor->callback_count >= 4) {
        return;
    }

    sensor->callbacks[sensor->callback_count] = cb;
    sensor->callback_user_data[sensor->callback_count] = user_data;
    sensor->callback_count++;
}

/*
 * 内部分发函数。
 *
 * 作用：
 * 将处理后的 sensor 值发给所有注册的回调。
 * 一般由 sensor_sample() 在采样结束后自动调用。
 */
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
