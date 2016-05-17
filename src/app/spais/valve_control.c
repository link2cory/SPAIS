/*
Author: Cory Perkins
*/
#include "dn_common.h"
#include "string.h"
#include "stdio.h"
#include "cli_task.h"
#include "loc_task.h"
#include "dn_system.h"
#include "dn_gpio.h"
#include "dnm_local.h"
#include "dn_fs.h"
#include "dn_adc.h"
#include "dn_exe_hdr.h"
#include "well_known_ports.h"
#include "Ver.h"


#include "valve_control_cfg.h"
//=========================== definitions =====================================
// devices
#define VALVE_CONTROL_DATA   DN_GPIO_PIN_1_DEV_ID
#define VALVE_CONTROL_ENABLE DN_GPIO_PIN_2_DEV_ID

// device states
#define GPIO_ON_STATE 0x01
#define GPIO_OFF_STATE 0x00
//=========================== variables =======================================
typedef struct {
    OS_STK valveControlTaskStack[TASK_APP_VALVE_CONTROL_STK_SIZE];
    INT8u  device_activation_pend
} valve_control_task_vars_t;

valve_control_task_vars_t valve_control_task_v;
//=========================== prototypes ======================================
static void valveControlTask(void* arg);
//=========================== initialization ==================================
void initializeValveControlTask() {
    INT8U osErr;

    // initialize module variables
    memset(&valve_control_task_v,0,sizeof(valve_control_task_vars_t));
    valve_control_task_v.period = PERIOD_DEFAULT;

    // create the data send task
    osErr = OSTaskCreateExt(
        valveControlTask,
        (void*) 0,
        (OS_STK*)(&valve_control_task_v.valveControlTaskStack[TASK_APP_VALVE_CONTROL_STK_SIZE- 1]),
        TASK_APP_VALVE_CONTROL_PRIORITY,
        TASK_APP_VALVE_CONTROL_PRIORITY,
        (OS_STK*)valve_control_task_v.valveControlTaskStack,
        TASK_APP_VALVE_CONTROL_STK_SIZE,
        (void*) 0,
        OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR
    );
    ASSERT(osErr == OS_ERR_NONE);
    OSTaskNameSet(TASK_APP_VALVE_CONTROL_PRIORITY, (INT8U*)TASK_APP_VALVE_CONTROL_NAME, &osErr);
    ASSERT(osErr == OS_ERR_NONE);
}
//=========================== Soil Moisture Sense task =================================
void valveControlTask(void* arg) {
    // error variables
    dn_error_t      dnErr;
    INT8U           osErr;

    // valve control data variables
    INT8U                   valveControlData;
    dn_gpio_ioctl_cfg_out_t valveControlDataCfg;

    // valve control enable variables
    INT8U                   valveControlEnable;
    dn_gpio_ioctl_cfg_out_t valveControlEnableCfg;

    // open and configure the valveControlData GPIO
    dnErr = dn_open(
        VALVE_CONTROL_DATA,
        NULL,
        0
    );
    ASSERT(dnErr==DN_ERR_NONE);

    // configure as output
    valveControlDataCfg.initialLevel = GPIO_OFF_STATE;
    dnErr = dn_ioctl(
        VALVE_CONTROL_DATA,     // device
        DN_IOCTL_GPIO_CFG_OUTPUT,     // request
        &valveControlDataCfg,       // args
        sizeof(valveControlDataCfg) // argLen
    );
    ASSERT(dnErr==DN_ERR_NONE);

    // open and configure the valveControlEnable GPIO
    dnErr = dn_open(
        VALVE_CONTROL_ENABLE,
        NULL,
        0
    );
    ASSERT(dnErr==DN_ERR_NONE);

    // configure as output
    valveControlEnableCfg.initialLevel = GPIO_OFF_STATE;
    dnErr = dn_ioctl(
        VALVE_CONTROL_ENABLE,     // device
        DN_IOCTL_GPIO_CFG_OUTPUT,     // request
        &valveControlEnableCfg,       // args
        sizeof(valveControlEnableCfg) // argLen
    );
    ASSERT(dnErr==DN_ERR_NONE);

    while (1) {
        // listen for valve-status-change request
        // todo: save the desired value to valveControlData

        // set valve control data pin as appropriate
        dnErr = dn_write(
            VALVE_CONTROL_DATA,
            &valveControlData,
            sizeof(valveControlData)
        );

        // enable valve control
        valveControlEnable = GPIO_ON_STATE;
        dnErr = dn_write(
            VALVE_CONTROL_ENABLE,
            &valveControlEnable,
            sizeof(valveControlEnable)
        );

        // wait the delay
        OSTimeDly(valve_control_task_v.device_activation_pend);

        // disable valve control
        valveControlEnable = GPIO_OFF_STATE;
        dnErr = dn_write(
            VALVE_CONTROL_ENABLE,
            &valveControlEnable,
            sizeof(valveControlEnable)
        );
    }
}
