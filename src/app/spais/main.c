/*
* Project: SPAIS
* Module: main.c
* Author: Cory Perkins
*/
#include "includes.h"
//=========================== typedefs =========================================
typedef enum NOTIFICATION_TYPE
{
    valve_control,
    configurator
} NOTIFICATION_TYPE;

//=========================== variables ========================================
OS_EVENT *joinedSem;

//=========================== prototypes =======================================
dn_error_t networkNotificationCallback(dn_api_loc_notif_received_t* notification, INT8U length);

//=========================== initialization ===================================
/*
* p2_init - initialize the program execution
*
* @return 0
*/
int p2_init(void) {
    INT8U os_error;

    // create a semaphore to indicate mote joined the network
    joinedSem = OSSemCreate(0);
    ASSERT(joinedSem != NULL);

    // initialize local interface task
    loc_task_init(
        JOIN_YES,       // fJoin
        NULL,           // netId
        WKP_USER_1,     // udpPort
        joinedSem,      // joinedSem
        5000,           // bandwidth
        NULL            // serviceSem
    );

    // initialize command line interface task (debug only)
    cli_task_init(
      "spais", // appName
      NULL     // cliCmds
   );

    // initialize/create any other tasks
    // TODO: send the joinedSem to these methods and use it similarly to the
    // loc_task_init() function
    initializeMoistureSenseTask(joinedSem);
    initializeDataSendTask();
    initializeValveControlTask();

    // register the network notification callback
    dnm_loc_registerRxNotifCallback(networkNotificationCallback);

    return 0;
}

/*
* networkNotificationCallback - route notification data to the appropriate task
*                               Caution: this method is a callback, it should
*                               NEVER be called directly!
* @return dn_error_t DN_ERR_NONE
*/
dn_error_t networkNotificationCallback(dn_api_loc_notif_received_t* notification, INT8U length) {
    // dnm_ucli_printf("Notification received!\r\n");

    NOTIFICATION_TYPE notification_type;
    notification_type = (NOTIFICATION_TYPE) notification->data[0];
    // extract the notification type and handle it appropriately
    if (notification_type == valve_control) {
        // send the following byte to the valve control task
        // dnm_ucli_printf("Received a Valve Control notification!\r\n");
        postValveControlData(notification->data[1]);
    } else if (notification_type == configurator) {
        // dnm_ucli_printf("Received a Configurator notification!\r\n");
        // TODO: handle configurator notifications by sending the rest of the data to
        // the configurator task
    } else {
        // got a notification that is not supported
        // dnm_ucli_printf("There is no support for the given notification!\r\n");
        // TODO: Consider setting the return value to SOME_ERROR
   }

   return DN_ERR_NONE;
}
//==============================================================================
//=========================== install a kernel header ==========================
//==============================================================================

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
