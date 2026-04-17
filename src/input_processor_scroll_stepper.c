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
    const uint16_t *codes;
};

static bool code_matches(uint16_t code, const uint16_t *codes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (codes[i] == code) {
            return true;
        }
    }

    return false;
}

static int stepper_handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                                uint32_t param2, struct zmk_input_processor_state *state) {
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);
    ARG_UNUSED(state);

    const struct stepper_config *cfg = dev->config;

    if (event->type != cfg->type || event->value == 0 ||
        !code_matches(event->code, cfg->codes, cfg->codes_len)) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    event->value = event->value > 0 ? cfg->step_value : -((int16_t)cfg->step_value);
    LOG_DBG("stepped code=%u value=%d", event->code, event->value);

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api stepper_driver_api = {
    .handle_event = stepper_handle_event,
};

#define STEPPER_INST(n)                                                                            \
    static const uint16_t stepper_codes_##n[] = DT_INST_PROP(n, codes);                           \
    BUILD_ASSERT(DT_INST_PROP_OR(n, step_value, 1) > 0, "step-value must be positive");          \
    static const struct stepper_config stepper_config_##n = {                                     \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                            \
        .codes_len = DT_INST_PROP_LEN(n, codes),                                                   \
        .step_value = DT_INST_PROP_OR(n, step_value, 1),                                           \
        .codes = stepper_codes_##n,                                                                \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &stepper_config_##n, POST_KERNEL,                  \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &stepper_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STEPPER_INST)
