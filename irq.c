#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/device.h>

#include "device.h"

/* IRQ handler */
static irqreturn_t esp32_irq_handler(int irq, void *data)
{
    pr_info("IRQ_TRIGGERED !\n");
    return IRQ_HANDLED;
}

int irq_init(struct my_device *dev)
{
    int ret;


    ret = gpio_direction_input(HOST_GPIO_IN);
    if (ret) {
        dev_err(dev->device, "gpio_direction_input(%d) failed: %d\n",
                HOST_GPIO_IN, ret);
        return ret;
    }

    dev->irq = gpio_to_irq(HOST_GPIO_IN);
    if (dev->irq < 0) {
        dev_err(dev->device, "gpio_to_irq(%d) failed: %d\n",
                HOST_GPIO_IN, dev->irq);
        return dev->irq;
    }

    ret = devm_request_irq(dev->device,
                           dev->irq,
                           esp32_irq_handler,
                           IRQF_TRIGGER_RISING,
                           "esp32_sensor_irq",
                           dev);
    if (ret) {
        dev_err(dev->device, "request_irq(%d) failed: %d\n",
                dev->irq, ret);
        return ret;
    }

    dev_info(dev->device, "GPIO %d mapped to IRQ %d\n",
             HOST_GPIO_IN, dev->irq);

    return 0;
}
