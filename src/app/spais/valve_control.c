/*
* Project: SPAIS
* Module: valve_control.c
* Author: Cory Perkins
*/
#include "includes.h"
//=========================== definitions ======================================
#define VALVE_CONTROL_DATA             DN_GPIO_PIN_1_DEV_ID
#define VALVE_CONTROL_ENABLE           DN_GPIO_PIN_2_DEV_ID
#define DEVICE_ACTIVATION_PEND_DEFAULT 50

//============================ typedefs ========================================
typedef struct {
    OS_STK valveControlTaskStack[TASK_APP_VALVE_CONTROL_STK_SIZE];
    INT8U  device_activation_pend;
} VALVE_CONTROL_TASK_VARS_T;

//=========================== variables ========================================
VALVE_CONTROL_TASK_VARS_T valveControlTaskVars;

// message queue variables
OS_EVENT *valveControlDataQueue;
void     *valveControlDataMsg[2];

//=========================== prototypes =======================================
static void valveControlTask(void* arg);
static VALVE_STATUS retrieveValveControlData();
static void changeValveStatus(VALVE_STATUS valve_control_data);
static void initializeValveControl();

//======================= function definitions =================================
/*
* initializeValveControlTask - initialize the valve control task and any
*                              hardware as necessary
*/
void initializeValveControlTask() {
    INT8U os_error;

    // initialize module variables
    memset(&valveControlTaskVars,0,sizeof(VALVE_CONTROL_TASK_VARS_T));
    // TODO: check the configuration file for any changes to these defaults
    valveControlTaskVars.device_activation_pend = DEVICE_ACTIVATION_PEND_DEFAULT;

    // create the valve control data queue
    valveControlDataQueue = OSQCreate(&valveControlDataMsg[0], 2);

    // create the data send task
    os_error = OSTaskCreateExt(
        valveControlTask,
        (void*) 0,
        (OS_STK*)(&valveControlTaskVars.valveControlTaskStack[TASK_APP_VALVE_CONTROL_STK_SIZE- 1]),
        TASK_APP_VALVE_CONTROL_PRIORITY,
        TASK_APP_VALVE_CONTROL_PRIORITY,
        (OS_STK*)valveControlTaskVars.valveControlTaskStack,
        TASK_APP_VALVE_CONTROL_STK_SIZE,
        (void*) 0,
        OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR
    );
    ASSERT(os_error == OS_ERR_NONE);

    OSTaskNameSet(TASK_APP_VALVE_CONTROL_PRIORITY, (INT8U*)TASK_APP_VALVE_CONTROL_NAME, &os_error);
    ASSERT(os_error == OS_ERR_NONE);

    initializeValveControl();
}

/*
* ValveControlTask - service any valve status change requests
*/
static void valveControlTask(void* arg) {
    VALVE_STATUS valve_control_data;

    while (1) {
        // listen for valve-status-change request
        valve_control_data = retrieveValveControlData();

        // dnm_ucli_printf("Received valve control data: %02x \r\n", valve_control_data);

        changeValveStatus(valve_control_data);
    }
}

/*
* initializeValveControlTask - check for any valve status change requests
*                              Caution: This function PENDS until there is data!
*
* @return VALVE_STATUS valve_control_data the desired valve status
*/
static VALVE_STATUS retrieveValveControlData() {
    INT8U        os_error;
    VALVE_STATUS valve_control_data;

    valve_control_data = (VALVE_STATUS) OSQPend(
        valveControlDataQueue,
        0,
        &os_error
    );

    return valve_control_data;
}

/*
* postValveControlData - post a valve status change request to the message
*                        queue.  It will be serviced later
*
* @param VALVE_STATUS valve_control_data the desired valve status
*/
void postValveControlData(VALVE_STATUS valve_control_data) {
    INT8U os_error;

    os_error = OSQPost(valveControlDataQueue, (void *)valve_control_data);
    ASSERT(os_error == OS_ERR_NONE);
}

/*
* initializeValveControl - initialize the GPIO pins tied to the valve control
*                          enable switch, as well as the valve control direction
*                          data for easy toggling of each.  For controlling
*                          these pins see changeValveStatus()
*/
static void initializeValveControl() {
    // error variables
    dn_error_t      dn_error;
    INT8U           os_error;

    dn_gpio_ioctl_cfg_out_t valve_control_cfg;
    valve_control_cfg.initialLevel = FALSE;

    // open and configure the valve_control_data GPIO
    dn_error = dn_open(
        VALVE_CONTROL_DATA,
        NULL,
        0
    );
    ASSERT(dn_error == DN_ERR_NONE);

    // configure as output
    dn_error = dn_ioctl(
        VALVE_CONTROL_DATA,            // device
        DN_IOCTL_GPIO_CFG_OUTPUT,      // request
        &valve_control_cfg,       // args
        sizeof(valve_control_cfg) // argLen
    );
    ASSERT(dn_error == DN_ERR_NONE);

    // open and configure the valve_control_enable GPIO
    dn_error = dn_open(
        VALVE_CONTROL_ENABLE,
        NULL,
        0
    );
    ASSERT(dn_error == DN_ERR_NONE);

    // configure as output
    dn_error = dn_ioctl(
        VALVE_CONTROL_ENABLE,     // device
        DN_IOCTL_GPIO_CFG_OUTPUT,     // request
        &valve_control_cfg,       // args
        sizeof(valve_control_cfg) // argLen
    );
    ASSERT(dn_error == DN_ERR_NONE);
}

/*
* changeValveStatus - changes the status of the valve to the passed valve status
*                     (open or closed)
*                     Caution: Valve Control must be initialized prior to
*                     calling this function.  See initializeValveControl()
*
* @param VALVE_STATUS valve_control_data the desired status of the valve
*/
static void changeValveStatus(VALVE_STATUS valve_control_data) {
    dn_error_t dn_error;
    INT8U      valve_control_enable;

    // set valve control data pin as appropriate
    dn_error = dn_write(
        VALVE_CONTROL_DATA,
        &valve_control_data,
        sizeof(valve_control_data)
    );

    // enable valve control
    valve_control_enable = TRUE;
    dn_error = dn_write(
        VALVE_CONTROL_ENABLE,
        &valve_control_enable,
        sizeof(valve_control_enable)
    );
    ASSERT(dn_error == DN_ERR_NONE);

    // wait the delay
    OSTimeDly(valveControlTaskVars.device_activation_pend);

    // disable valve control
    valve_control_enable = FALSE;
    dn_error = dn_write(
        VALVE_CONTROL_ENABLE,
        &valve_control_enable,
        sizeof(valve_control_enable)
    );
}