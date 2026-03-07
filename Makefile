obj-m := mydevice.o
mydevice-objs := main.o esp32_pm_i2c.o f_ops.o pm.o irq.o 

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

BUILD_DIR := $(PWD)/build
USER := user

all: prepare module user

prepare:
	mkdir -p $(BUILD_DIR)

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	cp -f mydevice.ko $(BUILD_DIR)/

user:
	gcc -Wall -Wextra -o $(BUILD_DIR)/$(USER) user.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf $(BUILD_DIR)
