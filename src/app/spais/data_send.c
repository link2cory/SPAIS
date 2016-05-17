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


#include "data_send_cfg.h"
#include "moisture_sense_cfg.h"
//=========================== definitions =====================================
// defaults
//=========================== variables =======================================
typedef struct {
    OS_STK          dataSendTaskStack[TASK_APP_DATA_SEND_STK_SIZE];
} data_send_task_vars_t;

data_send_task_vars_t data_send_task_v;
//=========================== prototypes ======================================
static void dataSendTask(void* arg);
//=========================== initialization ==================================
void initializeDataSendTask() {
    INT8U                     osErr;

    // initialize module variables
    memset(&data_send_task_v,0,sizeof(data_send_task_vars_t));

    // create the data send task
    osErr = OSTaskCreateExt(
        dataSendTask,
        (void*) 0,
        (OS_STK*)(&data_send_task_v.dataSendTaskStack[TASK_APP_DATA_SEND_STK_SIZE- 1]),
        TASK_APP_DATA_SEND_PRIORITY,
        TASK_APP_DATA_SEND_PRIORITY,
        (OS_STK*)data_send_task_v.dataSendTaskStack,
        TASK_APP_DATA_SEND_STK_SIZE,
        (void*) 0,
        OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR
    );
    ASSERT(osErr == OS_ERR_NONE);
    OSTaskNameSet(TASK_APP_DATA_SEND_PRIORITY, (INT8U*)TASK_APP_DATA_SEND_NAME, &osErr);
    ASSERT(osErr == OS_ERR_NONE);
}
//=========================== Soil Moisture Sense task =================================
void dataSendTask(void* arg) {
    dn_error_t      dnErr;
    INT8U           osErr;

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
            dnErr = dnm_loc_sendtoCmd(pkToSend, 1, &return_code);
            ASSERT (dnErr == DN_ERR_NONE);
        } else {
            // there is no new data to send.  Do Nothing.
        }

    }
}
