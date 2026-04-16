/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_trackball_accel

#include <limits.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/input_processor.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct trackball_accel_config {
    uint8_t type;
    size_t codes_len;
    const uint16_t *codes;
    uint16_t base_scale;
    uint16_t max_scale;
    uint16_t acceleration_threshold;
    uint16_t acceleration_cap;
    uint16_t reset_ms;
    uint8_t smoothing_percent;
};

struct trackball_accel_axis_state {
    int64_t last_ts_ms;
    uint32_t speed_counts_per_sec;
};

struct trackball_accel_data {
    struct trackball_accel_axis_state *states;
};

static int code_index(uint16_t code, const uint16_t *codes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (codes[i] == code) {
            return (int)i;
        }
    }

    return -ENODEV;
}

static struct trackball_accel_axis_state *
state_for(const struct trackball_accel_config *cfg, struct trackball_accel_data *data,
          uint8_t input_device_index, size_t code_idx) {
    size_t flat_idx = ((size_t)input_device_index * cfg->codes_len) + code_idx;
    return &data->states[flat_idx];
}

static uint32_t update_speed(const struct trackball_accel_config *cfg,
                             struct trackball_accel_axis_state *state, uint32_t abs_delta,
                             int64_t now_ms) {
    if (abs_delta == 0U) {
        state->last_ts_ms = now_ms;
        state->speed_counts_per_sec = 0U;
        return 0U;
    }

    if (state->last_ts_ms == 0) {
        state->last_ts_ms = now_ms;
        state->speed_counts_per_sec = 0U;
        return 0U;
    }

    int64_t dt_ms = now_ms - state->last_ts_ms;
    state->last_ts_ms = now_ms;

    if (dt_ms <= 0) {
        dt_ms = 1;
    }

    uint32_t instant_speed = (uint32_t)MIN(((int64_t)abs_delta * 1000LL) / dt_ms, INT32_MAX);

    if (dt_ms > cfg->reset_ms) {
        state->speed_counts_per_sec = instant_speed;
        return 0U;
    }

    if (state->speed_counts_per_sec == 0U) {
        state->speed_counts_per_sec = instant_speed;
        return instant_speed;
    }

    uint32_t smoothing = cfg->smoothing_percent;
    uint32_t retained = 100U - smoothing;
    uint32_t smoothed =
        ((instant_speed * smoothing) + (state->speed_counts_per_sec * retained)) / 100U;

    state->speed_counts_per_sec = smoothed;
    return smoothed;
}

static uint32_t scale_for_speed(const struct trackball_accel_config *cfg, uint32_t speed) {
    if (speed <= cfg->acceleration_threshold || cfg->max_scale <= cfg->base_scale) {
        return cfg->base_scale;
    }

    if (cfg->acceleration_cap <= cfg->acceleration_threshold || speed >= cfg->acceleration_cap) {
        return cfg->max_scale;
    }

    uint32_t progress = speed - cfg->acceleration_threshold;
    uint32_t speed_range = cfg->acceleration_cap - cfg->acceleration_threshold;
    uint32_t scale_range = cfg->max_scale - cfg->base_scale;

    return cfg->base_scale +
           (uint32_t)(((int64_t)progress * progress * scale_range) /
                      ((int64_t)speed_range * speed_range));
}

static int apply_scale(struct input_event *event, uint32_t scale_permille,
                       struct zmk_input_processor_state *state) {
    int64_t scaled_value = (int64_t)event->value * scale_permille;

    if (state != NULL && state->remainder != NULL) {
        scaled_value += *state->remainder;
    }

    int32_t integral = (int32_t)(scaled_value / 1000LL);

    if (state != NULL && state->remainder != NULL) {
        *state->remainder = (int16_t)(scaled_value - ((int64_t)integral * 1000LL));
    }

    event->value = CLAMP(integral, INT16_MIN, INT16_MAX);
    return ZMK_INPUT_PROC_CONTINUE;
}

static int trackball_accel_handle_event(const struct device *dev, struct input_event *event,
                                        uint32_t param1, uint32_t param2,
                                        struct zmk_input_processor_state *state) {
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);

    const struct trackball_accel_config *cfg = dev->config;
    struct trackball_accel_data *data = dev->data;

    if (event->type != cfg->type) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    int idx = code_index(event->code, cfg->codes, cfg->codes_len);
    if (idx < 0) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    uint8_t input_device_index = state != NULL ? state->input_device_index : 0U;
    if (input_device_index >= CONFIG_ZMK_TRACKBALL_ACCEL_MAX_INPUT_DEVICES) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    struct trackball_accel_axis_state *axis_state =
        state_for(cfg, data, input_device_index, (size_t)idx);
    uint32_t abs_delta =
        event->value < 0 ? (uint32_t)(-(int64_t)event->value) : (uint32_t)event->value;
    uint32_t speed = update_speed(cfg, axis_state, abs_delta, k_uptime_get());
    uint32_t scale = scale_for_speed(cfg, speed);

    LOG_DBG("code=%u value=%d speed=%u scale=%u", event->code, event->value, speed, scale);

    return apply_scale(event, scale, state);
}

static const struct zmk_input_processor_driver_api trackball_accel_driver_api = {
    .handle_event = trackball_accel_handle_event,
};

#define TRACKBALL_ACCEL_INST(n)                                                                    \
    static const uint16_t trackball_accel_codes_##n[] = DT_INST_PROP(n, codes);                   \
    static struct trackball_accel_axis_state trackball_accel_states_##n                            \
        [CONFIG_ZMK_TRACKBALL_ACCEL_MAX_INPUT_DEVICES][DT_INST_PROP_LEN(n, codes)] = {};          \
    static struct trackball_accel_data trackball_accel_data_##n = {                                \
        .states = &trackball_accel_states_##n[0][0],                                               \
    };                                                                                             \
    static const struct trackball_accel_config trackball_accel_config_##n = {                      \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                            \
        .codes_len = DT_INST_PROP_LEN(n, codes),                                                   \
        .codes = trackball_accel_codes_##n,                                                        \
        .base_scale = DT_INST_PROP_OR(n, base_scale, 1000),                                        \
        .max_scale = DT_INST_PROP_OR(n, max_scale, 2200),                                          \
        .acceleration_threshold = DT_INST_PROP_OR(n, acceleration_threshold, 150),                \
        .acceleration_cap = DT_INST_PROP_OR(n, acceleration_cap, 1400),                           \
        .reset_ms = DT_INST_PROP_OR(n, reset_ms, 40),                                              \
        .smoothing_percent = DT_INST_PROP_OR(n, smoothing_percent, 35),                            \
    };                                                                                             \
    BUILD_ASSERT(DT_INST_PROP_LEN(n, codes) > 0, "Trackball accel processor needs at least one code"); \
    BUILD_ASSERT(DT_INST_PROP_OR(n, smoothing_percent, 35) <= 100,                                \
                 "Trackball accel smoothing-percent must be 0..100");                             \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, &trackball_accel_data_##n, &trackball_accel_config_##n, \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                        \
                          &trackball_accel_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TRACKBALL_ACCEL_INST)
