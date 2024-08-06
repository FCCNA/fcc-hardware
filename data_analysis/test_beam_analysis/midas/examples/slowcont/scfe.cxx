/********************************************************************\

  Name:         frontend.c
  Created by:   Stefan Ritt

  Contents:     Example Slow Control Frontend program. Defines two
                slow control equipments, one for a HV device and one
                for a multimeter (usually a general purpose PC plug-in
                card with A/D inputs/outputs. As a device driver,
                the "null" driver is used which simulates a device
                without accessing any hardware. The used class drivers
                cd_hv and cd_multi act as a link between the ODB and
                the equipment and contain some functionality like
                ramping etc. To form a fully functional frontend,
                the device driver "null" has to be replaces with
                real device drivers.

  $Id$

\********************************************************************/

#include <stdio.h>
#include "midas.h"
#include "mfe.h"
#include "history.h"
#include "class/hv.h"
#include "class/multi.h"
#include "device/nulldev.h"
#include "bus/null.h"

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "SC Frontend";
/* The frontend file name, don't change it */
const char *frontend_file_name = __FILE__;

/*-- Equipment list ------------------------------------------------*/

/* device driver list */
DEVICE_DRIVER hv_driver[] = {
   {"Dummy Device", nulldev, 16, null},
   {""}
};

DEVICE_DRIVER multi_driver[] = {
   {"Input", nulldev, 3, null, DF_INPUT},
   {"Output", nulldev, 2, null, DF_OUTPUT},
   {""}
};

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {

   {"Environment",              /* equipment name */
    {10, 0,                     /* event ID, trigger mask */
     "SYSTEM",                  /* event buffer */
     EQ_SLOW,                   /* equipment type */
     0,                         /* event source */
     "FIXED",                   /* format */
     TRUE,                      /* enabled */
     RO_RUNNING | RO_TRANSITIONS,        /* read when running and on transitions */
     60000,                     /* produce event every 60 sec */
     0,                         /* stop run after this event limit */
     0,                         /* number of sub events */
     1,                         /* log history every second */
     "", "", ""} ,
    cd_multi_read,              /* readout routine */
    cd_multi,                   /* class driver main routine */
    multi_driver,               /* device driver list */
    NULL,                       /* init string */
    },

   {""}
};


/*-- Frontend Init -------------------------------------------------*/

INT frontend_init()
{
   hs_define_panel("Environment", "Sines", {
                      "Environment/Input:Input Channel 0",
                      "Environment/Input:Input Channel 1"
                   });

   return CM_SUCCESS;
}
