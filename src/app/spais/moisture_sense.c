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


#include "moisture_sense_cfg.h"
//=========================== definitions =====================================
// devices
#define MOISTURE_SENSOR_ADC DN_ADC_AI_0_DEV_ID
#define MOISTURE_SENSOR_PWR_CTRL DN_GPIO_PIN_0_DEV_ID
#define UDP_PORT 0xF0B9;

// defaults
#define DEVICE_ACTIVATION_PEND_DEFAULT 20
#define PERIOD_DEFAULT                 300000 // 5 minutes

// filenames
#define MOISTURE_SENSE_CONFIG_FILENAME "moisture-sense.cfg"

// device states
#define GPIO_ON_STATE 0x01
#define GPIO_OFF_STATE 0x00

// format of config file
typedef struct{
    INT8U  device_activation_pend;
    INT16U period;
} moisture_sense_configFileStruct_t;
//=========================== variables =======================================
typedef struct {
    OS_STK          moistureSenseTaskStack[TASK_APP_MOISTURE_SENSE_STK_SIZE];
    INT8U           device_activation_pend; // time to wait (in ms) between activating the sensor and reading its output
    INT32U          period;                 // period (in ms) between moisture sensing
} moisture_sense_task_vars_t;

moisture_sense_task_vars_t moisture_sense_task_v;

typedef struct {
    INT16U value;
    INT8U new_flag;
} moisture_sense_data;

moisture_sense_data moistureSenseData;

OS_EVENT *moistureSenseDataMutex;
//=========================== prototypes ======================================
static void moistureSenseTask(void* arg);
static void storeMoistureSenseData(INT16U valueToStore);
//=========================== initialization ==================================
void initializeMoistureSenseTask() {
    INT8U                     osErr;

    moistureSenseData.value = NULL;
    moistureSenseData.new_flag = FALSE;

    // initialize module variables
    memset(&moisture_sense_task_v,0,sizeof(moisture_sense_task_vars_t));
    moisture_sense_task_v.device_activation_pend = DEVICE_ACTIVATION_PEND_DEFAULT;
    moisture_sense_task_v.period = PERIOD_DEFAULT;

    // create the mutex used for accessing the moisture sense data
    moistureSenseDataMutex = OSMutexCreate(53, &osErr);
    ASSERT(osErr == OS_ERR_NONE);

    // create the moisture sense task
    osErr = OSTaskCreateExt(
        moistureSenseTask,
        (void*) 0,
        (OS_STK*)(&moisture_sense_task_v.moistureSenseTaskStack[TASK_APP_MOISTURE_SENSE_STK_SIZE- 1]),
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
//=========================== Soil Moisture Sense task =================================
static void moistureSenseTask(void* arg) {
    dn_error_t                     dnErr;
    INT8U                          osErr;

    // Moisture Sensor Power Control variables
    INT8U                   sensorState;
    dn_gpio_ioctl_cfg_out_t moistureSensorCtrlCfg;

    // ADC variables
    dn_adc_drv_open_args_t  adcOpenArgs;
    INT16U                  adcValue;
    int                     numBytesRead;

    // open and configure the MOISTURE_SENSOR_PWR_CTRL
    dnErr = dn_open(
        MOISTURE_SENSOR_PWR_CTRL,
        NULL,
        0
    );
    ASSERT(dnErr==DN_ERR_NONE);

    // configure as output
    moistureSensorCtrlCfg.initialLevel = GPIO_OFF_STATE;
    dnErr = dn_ioctl(
        MOISTURE_SENSOR_PWR_CTRL,     // device
        DN_IOCTL_GPIO_CFG_OUTPUT,     // request
        &moistureSensorCtrlCfg,       // args
        sizeof(moistureSensorCtrlCfg) // argLen
    );
    ASSERT(dnErr==DN_ERR_NONE);

    // open and configure the ADC
    // todo: do the math and see if the nominal values can be easily found
    adcOpenArgs.rdacOffset  = 0;
    adcOpenArgs.vgaGain     = 0;
    adcOpenArgs.fBypassVga  = 1;
    dnErr = dn_open(
        DN_ADC_AI_0_DEV_ID,         // device
        &adcOpenArgs,                  // args
        sizeof(adcOpenArgs)            // argLen
    );
    ASSERT(dnErr==DN_ERR_NONE);

    while (1) {
        // activate the sensor
        sensorState = GPIO_ON_STATE;
        dnErr = dn_write(
            MOISTURE_SENSOR_PWR_CTRL,
            &sensorState,
            sizeof(sensorState)
        );
        ASSERT(dnErr==DN_ERR_NONE);

        // wait the configured number of ms
        OSTimeDly(moisture_sense_task_v.device_activation_pend);

        // read the adc
        numBytesRead = dn_read(
            DN_ADC_AI_0_DEV_ID, // device
            &adcValue,            // buf
            sizeof(adcValue)      // bufSize
        );
        ASSERT(numBytesRead== sizeof(adcValue));

        // deactivate the sensor
        sensorState = GPIO_OFF_STATE;
        dnErr = dn_write(
            MOISTURE_SENSOR_PWR_CTRL,
            &sensorState,
            sizeof(sensorState)
        );
        ASSERT(dnErr==DN_ERR_NONE);

        // store value to be sent later
        storeMoistureSenseData(adcValue);

        // pend for PERIOD ms
        OSTimeDly(moisture_sense_task_v.period);
    }
}

static void storeMoistureSenseData(INT16U valueToStore) {
    INT8U                     osErr;

    // get the mutex
    OSMutexPend(moistureSenseDataMutex, 0, &osErr);
    ASSERT(osErr==OS_ERR_NONE);

    // store the data
    moistureSenseData.value = valueToStore;

    // give the mutex back
    osErr = OSMutexPost(moistureSenseDataMutex);
    ASSERT(osErr==OS_ERR_NONE);
}

INT16U retrieveMoistureSenseData() {
    INT8U  osErr;
    INT16U valueToRetrieve;

    // get the mutex
    OSMutexPend(moistureSenseDataMutex, 0, &osErr);
    ASSERT(osErr==OS_ERR_NONE);

    // check for new data
    if (moistureSenseData.new_flag == TRUE) {
        // reset the flag
        moistureSenseData.new_flag = FALSE;
        // store the data
        valueToRetrieve = moistureSenseData.value;
    } else {
        valueToRetrieve = NULL;
    }

    // give the mutex back
    osErr = OSMutexPost(moistureSenseDataMutex);
    ASSERT(osErr==OS_ERR_NONE);

    return valueToRetrieve;
}