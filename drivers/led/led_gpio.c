/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_leds

/**
 * @file
 * @brief GPIO driven LEDs
 */

#include <drivers/led.h>
#include <drivers/gpio.h>
#include <device.h>
#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(led_gpio, CONFIG_LED_LOG_LEVEL);

struct led_gpio_config {
	size_t num_leds;
	const struct gpio_dt_spec *led;
};


static int led_gpio_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{

	const struct led_gpio_config *config = dev->config;
	const struct gpio_dt_spec *led_gpio;

	if ((led >= config->num_leds) || (value > 100)) {
		return -EINVAL;
	}

	led_gpio = &config->led[led];

	return gpio_pin_set(led_gpio->port, led_gpio->pin, (value >= 50));
}

static int led_gpio_on(const struct device *dev, uint32_t led)
{
	return led_gpio_set_brightness(dev, led, 100);
}

static int led_gpio_off(const struct device *dev, uint32_t led)
{
	return led_gpio_set_brightness(dev, led, 0);
}

static int led_gpio_init(const struct device *dev)
{
	const struct led_gpio_config *config = dev->config;
	int err = 0;

	if (!config->num_leds) {
		LOG_ERR("%s: no LEDs found (DT child nodes missing)", dev->name);
		err = -ENODEV;
	}

	for (size_t i = 0; (i < config->num_leds) && !err; i++) {
		const struct gpio_dt_spec *led = &config->led[i];

		if (device_is_ready(led->port)) {
			err = gpio_pin_configure(led->port, led->pin,
						 led->dt_flags | GPIO_OUTPUT_INACTIVE);

			if (err) {
				LOG_ERR("Cannot configure GPIO (err %d)", err);
			}
		} else {
			LOG_ERR("%s: GPIO device not ready", dev->name);
			err = -ENODEV;
		}
	}

	return err;
}

static const struct led_driver_api led_gpio_api = {
	.on		= led_gpio_on,
	.off		= led_gpio_off,
	.set_brightness	= led_gpio_set_brightness,
};

#define LED_GPIO_DT_SPEC(led_node_id)				\
	GPIO_DT_SPEC_GET(led_node_id, gpios),			\

#define LED_GPIO_DEVICE(i)					\
								\
static const struct gpio_dt_spec gpio_dt_spec_##i[] = {	\
	DT_INST_FOREACH_CHILD(i, LED_GPIO_DT_SPEC)		\
};								\
								\
static const struct led_gpio_config led_gpio_config_##i = {	\
	.num_leds	= ARRAY_SIZE(gpio_dt_spec_##i),	\
	.led		= gpio_dt_spec_##i,			\
};								\
								\
DEVICE_DT_INST_DEFINE(i, &led_gpio_init, device_pm_control_nop,	\
		      NULL, &led_gpio_config_##i,		\
		      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,	\
		      &led_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(LED_GPIO_DEVICE)
