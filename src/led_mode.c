/*
 * Behavior &led_mode N : chon 1 trong 9 che do hieu ung cho 6 day LED (GPIO, active-low).
 * Chi chay khi co nguon USB (khong co 5V thi LED vo hinh, de tiet kiem pin).
 */

#define DT_DRV_COMPAT zmk_behavior_led_mode

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/usb.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define NUM_LEDS DT_INST_PROP_LEN(0, leds_gpios)
#define LED_SPEC(idx, _) GPIO_DT_SPEC_INST_GET_BY_IDX(0, leds_gpios, idx)

#define TICK_MS 100
#define IDLE_MS 1000
#define LED_MASK ((1u << NUM_LEDS) - 1u)

static const struct gpio_dt_spec leds[] = {LISTIFY(NUM_LEDS, LED_SPEC, (, ), 0)};

static struct k_work_delayable anim_work;
static uint8_t cur_mode = 2; /* mac dinh: sang het */
static uint32_t tick;
static uint32_t rnd = 0x1234ABCDu;

static const uint8_t burst[8] = {0x0C, 0x1E, 0x3F, 0x33, 0x21, 0x00, 0x00, 0x00};

static uint32_t xorshift(void) {
    rnd ^= rnd << 13;
    rnd ^= rnd >> 17;
    rnd ^= rnd << 5;
    return rnd;
}

static uint8_t frame(uint8_t mode, uint32_t t) {
    uint32_t p;

    switch (mode) {
    case 1: /* tat het */
        return 0;
    case 2: /* sang het */
        return LED_MASK;
    case 3: /* chay duoi mot chieu */
        return 1u << (t % NUM_LEDS);
    case 4: /* di qua di lai (Knight Rider) */
        p = t % (2 * NUM_LEDS - 2);
        return 1u << (p < NUM_LEDS ? p : (2 * NUM_LEDS - 2) - p);
    case 5: /* nhap nhay 1Hz */
        return (t % 10) < 5 ? LED_MASK : 0;
    case 6: /* xen ke chan le */
        return (t % 10) < 5 ? 0x15 : 0x2A;
    case 7: /* day dan roi tat dan */
        p = t % (2 * NUM_LEDS);
        return p < NUM_LEDS ? ((1u << (p + 1)) - 1u) : ((LED_MASK << (p - NUM_LEDS + 1)) & LED_MASK);
    case 8: /* lap lanh ngau nhien */
        return xorshift() & LED_MASK;
    case 9: /* no tu giua ra */
        return burst[t % 8];
    default:
        return LED_MASK;
    }
}

static void apply(uint8_t mask) {
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_pin_set_dt(&leds[i], (mask >> i) & 1);
    }
}

static void anim_fn(struct k_work *work) {
    ARG_UNUSED(work);

    if (zmk_usb_get_conn_state() == ZMK_USB_CONN_NONE) {
        apply(0);
        k_work_reschedule(&anim_work, K_MSEC(IDLE_MS));
        return;
    }

    apply(frame(cur_mode, tick++));
    k_work_reschedule(&anim_work, K_MSEC(TICK_MS));
}

static int on_pressed(struct zmk_behavior_binding *binding,
                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);

    if (binding->param1 > 9) {
        return -EINVAL;
    }
    if (binding->param1 == 0) { /* &led_mode 0 : chuyen sang che do ke tiep (1..9, vong lai) */
        cur_mode = (cur_mode % 9) + 1;
    } else {
        cur_mode = (uint8_t)binding->param1;
    }
    tick = 0;
    k_work_reschedule(&anim_work, K_NO_WAIT);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_released(struct zmk_behavior_binding *binding,
                       struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api led_mode_api = {
    .binding_pressed = on_pressed,
    .binding_released = on_released,
};

static int led_mode_init(const struct device *dev) {
    ARG_UNUSED(dev);

    for (int i = 0; i < NUM_LEDS; i++) {
        if (!gpio_is_ready_dt(&leds[i])) {
            return -ENODEV;
        }
        gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
    }

    k_work_init_delayable(&anim_work, anim_fn);
    k_work_schedule(&anim_work, K_MSEC(500));
    return 0;
}

BEHAVIOR_DT_INST_DEFINE(0, led_mode_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &led_mode_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY */
