subdir-ccflags-y += -I$(ZEPHYR_BASE)/drivers
subdir-ccflags-y += -I$../../x86/src
subdir-ccflags-y += -Dzjs_free=free
subdir-ccflags-y += -Dzjs_malloc=malloc
subdir-ccflags-y += -DBUILD_MODULE_PME -DBUILD_MODULE_SENSOR

obj-y += main.o
obj-y += CuriePME.o
obj-y += ../../x86/src/zjs_common.o
obj-y += ../../x86/src/zjs_ipm.o
