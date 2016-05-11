/*
Copyright (c) 2013, Dust Networks.  All rights reserved.
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
#include "app_task_cfg.h"
#include "Ver.h"

//=========================== definitions =====================================
// devices
#define MOISTURE_SENSOR_ADC
#define MOISTURE_SENSOR_PWR_CTRL DN_GPIO_PIN_0_DEV_ID

// defaults
#define DEVICE_ACTIVATION_PEND_DEFAULT 20
#define PERIOD_DEFAULT                 600000

// filenames
#define MOISTURE_SENSE_CONFIG_FILENAME "moistureSense.cfg"

// device states
#define GPIO_ON_STATE 1
#define GPIO_OFF_STATE 0

// format of config file
typedef struct{
    INT8U           device_activation_pend;
    INT16U          period;
} moisture_sense_configFileStruct_t;

//=========================== variables =======================================
typedef struct {
    OS_STK          moistureSenseTaskStack[TASK_APP_MOISTURE_STK_SIZE];
    INT8U           device_activation_pend; // time to wait (in ms) between activating the sensor and reading its output
    INT16U          period;                 ///< period (in ms) between transmissions
} moisture_sense_task_vars_t;

moisture_sense_task_vars_t moisture_sense_task_v;
//=========================== prototypes ======================================
//===== GPIO notification task
static void initializeMoistureSenseTask();
static void moistureSenseTask(void *arg);

//=========================== const ===========================================



//=========================== initialization ==================================

static void initializeMoistureSenseTask() {
    INT8U                     osErr;
    
    //===== initialize module variables
    memset(&moisture_sense_task_v,0,sizeof(moisture_sense_task_vars_t));
    moisture_sense_task_v.device_activation_pend = DEVICE_ACTIVATION_PEND_DEFAULT;
    moisture_sense_task_v.period     = PERIOD_DEFAULT;
    
    //===== create the moisture sense task
    osErr = OSTaskCreateExt(
        gpioSampleTask,
        (void*) 0,
        (OS_STK*)(&moisture_sense_task_v.moistureSenseTaskStack[TASK_APP_GPIOSAMPLE_STK_SIZE- 1]),
        TASK_APP_MOISTURE_SENSE_PRIORITY,
        TASK_APP_MOISTURE_SENSE_PRIORITY,
        (OS_STK*)moisture_sense_task_v.moistureSenseTaskStack,
        TASK_APP_MOISTURE_SENSE_STK_SIZE,
        (void*) 0,
        OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR
    );
    ASSERT(osErr == OS_ERR_NONE);
    OSTaskNameSet(TASK_APP_MOISTURE_SENSE_PRIORITY, (INT8U*)TASK_APP_MOISTURE_SENSE_NAME, &osErr);
    ASSERT(osErr == OS_ERR_NONE);
}

//=========================== GPIO notif task =================================

static void moistureSenseTask(void* arg) {
    dn_error_t                     dnErr;
    INT8U                          osErr;
 
    dn_adc_drv_open_args_t  openArgs;
    INT16U                  adcVal;
    INT8U                   sensorState;
 
    // todo: probably don't need these
    dn_gpio_ioctl_cfg_in_t         gpioInCfg;
    dn_gpio_ioctl_cfg_out_t        gpioOutCfg;
    INT8U                          samplePinLevel;
    INT8U                          pkBuf[sizeof(loc_sendtoNW_t) + 1];
    loc_sendtoNW_t*                pkToSend;
    INT8U                          rc;
    
    //===== open and configure the MOISTURE_SENSOR_PWR_CTRL
    dn_open(
        MOISTURE_SENSOR_PWR_CTRL,
        NULL,
        0
    );
 
    // set the pull mode of the interla pull resistor for the activator
    gpioInCfg.pullMode = DN_GPIO_PULL_NONE;
    dn_ioctl(
        SAMPLE_PIN,
        DN_IOCTL_GPIO_CFG_INPUT,
        &gpioInCfg,
        sizeof(gpioInCfg)
    );
    
    // initialize the ADC
    // open ADC channel
    openArgs.rdacOffset  = 0;
    openArgs.vgaGain     = 0;
    openArgs.fBypassVga  = 1;
    dnErr = dn_open(
        DN_ADC_AI_0_DEV_ID,         // device
        &openArgs,                  // args
        sizeof(openArgs)            // argLen 
    );
    ASSERT(dnErr==DN_ERR_NONE);
 
    //===== initialize packet variables
    // todo: i think this should be a mutex instead no need to actually send anything from this task
    
    //===== wait for the mote to have joined
    OSSemPend(moisture_sense_task_v.joinedSem,0,&osErr);
    ASSERT(osErr == OS_ERR_NONE);
    
    while (1) { // this is a task, it executes forever
        // activate the sensor
        sensorState = GPIO_ON_STATE;
        dnErr = dn_write(
            MOISTURE_SENSOR_PWR_CTRL,  // device
            &sensorState,                  // buf
            sizeof(sensorState)            // len
        );
        ASSERT(dnErr==DN_ERR_NONE);

        // wait the configured number of ms
        OSTimeDly(moisture_sense_task_v.device_activation_pend);

        // read the adc
        numBytesRead = dn_read(
            DN_ADC_AI_0_DEV_ID,        // device
            &adcVal,                    // buf
            sizeof(adcVal)              // bufSize 
        );
        ASSERT(numBytesRead== sizeof(adcVal));

        // store value to be sent later

        // deactivate the sensor
        sensorState = GPIO_OFF_STATE;
        dnErr = dn_write(
            MOISTURE_SENSOR_PWR_CTRL, // device
            &sensorState,             // buf
            sizeof(sensorState)       // len
        );

        // pend for PERIOD ms
        OSTimeDly(moisture_sense_task_v.period);
    }
}

//=============================================================================
//=========================== install a kernel header =========================
//=============================================================================

/**
 A kernel header is a set of bytes prepended to the actual binary image of this
 application. Thus header is needed for your application to start running.
 */

DN_CREATE_EXE_HDR(DN_VENDOR_ID_NOT_SET,
                  DN_APP_ID_NOT_SET,
                  VER_MAJOR,
                  VER_MINOR,
                  VER_PATCH,
                  VER_BUILD);
