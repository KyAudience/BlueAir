#ifndef SENSOR_PLATFORM_H
#define SENSOR_PLATFORM_H

#include <stdint.h>

typedef int32_t sensor_raw_t;

typedef struct {
    void *ctx;
    int (*read_raw)(void *ctx, sensor_raw_t *value);
} sensor_platform_ops;

typedef struct {
    void *ctx;
    const sensor_platform_ops *ops;
} sensor_platform;

#endif
