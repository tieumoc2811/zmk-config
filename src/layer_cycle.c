/*
 * Behavior &layer_cycle : moi lan bam chuyen sang layer ke tiep trong danh sach
 * "cycle-layers" (vong lai tu dau). Layer dau danh sach la layer mac dinh.
 * Chay duoc ca khi dang giu FN (layer FN khong bi anh huong).
 */

#define DT_DRV_COMPAT zmk_behavior_layer_cycle

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static const uint8_t cycle[] = DT_INST_PROP(0, cycle_layers);
#define CYCLE_LEN ARRAY_SIZE(cycle)

static int on_pressed(struct zmk_behavior_binding *binding,
                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

    /* tim layer hien tai: layer cuoi cung trong danh sach dang bat, mac dinh la phan tu 0 */
    int cur = 0;
    for (int i = 1; i < CYCLE_LEN; i++) {
        if (zmk_keymap_layer_active(cycle[i])) {
            cur = i;
        }
    }

    int next = (cur + 1) % CYCLE_LEN;

    for (int i = 1; i < CYCLE_LEN; i++) {
        zmk_keymap_layer_deactivate(cycle[i], true);
    }
    if (next != 0) {
        zmk_keymap_layer_activate(cycle[next], false);
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_released(struct zmk_behavior_binding *binding,
                       struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api layer_cycle_api = {
    .binding_pressed = on_pressed,
    .binding_released = on_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &layer_cycle_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY */
