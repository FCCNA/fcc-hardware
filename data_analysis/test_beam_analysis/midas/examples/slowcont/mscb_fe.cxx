/********************************************************************\

  Name:         mscb_fe.c
  Created by:   Stefan Ritt

  Contents:     Example Slow control frontend for the MSCB system

  $Id: sc_fe.c 20457 2012-12-03 09:50:35Z ritt $

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include "mscb.h"
#include "midas.h"
#include "odbxx.h"
#include "msystem.h"
#include "mfe.h"
#include "class/multi.h"
#include "class/generic.h"
#include "device/mscbdev.h"
#include "device/mscbhvr.h"
#include "device/mdevice.h"
#include "device/mdevice_mscb.h"

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "SC Frontend";
/* The frontend file name, don't change it */
const char *frontend_file_name = __FILE__;

/*-- Equipment list ------------------------------------------------*/

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {
   {
      "Environment",     /* equipment name */
      {10, 0,            /* event ID, trigger mask */
       "SYSTEM",         /* event buffer */
       EQ_SLOW,          /* equipment type */
       0,                /* event source */
       "MIDAS",          /* format */
       TRUE,             /* enabled */
       RO_ALWAYS, 60000, /* read full event every 60 sec */
       100,              /* read one value every 100 msec */
       0,                /* number of sub events */
       1,                /* log history every second */
       "", "", ""},
      cd_multi_read,     /* readout routine */
      cd_multi           /* class driver main routine */
   },

   {""}
};

/*-- Error dispatcher causing communiction alarm -------------------*/

void scfe_error(const char *error) {
   char str[256];

   strlcpy(str, error, sizeof(str));
   cm_msg(MERROR, "scfe_error", "%s", str);
   al_trigger_alarm("MSCB", str, "MSCB Alarm", "Communication Problem", AT_INTERNAL);
}

/*-- Frontend Init -------------------------------------------------*/

INT frontend_init() {
   /* set error dispatcher for alarm functionality */
   mfe_set_error(scfe_error);

   /* set maximal retry count */
   mscb_set_max_retry(100);

   /*---- set correct ODB device addresses ----*/

   mdevice_mscb envIn("Environment", "Input", DF_INPUT, "mscbxxx.psi.ch");
   envIn.define_var(1, 0, "Temperature 1", 0.1);
   envIn.define_var(1, 1, "Temperature 2", 0.1);
   envIn.define_history_panel("Temperatures", 0, 1);

   mdevice_mscb envOut("Environment", "Output", DF_OUTPUT, "mscbxxx.psi.ch");
   envOut.define_var(1, 8, "Heat control 1");
   envOut.define_var(1, 9, "Heat control 2");
   envIn.define_history_panel("Heat Control", 0, 1);

   return CM_SUCCESS;
}
