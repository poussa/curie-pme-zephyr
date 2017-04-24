subdir-ccflags-y += -I$(ZEPHYR_BASE)/drivers
subdir-ccflags-y += -Dzjs_free=free
subdir-ccflags-y += -Dzjs_malloc=malloc

export LDFLAGS += -u _printf_float
subdir-ccflags-$(CONFIG_X86) += -DZJS_PRINT_FLOATS

obj-y += main.o
obj-y += zjs_common.o
obj-y += zjs_ipm.o