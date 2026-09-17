#include <stdio.h>
#include <stdint.h>

#include "temperature_sensor.h"

#define DEMO_SAMPLE_COUNT 8

/*
 * 模拟平台上下文。
 *
 * 作用：
 *  - 让当前代码能够脱离具体 MCU
 *  - 通过一个简单的数组模拟原始传感器读数
 *  - 便于验证滤波和回调机制
 *
 * 用法：
 * fake_platform_ctx_t ctx = {0};
 * sensor_platform platform = { .ctx = &ctx, .ops = &fake_platform_ops };
 */
typedef struct {
    int raw_index;
} fake_platform_ctx_t;

/*
 * 模拟底层 ADC/原始值读取函数。
 *
 * 在真实 MCU 上，这个函数会被替换成：
 * - ADC_Read() / HAL_ADC_GetValue()
 * - I2C 读取寄存器
 * - SPI 读取数据
 */
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

/*
 * GUI 更新回调。
 *
 * 作用：
 *  - 将最新值显示到屏幕 / 页面 / 控件上
 *  - 这里简单用 printf 代替真实 GUI 更新逻辑
 */
static void gui_update_cb(const sensor_t *sensor, float value, void *user_data)
{
    (void)user_data;
    printf("[GUI] %s = %.2f C\n", sensor->name, value);
}

/*
 * MQTT 发布回调。
 *
 * 作用：
 *  - 将最终 filtered 值准备成 MQTT payload
 *  - 在实际项目中替换为 mqtt_publish() / mqtt_client_publish()
 */
static void mqtt_publish_cb(const sensor_t *sensor, float value, void *user_data)
{
    (void)user_data;
    printf("[MQTT] topic=sensor/%s payload=%.2f\n", sensor->name, value);
}

/*
 * 业务逻辑回调。
 *
 * 作用：
 *  - 判断是否超过阈值
 *  - 触发警报、控制风扇、调节阀门等业务控制
 */
static void business_check_cb(const sensor_t *sensor, float value, void *user_data)
{
    (void)user_data;
    if (value > 25.0f) {
        printf("[BUSINESS] %s -> high temperature alert\n", sensor->name);
    }
}

/*
 * 使用方法：
 * 1. 定义平台对象和平台读取回调
 * 2. 初始化 temperature_sensor_t
 * 3. 注册 GUI / MQTT / business 回调
 * 4. 循环调用 temperature_sensor_sample()
 */
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
