#include "sensor_filter.h"

#include <stdlib.h>
#include <string.h>

struct sensor_filter {
    sensor_filter_type type;
    int window_size;
    float alpha;
    float last_value;
    int count;
    int index;
    float *history;
    float *sorted;
};

static void insertion_sort(float *buf, int count)
{
    for (int i = 1; i < count; ++i) {
        float key = buf[i];
        int j = i - 1;
        while (j >= 0 && buf[j] > key) {
            buf[j + 1] = buf[j];
            j--;
        }
        buf[j + 1] = key;
    }
}

sensor_filter_t *sensor_filter_create(const sensor_filter_config *config)
{
    if (config == NULL) {
        return NULL;
    }

    sensor_filter_t *filter = (sensor_filter_t *)calloc(1, sizeof(sensor_filter_t));
    if (filter == NULL) {
        return NULL;
    }

    filter->type = config->type;
    filter->alpha = config->alpha > 0.0f ? config->alpha : 0.5f;
    filter->window_size = config->window_size > 0 ? config->window_size : 1;

    if (filter->type == SENSOR_FILTER_MOVING_AVG || filter->type == SENSOR_FILTER_MEDIAN) {
        filter->history = (float *)calloc((size_t)filter->window_size, sizeof(float));
        if (filter->history == NULL) {
            free(filter);
            return NULL;
        }
    }

    if (filter->type == SENSOR_FILTER_MEDIAN) {
        filter->sorted = (float *)calloc((size_t)filter->window_size, sizeof(float));
        if (filter->sorted == NULL) {
            free(filter->history);
            free(filter);
            return NULL;
        }
    }

    return filter;
}

void sensor_filter_destroy(sensor_filter_t *filter)
{
    if (filter == NULL) {
        return;
    }

    free(filter->sorted);
    free(filter->history);
    free(filter);
}

float sensor_filter_update(sensor_filter_t *filter, float sample)
{
    if (filter == NULL) {
        return sample;
    }

    switch (filter->type) {
    case SENSOR_FILTER_NONE:
        filter->last_value = sample;
        return sample;

    case SENSOR_FILTER_MOVING_AVG: {
        if (filter->window_size <= 1) {
            filter->last_value = sample;
            return sample;
        }

        filter->history[filter->index % filter->window_size] = sample;
        filter->index++;
        if (filter->count < filter->window_size) {
            filter->count++;
        }

        float sum = 0.0f;
        int count = filter->count;
        for (int i = 0; i < count; ++i) {
            int pos = (filter->index - count + i) % filter->window_size;
            if (pos < 0) {
                pos += filter->window_size;
            }
            sum += filter->history[pos];
        }

        filter->last_value = sum / (float)count;
        return filter->last_value;
    }

    case SENSOR_FILTER_IIR: {
        filter->last_value = filter->alpha * sample + (1.0f - filter->alpha) * filter->last_value;
        return filter->last_value;
    }

    case SENSOR_FILTER_MEDIAN: {
        if (filter->window_size <= 1) {
            filter->last_value = sample;
            return sample;
        }

        filter->history[filter->index % filter->window_size] = sample;
        filter->index++;
        if (filter->count < filter->window_size) {
            filter->count++;
        }

        int count = filter->count;
        for (int i = 0; i < count; ++i) {
            int pos = (filter->index - count + i) % filter->window_size;
            if (pos < 0) {
                pos += filter->window_size;
            }
            filter->sorted[i] = filter->history[pos];
        }

        insertion_sort(filter->sorted, count);
        if (count % 2 == 0) {
            filter->last_value = (filter->sorted[count / 2 - 1] + filter->sorted[count / 2]) * 0.5f;
        } else {
            filter->last_value = filter->sorted[count / 2];
        }

        return filter->last_value;
    }

    default:
        filter->last_value = sample;
        return sample;
    }
}

float sensor_filter_get_last(sensor_filter_t *filter)
{
    if (filter == NULL) {
        return 0.0f;
    }

    return filter->last_value;
}
