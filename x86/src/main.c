#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <shell/shell.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <ipm.h>
#include <ipm/ipm_quark_se.h>

#include <zjs_ipm.h>


#define PME_SHELL_MODULE "pme"
#define PME_IPM_TIMEOUT_TICKS 5000
#define PME_DEFAULT_SAMPLING_FREQ 100

char *sensor_name = "bmi160";

static struct k_sem sync_sem;

uint32_t sensor_print = 0;

static int shell_cmd_sensor(int argc, char *argv[])
{
    zjs_ipm_message_t send;
    zjs_ipm_message_t reply;

    if (argc != 2) {
        printk("shell: invalid usage\n");
        return 0;
    }

    send.id = MSG_ID_SENSOR;
    send.flags = 0 | MSG_SYNC_FLAG;
    send.user_data = (void *)&reply;
    send.error_code = ERROR_IPM_NONE;

    if (!strcmp(argv[1], "init")) {
        send.type = TYPE_SENSOR_INIT;
        send.data.sensor.channel = SENSOR_CHAN_ACCEL_XYZ;
    } 
    else if (!strcmp(argv[1], "start")) {
        send.type = TYPE_SENSOR_START;
        send.data.sensor.channel = SENSOR_CHAN_ACCEL_XYZ;
        send.data.sensor.frequency = PME_DEFAULT_SAMPLING_FREQ;
    }
    else if (!strcmp(argv[1], "stop")) {
        send.type = TYPE_SENSOR_STOP;    
        send.data.sensor.channel = SENSOR_CHAN_ACCEL_XYZ;
    } 
    else if (!strcmp(argv[1], "print")) {
        sensor_print = !sensor_print;
        return 0;
    } 
    else {
        printk("shell: invalid usage\n");
        return 0;        
    }

    send.data.sensor.controller = sensor_name;

    if (zjs_ipm_send(MSG_ID_SENSOR, &send) != 0) {
        printk("PME: IPM send failed\n");
        return ERROR_IPM_OPERATION_FAILED;
    }
    if (k_sem_take(&sync_sem, PME_IPM_TIMEOUT_TICKS)) {
        printk("FATAL ERROR, ipm timed out\n");
        return ERROR_IPM_OPERATION_FAILED;
    }

    return 0;
}

static int shell_cmd_pme(int argc, char *argv[])
{
    zjs_ipm_message_t send;
    zjs_ipm_message_t reply;

    send.id = MSG_ID_PME;
    send.flags = 0 | MSG_SYNC_FLAG;
    send.user_data = (void *)&reply;
    send.error_code = ERROR_IPM_NONE;

    if (!strcmp(argv[1], "init")) {
        send.type = TYPE_PME_INIT;
    } else if (!strcmp(argv[1], "learn-test")) {
        if (argc != 7) {
            printk("usage: %s v1 v2 v3 v4 category\n", argv[1]);
            return 0;
        }
        send.type = TYPE_PME_LEARN_TEST;
        send.data.pme.vector[0] = atoi(argv[2]);
        send.data.pme.vector[1] = atoi(argv[3]);
        send.data.pme.vector[2] = atoi(argv[4]);
        send.data.pme.vector[3] = atoi(argv[5]);
        send.data.pme.count = 4;
        send.data.pme.category = atoi(argv[6]);

    } else if (!strcmp(argv[1], "classify-test")) {
        if (argc != 6) {
            printk("usage: %s v1 v2 v3 v4\n", argv[1]);
        }
        send.type = TYPE_PME_CLASSIFY_TEST;
        send.data.pme.vector[0] = atoi(argv[2]);
        send.data.pme.vector[1] = atoi(argv[3]);
        send.data.pme.vector[2] = atoi(argv[4]);
        send.data.pme.vector[3] = atoi(argv[5]);
        send.data.pme.count = 4;

    } else if (!strcmp(argv[1], "learn")) {
        send.type = TYPE_PME_LEARN_IMU;
        if (argc != 3) {
            printk("shell: invalid usage\n");
            return 0;        
        }
        send.data.pme.category = atoi(argv[2]);
    } else if (!strcmp(argv[1], "classify")) {
        send.type = TYPE_PME_CLASSIFY_IMU;
    } else if (!strcmp(argv[1], "read")) {
        send.type = TYPE_PME_READ_NEURONS;
    } else {
        printk("shell: invalid usage\n");
        return 0;        
    }

    if (zjs_ipm_send(MSG_ID_PME, &send) != 0) {
        printk("PME: IPM send failed\n");
        return ERROR_IPM_OPERATION_FAILED;
    }
    if (k_sem_take(&sync_sem, PME_IPM_TIMEOUT_TICKS)) {
        printk("FATAL ERROR, ipm timed out\n");
        return ERROR_IPM_OPERATION_FAILED;
    }

    return 0;
}

void sensor_ipm_callback(void *context, uint32_t id, volatile void *data)
{
    if (id != MSG_ID_SENSOR) {
        printk("PME: IPM invalid ID\n");
        return;
    }
    zjs_ipm_message_t *msg = (zjs_ipm_message_t*)(*(uintptr_t *)data);
    if ((msg->flags & MSG_SYNC_FLAG) == MSG_SYNC_FLAG) {
        zjs_ipm_message_t *result = (zjs_ipm_message_t*)msg->user_data;
        // synchrounus ipm, copy the results
        if (result) {
            memcpy(result, msg, sizeof(zjs_ipm_message_t));
        }
        // un-block sync api
        k_sem_give(&sync_sem);
    } else if (msg->type == TYPE_SENSOR_EVENT_READING_CHANGE) {
        // value change event,
        double x = msg->data.sensor.reading.x;
        double y = msg->data.sensor.reading.y;
        double z = msg->data.sensor.reading.z;
        if (sensor_print)
            printf("PME: sensor val=%f %f %f\n", x, y, z);
    } else {
        printk("unsupported message received\n");
    }
}

void pme_ipm_callback(void *context, uint32_t id, volatile void *data)
{
    if (id != MSG_ID_PME) {
        printk("PME: IPM invalid ID\n");
        return;
    }
    zjs_ipm_message_t *msg = (zjs_ipm_message_t*)(*(uintptr_t *)data);
    zjs_ipm_message_t *result;

    if ((msg->flags & MSG_SYNC_FLAG) == MSG_SYNC_FLAG) {
         result = (zjs_ipm_message_t*)msg->user_data;
        // synchrounus ipm, copy the results
        if (result) {
            memcpy(result, msg, sizeof(zjs_ipm_message_t));
        }
        // un-block sync api
        k_sem_give(&sync_sem);
    } 

    if (msg->type == TYPE_PME_CLASSIFY_TEST || msg->type == TYPE_PME_CLASSIFY_IMU) {
        printf("PME: classify category=%d\n", msg->data.pme.category);
    } 
}

static struct shell_cmd commands[] = {
        { "sensor", shell_cmd_sensor, "init start stop print" },
        { "pme", shell_cmd_pme, "init | learn category | classify" },
        { NULL, NULL, NULL }
};

void main(void)
{
    SHELL_REGISTER(PME_SHELL_MODULE, commands);

    zjs_ipm_init();
    zjs_ipm_register_callback(MSG_ID_SENSOR, sensor_ipm_callback);
    zjs_ipm_register_callback(MSG_ID_PME, pme_ipm_callback);

    k_sem_init(&sync_sem, 0, 1);

    shell_register_default_module(PME_SHELL_MODULE);

    k_sleep(TICKS_UNLIMITED);
}
