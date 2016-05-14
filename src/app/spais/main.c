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
#include "app_cfg.h"

// SPAIS specific
#include "main_cfg.h"
#include "moisture_sense_cfg.h"
#include "data_send_cfg.h"
//=========================== definitions =====================================
//=========================== variables =======================================
typedef struct {
    INT16U moisture;
} data_send_t;

data_send_t data_send_v;

OS_EVENT *joinedSem;
//=========================== initialization ==================================

/**
 \brief This is the entry point in the application code.
 */
int p2_init(void) {
    INT8U osErr;

    // create a semaphore to indicate mote joined
    joinedSem = OSSemCreate(0);
    ASSERT (joinedSem!=NULL);

    // local interface task
    loc_task_init(
        JOIN_YES,       // fJoin
        NULL,           // netId
        WKP_USER_1,       // udpPort
        joinedSem,      // joinedSem
        BANDWIDTH_NONE, // bandwidth
        NULL            // serviceSem
    );

    // initialize/create any other tasks
    initializeMoistureSenseTask();
    initializeDataSendTask();

    // task initialize/create end

    // wait for the mote to have joined
    OSSemPend(joinedSem,0,&osErr);
    ASSERT(osErr == OS_ERR_NONE);

    return 0;
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
