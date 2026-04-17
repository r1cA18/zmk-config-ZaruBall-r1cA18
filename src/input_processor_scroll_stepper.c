/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_stepper

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/input_processor.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct stepper_config {
    uint8_t type;
    size_t codes_len;
    uint16_t step_value;
    uint16_t step_threshold;
    const uint16_t *codes;
};

struct stepper_data {
    int16_t *accumulators;
};

static bool code_matches(uint16_t code, const uint16_t *codes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (codes[i] == code) {
            return true;
        }
    }

    return false;
}

static int code_index(uint16_t code, const uint16_t *codes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (codes[i] == code) {
            return (int)i;
        }
    }

    return -ENODEV;
}

static int stepper_handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                                uint32_t param2, struct zmk_input_processor_state *state) {
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);

    const struct stepper_config *cfg = dev->config;
    struct stepper_data *data = dev->data;

    if (event->type != cfg->type || event->value == 0 ||
        !code_matches(event->code, cfg->codes, cfg->codes_len)) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    int idx = code_index(event->code, cfg->codes, cfg->codes_len);
    if (idx < 0) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    uint8_t input_device_index = state != NULL ? state->input_device_index : 0U;
    size_t flat_idx = ((size_t)input_device_index * cfg->codes_len) + (size_t)idx;
    int16_t *acc = &data->accumulators[flat_idx];
    *acc += event->value;

    if (*acc >= (int16_t)cfg->step_threshold) {
        event->value = cfg->step_value;
        *acc -= (int16_t)cfg->step_threshold;
    } else if (*acc <= -((int16_t)cfg->step_threshold)) {
        event->value = -((int16_t)cfg->step_value);
        *acc += (int16_t)cfg->step_threshold;
    } else {
        event->value = 0;
    }

    LOG_DBG("stepped code=%u value=%d acc=%d", event->code, event->value, *acc);

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api stepper_driver_api = {
    .handle_event = stepper_handle_event,
};

#define STEPPER_INST(n)                                                                            \
    static const uint16_t stepper_codes_##n[] = DT_INST_PROP(n, codes);                           \
    BUILD_ASSERT(DT_INST_PROP_OR(n, step_value, 1) > 0, "step-value must be positive");          \
    BUILD_ASSERT(DT_INST_PROP_OR(n, step_threshold, 4) > 0, "step-threshold must be positive");  \
    static int16_t stepper_accumulators_##n[4][DT_INST_PROP_LEN(n, codes)] = {};                  \
    static struct stepper_data stepper_data_##n = {                                                \
        .accumulators = &stepper_accumulators_##n[0][0],                                           \
    };                                                                                             \
    static const struct stepper_config stepper_config_##n = {                                     \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                            \
        .codes_len = DT_INST_PROP_LEN(n, codes),                                                   \
        .step_value = DT_INST_PROP_OR(n, step_value, 1),                                           \
        .step_threshold = DT_INST_PROP_OR(n, step_threshold, 4),                                   \
        .codes = stepper_codes_##n,                                                                \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, &stepper_data_##n, &stepper_config_##n, POST_KERNEL,     \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &stepper_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STEPPER_INST)
