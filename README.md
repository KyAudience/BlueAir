# BlueAir

这是一个基于 C 语言的跨 MCU 传感器框架，目标是让不同的 sensor（温度、湿度、气压、光照等）可以统一采样、统一滤波，并且让上层 UI、MQTT、业务逻辑彼此解耦。

## 1. 项目定位

BlueAir 不是某个单一 MCU 的驱动层代码，而是一个可复用的通用嵌入式传感器框架。它适用于：

- STM32 / ESP32 / Linux host / 模拟平台
- 温度、湿度、压力、光照等传感器
- 工业、智能家居、环境监测、IoT 设备

设计目标：

- 把 MCU 差异从业务逻辑中抽离
- 把不同传感器的量程换算抽象统一
- 把滤波算法变成可插拔组件
- 让 UI、MQTT、业务逻辑都依赖回调而非直接耦合

---

## 2. 架构概览

```text
+-------------------+
|  Application      |
|  UI / MQTT /      |
|  Business Logic   |
+---------+---------+
          |
          | callback
          v
+-------------------+
| Sensor Core       |
|  - sample         |
|  - convert        |
|  - filter         |
|  - notify         |
+---------+---------+
          |
          v
+-------------------+
| Platform Layer    |
|  ADC / GPIO / I2C |
|  read_raw()       |
+-------------------+
```

分层说明：

1. Platform Layer：底层 MCU/驱动接口，负责读取原始值
2. Sensor Core：负责统一采样、转换、滤波、派发
3. Application：UI 显示、MQTT 上报、报警逻辑等业务代码

---

## 3. 代码结构说明

当前仓库中的核心文件：

- `include/sensor_platform.h`：平台抽象
- `include/sensor_filter.h`：滤波算法接口
- `include/sensor.h`：通用 sensor 结构体和回调注册接口
- `include/temperature_sensor.h`：温度传感器类型
- `src/sensor.c`：通用采样与回调分发
- `src/sensor_filter.c`：滤波器实现
- `src/temperature_sensor.c`：温度传感器初始化与采样
- `src/demo.c`：示例用例，演示 GUI/MQTT/业务回调

---

## 4. 温度 / 湿度传感器参数是怎么滤波的

### 4.1 采样链路

采样链路从底层原始值到可用工程值，是这样的：

```c
sensor_raw_t raw_value = 0;
if (sensor->platform.ops->read_raw(sensor->platform.ctx, &raw_value) != 0) {
    return -1;
}

float converted = ((float)raw_value) * sensor->scale + sensor->offset;
float filtered = sensor_filter_update(sensor->filter, converted);
```

这段代码表明：

1. 读取原始原始值 `raw_value`
2. 调用 `scale` 和 `offset` 做工程值转换
3. 调用滤波器 `sensor_filter_update()` 修正噪声
4. 输出最终值 `filtered`

### 4.2 温度换算参数

当前温度示例使用：

```c
.scale = 0.05f;
.offset = -20.0f;
```

对应公式：

```c
temperature_c = raw * 0.05f - 20.0f;
```

它是一个示例公式，用来说明框架结构，不是某个真实传感器芯片的最终标定公式。

如果要扩展为湿度传感器，可用：

```c
sensor->scale = 0.1f;
sensor->offset = 0.0f;
```

也可根据 datasheet 中的真实换算公式去替换。关键点是：

- 传感器本身只负责“将 raw -> engineering value”
- 这部分逻辑独立于 UI 和 MQTT

### 4.3 滤波策略

当前框架支持 4 种滤波方式：

#### ① `SENSOR_FILTER_NONE`

不做滤波，直接输出原值。

```c
filter->last_value = sample;
return sample;
```

适合：

- 采样值稳定
- 需要保留原始波动
- 业务层自己处理抖动

#### ② `SENSOR_FILTER_MOVING_AVG`

滑动平均滤波：对最近 N 个样本求平均。

```c
average = sum(history) / count
```

适合：

- 温度、湿度等变化较慢的环境参数
- 去除随机噪声与采样抖动

示例中的配置：

```c
sensor_filter_config cfg = {
    .type = SENSOR_FILTER_MOVING_AVG,
    .window_size = 5,
    .alpha = 0.5f
};
```

含义：最近 5 个样本求平均，适合较平稳的温/湿度场景。

#### ③ `SENSOR_FILTER_IIR`

一阶低通 IIR：

```c
y[n] = alpha * x[n] + (1 - alpha) * y[n-1]
```

适合：

- 快速响应且需要平滑
- 低通滤波场景

其中 `alpha` 更大时响应更快、噪声更明显；`alpha` 更小时更平滑、但延迟更大。

#### ④ `SENSOR_FILTER_MEDIAN`

中值滤波：对窗口中的样本排序，取中间值。

适合：

- 抑制突发尖峰干扰
- 适合“偶发异常值”较多的场景

---

## 5. UI 显示怎么解耦的

关键在于：传感器层不关心 UI 是什么，也不关心显示组件如何实现。

传感器只负责：

- 采样
- 转换
- 滤波
- 通知所有注册者

### 5.1 回调注册

```c
sensor_register_callback(&temp_sensor.base, gui_update_cb, NULL);
sensor_register_callback(&temp_sensor.base, mqtt_publish_cb, NULL);
sensor_register_callback(&temp_sensor.base, business_check_cb, NULL);
```

这几项分别表示：

- `gui_update_cb`：更新界面控件
- `mqtt_publish_cb`：准备发送 MQTT 报文
- `business_check_cb`：业务计算、阈值判定、报警处理

### 5.2 回调分发

采样完毕后，所有回调都会被统一触发：

```c
sensor_notify_callbacks(sensor, filtered);
```

这样做的好处是：

- GUI 层只关心如何显示，不关心底层采样
- MQTT 层只关心如何发数据，不关心传感器的采样实现
- 业务层只关心阈值判定，不关心数据读取和矩阵转换

这是一种典型的观察者模式（Observer Pattern）。

---

## 6. 怎么发送给 MQTT 的

当前 `demo.c` 里是用打印语句模拟 MQTT：

```c
static void mqtt_publish_cb(const sensor_t *sensor, float value, void *user_data)
{
    (void)user_data;
    printf("[MQTT] topic=sensor/%s payload=%.2f\n", sensor->name, value);
}
```

意思就是：

- sensor 采样结束
- 回调被触发
- MQTT 处理函数拿到最终值
- 这里可以调用实际网络 API：

```c
mqtt_publish("sensor/TEMP_1", value);
```

或者：

```c
mqtt_client_publish(client, "sensor/TEMP_1", payload, qos, retain);
```

因此，真正的实现方式是：

- sensor 模块负责“把数据产出出来”
- MQTT 模块负责“真正发给 broker”
- 这两者之间通过回调完成解耦

---

## 7. 业务计算怎么做

业务逻辑同样走回调：

```c
static void business_check_cb(const sensor_t *sensor, float value, void *user_data)
{
    if (value > 25.0f) {
        printf("[BUSINESS] %s -> high temperature alert\n", sensor->name);
    }
}
```

这里的含义是：

- 业务模块直接看最终值 `value`
- 对阈值进行判断
- 可触发报警、控制风扇、调节设备状态等

它不依赖任何具体 MCU，完全独立于底层接口。 

---

## 8. 调用链总结

```text
raw ADC/I2C value
      |
      v
sensor_sample()
      |
      +--> scale / offset conversion
      |
      +--> filter_update()
      |
      v
sensor_notify_callbacks()
      |
      +--> GUI callback
      |
      +--> MQTT callback
      |
      +--> business callback
```

这就是当前代码的核心思路：

- 底层读数是目标平台相关
- 业务逻辑是应用相关
- 滤波算法是算法相关
- 他们都通过统一接口接在 sensor core 上

---

## 9. 扩展到温湿度 / 多传感器场景

如果以后要做温湿度组合传感器，只需要：

1. 继续复用 `sensor_t`
2. 创建多个 sensor 实例：`temp_sensor`、`humidity_sensor`
3. 分别给它们设置自己的：
   - `scale`
   - `offset`
   - `filter` 类型
   - `window_size` / `alpha`
4. 各自注册对应回调

例如：

```c
temperature_sensor_init(..., SENSOR_FILTER_MOVING_AVG, &temp_cfg);
humidity_sensor_init(..., SENSOR_FILTER_IIR, &hum_cfg);
```

这样可以实现：

- 温度和湿度独立采样
- 热/湿不同滤波参数
- GUI 分别显示不同数值
- MQTT 分别上报不同 topic
- 业务逻辑分别判断报警条件

---

## 10. 结论

BlueAir 当前这套代码的最核心价值是：

- 抽象底层平台差异
- 让滤波可插拔
- 让 UI、MQTT、业务逻辑解耦
- 让单个 sensor 框架能扩展到多种 MCU 和多种应用场景

如果将它进一步落地，未来可以扩展为：

- 温湿度综合监测模块
- 设备运行状态监控
- 局域网/云端 MQTT 采集
- 规则引擎报警与控制

它已经具备了“跨 MCU + 传感器框架 + 应用层解耦”的基本骨架。