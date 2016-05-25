/*
* Project: SPAIS
* Module: moisture_sense.c
* Author: Cory Perkins
*/
#include "includes.h"
//=========================== definitions ======================================
// devices
#define MOISTURE_SENSOR_ADC      DN_ADC_AI_0_DEV_ID
#define MOISTURE_SENSOR_PWR_CTRL DN_GPIO_PIN_0_DEV_ID

// defaults
#define DEVICE_ACTIVATION_PEND_DEFAULT 20
#define PERIOD_DEFAULT                 300000 // 5 minutes

// filenames
#define MOISTURE_SENSE_CONFIG_FILE "moisture-sense.cfg"

//============================ typedefs ========================================
typedef struct {
    OS_STK          moistureSenseTaskStack[TASK_APP_MOISTURE_SENSE_STK_SIZE];
    INT8U           device_activation_pend;
    INT32U          period;
} MOISTURE_SENSE_TASK_VARS_T;

//=========================== variables ========================================
MOISTURE_SENSE_TASK_VARS_T moistureSenseTaskVars;
OS_EVENT                  *moistureDataQueue;
void                      *moistureDataMsg[2];

//========================== prototypes ========================================
static void   moistureSenseTask(void* arg);
static void   postMoistureData(INT16U moisture_data);
static void   changeSensorPowerState(INT8U new_sensor_state);
static void   initializeMoistureSensorCtrl();
static void   initializeMoistureSensorADC();
static INT16U senseMoisture();

//======================= function definitions =================================
/*
* initializeMoistureSenseTask - responsible for all initialization of the
*                               moistureSenseTask
*/
void initializeMoistureSenseTask() {
    INT8U                     os_error;

    // initialize module variables
    // TODO: check the configuration file for any changes to these defaults
    memset(&moistureSenseTaskVars,0,sizeof(MOISTURE_SENSE_TASK_VARS_T));
    moistureSenseTaskVars.device_activation_pend = DEVICE_ACTIVATION_PEND_DEFAULT;
    moistureSenseTaskVars.period = PERIOD_DEFAULT;

    // create the moisture data message queue
    moistureDataQueue = OSQCreate(&moistureDataMsg[0], 2);

    initializeMoistureSensorCtrl();
    initializeMoistureSensorADC();

    // create the moisture sense task
    os_error = OSTaskCreateExt(
        moistureSenseTask,
        (void*) 0,
        (OS_STK*)(&moistureSenseTaskVars.moistureSenseTaskStack[TASK_APP_MOISTURE_SENSE_STK_SIZE- 1]),
        TASK_APP_MOISTURE_SENSE_PRIORITY,
        TASK_APP_MOISTURE_SENSE_PRIORITY,
        (OS_STK*)moistureSenseTaskVars.moistureSenseTaskStack,
        TASK_APP_MOISTURE_SENSE_STK_SIZE,
        (void*) 0,
        OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR
    );
    ASSERT(os_error == OS_ERR_NONE);

    OSTaskNameSet(
        TASK_APP_MOISTURE_SENSE_PRIORITY,
        (INT8U*)TASK_APP_MOISTURE_SENSE_NAME,
        &os_error
    );
    ASSERT(os_error == OS_ERR_NONE);
}

/*
* moistureSenseTask - obtain periodic moisture data readings and post the
*                     results publicly
*/
static void moistureSenseTask(void* arg) {
    INT16U     moisture;

    while (1) {
        changeSensorPowerState(TRUE);

        // wait the configured number of ms
        OSTimeDly(moistureSenseTaskVars.device_activation_pend);

        moisture = senseMoisture();

        changeSensorPowerState(FALSE);

        // dnm_ucli_printf("Sensed raw moisture value of: %02x \r\n", moisture);

        postMoistureData(moisture);

        // pend for PERIOD ms
        OSTimeDly(moistureSenseTaskVars.period);
    }
}

/*
* retrieveMoistureSenseData - retrieve the latest moisture data
*                             Caution: This function PENDS until there is data!
*/
INT16U retrieveMoistureSenseData() {
    INT8U  os_error;
    INT16U moisture_data;

    moisture_data = (INT16U) OSQPend(moistureDataQueue, 0, &os_error);

    return moisture_data;
}

/*
* postMoistureData - post a new moisture data point.  It can be accessed
*                    publicly by calling retrieveMoistureSenseData
*/
static void postMoistureData(INT16U moisture_data) {
    INT8U os_error;

    os_error = OSQPost(moistureDataQueue, (void *)moisture_data);
    ASSERT(os_error == OS_ERR_NONE);
}

/*
* changeSensorPowerState - activate/deactivate the moisture sensor
*
* @param INT8U new_sensor_state TRUE for ON, FALSE for OFF
*/
static void changeSensorPowerState(INT8U new_sensor_state) {
    dn_error_t dn_error;

    dn_error = dn_write(
        MOISTURE_SENSOR_PWR_CTRL,
        &new_sensor_state,
        sizeof(new_sensor_state)
    );
    ASSERT(dn_error == DN_ERR_NONE);
}

/*
* senseMoisture - take a soil moisture reading.
*                 Caution: The ADC must be initialized prior to calling this
*                 function.  See initializeMoistureSensorADC()
*
* @return INT16U moisture ADC conversion representing the current moisture
*/
static INT16U senseMoisture() {
    INT16U                  moisture;
    int                     num_bytes_read;

    // read the adc
    num_bytes_read = dn_read(
        DN_ADC_AI_0_DEV_ID, // device
        &moisture,            // buf
        sizeof(moisture)      // bufSize
    );
    ASSERT(num_bytes_read == sizeof(moisture));

    return moisture;
}

/*
* initializeMoistureSensorCtrl - initialize the GPIO pin tied to the switch
*                                controlling the flow of power to the soil
*                                moisture sensor.  For toggling its power
*                                status, see changeSensorPowerState()
*/
static void initializeMoistureSensorCtrl() {
    dn_error_t              dn_error;
    dn_gpio_ioctl_cfg_out_t moisture_sensor_ctrl_cfg;

    // open and configure the MOISTURE_SENSOR_PWR_CTRL
    dn_error = dn_open(
        MOISTURE_SENSOR_PWR_CTRL,
        NULL,
        0
    );
    ASSERT(dn_error == DN_ERR_NONE);

    // configure as output
    moisture_sensor_ctrl_cfg.initialLevel = FALSE;
    dn_error = dn_ioctl(
        MOISTURE_SENSOR_PWR_CTRL,        // device
        DN_IOCTL_GPIO_CFG_OUTPUT,        // request
        &moisture_sensor_ctrl_cfg,       // args
        sizeof(moisture_sensor_ctrl_cfg) // argLen
    );
    ASSERT(dn_error == DN_ERR_NONE);
}

/*
* initializeMoistureSensorADC - initialize the ADC which is tied to the soil
*                               moisture sensor output for easy soil moisture
*                               readings.  For taking such a reading, see
*                               senseMoisture()
*/
static void initializeMoistureSensorADC() {
    dn_error_t             dn_error;
    dn_adc_drv_open_args_t adc_open_args;

    // open and configure the ADC
    // TODO: do the math and see if the nominal values can be easily found
    adc_open_args.rdacOffset  = 0;
    adc_open_args.vgaGain     = 0;
    adc_open_args.fBypassVga  = 1;
    dn_error = dn_open(
        DN_ADC_AI_0_DEV_ID,   // device
        &adc_open_args,       // args
        sizeof(adc_open_args) // argLen
    );
    ASSERT(dn_error == DN_ERR_NONE);
}