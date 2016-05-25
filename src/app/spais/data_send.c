/*
* Project: SPAIS
* Module: data_send.c
* Author: Cory Perkins
*/
#include "includes.h"
//=========================== definitions ======================================
// defaults
//============================ typedefs ========================================
typedef struct {
    OS_STK          dataSendTaskStack[TASK_APP_DATA_SEND_STK_SIZE];
} DATA_SEND_TASK_VARS_T;

//=========================== variables ========================================
DATA_SEND_TASK_VARS_T dataSendTaskVars;
//=========================== prototypes =======================================
static void dataSendTask(void* arg);
//======================= function definitions =================================
/*
* initializeDataSendTask - initialize the data send task
*/
void initializeDataSendTask() {
    INT8U                     os_error;

    // initialize module variables
    memset(&dataSendTaskVars,0,sizeof(DATA_SEND_TASK_VARS_T));

    // create the data send task
    os_error = OSTaskCreateExt(
        dataSendTask,
        (void*) 0,
        (OS_STK*)(&dataSendTaskVars.dataSendTaskStack[TASK_APP_DATA_SEND_STK_SIZE- 1]),
        TASK_APP_DATA_SEND_PRIORITY,
        TASK_APP_DATA_SEND_PRIORITY,
        (OS_STK*)dataSendTaskVars.dataSendTaskStack,
        TASK_APP_DATA_SEND_STK_SIZE,
        (void*) 0,
        OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR
    );
    ASSERT(os_error == OS_ERR_NONE);

    OSTaskNameSet(TASK_APP_DATA_SEND_PRIORITY, (INT8U*)TASK_APP_DATA_SEND_NAME, &os_error);
    ASSERT(os_error == OS_ERR_NONE);
}

/*
* dataSendTask - retrieve any publicly posted data and send it to the manager
*/
void dataSendTask(void* arg) {
    dn_error_t      dn_error;
    INT8U           os_error;

    // local interface variables
    loc_sendtoNW_t* pkToSend;
    INT8U           pkBuf[sizeof(loc_sendtoNW_t) + 1];
    INT8U           return_code;

    // moisture data
    INT16U          moistureSenseData;
    INT8U moisture_data_first_byte;
    INT8U moisture_data_second_byte;

    // initialize packet variables
    pkToSend = (loc_sendtoNW_t*)pkBuf;

    while (1) {
        // get whatever data needs to be sent to the network
        moistureSenseData = retrieveMoistureSenseData();

        // Debug Code
        // dnm_ucli_printf("dataSendTask Data Received: ");
        // dnm_ucli_printf("%02x", moistureSenseData);
        // dnm_ucli_printf("\r\n");

        if (moistureSenseData != NULL) {
            // todo: is this the best way to do this? I doubt it, it feels sloppy
            moisture_data_first_byte = (INT8U) moistureSenseData;
            moisture_data_second_byte = (INT8U) (moistureSenseData >> 8) & 0xff;

            // fill in packet "header"
            pkToSend->locSendTo.socketId    = loc_getSocketId();
            pkToSend->locSendTo.destAddr    = DN_MGR_IPV6_MULTICAST_ADDR;
            pkToSend->locSendTo.destPort    = WKP_USER_2;
            pkToSend->locSendTo.serviceType = DN_API_SERVICE_TYPE_BW;
            pkToSend->locSendTo.priority    = DN_API_PRIORITY_MED;
            pkToSend->locSendTo.packetId    = 0xFFFF;

            // fill in the payload
            pkToSend->locSendTo.payload[0] = moisture_data_first_byte;
            pkToSend->locSendTo.payload[1] = moisture_data_second_byte;

            // send the packet
            dn_error = dnm_loc_sendtoCmd(pkToSend, 1, &return_code);
            ASSERT (dn_error == DN_ERR_NONE);
        } else {
            // there is no new data to send.  Do Nothing.
        }

    }
}
