#ifndef SENSOR_FILTER_H
#define SENSOR_FILTER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SENSOR_FILTER_NONE = 0,
    SENSOR_FILTER_MOVING_AVG,
    SENSOR_FILTER_IIR,
    SENSOR_FILTER_MEDIAN
} sensor_filter_type;

typedef struct {
    sensor_filter_type type;
    int window_size;
    float alpha;
} sensor_filter_config;

typedef struct sensor_filter sensor_filter_t;

sensor_filter_t *sensor_filter_create(const sensor_filter_config *config);
void sensor_filter_destroy(sensor_filter_t *filter);
float sensor_filter_update(sensor_filter_t *filter, float sample);
float sensor_filter_get_last(sensor_filter_t *filter);

#ifdef __cplusplus
}
#endif

#endif
