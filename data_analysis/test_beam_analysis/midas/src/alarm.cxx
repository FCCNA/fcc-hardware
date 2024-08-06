/********************************************************************\

  Name:         ALARM.C
  Created by:   Stefan Ritt

  Contents:     MIDAS alarm functions

  $Id$

\********************************************************************/

#include "midas.h"
#include "msystem.h"
#ifndef HAVE_STRLCPY
#include "strlcpy.h"
#endif
#include <assert.h>

/**dox***************************************************************/
/** @file alarm.c
The Midas Alarm file
*/

/** @defgroup alfunctioncode Midas Alarm Functions (al_xxx)
 */

/**dox***************************************************************/
/** @addtogroup alfunctioncode
 *
 *  @{  */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************\
*                                                                    *
*                     Alarm functions                                *
*                                                                    *
\********************************************************************/

/********************************************************************/
BOOL al_evaluate_condition(const char* alarm_name, const char *condition, std::string *pvalue)
{
   int i, j, size, status;

   //char str[256];
   //strcpy(str, condition);

   std::vector<char> buf;
   buf.insert(buf.end(), condition, condition+strlen(condition)+1);
   char* str = buf.data();

   /* find value and operator */
   char op[3];
   op[0] = op[1] = op[2] = 0;
   for (i = strlen(str) - 1; i > 0; i--)
      if (strchr("<>=!&", str[i]) != NULL)
         break;
   op[0] = str[i];
   for (j = 1; str[i + j] == ' '; j++)
      ;

   std::string value2_str = str + i + j;
   double      value2 = atof(value2_str.c_str());

   str[i] = 0;

   if (i > 0 && strchr("<>=!&", str[i - 1])) {
      op[1] = op[0];
      op[0] = str[--i];
      str[i] = 0;
   }

   i--;
   while (i > 0 && str[i] == ' ')
      i--;
   str[i + 1] = 0;

   /* check if function */
   std::string function;
   if (str[i] == ')') {
      str[i--] = 0;
      if (strchr(str, '(')) {
         *strchr(str, '(') = 0;
         function = str;
         for (i = strlen(str) + 1, j = 0; str[i]; i++, j++)
            str[j] = str[i];
         str[j] = 0;
         i = j - 1;
      }
   }

   /* find key */
   int idx1 = 0;
   int idx2 = 0;

   if (str[i] == ']') {
      str[i--] = 0;
      if (str[i] == '*') {
         idx1 = -1;
         while (i > 0 && str[i] != '[')
            i--;
         str[i] = 0;
      } else if (strchr(str, '[') && strchr(strchr(str, '['), '-')) {
         while (i > 0 && isdigit(str[i]))
            i--;
         idx2 = atoi(str + i + 1);
         while (i > 0 && str[i] != '[')
            i--;
         idx1 = atoi(str + i + 1);
         str[i] = 0;
      } else {
         while (i > 0 && isdigit(str[i]))
            i--;
         idx1 = idx2 = atoi(str + i + 1);
         str[i] = 0;
      }
   }

   HNDLE hDB, hkey;
   cm_get_experiment_database(&hDB, NULL);
   db_find_key(hDB, 0, str, &hkey);
   if (!hkey) {
      cm_msg(MERROR, "al_evaluate_condition", "Alarm \"%s\": Cannot find ODB key \"%s\" to evaluate alarm condition", alarm_name, str);
      if (pvalue)
         *pvalue = "(cannot find in odb)";
      return FALSE;
   }

   KEY key;
   db_get_key(hDB, hkey, &key);

   if (idx1 < 0) {
      idx1 = 0;
      idx2 = key.num_values - 1;
   }

   //printf("Alarm \"%s\": [%s], idx1 %d, idx2 %d, op [%s], value2 [%s] %g, function [%s]\n", alarm_name, condition, idx1, idx2, op, value2_str.c_str(), value2, function.c_str());

   for (int idx = idx1; idx <= idx2; idx++) {
      std::string value1_str;
      double value1 = 0;

      if (equal_ustring(function.c_str(), "access")) {
         /* check key access time */
         DWORD dtime;
         db_get_key_time(hDB, hkey, &dtime);
         value1_str = msprintf("%d", dtime);
         value1 = atof(value1_str.c_str());
      } else if (equal_ustring(function.c_str(), "access_running")) {
         /* check key access time if running */
         DWORD dtime;
         db_get_key_time(hDB, hkey, &dtime);
         int state = 0;
         size = sizeof(state);
         db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, FALSE);
         if (state == STATE_RUNNING) {
            value1_str = msprintf("%d", dtime).c_str();
            value1 = atof(value1_str.c_str());
         } else {
            value1_str = "0";
            value1 = 0;
         }
      } else {
         /* get key data and convert to double */
         status = db_get_key(hDB, hkey, &key);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "al_evaluate_condition", "Alarm \"%s\": Cannot get ODB key \"%s\" to evaluate alarm condition", alarm_name, str);
            if (pvalue)
               *pvalue = "(odb error)";
            return FALSE;
         } else {
            char data[256];
            size = sizeof(data);
            status = db_get_data_index(hDB, hkey, data, &size, idx, key.type);
            if (status != DB_SUCCESS) {
               cm_msg(MERROR, "al_evaluate_condition", "Alarm \"%s\": Cannot get ODB value \"%s\" index %d to evaluate alarm condition", alarm_name, str, idx);
               if (pvalue)
                  *pvalue = "(odb error)";
               return FALSE;
            }

            value1_str = db_sprintf(data, size, 0, key.type);
            value1 = atof(value1_str.c_str());
         }
      }

      /* convert boolean values to integers */
      if (key.type == TID_BOOL) {
         value1 = (value1_str[0] == 'Y' || value1_str[0] == 'y' || value1_str[0] == '1');
         value2 = (value2_str[0] == 'Y' || value2_str[0] == 'y' || value2_str[0] == '1');
      }
      
      /* return value */
      if (pvalue) {
         if (idx1 != idx2)
            *pvalue = msprintf("[%d] %s", idx, value1_str.c_str());
         else
            *pvalue = value1_str;
      }
      
      /* now do logical operation */
      if (strcmp(op, "=") == 0)
         if (value1 == value2)
            return TRUE;
      if (strcmp(op, "==") == 0)
         if (value1 == value2)
            return TRUE;
      if (strcmp(op, "!=") == 0)
         if (value1 != value2)
            return TRUE;
      if (strcmp(op, "<") == 0)
         if (value1 < value2)
            return TRUE;
      if (strcmp(op, ">") == 0)
         if (value1 > value2)
            return TRUE;
      if (strcmp(op, "<=") == 0)
         if (value1 <= value2)
            return TRUE;
      if (strcmp(op, ">=") == 0)
         if (value1 >= value2)
            return TRUE;
      if (strcmp(op, "&") == 0)
         if (((unsigned int) value1 & (unsigned int) value2) > 0)
            return TRUE;
   }

   return FALSE;
}

/**dox***************************************************************/
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Trigger a certain alarm.
\code  ...
  lazy.alarm[0] = 0;
  size = sizeof(lazy.alarm);
  db_get_value(hDB, pLch->hKey, "Settings/Alarm Class", lazy.alarm, &size, TID_STRING, TRUE);

  // trigger alarm if defined
  if (lazy.alarm[0])
    al_trigger_alarm("Tape", "Tape full...load new one!", lazy.alarm, "Tape full", AT_INTERNAL);
  ...
\endcode
@param alarm_name Alarm name, defined in /alarms/alarms
@param alarm_message Optional message which goes with alarm
@param default_class If alarm is not yet defined under
                    /alarms/alarms/\<alarm_name\>, a new one
                    is created and this default class is used.
@param cond_str String displayed in alarm condition
@param type Alarm type, one of AT_xxx
@return AL_SUCCESS, AL_INVALID_NAME
*/
INT al_trigger_alarm(const char *alarm_name, const char *alarm_message, const char *default_class, const char *cond_str,
                     INT type) {
   if (rpc_is_remote())
      return rpc_call(RPC_AL_TRIGGER_ALARM, alarm_name, alarm_message, default_class, cond_str, type);

#ifdef LOCAL_ROUTINES
   {
      int status, size;
      HNDLE hDB, hkeyalarm, hkey;
      ALARM a;
      BOOL flag;
      ALARM_ODB_STR(alarm_odb_str);

      cm_get_experiment_database(&hDB, NULL);

      /* check online mode */
      flag = TRUE;
      size = sizeof(flag);
      db_get_value(hDB, 0, "/Runinfo/Online Mode", &flag, &size, TID_INT, TRUE);
      if (!flag)
         return AL_SUCCESS;

      /* find alarm */
      std::string alarm_path = msprintf("/Alarms/Alarms/%s", alarm_name);

      db_find_key(hDB, 0, alarm_path.c_str(), &hkeyalarm);
      if (!hkeyalarm) {
         /* alarm must be an internal analyzer alarm, so create a default alarm */
         status = db_create_record(hDB, 0, alarm_path.c_str(), strcomb1(alarm_odb_str).c_str());
         db_find_key(hDB, 0, alarm_path.c_str(), &hkeyalarm);
         if (!hkeyalarm) {
            cm_msg(MERROR, "al_trigger_alarm", "Cannot create alarm record for alarm \"%s\", db_create_record() status %d", alarm_path.c_str(), status);
            return AL_ERROR_ODB;
         }

         if (default_class && default_class[0])
            db_set_value(hDB, hkeyalarm, "Alarm Class", default_class, 32, 1, TID_STRING);
         status = TRUE;
         db_set_value(hDB, hkeyalarm, "Active", &status, sizeof(status), 1, TID_BOOL);
      }

      /* set parameters for internal alarms */
      if (type != AT_EVALUATED && type != AT_PERIODIC) {
         db_set_value(hDB, hkeyalarm, "Type", &type, sizeof(INT), 1, TID_INT);
         char str[256];
         strlcpy(str, cond_str, sizeof(str));
         db_set_value(hDB, hkeyalarm, "Condition", str, 256, 1, TID_STRING);
      }

      size = sizeof(a);
      status = db_get_record1(hDB, hkeyalarm, &a, &size, 0, strcomb1(alarm_odb_str).c_str());
      if (status != DB_SUCCESS || a.type < 1 || a.type > AT_LAST) {
         /* make sure alarm record has right structure */
         size = sizeof(a);
         status = db_get_record1(hDB, hkeyalarm, &a, &size, 0, strcomb1(alarm_odb_str).c_str());
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "al_trigger_alarm", "Cannot get alarm record for alarm \"%s\", db_get_record1() status %d", alarm_path.c_str(), status);
            return AL_ERROR_ODB;
         }
      }

      /* if internal alarm, check if active and check interval */
      if (a.type != AT_EVALUATED && a.type != AT_PERIODIC) {
         /* check global alarm flag */
         flag = TRUE;
         size = sizeof(flag);
         db_get_value(hDB, 0, "/Alarms/Alarm system active", &flag, &size, TID_BOOL, TRUE);
         if (!flag)
            return AL_SUCCESS;

         if (!a.active)
            return AL_SUCCESS;

         if ((INT) ss_time() - (INT) a.checked_last < a.check_interval)
            return AL_SUCCESS;

         /* now the alarm will be triggered, so save time */
         a.checked_last = ss_time();
      }

      if (a.type == AT_PROGRAM) {
         /* alarm class of "program not running" alarms is set in two places,
          * complain if they do not match */
         if (!equal_ustring(a.alarm_class, default_class)) {
            cm_msg(MERROR, "al_trigger_alarm",
                   "Program alarm class mismatch: \"%s/Alarm class\" set to \"%s\" does not match \"/Programs/%s/Alarm "
                   "Class\" set to \"%s\"",
                   alarm_path.c_str(), a.alarm_class, alarm_name, default_class);
         }
      }

      /* write back alarm message for internal alarms */
      if (a.type != AT_EVALUATED && a.type != AT_PERIODIC) {
         strlcpy(a.alarm_message, alarm_message, sizeof(a.alarm_message));
      }

      /* now trigger alarm class defined in this alarm */
      if (a.alarm_class[0])
         al_trigger_class(a.alarm_class, alarm_message, a.triggered > 0);

      /* check for and trigger "All" class */
      if (db_find_key(hDB, 0, "/Alarms/Classes/All", &hkey) == DB_SUCCESS)
         al_trigger_class("All", alarm_message, a.triggered > 0);

      /* signal alarm being triggered */
      std::string now = cm_asctime();

      if (!a.triggered)
         strlcpy(a.time_triggered_first, now.c_str(), sizeof(a.time_triggered_first));

      a.triggered++;
      strlcpy(a.time_triggered_last, now.c_str(), sizeof(a.time_triggered_last));

      a.checked_last = ss_time();

      status = db_set_record(hDB, hkeyalarm, &a, sizeof(a), 0);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "al_trigger_alarm", "Cannot update alarm record for alarm \"%s\", db_set_record() status %d", alarm_path.c_str(), status);
         return AL_ERROR_ODB;
      }
   }
#endif /* LOCAL_ROUTINES */

   return AL_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
INT al_trigger_class(const char *alarm_class, const char *alarm_message, BOOL first)
/********************************************************************\

  Routine: al_trigger_class

  Purpose: Trigger a certain alarm class

  Input:
    char   *alarm_class     Alarm class, must be defined in
                            /alarms/classes
    char   *alarm_message   Optional message which goes with alarm
    BOOL   first            TRUE if alarm is triggered first time
                            (used for elog)

  Output:

  Function value:
    AL_INVALID_NAME         Alarm class not defined
    AL_SUCCESS              Successful completion

\********************************************************************/
{
   int status, size, state;
   HNDLE hDB, hkeyclass;
   ALARM_CLASS ac;
   ALARM_CLASS_STR(alarm_class_str);
   DWORD now = ss_time();

   cm_get_experiment_database(&hDB, NULL);

   /* get alarm class */
   std::string alarm_path = msprintf("/Alarms/Classes/%s", alarm_class);
   db_find_key(hDB, 0, alarm_path.c_str(), &hkeyclass);
   if (!hkeyclass) {
      cm_msg(MERROR, "al_trigger_class", "Alarm class \"%s\" for alarm \"%s\" not found in ODB", alarm_class, alarm_message);
      return AL_INVALID_NAME;
   }

   size = sizeof(ac);
   status = db_get_record1(hDB, hkeyclass, &ac, &size, 0, strcomb1(alarm_class_str).c_str());
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "al_trigger_class", "Cannot get alarm class record \"%s\", db_get_record1() status %d", alarm_path.c_str(), status);
      return AL_ERROR_ODB;
   }

   std::string msg;

   /* write system message */
   if (ac.write_system_message && (now - ac.system_message_last >= (DWORD) ac.system_message_interval)) {
      if (equal_ustring(alarm_class, "All"))
         msg = msprintf("General alarm: %s", alarm_message);
      else
         msg = msprintf("%s: %s", alarm_class, alarm_message);
      cm_msg(MTALK, "al_trigger_class", "%s", msg.c_str());
      ac.system_message_last = now;
   }

   /* write elog message on first trigger if using internal ELOG */
   if (ac.write_elog_message && first) {
      BOOL external_elog = FALSE;
      size = sizeof(external_elog);
      db_get_value(hDB, 0, "/Elog/External Elog", &external_elog, &size, TID_BOOL, FALSE);
      if (!external_elog) {
         char tag[32];
         tag[0] = 0;
         el_submit(0, "Alarm system", "Alarm", "General", alarm_class, msg.c_str(), "", "plain", "", "", 0, "", "", 0, "", "", 0, tag, sizeof(tag));
      }
   }

   /* execute command */
   if (ac.execute_command[0] && ac.execute_interval > 0 && (INT) ss_time() - (INT) ac.execute_last > ac.execute_interval) {
      if (equal_ustring(alarm_class, "All"))
         msg = msprintf("General alarm: %s", alarm_message);
      else
         msg = msprintf("%s: %s", alarm_class, alarm_message);
      std::string command = msprintf(ac.execute_command, msg.c_str());
      cm_msg(MINFO, "al_trigger_class", "Execute: %s", command.c_str());
      ss_system(command.c_str());
      ac.execute_last = ss_time();
   }

   /* stop run */
   if (ac.stop_run) {
      state = STATE_STOPPED;
      size = sizeof(state);
      db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, TRUE);
      if (state != STATE_STOPPED) {
         cm_msg(MINFO, "al_trigger_class", "Stopping the run from alarm class \'%s\', message \'%s\'", alarm_class,
                alarm_message);
         cm_transition(TR_STOP, 0, NULL, 0, TR_DETACH, FALSE);
      }
   }

   status = db_set_record(hDB, hkeyclass, &ac, sizeof(ac), 0);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "al_trigger_class", "Cannot update alarm class record");
      return AL_ERROR_ODB;
   }

   return AL_SUCCESS;
}

/**dox***************************************************************/
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Reset (acknoledge) alarm.

@param alarm_name Alarm name, defined in /alarms/alarms
@return AL_SUCCESS, AL_RESETE, AL_INVALID_NAME
*/
INT al_reset_alarm(const char *alarm_name) {
   int status, size, i;
   HNDLE hDB, hkeyalarm, hkeyclass, hsubkey;
   KEY key;
   ALARM a;
   ALARM_CLASS ac;
   ALARM_ODB_STR(alarm_str);
   ALARM_CLASS_STR(alarm_class_str);

   cm_get_experiment_database(&hDB, NULL);

   if (alarm_name == NULL) {
      /* reset all alarms */
      db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyalarm);
      if (hkeyalarm) {
         for (i = 0;; i++) {
            db_enum_link(hDB, hkeyalarm, i, &hsubkey);

            if (!hsubkey)
               break;

            db_get_key(hDB, hsubkey, &key);
            al_reset_alarm(key.name);
         }
      }
      return AL_SUCCESS;
   }

   /* find alarm and alarm class */
   std::string str = msprintf("/Alarms/Alarms/%s", alarm_name);
   db_find_key(hDB, 0, str.c_str(), &hkeyalarm);
   if (!hkeyalarm) {
      /*cm_msg(MERROR, "al_reset_alarm", "Alarm %s not found in ODB", alarm_name);*/
      return AL_INVALID_NAME;
   }

   size = sizeof(a);
   status = db_get_record1(hDB, hkeyalarm, &a, &size, 0, strcomb1(alarm_str).c_str());
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "al_reset_alarm", "Cannot get alarm record");
      return AL_ERROR_ODB;
   }

   str = msprintf("/Alarms/Classes/%s", a.alarm_class);
   db_find_key(hDB, 0, str.c_str(), &hkeyclass);
   if (!hkeyclass) {
      cm_msg(MERROR, "al_reset_alarm", "Alarm class %s not found in ODB", a.alarm_class);
      return AL_INVALID_NAME;
   }

   size = sizeof(ac);
   status = db_get_record1(hDB, hkeyclass, &ac, &size, 0, strcomb1(alarm_class_str).c_str());
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "al_reset_alarm", "Cannot get alarm class record");
      return AL_ERROR_ODB;
   }

   if (a.triggered) {
      a.triggered = 0;
      a.time_triggered_first[0] = 0;
      a.time_triggered_last[0] = 0;
      a.checked_last = 0;

      ac.system_message_last = 0;
      ac.execute_last = 0;

      status = db_set_record(hDB, hkeyalarm, &a, sizeof(a), 0);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "al_reset_alarm", "Cannot update alarm record");
         return AL_ERROR_ODB;
      }
      status = db_set_record(hDB, hkeyclass, &ac, sizeof(ac), 0);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "al_reset_alarm", "Cannot update alarm class record");
         return AL_ERROR_ODB;
      }
      cm_msg(MINFO, "al_reset_alarm", "Alarm \"%s\" reset", alarm_name);
      return AL_RESET;
   }

   return AL_SUCCESS;
}

/********************************************************************/
/**
Scan ODB for alarms.
@return AL_SUCCESS
*/
INT al_check() {
   if (rpc_is_remote())
      return rpc_call(RPC_AL_CHECK);

#ifdef LOCAL_ROUTINES
   {
      INT status, size, semaphore;
      HNDLE hDB, hkeyroot, hkey;
      KEY key;
      //char str[256];
      PROGRAM_INFO_STR(program_info_str);
      PROGRAM_INFO program_info;
      BOOL flag;

      ALARM_CLASS_STR(alarm_class_str);
      ALARM_ODB_STR(alarm_odb_str);
      ALARM_PERIODIC_STR(alarm_periodic_str);

      cm_get_experiment_database(&hDB, NULL);

      if (hDB == 0)
         return AL_SUCCESS; /* called from server not yet connected */

      /* check online mode */
      flag = TRUE;
      size = sizeof(flag);
      db_get_value(hDB, 0, "/Runinfo/Online Mode", &flag, &size, TID_INT, TRUE);
      if (!flag)
         return AL_SUCCESS;

      /* check global alarm flag */
      flag = TRUE;
      size = sizeof(flag);
      db_get_value(hDB, 0, "/Alarms/Alarm system active", &flag, &size, TID_BOOL, TRUE);
      if (!flag)
         return AL_SUCCESS;

      /* request semaphore */
      cm_get_experiment_semaphore(&semaphore, NULL, NULL, NULL);
      status = ss_semaphore_wait_for(semaphore, 100);
      if (status == SS_TIMEOUT)
         return AL_SUCCESS; /* someone else is doing alarm business */
      if (status != SS_SUCCESS) {
         printf("al_check: Something is wrong with our semaphore, ss_semaphore_wait_for() returned %d, aborting.\n",
                status);
         // abort(); // DOES NOT RETURN
         printf("al_check: Cannot abort - this will lock you out of odb. From this point, MIDAS will not work "
                "correctly. Please read the discussion at https://midas.triumf.ca/elog/Midas/945\n");
         // NOT REACHED
         return AL_SUCCESS;
      }

      /* check ODB alarms */
      db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyroot);
      if (!hkeyroot) {
         /* create default ODB alarm */
         status = db_create_record(hDB, 0, "/Alarms/Alarms/Demo ODB", strcomb1(alarm_odb_str).c_str());
         db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyroot);
         if (!hkeyroot) {
            ss_semaphore_release(semaphore);
            return AL_SUCCESS;
         }

         status = db_create_record(hDB, 0, "/Alarms/Alarms/Demo periodic", strcomb1(alarm_periodic_str).c_str());
         db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyroot);
         if (!hkeyroot) {
            ss_semaphore_release(semaphore);
            return AL_SUCCESS;
         }

         /* create default alarm classes */
         status = db_create_record(hDB, 0, "/Alarms/Classes/Alarm", strcomb1(alarm_class_str).c_str());
         status = db_create_record(hDB, 0, "/Alarms/Classes/Warning", strcomb1(alarm_class_str).c_str());
         if (status != DB_SUCCESS) {
            ss_semaphore_release(semaphore);
            return AL_SUCCESS;
         }
      }

      for (int i = 0;; i++) {
         ALARM a;

         status = db_enum_key(hDB, hkeyroot, i, &hkey);
         if (status == DB_NO_MORE_SUBKEYS)
            break;

         db_get_key(hDB, hkey, &key);

         size = sizeof(a);
         status = db_get_record1(hDB, hkey, &a, &size, 0, strcomb1(alarm_odb_str).c_str());
         if (status != DB_SUCCESS || a.type < 1 || a.type > AT_LAST) {
            /* make sure alarm record has right structure */
            db_check_record(hDB, hkey, "", strcomb1(alarm_odb_str).c_str(), TRUE);
            size = sizeof(a);
            status = db_get_record1(hDB, hkey, &a, &size, 0, strcomb1(alarm_odb_str).c_str());
            if (status != DB_SUCCESS || a.type < 1 || a.type > AT_LAST) {
               cm_msg(MERROR, "al_check", "Cannot get alarm record");
               continue;
            }
         }

         /* check periodic alarm only when active */
         if (a.active && a.type == AT_PERIODIC && a.check_interval > 0 && (INT) ss_time() - (INT) a.checked_last > a.check_interval) {
            /* if checked_last has not been set, set it to current time */
            if (a.checked_last == 0) {
               a.checked_last = ss_time();
               db_set_record(hDB, hkey, &a, size, 0);
            } else
               al_trigger_alarm(key.name, a.alarm_message, a.alarm_class, "", AT_PERIODIC);
         }

         /* check alarm only when active and not internal */
         if (a.active && a.type == AT_EVALUATED && a.check_interval > 0 && (INT) ss_time() - (INT) a.checked_last > a.check_interval) {
            /* if condition is true, trigger alarm */
            std::string value;
            if (al_evaluate_condition(key.name, a.condition, &value)) {
               std::string str = msprintf(a.alarm_message, value.c_str());
               al_trigger_alarm(key.name, str.c_str(), a.alarm_class, "", AT_EVALUATED);
            } else {
               a.checked_last = ss_time();
               status = db_set_value(hDB, hkey, "Checked last", &a.checked_last, sizeof(DWORD), 1, TID_DWORD);
               if (status != DB_SUCCESS) {
                  cm_msg(MERROR, "al_check", "Cannot change alarm record");
                  continue;
               }
            }
         }
      }

      /* check /programs alarms */
      db_find_key(hDB, 0, "/Programs", &hkeyroot);
      if (hkeyroot) {
         for (int i = 0;; i++) {
            status = db_enum_key(hDB, hkeyroot, i, &hkey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            db_get_key(hDB, hkey, &key);

            /* don't check "execute on xxx" */
            if (key.type != TID_KEY)
               continue;

            size = sizeof(program_info);
            status = db_get_record1(hDB, hkey, &program_info, &size, 0, strcomb1(program_info_str).c_str());
            if (status != DB_SUCCESS) {
               cm_msg(MERROR, "al_check",
                      "Cannot get program info record for program \"%s\", db_get_record1() status %d", key.name,
                      status);
               continue;
            }

            time_t now = ss_time();

            // get name of this client
            std::string name = rpc_get_name();
            std::string str = name;

            // truncate name of this client to the length of program name in /Programs/xxx
            // to get rid if "odbedit1", "odbedit2", etc.
            str.resize(strlen(key.name));

            //printf("str [%s], key.name [%s]\n", str.c_str(), key.name);

            if (!equal_ustring(str.c_str(), key.name) && cm_exist(key.name, FALSE) == CM_NO_CLIENT) {
               if (program_info.first_failed == 0) {
                  program_info.first_failed = (DWORD) now;
                  db_set_record(hDB, hkey, &program_info, sizeof(program_info), 0);
               }

               // printf("check %d-%d = %d >= %d\n", now, program_info.first_failed, now - program_info.first_failed,
               // program_info.check_interval / 1000);

               /* fire alarm when not running for more than what specified in check interval */
               if (now - program_info.first_failed >= program_info.check_interval / 1000) {
                  /* if not running and alarm class defined, trigger alarm */
                  if (program_info.alarm_class[0]) {
                     std::string str = msprintf("Program %s is not running", key.name);
                     al_trigger_alarm(key.name, str.c_str(), program_info.alarm_class, "Program not running", AT_PROGRAM);
                  }

                  /* auto restart program */
                  if (program_info.auto_restart && program_info.start_command[0]) {
                     ss_system(program_info.start_command);
                     program_info.first_failed = 0;
                     cm_msg(MTALK, "al_check", "Program %s restarted", key.name);
                  }
               }
            } else {
               if (program_info.first_failed != 0) {
                  program_info.first_failed = 0;
                  db_set_record(hDB, hkey, &program_info, sizeof(program_info), 0);
               }
            }
         }
      }

      ss_semaphore_release(semaphore);
   }
#endif /* LOCAL_COUTINES */

   return SUCCESS;
}

/********************************************************************/
/**
 Scan ODB for alarms.
 @return AL_SUCCESS
 */
INT al_get_alarms(std::string *presult)
{
   if (!presult)
      return 0;

   HNDLE hDB, hkey;

   cm_get_experiment_database(&hDB, NULL);
   presult->clear();
   int n = 0;
   db_find_key(hDB, 0, "/Alarms/Alarms", &hkey);
   if (hkey) {
      /* check global alarm flag */
      int flag = TRUE;
      int size = sizeof(flag);
      db_get_value(hDB, 0, "/Alarms/Alarm System active", &flag, &size, TID_BOOL, FALSE);
      if (flag) {
         for (int i = 0;; i++) {
            HNDLE hsubkey;

            db_enum_link(hDB, hkey, i, &hsubkey);

            if (!hsubkey)
               break;

            KEY key;
            db_get_key(hDB, hsubkey, &key);

            flag = 0;
            size = sizeof(flag);
            db_get_value(hDB, hsubkey, "Triggered", &flag, &size, TID_INT, FALSE);
            if (flag) {
               n++;

               std::string alarm_class, msg;
               int alarm_type;

               db_get_value_string(hDB, hsubkey, "Alarm Class", 0, &alarm_class);
               db_get_value_string(hDB, hsubkey, "Alarm Message", 0, &msg);

               size = sizeof(alarm_type);
               db_get_value(hDB, hsubkey, "Type", &alarm_type, &size, TID_INT, FALSE);

               std::string str;

               if (alarm_type == AT_EVALUATED) {
                  db_get_value_string(hDB, hsubkey, "Condition", 0, &str);

                  /* retrieve value */
                  std::string value;
                  al_evaluate_condition(key.name, str.c_str(), &value);
                  str = msprintf(msg.c_str(), value.c_str());
               } else
                  str = msg;

               *presult += alarm_class;
               *presult += ": ";
               *presult += str;
               *presult += "\n";
            }
         }
      }
   }

   return n;
}

/********************************************************************/
/**
Create an alarm that will trigger when an ODB condition is met.

@param name         Alarm name, defined in /alarms/alarms
@param class        Alarm class to be triggered
@param condition    Alarm condition to be evaluated
@param message      Alarm message
@return AL_SUCCESS
*/
INT EXPRT al_define_odb_alarm(const char *name, const char *condition, const char *aclass, const char *message) {
   HNDLE hDB, hKey;
   ALARM_ODB_STR(alarm_odb_str);

   cm_get_experiment_database(&hDB, nullptr);

   std::string str = msprintf("/Alarms/Alarms/%s", name);

   db_create_record(hDB, 0, str.c_str(), strcomb1(alarm_odb_str).c_str());
   db_find_key(hDB, 0, str.c_str(), &hKey);
   if (!hKey)
      return DB_NO_MEMORY;

   db_set_value(hDB, hKey, "Condition", condition, 256, 1, TID_STRING);
   db_set_value(hDB, hKey, "Alarm Class", aclass, 32, 1, TID_STRING);
   db_set_value(hDB, hKey, "Alarm Message", message, 80, 1, TID_STRING);

   return AL_SUCCESS;
}

/**dox***************************************************************/
/** @} */ /* end of alfunctioncode */

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
