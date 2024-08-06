/********************************************************************\

  Name:         epics_fe.c
  Created by:   Stefan Ritt

  Contents:     General EPICS frontend used for the MSetPoint system

\********************************************************************/

#include <cstring>

#include <epicsStdlib.h>
#include "cadef.h"
#include "dbDefs.h"
#include "epicsTime.h"

#include "midas.h"
#include "mfe.h"
#include "odbxx.h"

/*-- Globals -------------------------------------------------------*/

// timeout in seconds for caget operations
#define CAGET_TIMEOUT 30.0

// device types, must match msp_config.html:dt_string[]
#define DT_MAGNET      1
#define DT_BEAMBLOCKER 2
#define DT_PSA         3
#define DT_SEPARATOR   4
#define DT_SLIT        5
#define DT_VALUE       6

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "EPICS Frontend";
/* The frontend file name, don't change it */
const char *frontend_file_name = __FILE__;

/*-- Equipment list ------------------------------------------------*/

BOOL equipment_common_overwrite = TRUE;

int epics_read(char *pevent, int off);
int epics_loop(void);
int epics_init(void);
int epics_get_demand(int channel);
int epics_get_measured(int channel);

void demandCallback(midas::odb &o);
void commandCallback(midas::odb &o);

EQUIPMENT equipment[] = {

   {"EPICS",                    /* equipment name */
    {21, 0,                     /* event ID, trigger mask */
     "SYSTEM",                  /* event buffer */
     EQ_PERIODIC,               /* equipment type */
     0,                         /* event source */
     "MIDAS",                   /* format */
     TRUE,                      /* enabled */
     RO_ALWAYS,
     60000,                     /* read event every 60 sec */
     0,                         /* readout pause */
     0,                         /* number of sub events */
     10,                        /* log history every 10 seconds */
     "", "", ""} ,
    epics_read                  /* readout routine */
    },

   {""}
};

struct {
    int          length = 0;
    midas::odb   settings;
    midas::odb   variables;
    chid         *demand = nullptr;
    chid         *measured = nullptr;
    chid         *status = nullptr;
    chid         *command = nullptr;
    float        *demandCache = nullptr;
    unsigned int updateInterval;
    bool         allowWriteAccess;
} beamline;

/*-- Error dispatcher causing communiction alarm -------------------*/

void epics_fe_error(const char *error)
{
   cm_msg(MERROR, "epics_fe_error", "%s", error);
}

/*-- Readout routine -----------------------------------------------*/

int epics_read(char *pevent, int off)
{
   float *pdata;

   // init bank structure
   bk_init(pevent);

   // create a bank with demand values
   bk_create(pevent, "DMND", TID_FLOAT, (void **)&pdata);
   for (int i = 0; i < beamline.length; i++)
      *pdata++ = beamline.variables["Demand"][i];
   bk_close(pevent, pdata);

   // create a bank with measured values
   bk_create(pevent, "MSRD", TID_FLOAT, (void **)&pdata);
   for (int i = 0; i < beamline.length; i++)
      *pdata++ = beamline.variables["Measured"][i];
   bk_close(pevent, pdata);

   return bk_size(pevent);
}

/*-- Frontend Init -------------------------------------------------*/

int frontend_init()
{
   /* set error dispatcher for alarm functionality */
   mfe_set_error(epics_fe_error);

   // check for EPICS gateway in ODB
   if (!midas::odb::exists("/Equipment/EPICS/Settings/EPICS gateway")) {
      std::cout << "Missing \"Equipment/EPICS/Settings/EPICS gateway\" in ODB." << std::endl;
      std::cout << "Please load beamline configuration file into ODB." << std::endl;
      return FE_ERR_ODB;
   }

   // load EPICS settings from ODB
   beamline.settings.connect("/Equipment/EPICS/Settings");
   beamline.length = beamline.settings["Names"].size();
   beamline.variables.connect("/Equipment/EPICS/Variables");

   beamline.updateInterval = beamline.settings["Update interval"];
   beamline.allowWriteAccess = beamline.settings["Allow write access"];

   // setup environment
   std::string gw = beamline.settings["EPICS Gateway"];
   setenv("EPICS_CA_ADDR_LIST", gw.c_str(), 1);
   setenv("EPICS_CA_AUTO_ADDR_LIST", "NO", 1);

   // create or correct "Variables" in ODB
   if (!midas::odb::exists("/Equipment/EPICS/Variables/Demand"))
      beamline.variables["Demand"] = std::vector<float>(beamline.length);
   else
      beamline.variables["Demand"].resize(beamline.length);

   if (!midas::odb::exists("/Equipment/EPICS/Variables/Measured"))
      beamline.variables["Measured"] = std::vector<float>(beamline.length);
   else
      beamline.variables["Measured"].resize(beamline.length);

   std::string startCommand(__FILE__);
   std::string s("build/epics_fe");
   auto i = startCommand.find("epics_fe.cxx");
   startCommand.replace(i, s.length(), s);
   startCommand = startCommand.substr(0, i + s.length());

   // set start command in ODB if not already set
   if (!midas::odb::exists("/Programs/EPICS Frontend/Start command")) {
      midas::odb efe("/Programs/EPICS Frontend");
      efe["Start command"].set_string_size(startCommand, 256);
   } else {
      midas::odb efe("/Programs/EPICS Frontend");
      if (efe["Start command"] == std::string(""))
         efe["Start command"].set_string_size(startCommand, 256);
   }

   install_frontend_loop(epics_loop);

   return epics_init();
}

/*------------------------------------------------------------------*/

int epics_init(void)
{
   int status = FE_SUCCESS;

   /* initialize driver */
   status = ca_task_initialize();
   if (!(status & CA_M_SUCCESS)) {
      cm_msg(MERROR, "epics_init", "Unable to initialize EPICS");
      return FE_ERR_HW;
   }

   beamline.demand = (chid *) calloc(beamline.length, sizeof(chid));
   beamline.measured = (chid *) calloc(beamline.length, sizeof(chid));
   beamline.status = (chid *) calloc(beamline.length, sizeof(chid));
   beamline.command = (chid *) calloc(beamline.length, sizeof(chid));
   beamline.demandCache = (float *) calloc(beamline.length, sizeof(float));

   for (int i = 0; i < beamline.length; i++) {

      if (!beamline.settings["Enabled"][i]) {
         printf("Channel %d disabled\r", i);
         fflush(stdout);
         continue;
      }
      printf("Channel %d\r", i);
      fflush(stdout);

      std::string demand = beamline.settings["CA Demand"][i];
      if (demand != std::string("")) {
         std::string name = beamline.settings["CA Name"][i];
         name += demand;
         status = ca_create_channel(name.c_str(), nullptr, nullptr, 0, &(beamline.demand[i]));
         SEVCHK(status, "ca_create_channel");
         if (ca_pend_io(5.0) == ECA_TIMEOUT) {
            cm_msg(MERROR, "epics_init", "Cannot connect to EPICS channel %s", name.c_str());
            status = FE_ERR_HW;
            break;
         }
      }

      std::string measured = beamline.settings["CA Measured"][i];
      if (measured != std::string("")) {
         std::string name = beamline.settings["CA Name"][i];
         name += measured;
         status = ca_create_channel(name.c_str(), nullptr, nullptr, 0, &(beamline.measured[i]));
         SEVCHK(status, "ca_create_channel");
         if (ca_pend_io(5.0) == ECA_TIMEOUT) {
            cm_msg(MERROR, "epics_init", "Cannot connect to EPICS channel %s", name.c_str());
            status = FE_ERR_HW;
            break;
         }
      }

      std::string stat = beamline.settings["CA Status"][i];
      if (stat != std::string("")) {
         std::string name = beamline.settings["CA Name"][i];
         name += stat;
         status = ca_create_channel(name.c_str(), nullptr, nullptr, 0, &(beamline.status[i]));
         SEVCHK(status, "ca_create_channel");
         if (ca_pend_io(5.0) == ECA_TIMEOUT) {
            cm_msg(MERROR, "epics_init", "Cannot connect to EPICS channel %s", name.c_str());
            status = FE_ERR_HW;
            break;
         }
      }

      std::string command = beamline.settings["CA Command"][i];
      if (command != std::string("")) {
         std::string name = beamline.settings["CA Name"][i];
         if ((int)beamline.settings["Device type"][i] == DT_SEPARATOR)
            name = name.substr(0, name.size() - 1); // Strip 'N' from SEP41VHVN
         name += command;
         status = ca_create_channel(name.c_str(), nullptr, nullptr, 0, &(beamline.command[i]));
         SEVCHK(status, "ca_create_channel");
         if (ca_pend_io(5.0) == ECA_TIMEOUT) {
            cm_msg(MERROR, "epics_init", "Cannot connect to EPICS channel %s", name.c_str());
            status = FE_ERR_HW;
            break;
         }
      }
   }

   // read initial demand values
   for (int i = 0; i < beamline.length; i++) {
      beamline.demandCache[i] = (float) ss_nan();
      epics_get_demand(i);
   }

   // install callback for demand changes
   beamline.variables["Demand"].watch(demandCallback);

   // install callback for direct commands
   beamline.settings["Command"].watch(commandCallback);

   printf("\n");

   return status;
}

/*------------------------------------------------------------------*/

int epics_get_demand(int channel)
{
   int status;
   char str[80];
   float d;

   // Skip channels which do not support demand values
   if (beamline.demand[channel] == nullptr)
      return FE_SUCCESS;

   // Skip disabled channels
   if (!beamline.settings["Enabled"][channel])
      return FE_SUCCESS;

   std::string name = beamline.settings["Names"][channel];

   if ((int)beamline.settings["Device type"][channel] == DT_BEAMBLOCKER) {
      status = ca_get(DBR_STRING, beamline.demand[channel], str);
      SEVCHK(status, "ca_get");
      if (ca_pend_io(CAGET_TIMEOUT) == ECA_TIMEOUT) {
         cm_msg(MERROR, "epics_get_demand", "Timeout on EPICS channel %s",
                name.c_str());
         return FE_ERR_HW;
      }
      d = (str[0] == 'O') ? 1 : 0;
   } else {
      status = ca_get(DBR_FLOAT, beamline.demand[channel], &d);
      SEVCHK(status, "ca_get");
      if (ca_pend_io(CAGET_TIMEOUT) == ECA_TIMEOUT) {
         cm_msg(MERROR, "epics_get_demand", "Timeout on EPICS channel %s",
                name.c_str());
         return FE_ERR_HW;
      }
   }

   // set demand in ODB only if it differs (avoid loops via hotlink)
   if (d != beamline.demandCache[channel]) {
      beamline.variables["Demand"][channel] = d;
      beamline.demandCache[channel] = d;
   }

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

int epics_get_measured(int channel)
{
   int status;
   char str[80];

   // Skip write-only channels
   if (beamline.measured[channel] == nullptr)
      return FE_SUCCESS;

   // Skip disabled channels
   if (!beamline.settings["Enabled"][channel])
      return FE_SUCCESS;

   std::string name = beamline.settings["Names"][channel];

   if ((int)beamline.settings["Device type"][channel] == DT_BEAMBLOCKER ||
       (int)beamline.settings["Device type"][channel] == DT_PSA) {

      int d;
      float f;
      status = ca_get(DBR_LONG, beamline.measured[channel], &d);
      SEVCHK(status, "ca_get");
      if (ca_pend_io(CAGET_TIMEOUT) == ECA_TIMEOUT) {
         cm_msg(MERROR, "epics_get_measured", "Timeout on EPICS channel %s",
                name.c_str());
         return FE_ERR_HW;
      }

      if ((int)beamline.settings["Device type"][channel] == DT_BEAMBLOCKER) {
         // bit0: open, bit1: closed, bit2: psa ok
         if ((d & 3) == 0)
            f = 0.5;
         else if (d & 1)
            f = 1;
         else if (d & 2)
            f = 0;
         else
            f = (float)ss_nan();
      } else {
         // bit2: PSA Ok
         if (d & 4)
            f = 1;
         else
            f = 0;
      }

      beamline.variables["Measured"][channel] = f;

   } else {

      float f;
      status = ca_get(DBR_FLOAT, beamline.measured[channel], &f);
      SEVCHK(status, "ca_get");
      if (ca_pend_io(CAGET_TIMEOUT) == ECA_TIMEOUT) {
         cm_msg(MERROR, "epics_get_measured", "Timeout on EPICS channel %s",
                name.c_str());
         return FE_ERR_HW;
      }
      beamline.variables["Measured"][channel] = f;
   }

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

int epics_get_status(int channel)
{
   int status;
   char s[80];

   // Skip channels which do not support status readback
   if (beamline.status[channel] == nullptr)
      return FE_SUCCESS;

   // Skip disabled channels
   if (!beamline.settings["Enabled"][channel])
      return FE_SUCCESS;

   status = ca_get(DBR_STRING, beamline.status[channel], s);
   SEVCHK(status, "ca_get");
   std::string name = beamline.settings["Names"][channel];
   if (ca_pend_io(CAGET_TIMEOUT) == ECA_TIMEOUT) {
      cm_msg(MERROR, "epics_get_status", "Timeout on EPICS channel %s",
             name.c_str());
      return FE_ERR_HW;
   }

   // translate to Englisch
   std::string str(s);
   if (str == "Eingeschaltet")
      str = "On";
   else if (str == "Ausgeschaltet")
      str = "Off";
   else if (str.compare(1, str.size()-1, "berlast") == 0)
      str = "Overload";

   beamline.settings["Status"][channel].set_string_size(str, 64);

   return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

void epics_set_demand(int channel, float value)
{
   int status, channelType;
   char s[80];

   // skip readonly channels
   if (beamline.demand[channel] == nullptr)
      return;

   // Skip disabled channels
   if (!beamline.settings["Enabled"][channel])
      return;

   channelType = beamline.settings["Device type"][channel];
   std::string channelName = beamline.settings["Names"][channel];

   if (channelType == DT_BEAMBLOCKER)
      status = ca_put(DBR_STRING, beamline.demand[channel], value == 1 ? "OPEN" : "CLOSE");

   else if (channelType == DT_SLIT) {
      status = ca_put(DBR_FLOAT, beamline.demand[channel], &value);
      if (status != CA_M_SUCCESS)
         cm_msg(MERROR, "epics_set_demand", "Cannot write to EPICS channel %s", channelName.c_str());

      if (beamline.command[channel] == nullptr) {
         cm_msg(MERROR, "epics_set_damand", "No \"CA Command\" defined for EPICS slit channel %s", channelName.c_str());
         return;
      }
      status = ca_put(DBR_STRING, beamline.command[channel], "MOVE_TO_SOL");
      if (status != CA_M_SUCCESS)
         cm_msg(MERROR, "epics_set_demand", "Cannot write to EPICS channel %s", channelName.c_str());
   }

   else if (channelType == DT_SEPARATOR) {
      status = ca_put(DBR_FLOAT, beamline.demand[channel], &value);
      if (status != CA_M_SUCCESS)
         cm_msg(MERROR, "epics_set_demand", "Cannot write to EPICS channel %s", channelName.c_str());

      if (beamline.command[channel] == nullptr) {
         cm_msg(MERROR, "epics_set_damand", "No \"CA Command\" defined for EPICS separator channel %s", channelName.c_str());
         return;
      }
      short int d = 3; // HVSET command
      status = ca_put(DBR_SHORT, beamline.command[channel], &d);
      if (status != CA_M_SUCCESS)
         cm_msg(MERROR, "epics_set_demand", "Cannot write to EPICS channel %s", channelName.c_str());
   } else
      status = ca_put(DBR_FLOAT, beamline.demand[channel], &value);

   if (status != CA_M_SUCCESS)
      cm_msg(MERROR, "epics_set_demand", "Cannot write to EPICS channel \"%s\"", channelName.c_str());
}

/*------------------------------------------------------------------*/

void epics_send_command(int channel, std::string cmd)
{
   int status, channelType;

   /* skip readonly channels */
   if (beamline.command[channel] == nullptr)
      return;

   // Skip disabled channels
   if (!beamline.settings["Enabled"][channel])
      return;

   channelType = beamline.settings["Device type"][channel];
   std::string channelName = beamline.settings["Names"][channel];

   status = ca_put(DBR_STRING, beamline.command[channel], cmd.c_str());
   if (status != CA_M_SUCCESS)
      cm_msg(MERROR, "epics_send_command", "Cannot send command \"%s\" to EPICS channel \"%s\"",
             cmd.c_str(), channelName.c_str());
}

/*------------------------------------------------------------------*/

void demandCallback(midas::odb &o) {
   std::vector<float> newDemand = o;

    // check write flag
    if (!beamline.allowWriteAccess)
        return;

    for (int i = 0; i < beamline.length; i++) {
      if (newDemand[i] != beamline.demandCache[i]) {
         epics_set_demand(i, newDemand[i]);
         // printf("Set %d to %f\n", i, newDemand[i]);
         beamline.demandCache[i] = newDemand[i];
      }
   }
}

/*------------------------------------------------------------------*/

void commandCallback(midas::odb &o) {
   std::vector<std::string> cmd = o;

   // check write flag
   if (!beamline.allowWriteAccess)
      return;

   for (int i = 0; i < beamline.length; i++) {
      if (!cmd[i].empty()) {
         epics_send_command(i, cmd[i]);
//         printf("Send command \"%s\" to \"%s\"\n", cmd[i].c_str(),
//                beamline.settings["Names"][i].s().c_str());
         o[i] = ""; // delete command
      }
   }
}

/*------------------------------------------------------------------*/

int epics_loop(void)
{
   static DWORD last_time_measured = 0;
   static DWORD last_time_status = 0;
   static DWORD last_time_demand = 0;

   // read values once per second
   if (ss_millitime() - last_time_measured > beamline.updateInterval) {
      for (int i = 0; i < beamline.length; i++)
         epics_get_measured(i);
      last_time_measured = ss_millitime();
   }

   // read status once every ten seconds
   if (ss_time() >= last_time_status + 10) {
      for (int i = 0; i < beamline.length; i++)
         epics_get_status(i);
      last_time_status = ss_time();
   }

   if (beamline.allowWriteAccess) {
      // read demand values once every five minute (could have been changed by SetPoint)
      if (ss_time() >= last_time_demand + 10 * 60 * 5) {
         for (int i = 0; i < beamline.length; i++)
            epics_get_demand(i);
         last_time_demand = ss_time();
      }
   } else {
      // read demand values once every ten seconds (could have been changed by other MSetPoint)
      if (ss_time() >= last_time_demand + 10) {
         for (int i = 0; i < beamline.length; i++)
            epics_get_demand(i);
         last_time_demand = ss_time();
      }

   }

   ss_sleep(10); // don't eat all CPU
   return CM_SUCCESS;
}