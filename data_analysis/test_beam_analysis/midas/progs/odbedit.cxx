/********************************************************************\

  Name:         odbedit.cxx
  Created by:   Stefan Ritt

  Contents:     Command-line interface to the MIDAS online data base.

  $Id$

\********************************************************************/

#include <stdio.h>
#include "midas.h"
#include "msystem.h"
#include <assert.h>

#include <string>

#ifndef HAVE_STRLCPY
#include "strlcpy.h"
#endif

extern INT cmd_edit(const char *prompt, char *cmd, INT(*dir) (char *, INT *), INT(*idle) ());

BOOL need_redraw;
BOOL in_cmd_edit;
char pwd[256];
BOOL cmd_mode;

typedef struct {
   int flags;
   char pattern[32];
   int index;
} PRINT_INFO;

#define PI_LONG      (1<<0)
#define PI_RECURSIVE (1<<1)
#define PI_VALUE     (1<<2)
#define PI_HEX       (1<<3)
#define PI_PAUSE     (1<<4)

MUTEX_T *tm;

/*------------------------------------------------------------------*/

INT thread(void *p)
{
   char str[32];
   HNDLE hDB;
   cm_get_experiment_database(&hDB, NULL);
   do {
      sprintf(str, "%s", ss_tid_to_string(ss_gettid()).c_str());
      db_set_value(hDB, 0, "/Experiment/Name", str, sizeof(str), 1, TID_STRING);
   } while (1);

   return 0;
}

/*------------------------------------------------------------------*/

BOOL key_modified;

void key_update(HNDLE hDB, HNDLE hkey, void *info)
{
   key_modified = TRUE;
}

/*------------------------------------------------------------------*/

void print_help(char *command)
{
#ifndef OS_MSDOS                /* save some DGROUP memory under MS-DOS */
   if (!command[0]) {
      printf("Database commands ([] are options, <> are placeholders):\n\n");
      printf("alarm                   - reset all alarms\n");
      printf("cd <dir>                - change current directory\n");
      printf("chat                    - enter chat mode\n");
      printf("chmod <mode> <key>      - change access mode of a key\n");
      printf("                          1=read | 2=write | 4=delete\n");
      printf("cleanup [client] [-f]   - delete hanging clients [force]\n");
      printf("copy <src> <dest>       - copy a subtree to a new location\n");
      printf("create <type> <key>     - create a key of a certain type\n");
      printf("create <type> <key>[n]  - create an array of size [n]\n");
      printf("create string <key>[n][m]  - create an array of [n] strings of [m] characters\n");
      printf("del/rm [-l] [-f] <key>  - delete a key and its subkeys\n");
      printf("  -l                      follow links\n");
      printf("  -f                      force deletion without asking\n");
      printf
          ("exec <key>/<cmd>        - execute shell command (stored in key) on server\n");
      printf("exp <key> <filename>    - import key into ASCII file\n");
      printf("find <pattern>          - find a key with wildcard pattern\n");
      printf("help/? [command]        - print this help [for a specific command]\n");
      printf("hi [analyzer] [id]      - tell analyzer to clear histos\n");
      printf("imp <filename> [key]    - import ASCII file into string key\n");
      printf("json <odb path>         - print \"ODB save\" encoding of current directory or given odb path\n");
      printf("jsls                    - print \"ls\" encoding of current directory\n");
      printf("jsvalues                - print \"get_values\" encoding of current directory\n");
      printf("ln <source> <linkname>  - create a link to <source> key\n");
      printf
          ("load <file>             - load database from .ODB file at current position\n");
      printf("-- hit return for more --\r");
      getchar();
      printf("ls/dir [-lhvrp] [<pat>] - show database entries which match pattern\n");
      printf("  -l                      detailed info\n");
      printf("  -h                      hex format\n");
      printf("  -v                      only value\n");
      printf("  -r                      show database entries recursively\n");
      printf("  -p                      pause between screens\n");
      printf("make [analyzer name]    - create experim.h\n");
      printf("mem [-v]                - show memeory usage [verbose]\n");
      printf("mkdir <subdir>          - make new <subdir>\n");
      printf("move <key> [top/bottom/[n]] - move key to position in keylist\n");
      printf("msg [user] <msg>        - send chat message (from interactive odbedit)\n");
      printf("msg <facility> <type> <name> <msg> - send message to [facility] log\n");
      printf("old [n]                 - display old n messages\n");
      printf("passwd                  - change MIDAS password\n");
      printf("pause                   - pause current run\n");
      printf("pwd                     - show current directory\n");
      printf("resume                  - resume current run\n");
      printf("rename <old> <new>      - rename key\n");
      printf("-- hit return for more --\r");
      getchar();
      printf("save [-c -s -x -j -cs] <file>  - save database at current position\n");
      printf("                          in ASCII format\n");
      printf("  -c                      as a C structure\n");
      printf("  -s                      as a #define'd string\n");
      printf("  -x                      as an XML file, or use file.xml\n");
      printf("  -j                      as a JSON file, or use file.json\n");
      printf("  -z                      as value-only JSON file\n");
      printf("set <key> <value>       - set the value of a key\n");
      printf("set <key>[i] <value>    - set the value of index i\n");
      printf("set <key>[*] <value>    - set the value of all indices of a key\n");
      printf("set <key>[i..j] <value> - set the value of all indices i..j\n");
      printf("scl [-w]                - show all active clients [with watchdog info]\n");
      printf("shutdown <client>/all   - shutdown individual or all clients\n");
      printf("sor                     - show open records in current subtree\n");
      printf("start [number][now][-v] - start a run [with a specific number],\n");
      printf
          ("                          [now] w/o asking parameters, [-v] debug output\n");
      printf("stop [-v]               - stop current run, [-v] debug output\n");
      printf("test_rpc                - test mserver RPC connection and parameter encoding and decoding\n");
      printf("trunc <key> <index>     - truncate key to [index] values\n");
      printf("ver                     - show MIDAS library version\n");
      printf("webpasswd               - change WWW password for mhttpd\n");
      printf("wait <key>              - wait for key to get modified\n");
      printf("watch <key>             - watch key or ODB tree to be modified\n");

      printf("\nquit/exit               - exit\n");
      return;
   }

   if (equal_ustring(command, "cd")) {
      printf("cd <dir> - change current directory. Use \"cd /\" to change to the root\n");
      printf("           of the ODB, \"cd ..\" to change to the parent directory.\n");
   } else if (equal_ustring(command, "chat")) {
      printf("chat - enter chat mode. In this mode, users can \"talk\" to each other.\n");
      printf
          ("       Each user running ODBEdit connected to the same experiment can see\n");
      printf
          ("       the other messages and each user running ODBEdit in chat mode can\n");
      printf
          ("       produce messages. All messages are logged in the MIDAS logging file.\n");
   } else
      printf("No specific help available for command \"%s\".\n", command);

#endif
}

/*------------------------------------------------------------------*/

void process_message(HNDLE hBuf, HNDLE id, EVENT_HEADER * pheader, void *message)
{
   time_t tm;
   char str[80];

   /* prepare time */
   time(&tm);
   assert(sizeof(str) >= 32);
   ctime_r(&tm, str);
   str[19] = 0;

   /* print message text which comes after event header */
   if (in_cmd_edit)
      printf("\r%s %s\n", str + 11, (char *) message);
   else
      printf("\n%s %s\n", str + 11, (char *) message);

   need_redraw = TRUE;
}

int print_message(const char *msg)
{
   if (in_cmd_edit)
      printf("\r%s\n", msg);
   else
      printf("%s\n", msg);

   need_redraw = TRUE;
   return 0;
}

/*------------------------------------------------------------------*/

BOOL match(char *pat, char *str)
/* process a wildcard match */
{
   if (!*str)
      return *pat == '*' ? match(pat + 1, str) : !*pat;

   switch (*pat) {
   case '\0':
      return 0;
   case '*':
      return match(pat + 1, str) || match(pat, str + 1);
   case '?':
      return match(pat + 1, str + 1);
   default:
      return (toupper(*pat) == toupper(*str)) && match(pat + 1, str + 1);
   }
}

/*------------------------------------------------------------------*/

int ls_line, ls_abort;

BOOL check_abort(int flags, int l)
{
   int c;

   if ((flags & PI_PAUSE) && (l % 24) == 23) {
      printf("Press any key to continue or q to quit ");
      fflush(stdout);
      do {
         c = ss_getchar(0);
         if (c == 'q') {
            printf("\n");
            ls_abort = TRUE;
            return TRUE;
         } else if (c > 0) {
            printf("\r                                        \r");
            return FALSE;
         }

         int status = cm_yield(100);
         if (status == SS_ABORT || status == RPC_SHUTDOWN)
            break;

      } while (!cm_is_ctrlc_pressed());
   }

   if (cm_is_ctrlc_pressed()) {
      ls_abort = TRUE;
      return TRUE;
   }

   return FALSE;
}


INT print_key(HNDLE hDB, HNDLE hKey, KEY * pkey, INT level, void *info)
{
   INT i, status;
   static char data_str[50000], line[256];
   DWORD delta;
   PRINT_INFO *pi;
   KEY key;

   if (ls_abort)
      return 0;

   pi = (PRINT_INFO *) info;
   memcpy(&key, pkey, sizeof(KEY));

   /* if pattern set, check if match */
   if (pi->pattern[0] && !match(pi->pattern, pkey->name))
      return SUCCESS;

   if (pi->flags & PI_VALUE) {
      /* only print value */
      if (key.type != TID_KEY) {
         assert(key.total_size > 0);
         char* buf = (char*)malloc(key.total_size);
         int size = key.total_size;
         status = db_get_link_data(hDB, hKey, buf, &size, key.type);
         data_str[0] = 0;

         /* resolve links */
         if (key.type == TID_LINK) {
            if (strlen(buf) > 0 && buf[strlen(buf)-1] == ']')
               status = DB_SUCCESS;
            else
               status = db_find_key(hDB, 0, buf, &hKey);
            if (status == DB_SUCCESS) {
               status = db_get_key(hDB, hKey, &key);
               if (status == DB_SUCCESS) {
                  assert(key.total_size > 0);
                  buf = (char*)realloc(buf, key.total_size);
                  size = key.total_size;
                  if (key.type != TID_KEY)
                     status = db_get_data(hDB, hKey, buf, &size, key.type);
                  else
                     status = DB_TYPE_MISMATCH;
               }
            }
         }

         if (status == DB_NO_KEY)
            strcat(data_str, "<cannot resolve link>");
         else if (status == DB_NO_ACCESS)
            strcat(data_str, "<no read access>");
         else
            for (i = 0; i < key.num_values; i++) {
               if (pi->flags & PI_HEX)
                  db_sprintfh(data_str, buf, key.item_size, i, key.type);
               else
                  db_sprintf(data_str, buf, key.item_size, i, key.type);

               if ((pi->index != -1 && i == pi->index) || pi->index == -1)
                  printf("%s\n", data_str);
               if (check_abort(pi->flags, ls_line++))
                  return 0;
            }
         free(buf);
      }
   } else {
      /* print key name with value */
      memset(line, ' ', sizeof(line));
      line[sizeof(line)-1] = 0;
      sprintf(line + level * 4, "%s", key.name);
      if (key.type == TID_LINK) {
         if (key.total_size > 0) {
            char* buf = (char*)malloc(key.total_size);
            int size = key.total_size;
            db_get_link_data(hDB, hKey, buf, &size, key.type);
            sprintf(line + strlen(line), " -> %s", buf);
            free(buf);
         } else {
            sprintf(line + strlen(line), " -> (empty)");
         }
         if (pi->index != -1)
            sprintf(line + strlen(line), "[%d]", pi->index);
         if (strlen(line) >= 32) {
            printf("%s\n", line);
            memset(line, ' ', sizeof(line));
            line[80] = 0;
         } else {
            line[strlen(line)] = ' ';
         }
      } else {
         if (pi->index != -1)
            sprintf(line + strlen(line), "[%d]", pi->index);
         line[strlen(line)] = ' ';
      }

      if (key.type == TID_KEY) {
         if (pi->flags & PI_LONG)
            sprintf(line + 32, "DIR");
         else
            line[32] = 0;
         printf("%s\n", line);
         if (check_abort(pi->flags, ls_line++))
            return 0;
      } else {
         char* data_buf = NULL;

         if (key.total_size > 0) {
            data_buf = (char*)malloc(key.total_size);
            int size = key.total_size;
            status = db_get_link_data(hDB, hKey, data_buf, &size, key.type);
         } else {
            status = DB_SUCCESS;
         }
         
         data_str[0] = 0;

         /* resolve links */
         if (key.type == TID_LINK && data_buf) {
            if (strlen(data_buf) > 0 && data_buf[strlen(data_buf)-1] == ']')
               status = DB_SUCCESS;
            else
               status = db_find_key(hDB, 0, data_buf, &hKey);
            if (status == DB_SUCCESS) {
               status = db_get_key(hDB, hKey, &key);
               if (status == DB_SUCCESS) {
                  if (key.total_size > 0) {
                     data_buf = (char*)realloc(data_buf, key.total_size);
                     int size = key.total_size;
                     if (key.type != TID_KEY)
                        status = db_get_data(hDB, hKey, data_buf, &size, key.type);
                     else
                        status = DB_TYPE_MISMATCH;
                  } else {
                     if (data_buf)
                        free(data_buf);
                     data_buf = NULL;
                  }
               }
            }
         }

         if (status == DB_TYPE_MISMATCH)
            strcat(data_str, "<subdirectory>");
         else if (status == DB_NO_KEY || status == DB_INVALID_LINK)
            strcat(data_str, "<cannot resolve link>");
         else if (status == DB_NO_ACCESS)
            strcat(data_str, "<no read access>");
         else if (!data_buf)
            strcat(data_str, "<empty>");
         else {
            if (pi->flags & PI_HEX)
               db_sprintfh(data_str+strlen(data_str), data_buf, key.item_size, 0, key.type);
            else
               db_sprintf(data_str+strlen(data_str), data_buf, key.item_size, 0, key.type);
         }

         if (pi->flags & PI_LONG) {
            sprintf(line + 32, "%s", rpc_tid_name(key.type));
            line[strlen(line)] = ' ';
            sprintf(line + 40, "%d", key.num_values);
            line[strlen(line)] = ' ';
            sprintf(line + 46, "%d", key.item_size);
            line[strlen(line)] = ' ';

            db_get_key_time(hDB, hKey, &delta);
            if (delta < 60)
               sprintf(line + 52, "%ds", delta);
            else if (delta < 3600)
               sprintf(line + 52, "%1.0lfm", delta / 60.0);
            else if (delta < 86400)
               sprintf(line + 52, "%1.0lfh", delta / 3600.0);
            else if (delta < 86400 * 99)
               sprintf(line + 52, "%1.0lfh", delta / 86400.0);
            else
               sprintf(line + 52, ">99d");
            line[strlen(line)] = ' ';

            sprintf(line + 57, "%d", key.notify_count);
            line[strlen(line)] = ' ';

            if (key.access_mode & MODE_READ)
               line[61] = 'R';
            if (key.access_mode & MODE_WRITE)
               line[62] = 'W';
            if (key.access_mode & MODE_DELETE)
               line[63] = 'D';
            if (key.access_mode & MODE_EXCLUSIVE)
               line[64] = 'E';

            if (key.type == TID_STRING && strchr(data_str, '\n'))
               strcpy(line + 66, "<multi-line>");
            else if (key.num_values == 1)
               strcpy(line + 66, data_str);
            else
               line[66] = 0;
         } else if (key.num_values == 1) {
            if (key.type == TID_STRING && strchr(data_str, '\n'))
               strcpy(line + 32, "<multi-line>");
            else
               strcpy(line + 32, data_str);
         } else {
            while (line[strlen(line)-1] == ' ')
               line[strlen(line)-1] = 0;
         }

         if (line[0])
            printf("%s\n", line);

         if (key.type == TID_STRING && strchr(data_str, '\n'))
            puts(data_str);

         if (check_abort(pi->flags, ls_line++))
            return 0;

         if (key.num_values > 1) {
            for (i = 0; i < key.num_values; i++) {
               if (data_buf) {
                  if (pi->flags & PI_HEX)
                     db_sprintfh(data_str, data_buf, key.item_size, i, key.type);
                  else
                     db_sprintf(data_str, data_buf, key.item_size, i, key.type);
               } else {
                  strcpy(data_str, "<empty>");
               }

               memset(line, ' ', 80);
               line[80] = 0;

               if (pi->flags & PI_LONG) {
                  sprintf(line + 40, "[%d]", i);
                  line[strlen(line)] = ' ';

                  strcpy(line + 56, data_str);
               } else
                  strcpy(line + 32, data_str);

               if ((pi->index != -1 && i == pi->index) || pi->index == -1)
                  printf("%s\n", line);

               if (check_abort(pi->flags, ls_line++))
                  return 0;
            }
         }
         if (data_buf)
            free(data_buf);
      }
   }

   return SUCCESS;
}

/*------------------------------------------------------------------*/

void set_key(HNDLE hDB, HNDLE hKey, int index1, int index2, char *value)
{
   KEY key;
   char data[1000];
   int i, size, status = 0;

   db_get_link(hDB, hKey, &key);

   memset(data, 0, sizeof(data));
   db_sscanf(value, data, &size, 0, key.type);

   /* extend data size for single string if necessary */
   if ((key.type == TID_STRING || key.type == TID_LINK)
       && (int) strlen(data) + 1 > key.item_size && key.num_values == 1)
      key.item_size = strlen(data) + 1;

   if (key.item_size == 0)
      key.item_size = rpc_tid_size(key.type);

   if (key.num_values > 1 && index1 == -1) {
      for (i = 0; i < key.num_values; i++)
         status = db_set_link_data_index(hDB, hKey, data, key.item_size, i, key.type);
   } else if (key.num_values > 1 && index2 > index1) {
      for (i = index1; i < key.num_values && i <= index2; i++)
         status = db_set_link_data_index(hDB, hKey, data, key.item_size, i, key.type);
   } else if (key.num_values > 1 || index1 > 0)
      status = db_set_link_data_index(hDB, hKey, data, key.item_size, index1, key.type);
   else
      status = db_set_link_data(hDB, hKey, data, key.item_size, 1, key.type);

   if (status == DB_NO_ACCESS)
      printf("Write access not allowed\n");
}

/*------------------------------------------------------------------*/

void scan_tree(HNDLE hDB, HNDLE hKey, INT * total_size_key, INT * total_size_data,
               INT level, INT flags)
{
   INT i, j;
   //INT size;
   INT status;
   KEY key;
   HNDLE hSubkey;
   static char data_str[256], line[256];
   DWORD delta;

   if (cm_is_ctrlc_pressed()) {
      if (level == 0)
         cm_ack_ctrlc_pressed();
      return;
   }

   db_get_key(hDB, hKey, &key);

   *total_size_key += ALIGN8(sizeof(KEY));
   if (key.type == TID_KEY)
      *total_size_key += ALIGN8(sizeof(KEYLIST));
   else
      *total_size_data += ALIGN8(key.total_size);

   if (flags & 0x4) {
      /* only print value */
      if (key.type != TID_KEY) {
         assert(key.total_size > 0);
         char* buf = (char*)malloc(key.total_size);
         int size = key.total_size;
         status = db_get_data(hDB, hKey, buf, &size, key.type);
         if (status == DB_NO_ACCESS)
            strcpy(data_str, "<no read access>");
         else
            for (j = 0; j < key.num_values; j++) {
               if (flags & 0x8)
                  db_sprintfh(data_str, buf, key.item_size, j, key.type);
               else
                  db_sprintf(data_str, buf, key.item_size, j, key.type);
               printf("%s\n", data_str);
            }
         free(buf);
      }
   } else {
      /* print key name with value */
      memset(line, ' ', 80);
      line[80] = 0;
      sprintf(line + level * 4, "%s", key.name);
      line[strlen(line)] = ' ';

      if (key.type == TID_KEY) {
         line[32] = 0;
         printf("%s\n", line);
      } else {
         assert(key.total_size > 0);
         char* buf = (char*)malloc(key.total_size);
         int size = key.total_size;
         status = db_get_data(hDB, hKey, buf, &size, key.type);
         if (status == DB_NO_ACCESS)
            strcpy(data_str, "<no read access>");
         else {
            if (flags & 0x8)
               db_sprintfh(data_str, buf, key.item_size, 0, key.type);
            else
               db_sprintf(data_str, buf, key.item_size, 0, key.type);
         }

         if (flags & 0x1) {
            sprintf(line + 32, "%s", rpc_tid_name(key.type));
            line[strlen(line)] = ' ';
            sprintf(line + 40, "%d", key.num_values);
            line[strlen(line)] = ' ';
            sprintf(line + 46, "%d", key.item_size);
            line[strlen(line)] = ' ';

            db_get_key_time(hDB, hKey, &delta);
            if (delta < 60)
               sprintf(line + 52, "%ds", delta);
            else if (delta < 3600)
               sprintf(line + 52, "%1.0lfm", delta / 60.0);
            else if (delta < 86400)
               sprintf(line + 52, "%1.0lfh", delta / 3600.0);
            else if (delta < 86400 * 99)
               sprintf(line + 52, "%1.0lfh", delta / 86400.0);
            else
               sprintf(line + 52, ">99d");
            line[strlen(line)] = ' ';

            sprintf(line + 57, "%d", key.notify_count);
            line[strlen(line)] = ' ';

            if (key.access_mode & MODE_READ)
               line[61] = 'R';
            if (key.access_mode & MODE_WRITE)
               line[62] = 'W';
            if (key.access_mode & MODE_DELETE)
               line[63] = 'D';
            if (key.access_mode & MODE_EXCLUSIVE)
               line[64] = 'E';

            if (key.num_values == 1)
               strcpy(line + 66, data_str);
            else
               line[66] = 0;
         } else if (key.num_values == 1)
            strcpy(line + 32, data_str);
         else
            line[32] = 0;

         printf("%s\n", line);

         if (key.num_values > 1) {
            for (j = 0; j < key.num_values; j++) {
               if (flags & 0x8)
                  db_sprintfh(data_str, buf, key.item_size, j, key.type);
               else
                  db_sprintf(data_str, buf, key.item_size, j, key.type);

               memset(line, ' ', 80);
               line[80] = 0;

               if (flags & 0x1) {
                  sprintf(line + 40, "[%d]", j);
                  line[strlen(line)] = ' ';

                  strcpy(line + 56, data_str);
               } else
                  strcpy(line + 32, data_str);

               printf("%s\n", line);
            }
         }

         free(buf);
      }
   }

   /* recurse subtree */
   if (key.type == TID_KEY && (flags & 0x2)) {
      for (i = 0;; i++) {
         db_enum_link(hDB, hKey, i, &hSubkey);

         if (!hSubkey)
            break;

         scan_tree(hDB, hSubkey, total_size_key, total_size_data, level + 1, flags);

         if (cm_is_ctrlc_pressed()) {
            if (level == 0)
               cm_ack_ctrlc_pressed();
            return;
         }
      }
   }
}

/*------------------------------------------------------------------*/

/* complete partial key name, gets called from cmd_edit */
INT cmd_dir(char *line, INT * cursor)
{
   KEY key;
   HNDLE hDB, hKey, hSubkey;
   INT i, j, match, size;
   char str[256], *pc, partial[256], last_match[256];
   char head[256], tail[256], key_name[256], c;
   char test_key[256];
   BOOL blanks, mismatch;

   cm_get_experiment_database(&hDB, NULL);

   /* remember tail for later */
   strcpy(head, line);
   strcpy(tail, line + *cursor);
   line[*cursor] = 0;

   /* search beginning of key */
   pc = head;
   do {
      while (*pc && *pc != ' ')
         pc++;
      while (*pc && *pc == ' ')
         pc++;
   } while (*pc == '-');        /* skip flags */

   if (*pc) {
      strcpy(key_name, pc);
      *pc = 0;                  /* end of head */
   } else
      key_name[0] = 0;

   /* check if key exists (for "set <key>" completion) */
   if (strncmp(head, "set", 3) == 0 && strlen(key_name) > 0) {
      if (key_name[0] == '"')
         strcpy(str, key_name + 1);
      else
         strcpy(str, key_name);
      if (key_name[0] != '/') {
         strcpy(test_key, pwd);
         if (test_key[strlen(test_key) - 1] != '/')
            strcat(test_key, "/");
         strcat(test_key, str);
      } else {
         strcpy(test_key, str);
      }

      pc = test_key + strlen(test_key) - 1;
      while (pc > test_key && (*pc == ' ' || *pc == '"'))
         *pc-- = 0;
      int status = db_find_key(hDB, 0, test_key, &hSubkey);
      if (status == DB_SUCCESS) {
         /* retrieve key data */
         db_get_link(hDB, hSubkey, &key);

         if (key.type != TID_KEY) {
            if (strlen(key_name) > 0 && key_name[strlen(key_name)-1] != ' ')
               strcat(key_name, " ");

            assert(key.total_size > 0);
            char * buf = (char*)malloc(key.total_size);
            size = key.total_size;
            status = db_get_link_data(hDB, hSubkey, buf, &size, key.type);
            
            if (key.type == TID_STRING || key.type == TID_LINK)
               sprintf(str, "\"%s\"", buf);
            else
               db_sprintf(str, buf, size, 0, key.type);

            free(buf);
            
            strcpy(line, head);
            strcat(line, key_name);
            strcat(line, str);
            *cursor = strlen(line);
            strcat(line, tail);
            return TRUE;
         }
      }
   }

   /* combine pwd and key_name */
   pc = key_name;
   if (*pc == '"')
      pc++;

   if (*pc != '/') {
      strcpy(str, pwd);
      if (str[strlen(str) - 1] != '/')
         strcat(str, "/");
      strcat(str, pc);
   } else
      strcpy(str, pc);

   /* split key_name into known and new directory */
   for (pc = str + strlen(str) - 1; pc > str && *pc != '/'; pc--);

   if (*pc == '/') {
      *pc = 0;
      strcpy(partial, pc + 1);
   } else
      strcpy(partial, str);

   db_find_link(hDB, 0, str, &hKey);
   for (i = 0, match = 0;; i++) {
      db_enum_link(hDB, hKey, i, &hSubkey);

      if (!hSubkey)
         break;

      db_get_link(hDB, hSubkey, &key);
      strcpy(str, key.name);

      str[strlen(partial)] = 0;

      if (equal_ustring(str, partial))
         match++;
   }

   if (match != 1)
      printf("\r\n");

   for (i = 0;; i++) {
      db_enum_link(hDB, hKey, i, &hSubkey);

      if (!hSubkey)
         break;

      db_get_link(hDB, hSubkey, &key);
      strcpy(str, key.name);

      str[strlen(partial)] = 0;

      if (equal_ustring(str, partial)) {
         if (match == 1) {
            /* search split point */
            pc = key_name;
            if (strlen(key_name) > 0)
               for (pc = key_name + strlen(key_name) - 1; pc > key_name && *pc != '/';
                    pc--);
            if (*pc == '/')
               pc++;

            strcpy(pc, key.name);
            if (key.type == TID_KEY)
               strcat(pc, "/");

            /* insert '"' if blanks in name */
            if (strchr(key.name, ' ')) {
               if (key_name[0] != '"') {
                  for (i = strlen(key_name); i >= 0; i--)
                     key_name[i + 1] = key_name[i];

                  key_name[0] = '"';
               }
               if (key.type != TID_KEY)
                  strcat(key_name, "\"");
            }

            if (key.type != TID_KEY) {
               if (key_name[0] == '"' && key_name[strlen(key_name) - 1] != '"')
                  strcat(pc, "\" ");
               else
                  strcat(pc, " ");
            }

            strcpy(line, head);
            strcat(line, key_name);
            *cursor = strlen(line);
            strcat(line, tail);
            return TRUE;
         }
      }
      if (match == 0 || (match > 1 && equal_ustring(str, partial)))
         printf("%s\r\n", key.name);
   }

   if (match > 1 && key_name[0]) {
      blanks = FALSE;

      for (j = strlen(partial);; j++) {
         for (i = 0, c = 1, mismatch = FALSE;; i++) {
            db_enum_link(hDB, hKey, i, &hSubkey);

            if (!hSubkey)
               break;

            db_get_link(hDB, hSubkey, &key);
            strcpy(str, key.name);

            str[strlen(partial)] = 0;

            if (strchr(key.name, ' '))
               blanks = TRUE;

            if (equal_ustring(str, partial)) {
               strcpy(last_match, key.name);
               if (c == 1)
                  c = toupper(key.name[j]);
               else if (c != toupper(key.name[j])) {
                  mismatch = TRUE;
                  break;
               }
            }
         }

         if (mismatch || last_match[j] == 0)
            break;
      }

      /* search split point */
      for (pc = key_name + strlen(key_name) - 1; pc > key_name && *pc != '/'; pc--);
      if (*pc == '/')
         pc++;

      for (i = 0; i < j; i++)
         pc[i] = last_match[i];
      pc[i] = 0;

      /* insert '"' if blanks in name */
      if (blanks) {
         if (key_name[0] != '"') {
            for (i = strlen(key_name); i >= 0; i--)
               key_name[i + 1] = key_name[i];

            key_name[0] = '"';
         }
      }

      strcpy(line, head);
      strcat(line, key_name);
      *cursor = strlen(line);
      strcat(line, tail);
      return TRUE;
   }

   /* beep if not found */
   printf("\007");

   strcpy(line, head);
   strcat(line, key_name);
   *cursor = strlen(line);
   strcat(line, tail);
   return FALSE;
}

/*------------------------------------------------------------------*/

INT search_key(HNDLE hDB, HNDLE hKey, KEY * key, INT level, void *info)
{
   INT i, size, status;
   char *pattern;
   static char data_str[MAX_ODB_PATH], line[MAX_ODB_PATH];
   char path[MAX_ODB_PATH];

   pattern = (char *) info;

   if (match(pattern, key->name)) {
      db_get_path(hDB, hKey, path, MAX_ODB_PATH);
      strcpy(line, path);
      strcat(line, " : ");

      if (key->type != TID_KEY) {
         assert(key->total_size > 0);
         char* data = (char*)malloc(key->total_size);
         size = key->total_size;
         status = db_get_data(hDB, hKey, data, &size, key->type);

         if (status == DB_NO_ACCESS)
            strcpy(data_str, "<no read access>");
         else
            db_sprintf(data_str, data, key->item_size, 0, key->type);

         if (key->num_values == 1)
            strcat(line, data_str);

         printf("%s\n", line);

         if (key->num_values > 1)
            for (i = 0; i < key->num_values; i++) {
               db_sprintf(data_str, data, key->item_size, i, key->type);

               printf("  [%d] : %s\n", i, data_str);
            }
         free(data);
      } else {
         printf("%s\n", line);
      }
   }

   return SUCCESS;
}

/*------------------------------------------------------------------*/

void del_tree(HNDLE hDB, HNDLE hKey, INT level)
{
   INT i;
   KEY key;
   HNDLE hSubkey;

   for (i = 0;; i++) {
      db_enum_link(hDB, hKey, i, &hSubkey);

      if (!hSubkey)
         break;

      db_get_key(hDB, hSubkey, &key);
      if (key.type == TID_KEY)
         del_tree(hDB, hSubkey, level + 1);

      if (rand() < RAND_MAX / 10)
         db_delete_key(hDB, hSubkey, 0);
   }
}

/*------------------------------------------------------------------*/

static void xwrite(const char* filename, int fd, const void* data, int size)
{
   int wr = write(fd, data, size);
   if (wr != size) {
      cm_msg(MERROR, "xwrite", "cannot write to \'%s\', write(%d) returned %d, errno %d (%s)", filename, size, wr, errno, strerror(errno));
   }
}

void create_experim_h(HNDLE hDB, const char *analyzer_name)
{
   INT i, index, subindex, hfile, status, size;
   HNDLE hKey, hKeyRoot, hKeyEq, hDefKey, hKeyBank, hKeyPar;
   char str[100+80], eq_name[80], subeq_name[80];
   KEY key;
   time_t now;

   char experim_h_comment1[] =
       "/********************************************************************\\\n\
\n\
  Name:         experim.h\n\
  Created by:   ODBedit program\n\
\n\
  Contents:     This file contains C structures for the \"Experiment\"\n\
                tree in the ODB and the \"/Analyzer/Parameters\" tree.\n\
\n\
                Additionally, it contains the \"Settings\" subtree for\n\
                all items listed under \"/Equipment\" as well as their\n\
                event definition.\n\
\n\
                It can be used by the frontend and analyzer to work\n\
                with these information.\n\
\n\
                All C structures are accompanied with a string represen-\n\
                tation which can be used in the db_create_record function\n\
                to setup an ODB structure which matches the C structure.\n\
\n";
   char experim_h_comment2[] =
       "\\********************************************************************/\n\n";

   const char *file_name = "experim.h";

   /* create file */
   hfile = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
   if (hfile == -1)
      cm_msg(MERROR, "create_experim_h", "cannot open experim.h file.");

   /* write comment to file */
   xwrite(file_name, hfile, experim_h_comment1, strlen(experim_h_comment1));
   time(&now);
   char ctimebuf[32];
   ctime_r(&now, ctimebuf);
   sprintf(str, "  Created on:   %s\n", ctimebuf);
   xwrite(file_name, hfile, str, strlen(str));
   xwrite(file_name, hfile, experim_h_comment2, strlen(experim_h_comment2));

   /* write /experiment/run parameters */
   db_find_key(hDB, 0, "/Experiment/Run Parameters", &hKey);
   if (hKey) {
      sprintf(str, "#define EXP_PARAM_DEFINED\n\n");
      xwrite(file_name, hfile, str, strlen(str));
      db_save_struct(hDB, hKey, file_name, "EXP_PARAM", TRUE);
      db_save_string(hDB, hKey, file_name, "EXP_PARAM_STR", TRUE);
      lseek(hfile, 0, SEEK_END);
   }

   /* write /experiment/edit on start */
   db_find_key(hDB, 0, "/Experiment/Edit on start", &hKey);
   if (hKey) {
      sprintf(str, "#define EXP_EDIT_DEFINED\n\n");
      xwrite(file_name, hfile, str, strlen(str));
      db_save_struct(hDB, hKey, file_name, "EXP_EDIT", TRUE);
      db_save_string(hDB, hKey, file_name, "EXP_EDIT_STR", TRUE);
      lseek(hfile, 0, SEEK_END);
   }

   /* write /<Analyzer>/parameters tree */
   sprintf(str, "/%s/Parameters", analyzer_name);
   status = db_find_key(hDB, 0, str, &hKeyRoot);
   if (status != DB_SUCCESS) {
      printf("Analyzer \"%s\" not found in ODB, skipping analyzer parameters.\n",
             analyzer_name);
   } else {
      for (index = 0;; index++) {
         status = db_enum_key(hDB, hKeyRoot, index, &hKeyPar);
         if (status == DB_NO_MORE_SUBKEYS)
            break;

         db_get_key(hDB, hKeyPar, &key);
         strlcpy(eq_name, key.name, sizeof(eq_name));
         name2c(eq_name);
         for (i = 0; i < (int) strlen(eq_name); i++)
            eq_name[i] = toupper(eq_name[i]);

         lseek(hfile, 0, SEEK_END);
         sprintf(str, "#ifndef EXCL_%s\n\n", eq_name);
         xwrite(file_name, hfile, str, strlen(str));

         sprintf(str, "#define %s_PARAM_DEFINED\n\n", eq_name);
         xwrite(file_name, hfile, str, strlen(str));
         sprintf(str, "%s_PARAM", eq_name);
         db_save_struct(hDB, hKeyPar, file_name, str, TRUE);
         sprintf(str, "%s_PARAM_STR", eq_name);
         db_save_string(hDB, hKeyPar, file_name, str, TRUE);

         lseek(hfile, 0, SEEK_END);
         sprintf(str, "#endif\n\n");
         xwrite(file_name, hfile, str, strlen(str));
      }
   }

   /* loop through equipment list */
   status = db_find_key(hDB, 0, "/Equipment", &hKeyRoot);
   if (status == DB_SUCCESS)
      for (index = 0;; index++) {
         status = db_enum_key(hDB, hKeyRoot, index, &hKeyEq);
         if (status == DB_NO_MORE_SUBKEYS)
            break;

         db_get_key(hDB, hKeyEq, &key);
         strcpy(eq_name, key.name);
         name2c(eq_name);
         for (i = 0; i < (int) strlen(eq_name); i++)
            eq_name[i] = toupper(eq_name[i]);

         lseek(hfile, 0, SEEK_END);
         sprintf(str, "#ifndef EXCL_%s\n\n", eq_name);
         xwrite(file_name, hfile, str, strlen(str));

         size = sizeof(str);
         str[0] = 0;
         db_get_value(hDB, hKeyEq, "Common/Format", str, &size, TID_STRING, TRUE);

         /* if event is in fixed format, extract header file */
         if (equal_ustring(str, "Fixed")) {
            db_find_key(hDB, hKeyEq, "Variables", &hDefKey);
            if (hDefKey) {
               lseek(hfile, 0, SEEK_END);
               sprintf(str, "#define %s_EVENT_DEFINED\n\n", eq_name);
               xwrite(file_name, hfile, str, strlen(str));

               sprintf(str, "%s_EVENT", eq_name);
               db_save_struct(hDB, hDefKey, file_name, str, TRUE);
               sprintf(str, "%s_EVENT_STR", eq_name);
               db_save_string(hDB, hDefKey, file_name, str, TRUE);
            }
         }

         /* if event is in MIDAS format, extract bank definition */
         else if (equal_ustring(str, "MIDAS")) {
            db_find_key(hDB, hKeyEq, "Variables", &hDefKey);
            if (hDefKey) {
               for (i = 0;; i++) {
                  status = db_enum_key(hDB, hDefKey, i, &hKeyBank);
                  if (status == DB_NO_MORE_SUBKEYS)
                     break;

                  db_get_key(hDB, hKeyBank, &key);

                  if (key.type == TID_KEY) {
                     lseek(hfile, 0, SEEK_END);
                     sprintf(str, "#define %s_BANK_DEFINED\n\n", key.name);
                     xwrite(file_name, hfile, str, strlen(str));

                     sprintf(str, "%s_BANK", key.name);
                     db_save_struct(hDB, hKeyBank, file_name, str, TRUE);
                     sprintf(str, "%s_BANK_STR", key.name);
                     db_save_string(hDB, hKeyBank, file_name, str, TRUE);
                  }
               }
            }
         }

         /* Scan sub tree for that equipment */
         for (subindex = 0;; subindex++) {
            status = db_enum_key(hDB, hKeyEq, subindex, &hDefKey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            db_get_key(hDB, hDefKey, &key);
            strcpy(subeq_name, key.name);
            name2c(subeq_name);

            for (i = 0; i < (int) strlen(subeq_name); i++)
               subeq_name[i] = toupper(subeq_name[i]);

            /* Skip only the statistics */
            if (!equal_ustring(subeq_name, "statistics") &&
                !equal_ustring(subeq_name, "variables")) {
               lseek(hfile, 0, SEEK_END);
               sprintf(str, "#define %s_%s_DEFINED\n\n", eq_name, subeq_name);
               xwrite(file_name, hfile, str, strlen(str));

               sprintf(str, "%s_%s", eq_name, subeq_name);
               db_save_struct(hDB, hDefKey, file_name, str, TRUE);
               sprintf(str, "%s_%s_STR", eq_name, subeq_name);
               db_save_string(hDB, hDefKey, file_name, str, TRUE);
            }
         }

         lseek(hfile, 0, SEEK_END);
         sprintf(str, "#endif\n\n");
         xwrite(file_name, hfile, str, strlen(str));
      }

   close(hfile);

   std::string cwd = ss_getcwd();
   printf("\"experim.h\" has been written to %s/%s\n", cwd.c_str(), file_name);
}

/*------------------------------------------------------------------*/

INT cmd_idle()
{
   INT status;

   need_redraw = FALSE;

   status = cm_yield(100);

   /* abort if server connection is broken */
   if (status == SS_ABORT || status == RPC_SHUTDOWN) {
      if (status == SS_ABORT)
         printf("Server connection broken.\n");

      ss_getchar(1);
      cm_disconnect_experiment();
      exit(0);
   }

   /* check for broken client connections */
   rpc_client_check();

   return need_redraw;
}

/*------------------------------------------------------------------*/

void compose_name(char *pwd, char *name, char *full_name)
{
   if (name[0] != '/') {
      strcpy(full_name, pwd);
      if (full_name[strlen(full_name) - 1] != '/')
         strcat(full_name, "/");
      strcat(full_name, name);
   } else
      strcpy(full_name, name);
}

/*------------------------------------------------------------------*/

void assemble_prompt(char *prompt, int psize, char *host_name, char *exp_name, char *pwd)
{
   HNDLE hDB;
   char mask[256], str[32];
   int state, size;
   char *pp, *pm, *pc;
   time_t now;

   const char *state_char[] = { "U", "S", "P", "R" };
   const char *state_str[] = { "Unknown", "Stopped", "Paused", "Running" };

   cm_get_experiment_database(&hDB, NULL);

   size = sizeof(mask);
   strcpy(mask, "[%h:%e:%s]%p>");
   db_get_value(hDB, 0, "/System/Prompt", mask, &size, TID_STRING, TRUE);

   state = STATE_STOPPED;
   size = sizeof(state);
   db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, TRUE);
   if (state > STATE_RUNNING)
      state = 0;

   pm = mask;
   pp = prompt;
   memset(prompt, 0, psize);
   do {
      if (*pm != '%') {
         *pp++ = *pm++;
      } else {
         switch (*++pm) {
         case 't':
            time(&now);
            assert(sizeof(str) >= 32);
            ctime_r(&now, str);
            str[19] = 0;
            strcpy(pp, str + 11);
            break;
         case 'h':
            if (host_name[0])
               strcat(pp, host_name);
            else
               strcat(pp, "local");
            break;
         case 'e':
            if (exp_name[0])
               strcat(pp, exp_name);
            else
               strcat(pp, "Default");
            break;
         case 's':
            strcat(pp, state_char[state]);
            break;
         case 'S':
            strcat(pp, state_str[state]);
            break;
         case 'P':
            strcat(pp, pwd);
            break;
         case 'p':
            pc = pwd + strlen(pwd) - 1;
            while (*pc != '/' && pc != pwd)
               pc--;
            if (pc == pwd)
               strcat(pp, pwd);
            else
               strcat(pp, pc + 1);
            break;
         }
         pm++;
         pp += strlen(pp);
      }
   } while (*pm);
}

/*------------------------------------------------------------------*/

void watch_callback(HNDLE hDB, HNDLE hKey, INT index, void* info)
{
   KEY key;
   int status;
   
   std::string path = db_get_path(hDB, hKey);

   status = db_get_key(hDB, hKey, &key);
   if (status != DB_SUCCESS) {
      printf("callback for invalid or deleted hkey %d odb path %s\n", hKey, path.c_str());
      return;
   }

   if (key.type == TID_KEY)
      printf("%s modified\n", path.c_str());
   else {
      if (key.num_values == 1) {
         if (key.type == TID_STRING) {
            std::string data;
            db_get_value_string(hDB, 0, path.c_str(), 0, &data);
            printf("%s = \"%s\"\n", path.c_str(), data.c_str());
         } else {
            char data[10000], str[256];
            int size = sizeof(data);
            db_get_data(hDB, hKey, data, &size, key.type);
            db_sprintf(str, data, size, 0, key.type);
            printf("%s = %s\n", path.c_str(), str);
         }
      } else {
         if (index == -1) {
            printf("%s[*] modified\n", path.c_str());
         } else {
            if (key.type == TID_STRING) {
               std::string data;
               db_get_value_string(hDB, 0, path.c_str(), index, &data);
               printf("%s[%d] = \"%s\"\n", path.c_str(), index, data.c_str());
            } else {
               char data[10000], str[256];
               int size = sizeof(data);
               db_get_data_index(hDB, hKey, data, &size, index, key.type);
               db_sprintf(str, data, size, 0, key.type);
               printf("%s[%d] = %s\n", path.c_str(), index, str);
            }
         }
      }
   }
}

/*------------------------------------------------------------------*/

int command_loop(char *host_name, char *exp_name, char *cmd, char *start_dir)
{
   INT status = 0, i, k, state, size, old_run_number, new_run_number;
   char line[2000], prompt[256];
   char param[10][2000];
   char str[2000], str2[80], name[256], *pc, data_str[256];
   char old_password[32], new_password[32];
   INT nparam, flags, index1, index2, debug_flag, mthread_flag;
   WORD mode;
   HNDLE hDB, hKey, hKeyClient, hSubkey, hRootKey;
   KEY key;
   char user_name[80] = "";
   FILE *cmd_file = NULL;
   DWORD last_msg_time = 0;
   char message[2000], client_name[256], *p;
   INT n1, n2;
   PRINT_INFO print_info;

   cm_get_experiment_database(&hDB, &hKeyClient);

   /* command loop */
   if (start_dir[0])
      strcpy(pwd, start_dir);
   else
      strcpy(pwd, "/");

   /* check if dir exists */
   if (db_find_key(hDB, 0, pwd, &hKey) != DB_SUCCESS) {
      printf("Directory \"%s\" not found.\n", pwd);
      return -1;
   }

   /* open command file */
   if (cmd[0] == '@') {
      cmd_file = fopen(cmd + 1, "r");
      if (cmd_file == NULL) {
         printf("Command file %s not found.\n", cmd + 1);
         return -1;
      }
   }

   do {
      /* print prompt */
      if (!cmd_mode) {
         assemble_prompt(prompt, sizeof(prompt), host_name, exp_name, pwd);

         in_cmd_edit = TRUE;
         line[0] = 0;
         cmd_edit(prompt, line, cmd_dir, cmd_idle);
         in_cmd_edit = FALSE;
      } else if (cmd[0] != '@')
         strlcpy(line, cmd, sizeof(line));
      else {
         memset(line, 0, sizeof(line));
         char* s = fgets(line, sizeof(line), cmd_file);

         if (s == NULL || line[0] == 0)
            break;

         /* cut off CR */
         while (strlen(line) > 0 && line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = 0;

         if (line[0] == 0)
            continue;
      }

      /* analyze line */
      nparam = 0;
      pc = line;
      while (*pc == ' ')
         pc++;

      memset(param, 0, sizeof(param));
      do {
         if (*pc == '"') {
            pc++;
            for (i = 0; *pc && *pc != '"' && i<(int)sizeof(param[0])-1; i++)
               param[nparam][i] = *pc++;
            if (*pc)
               pc++;
         } else if (*pc == '\'') {
            pc++;
            for (i = 0; *pc && *pc != '\'' && i<(int)sizeof(param[0])-1; i++)
               param[nparam][i] = *pc++;
            if (*pc)
               pc++;
         } else if (*pc == '`') {
            pc++;
            for (i = 0; *pc && *pc != '`' && i<(int)sizeof(param[0])-1; i++)
               param[nparam][i] = *pc++;
            if (*pc)
               pc++;
         } else
            for (i = 0; *pc && *pc != ' ' && i<(int)sizeof(param[0])-1; i++)
               param[nparam][i] = *pc++;
         param[nparam][i] = 0;
         while (*pc == ' ')
            pc++;
         nparam++;
      } while (*pc);

      /* help */
      if ((param[0][0] == 'h' && param[0][1] == 'e') || param[0][0] == '?')
         print_help(param[1]);

      /* ls */
      else if ((param[0][0] == 'l' && param[0][1] == 's') ||
               (param[0][0] == 'd' && param[0][1] == 'i')) {
         db_find_key(hDB, 0, pwd, &hKey);
         print_info.flags = 0;
         print_info.pattern[0] = 0;
         ls_line = 0;
         ls_abort = FALSE;

         /* parse options */
         for (i = 1; i < 4; i++)
            if (param[i][0] == '-') {
               for (int j = 1; param[i][j] != ' ' && param[i][j]; j++) {
                  if (param[i][j] == 'l')
                     print_info.flags |= PI_LONG;
                  if (param[i][j] == 'r')
                     print_info.flags |= PI_RECURSIVE;
                  if (param[i][j] == 'v')
                     print_info.flags |= PI_VALUE;
                  if (param[i][j] == 'h')
                     print_info.flags |= PI_HEX;
                  if (param[i][j] == 'p')
                     print_info.flags |= PI_PAUSE;
               }
            }

         for (i = 1; param[i][0] == '-'; i++);

         /* check if parameter contains array index */
         print_info.index = -1;
         if (strchr(param[i], '[') && strchr(param[i], ']')) {
            for (p = strchr(param[i], '[') + 1; *p && *p != ']'; p++)
               if (!isdigit(*p))
                  break;

            if (*p && *p == ']') {
               print_info.index = atoi(strchr(param[i], '[') + 1);
               *strchr(param[i], '[') = 0;
            }
         }

         if (param[i][0]) {
            if (strpbrk(param[i], "*?") != NULL) {
               /* if parameter contains wildcards, set pattern */
               strcpy(print_info.pattern, param[i]);
            } else {
               if (param[i][0] == '/')
                  status = db_find_link(hDB, 0, param[i], &hKey);
               else
                  status = db_find_link(hDB, hKey, param[i], &hKey);

               if (status == DB_NO_KEY)
                  printf("key %s not found\n", param[i]);

               if (status == DB_INVALID_LINK)
                  printf("link %s points to invalid location\n", param[i]);
            }
         }

         if (hKey) {
            if ((print_info.flags & PI_LONG) && (print_info.flags & PI_VALUE) == 0) {
               printf
                   ("Key name                        Type    #Val  Size  Last Opn Mode Value\n");
               printf
                   ("---------------------------------------------------------------------------\n");
            }

            if (print_info.flags & PI_RECURSIVE) {
               db_scan_tree(hDB, hKey, 0, print_key, &print_info);
               if (cm_is_ctrlc_pressed())
                  cm_ack_ctrlc_pressed();
            } else {
               db_get_link(hDB, hKey, &key);
               if (key.type != TID_KEY)
                  print_key(hDB, hKey, &key, 0, &print_info);
               else
                  for (i = 0;; i++) {
                     if (cm_is_ctrlc_pressed()) {
                        cm_ack_ctrlc_pressed();
                        break;
                     }

                     db_enum_link(hDB, hKey, i, &hSubkey);

                     if (!hSubkey)
                        break;

                     db_get_link(hDB, hSubkey, &key);
                     status = print_key(hDB, hSubkey, &key, 0, &print_info);
                     if (status == 0)
                        break;
                  }
            }
         }
      }

      /* cd */
      else if (param[0][0] == 'c' && param[0][1] == 'd') {
         compose_name(pwd, param[1], str);

         status = db_find_key(hDB, 0, str, &hKey);

         if (strcmp(str, "/") == 0)
            strcpy(pwd, str);
         else if (status == DB_SUCCESS) {
            db_get_key(hDB, hKey, &key);
            if (key.type != TID_KEY)
               printf("key has no subkeys\n");
            else
               db_get_path(hDB, hKey, pwd, 256);
         } else
            printf("key not found\n");
      }

      /* pwd */
      else if (param[0][0] == 'p' && param[0][1] == 'w') {
         printf("%s\n", pwd);
      }

      /* create */
      else if (param[0][0] == 'c' && param[0][1] == 'r') {
         compose_name(pwd, param[2], str);

         /* check if array */

         k = -1;
         int j = 0;
         if (str[strlen(str) - 1] == ']') {
            if (strchr(str, '[')) {
               j = atoi(strchr(str, '[') + 1);
               strlcpy(str2, strchr(str, '[') + 1, sizeof(str2));
               *strchr(str, '[') = 0;
               if (strchr(str2, '['))
                  k = atoi(strchr(str2, '[')+1);
            }
         } else
            j = 1;

         /* get TID */
         for (i = 0; i < TID_LAST; i++) {
            if (equal_ustring(rpc_tid_name(i), param[1]))
               break;
         }

         if (i == TID_LAST) {
            printf("Unknown type. Must be one of:\n");
            printf("{ UINT8,INT8,UINT16,INT16,UINT32,INT32,UINT64,INT64,BOOL,FLOAT,DOUBLE,STRING }\n");
         } else {
            db_create_key(hDB, 0, str, i);
            db_find_key(hDB, 0, str, &hKey);
            db_get_key(hDB, hKey, &key);

            if (i == TID_STRING) {
               if (!cmd_mode && k == -1) {
                  printf("String length [%d]: ", NAME_LENGTH);
                  ss_gets(str, 256);
                  if (str[0])
                     key.item_size = atoi(str);
                  else
                     key.item_size = NAME_LENGTH;
               } else if (k == -1)
                  key.item_size = NAME_LENGTH;
               else
                  key.item_size = k;

               char *buf = (char*)malloc(key.item_size);
               memset(buf, 0, sizeof(key.item_size));
               db_set_link_data(hDB, hKey, buf, key.item_size, 1, key.type);
               free(buf);
            }

            if (j > 1) {
               if (key.type == TID_LINK)
                  key.item_size = NAME_LENGTH;
               char *buf = (char*)malloc(key.item_size);
               memset(buf, 0, sizeof(key.item_size));
               db_set_link_data_index(hDB, hKey, buf, key.item_size, j - 1, key.type);
               free(buf);
            }
         }
      }

      /* mkdir */
      else if (param[0][0] == 'm' && param[0][1] == 'k') {
         compose_name(pwd, param[1], str);
         db_create_key(hDB, 0, str, TID_KEY);
      }

      /* link */
      else if (param[0][0] == 'l' && param[0][1] == 'n') {
         compose_name(pwd, param[2], str);
         db_create_link(hDB, 0, str, param[1]);
      }

      /* copy */
      else if (param[0][0] == 'c' && (param[0][1] == 'o' || param[0][1] == 'p')) {
         /* test if destination exists */
         compose_name(pwd, param[2], str);
         status = db_find_link(hDB, 0, str, &hKey);
         if (status == DB_SUCCESS) {
            if (cmd_mode)
               str[0] = 'y';
            else {
               printf("Overwrite existing key\n\"%s\"\n(y/[n]) ", str);
               ss_gets(str, 256);
            }
            if (str[0] == 'y')
               db_delete_key(hDB, hKey, FALSE);
         }

         if (status == DB_NO_KEY || str[0] == 'y') {
            compose_name(pwd, param[1], str);

            status = db_find_link(hDB, 0, str, &hKey);
            if (status == DB_SUCCESS) {
               compose_name(pwd, param[2], str);

               db_get_key(hDB, hKey, &key);
               db_create_key(hDB, 0, str, key.type);

               if (key.type != TID_KEY) {
                  assert(key.total_size > 0);
                  char *buf = (char*)malloc(key.total_size);
                  size = key.total_size;
                  db_get_data(hDB, hKey, buf, &size, key.type);
                  db_find_key(hDB, 0, str, &hKey);
                  db_set_link_data(hDB, hKey, buf, size, key.num_values, key.type);
                  free(buf);
               } else {
                  char data[50000];
                  size = sizeof(data);
                  status = db_copy(hDB, hKey, data, &size, "");
                  if (status == DB_TRUNCATED) {
                     printf("error: db_copy() status %d, odbedit internal buffer is too small, size %d\n", status, size);
                  } else if (status != DB_SUCCESS) {
                     printf("error: db_copy() status %d\n", status);
                  } else {
                     db_find_key(hDB, 0, str, &hKey);
                     db_paste(hDB, hKey, data);
                  }
               }
            } else {
               printf("key not found\n");
            }
         }
      }

      /* delete */
      else if ((param[0][0] == 'd' && param[0][1] == 'e') ||
               (param[0][0] == 'r' && param[0][1] == 'm')) {
         flags = 0;
         if ((param[1][0] == '-' && param[1][1] == 'f') ||
             (param[2][0] == '-' && param[2][1] == 'f'))
            flags |= (1 << 0);
         if ((param[1][0] == '-' && param[1][1] == 'l') ||
             (param[2][0] == '-' && param[2][1] == 'l'))
            flags |= (1 << 1);

         for (i = 1; param[i][0] == '-'; i++);

         compose_name(pwd, param[i], str);

         status = db_find_link(hDB, 0, str, &hKey);
         db_get_key(hDB, hKey, &key);

         if (status == DB_SUCCESS) {
            if (flags & (1 << 0) || cmd_mode)
               str[0] = 'y';
            else {
               if (key.type == TID_KEY)
                  printf
                      ("Are you sure to delete the key\n\"%s\"\nand all its subkeys? (y/[n]) ",
                       str);
               else
                  printf("Are you sure to delete the key\n\"%s\"\n(y/[n]) ", str);

               ss_gets(str, 256);
            }

            if (str[0] == 'y') {
               status = db_delete_key(hDB, hKey, (flags & (1 << 1)) > 0);
               if (status == DB_NO_ACCESS) {
                  printf("deletion of key not allowed\n");
               } else if (status == DB_OPEN_RECORD) {
                  printf("key is open by other client\n");
               } else if (status != DB_SUCCESS) {
                  printf("Error, db_delete_key() status %d\n", status);
               }
            }
         } else
            printf("key not found\n");
      }

      /* set */
      else if (param[0][0] == 's' && param[0][1] == 'e') {
         /* check if index is supplied */
         index1 = index2 = 0;
         strcpy(str, param[1]);
         strarrayindex(str, &index1, &index2);
         compose_name(pwd, str, name);

         std::vector<HNDLE> keys;
         status = db_find_keys(hDB, 0, name, keys);

         if(status != DB_SUCCESS){
            printf("Error: Key \"%s\" not found\n", name);
            if (cmd_mode)
               return -1;
         } else {
            for(HNDLE hMatchedKey: keys){
               set_key(hDB, hMatchedKey, index1, index2, param[2]);
            }
         }
      }

      /* set mode */
      else if (param[0][0] == 'c' && param[0][1] == 'h' && param[0][2] == 'm') {
         if (param[1][0] == 0 && param[2][0] == 0) {
            printf("Please specify mode and key\n");
         } else {
            compose_name(pwd, param[2], str);
            
            mode = atoi(param[1]);
            
            if (strcmp(str, "/") != 0) {
               status = db_find_key(hDB, 0, str, &hKey);
            } else {
               status = DB_SUCCESS;
               hKey = 0;
            }
            
            if (status == DB_SUCCESS) {
               if (cmd_mode)
                  str[0] = 'y';
               else {
                  printf
                  ("Are you sure to change the mode of key\n  %s\nand all its subkeys\n",
                   str);
                  printf("to mode [%c%c%c%c]? (y/[n]) ", mode & MODE_READ ? 'R' : 0,
                         mode & MODE_WRITE ? 'W' : 0, mode & MODE_DELETE ? 'D' : 0,
                         mode & MODE_EXCLUSIVE ? 'E' : 0);
                  ss_gets(str, 256);
               }
               if (str[0] == 'y')
                  db_set_mode(hDB, hKey, mode, TRUE);
            } else {
               printf("Error: Key \"%s\" not found\n", str);
               if (cmd_mode)
                  return -1;
            }
         }
      }

      /* test_rpc */
      else if (strcmp(param[0], "test_rpc") == 0) {
         status = rpc_test_rpc();
         if (status == RPC_SUCCESS)
            printf("RPC test passed!\n");
         else
            printf("RPC test failed!\n");
      }

      /* truncate */
      else if (param[0][0] == 't' && param[0][1] == 'r') {
         if (param[1][0] == 0) {
            printf("Please specify key\n");
         } else {
            compose_name(pwd, param[1], str);
            
            status = db_find_key(hDB, 0, str, &hKey);
            
            i = atoi(param[2]);
            if (i == 0)
               i = 1;
            
            if (status == DB_SUCCESS)
               db_set_num_values(hDB, hKey, i);
            else {
               printf("Error: Key \"%s\" not found\n", str);
               if (cmd_mode)
                  return -1;
            }
         }
      }

      /* rename */
      else if (param[0][0] == 'r' && param[0][1] == 'e' && param[0][2] == 'n') {
         if (param[1][0] == 0) {
            printf("Please specify key\n");
         } else {
            compose_name(pwd, param[1], str);
            
            if (strcmp(str, "/") != 0)
               status = db_find_link(hDB, 0, str, &hKey);
            else
               hKey = 0;
            
            if (status == DB_SUCCESS || !hKey)
               db_rename_key(hDB, hKey, param[2]);
            else {
               printf("Error: Key \"%s\" not found\n", str);
               if (cmd_mode)
                  return -1;
            }
         }
      }

      /* move */
      else if (param[0][0] == 'm' && param[0][1] == 'o') {
         if (param[1][0] == 0) {
            printf("Please specify key\n");
         } else {
            compose_name(pwd, param[1], str);
            
            if (strcmp(str, "/") != 0)
               status = db_find_link(hDB, 0, str, &hKey);
            else
               hKey = 0;
            
            if (status == DB_SUCCESS || !hKey) {
               if (param[2][0] == 't')
                  i = 0;
               else if (param[2][0] == 'b')
                  i = -1;
               else
                  i = atoi(param[2]);
               
               status = db_reorder_key(hDB, hKey, i);
               if (status == DB_NO_ACCESS)
                  printf("no write access to key\n");
               if (status == DB_OPEN_RECORD)
                  printf("key is open by other client\n");
            } else {
               printf("Error: Key \"%s\" not found\n", str);
               if (cmd_mode)
                  return -1;
            }
         }
      }

      /* find key */
      else if (param[0][0] == 'f' && param[0][1] == 'i') {
         status = db_find_key(hDB, 0, pwd, &hKey);

         if (status == DB_SUCCESS)
            db_scan_tree(hDB, hKey, 0, search_key, (void *) param[1]);
         else
            printf("current key is invalid / no read access\n");
      }

      /* load */
      else if (param[0][0] == 'l' && param[0][1] == 'o') {
         db_find_key(hDB, 0, pwd, &hKey);

         db_load(hDB, hKey, param[1], FALSE);
      }

      /* save */
      else if (param[0][0] == 's' && param[0][1] == 'a') {
         db_find_key(hDB, 0, pwd, &hKey);

         if (strstr(param[1], ".xml") || strstr(param[1], ".XML"))
            db_save_xml(hDB, hKey, param[1]);
         else if (strstr(param[1], ".json") || strstr(param[1], ".js"))
            db_save_json(hDB, hKey, param[1]);
         else if (param[1][0] == '-') {
            if (param[1][1] == 'c' && param[1][2] == 's') {
               db_save_struct(hDB, hKey, param[2], NULL, FALSE);
               db_save_string(hDB, hKey, param[2], NULL, TRUE);
            }
            else if (param[1][1] == 'c')
               db_save_struct(hDB, hKey, param[2], NULL, FALSE);
            else if (param[1][1] == 's')
               db_save_string(hDB, hKey, param[2], NULL, FALSE);
            else if (param[1][1] == 'x')
               db_save_xml(hDB, hKey, param[2]);
            else if (param[1][1] == 'j')
               db_save_json(hDB, hKey, param[2]);
            else if (param[1][1] == 'z')
               db_save_json(hDB, hKey, param[2],
                            JSFLAG_RECURSE | JSFLAG_OMIT_LAST_WRITTEN | JSFLAG_FOLLOW_LINKS);
         } else
            db_save(hDB, hKey, param[1], FALSE);
      }

      /* json */
      else if (strncmp(param[0], "json", 8) == 0) {

         if (param[1][0] == '/') {
            db_find_key(hDB, 0, param[1], &hKey);
         } else if (strlen(param[1]) > 0) {
            db_find_key(hDB, 0, pwd, &hKey);
            db_find_key(hDB, hKey, param[1], &hKey);
         } else {
            db_find_key(hDB, 0, pwd, &hKey);
         }

	 char* buffer = NULL;
	 int buffer_size = 0;
	 int buffer_end = 0;

	 status = db_copy_json_save(hDB, hKey, &buffer, &buffer_size, &buffer_end);

	 printf("status: %d, json: %s\n", status, buffer);

	 if (buffer)
	   free(buffer);
      }

      /* jsvalues */
      else if (strncmp(param[0], "jsvalues", 8) == 0) {
         db_find_key(hDB, 0, pwd, &hKey);

	 char* buffer = NULL;
	 int buffer_size = 0;
	 int buffer_end = 0;

	 int omit_names = 0;
	 int omit_last_written = 0;
	 time_t omit_old_timestamp = 0;
	 int preserve_case = 0;

	 status = db_copy_json_values(hDB, hKey, &buffer, &buffer_size, &buffer_end, omit_names, omit_last_written, omit_old_timestamp,preserve_case);

	 printf("status: %d, json: %s\n", status, buffer);

	 if (buffer)
	   free(buffer);
      }

      /* jsls */
      else if (strncmp(param[0], "jsls", 4) == 0) {

         if (param[1][0] == '/') {
            db_find_key(hDB, 0, param[1], &hKey);
         } else if (strlen(param[1]) > 0) {
            db_find_key(hDB, 0, pwd, &hKey);
            db_find_key(hDB, hKey, param[1], &hKey);
         } else {
            db_find_key(hDB, 0, pwd, &hKey);
         }

	 char* buffer = NULL;
	 int buffer_size = 0;
	 int buffer_end = 0;

	 status = db_copy_json_ls(hDB, hKey, &buffer, &buffer_size, &buffer_end);

	 printf("jsls \"%s\", status: %d, json: %s\n", pwd, status, buffer);

	 if (buffer)
	   free(buffer);
      }

      /* make */
      else if (param[0][0] == 'm' && param[0][1] == 'a') {
         if (param[1][0])
            create_experim_h(hDB, param[1]);
         else
            create_experim_h(hDB, "Analyzer");
      }

      /* passwd */
      else if (param[0][0] == 'p' && param[0][1] == 'a' && param[0][2] == 's') {
         /*
            strcpy(str, ss_crypt("foob", "ar"));
            if(strcmp(str, "arlEKn0OzVJn.") != 0)
            printf("Warning: ss_crypt() works incorrect");
          */

         if (db_find_key(hDB, 0, "/Experiment/Security/Password", &hKey) == DB_SUCCESS) {
            size = sizeof(old_password);
            db_get_data(hDB, hKey, old_password, &size, TID_STRING);

            strcpy(str, ss_getpass("Old password: "));
            strcpy(str, ss_crypt(str, "mi"));

            if (strcmp(str, old_password) == 0 || strcmp(str, "mid7qBxsNMHux") == 0) {
               strcpy(str, ss_getpass("New password: "));
               strcpy(new_password, ss_crypt(str, "mi"));

               strcpy(str, ss_getpass("Retype new password: "));
               if (strcmp(new_password, ss_crypt(str, "mi")) != 0)
                  printf("Mismatch - password unchanged\n");
               else
                  db_set_data(hDB, hKey, new_password, 32, 1, TID_STRING);
            } else
               printf("Wrong password\n");
         } else {
            strcpy(str, ss_getpass("Password: "));
            strcpy(new_password, ss_crypt(str, "mi"));

            strcpy(str, ss_getpass("Retype password: "));
            if (strcmp(new_password, ss_crypt(str, "mi")) != 0)
               printf("Mismatch - password not set\n");
            else {
               /* set password */
               db_set_value(hDB, 0, "/Experiment/Security/Password", new_password, 32, 1,
                            TID_STRING);

               /* create empty allowed hosts and allowd programs entries */
               db_create_key(hDB, 0,
                             "/Experiment/Security/Allowed hosts/host.sample.domain",
                             TID_INT);
               db_create_key(hDB, 0, "/Experiment/Security/Allowed programs/mstat",
                             TID_INT);
            }
         }

      }

      /* webpasswd */
      else if (param[0][0] == 'w' && param[0][1] == 'e' && param[0][2] == 'b') {
         if (db_find_key(hDB, 0, "/Experiment/Security/Web Password", &hKey) ==
             DB_SUCCESS) {
            size = sizeof(old_password);
            db_get_data(hDB, hKey, old_password, &size, TID_STRING);

            strcpy(str, ss_getpass("Old password: "));
            strcpy(str, ss_crypt(str, "mi"));

            if (strcmp(str, old_password) == 0 || strcmp(str, "mid7qBxsNMHux") == 0) {
               strcpy(str, ss_getpass("New password: "));
               strcpy(new_password, ss_crypt(str, "mi"));

               strcpy(str, ss_getpass("Retype new password: "));
               if (strcmp(new_password, ss_crypt(str, "mi")) != 0)
                  printf("Mismatch - password unchanged\n");
               else
                  db_set_data(hDB, hKey, new_password, 32, 1, TID_STRING);
            } else
               printf("Wrong password\n");
         } else {
            strcpy(str, ss_getpass("Password: "));
            strcpy(new_password, ss_crypt(str, "mi"));

            strcpy(str, ss_getpass("Retype password: "));
            if (strcmp(new_password, ss_crypt(str, "mi")) != 0)
               printf("Mismatch - password not set\n");
            else
               /* set password */
               db_set_value(hDB, 0, "/Experiment/Security/Web Password", new_password, 32,
                            1, TID_STRING);
         }
      }

      /* hi */
      else if (param[0][0] == 'h' && param[0][1] == 'i') {
         HNDLE hConn;

         client_name[0] = 0;

         if (!isalpha(param[1][0])) {
            /* find client which exports RPC_ANA_CLEAR_HISTOS */
            status = db_find_key(hDB, 0, "System/Clients", &hRootKey);
            if (status == DB_SUCCESS) {
               for (i = 0;; i++) {
                  status = db_enum_key(hDB, hRootKey, i, &hSubkey);
                  if (status == DB_NO_MORE_SUBKEYS) {
                     printf
                         ("No client currently exports the CLEAR HISTO functionality.\n");
                     break;
                  }

                  sprintf(str, "RPC/%d", RPC_ANA_CLEAR_HISTOS);
                  status = db_find_key(hDB, hSubkey, str, &hKey);
                  if (status == DB_SUCCESS) {
                     size = sizeof(client_name);
                     db_get_value(hDB, hSubkey, "Name", client_name, &size, TID_STRING,
                                  TRUE);
                     break;
                  }
               }
            }

            if (isdigit(param[1][0]))
               n1 = atoi(param[1]);
            else
               n1 = 0;          /* all histos by default */

            if (isdigit(param[2][0]))
               n2 = atoi(param[2]);
            else
               n2 = n1;         /* single histo by default */
         } else {
            strcpy(client_name, param[1]);

            if (isdigit(param[2][0]))
               n1 = atoi(param[2]);
            else
               n1 = 0;          /* all histos by default */

            if (isdigit(param[3][0]))
               n2 = atoi(param[3]);
            else
               n2 = n1;         /* single histo by default */
         }

         if (client_name[0]) {
            if (cm_connect_client(client_name, &hConn) == CM_SUCCESS) {
               rpc_client_call(hConn, RPC_ANA_CLEAR_HISTOS, n1, n2);
               //cm_disconnect_client(hConn, FALSE);
            } else
               printf("Cannot connect to client %s\n", client_name);
         }
      }

      /* import */
      else if (param[0][0] == 'i' && param[0][1] == 'm') {
         int fh;

         fh = open(param[1], O_RDONLY | O_TEXT);
         if (fh < 0)
            printf("File %s not found\n", param[1]);
         else {
            size = lseek(fh, 0, SEEK_END);
            lseek(fh, 0, SEEK_SET);
            assert(size >= 0);
            char* buf = (char*)malloc(size+1);
            size = read(fh, buf, size);
            close(fh);
            if (size == 0 ) {
               buf[size] = 0;
               size++;
            } else if (buf[size-1] != 0) {
               buf[size] = 0;
               size++;
            }

            if (param[2][0] == 0) {
               printf("Key name: ");
               ss_gets(name, 256);
            } else
               strcpy(name, param[2]);

            compose_name(pwd, name, str);

            db_create_key(hDB, 0, str, TID_STRING);
            db_find_key(hDB, 0, str, &hKey);
            db_set_data(hDB, hKey, buf, size, 1, TID_STRING);
            free(buf);
         }

      }

      /* export */
      else if (param[0][0] == 'e' && param[0][1] == 'x' && param[0][2] == 'p') {
         FILE *f;

         if (param[1][0] == 0)
            printf("please specify key\n");
         else {
            compose_name(pwd, param[1], str);

            db_find_key(hDB, 0, str, &hKey);
            if (hKey == 0)
               printf("Error: Key \"%s\" not found\n", param[1]);
            else {
               if (param[2][0] == 0) {
                  printf("File name: ");
                  ss_gets(name, 256);
               } else
                  strcpy(name, param[2]);

               f = fopen(name, "w");
               if (f == NULL)
                  printf("Cannot open file \"%s\"\n", name);
               else {
                  db_get_key(hDB, hKey, &key);
                  if (key.type != TID_STRING)
                     printf("Only export of STRING key possible\n");
                  else {
                     char* buf = (char*) malloc(key.total_size);
                     size = key.total_size;
                     memset(buf, 0, size);
                     db_get_data(hDB, hKey, buf, &size, key.type);
                     fprintf(f, "%s", buf);
                     fclose(f);
                     free(buf);
                  }
               }
            }
         }
      }

      /* alarm reset */
      else if (param[0][0] == 'a' && param[0][1] == 'l') {
         /* go through all alarms */
         db_find_key(hDB, 0, "/Alarms/Alarms", &hKey);
         if (hKey) {
            for (i = 0;; i++) {
               db_enum_link(hDB, hKey, i, &hSubkey);

               if (!hSubkey)
                  break;

               db_get_key(hDB, hSubkey, &key);
               status = al_reset_alarm(key.name);
               if (status == AL_RESET)
                  printf("Alarm of class \"%s\" reset sucessfully\n", key.name);
            }
         }
      }

      /* mem */
      else if (param[0][0] == 'm' && param[0][1] == 'e') {
         if (rpc_is_remote())
            printf("This function works only locally\n");
         else {
#ifdef LOCAL_ROUTINES
            char* buf = NULL;
            db_show_mem(hDB, &buf, param[1][0]);
            if (buf) {
               puts(buf);
               free(buf);
            }
#else
            printf("This MIDAS only works remotely\n");
#endif // LOCAL_ROUTINES
         }
      }

      /* sor (show open records) */
      else if (param[0][0] == 's' && param[0][1] == 'o') {
         db_find_key(hDB, 0, pwd, &hKey);
         char data[50000];
         db_get_open_records(hDB, hKey, data, sizeof(data), FALSE);
         printf("%s", data);
      }

      /* scl (show clients ) */
      else if (param[0][0] == 's' && param[0][1] == 'c') {
         status = db_find_key(hDB, 0, "System/Clients", &hKey);
         if (status != DB_SUCCESS)
            cm_msg(MERROR, "command_loop",
                   "cannot find System/Clients entry in database");
         else {
            if (param[1][1] == 'w')
               printf("Name                Host                Timeout    Last called\n");
            else
               printf("Name                Host\n");

            /* search database for clients with transition mask set */
            for (i = 0, status = 0;; i++) {
               status = db_enum_key(hDB, hKey, i, &hSubkey);
               if (status == DB_NO_MORE_SUBKEYS)
                  break;

               if (status == DB_SUCCESS) {
                  size = sizeof(name);
                  db_get_value(hDB, hSubkey, "Name", name, &size, TID_STRING, TRUE);
                  printf("%s", name);
                  for (int j = 0; j < 20 - (int) strlen(name); j++)
                     printf(" ");

                  size = sizeof(str);
                  db_get_value(hDB, hSubkey, "Host", str, &size, TID_STRING, TRUE);
                  printf("%s", str);
                  for (int j = 0; j < 20 - (int) strlen(str); j++)
                     printf(" ");

                  /* display optional watchdog info */
                  if (param[1][1] == 'w') {
                     DWORD timeout, last;

                     status = cm_get_watchdog_info(hDB, name, &timeout, &last);
                     printf("%-10d %-10d", timeout, last);
                  }

                  printf("\n");
               }
            }
         }
      }

      /* start */
      else if (param[0][0] == 's' && param[0][1] == 't' && param[0][2] == 'a') {
         debug_flag =   ((param[1][0] == '-' && param[1][1] == 'v') ||
                         (param[2][0] == '-' && param[2][1] == 'v') ||
                         (param[3][0] == '-' && param[3][1] == 'v'));
         mthread_flag = ((param[1][0] == '-' && param[1][1] == 'm') ||
                         (param[2][0] == '-' && param[2][1] == 'm') ||
                         (param[3][0] == '-' && param[3][1] == 'm'));

         /* check if run is already started */
         size = sizeof(i);
         i = STATE_STOPPED;
         db_get_value(hDB, 0, "/Runinfo/State", &i, &size, TID_INT, TRUE);
         if (i == STATE_RUNNING) {
            printf("Run is already started\n");
         } else if (i == STATE_PAUSED) {
            printf("Run is paused, please use \"resume\"\n");
         } else {
            /* get present run number */
            old_run_number = 0;
            status = db_get_value(hDB, 0, "/Runinfo/Run number", &old_run_number, &size, TID_INT, TRUE);
            assert(status == SUCCESS);
            assert(old_run_number >= 0);

            /* edit run parameter if command is not "start now" */
            if ((param[1][0] == 'n' && param[1][1] == 'o' && param[1][2] == 'w') ||
                cmd_mode) {
               new_run_number = old_run_number + 1;
               line[0] = 'y';
            } else {
               db_find_key(hDB, 0, "/Experiment/Edit on start", &hKey);
               do {
                  if (hKey) {
                     for (i = 0;; i++) {
                        db_enum_link(hDB, hKey, i, &hSubkey);

                        if (!hSubkey)
                           break;

                        db_get_key(hDB, hSubkey, &key);
                        std::string str = key.name;

                        if (equal_ustring(str.c_str(), "Edit run number"))
                           continue;

                        if (str.find("Options ") == 0)
                           continue;

                        db_enum_key(hDB, hKey, i, &hSubkey);
                        db_get_key(hDB, hSubkey, &key);

                        assert(key.total_size > 0);
                        char *buf = (char*)malloc(key.total_size);

                        size = key.total_size;
                        status = db_get_data(hDB, hSubkey, buf, &size, key.type);
                        if (status != DB_SUCCESS) {
                           free(buf);
                           continue;
                        }

                        for (int j = 0; j < key.num_values; j++) {
                           db_sprintf(data_str, buf, key.item_size, j, key.type);
                           char xprompt[256];
                           strlcpy(xprompt, str.c_str(), sizeof(xprompt));
                           if (key.num_values == 1) {
                              strlcat(xprompt, " : ", sizeof(xprompt));
                           } else {
                              char tmp[256];
                              sprintf(tmp, "[%d] : ", j);
                              strlcat(xprompt, tmp, sizeof(xprompt));
                           }

                           strcpy(line, data_str);
                           in_cmd_edit = TRUE;
                           cmd_edit(xprompt, line, NULL, cmd_idle);
                           in_cmd_edit = FALSE;

                           if (line[0]) {
                              db_sscanf(line, buf, &size, j, key.type);
                              db_set_data_index(hDB, hSubkey, buf, key.item_size, j, key.type);
                           }
                        }

                        free(buf);
                     }
                  }

                  /* increment run number */
                  new_run_number = old_run_number + 1;
                  
                  if (db_find_key(hDB, 0, "/Experiment/Edit on start/Edit Run number", &hKey) ==
                      DB_SUCCESS && db_get_data(hDB, hKey, &i, &size, TID_BOOL) && i == 0) {
                     printf("Run number: %d\n", new_run_number);
                  } else {
                     
                     printf("Run number [%d]: ", new_run_number);
                     ss_gets(line, 256);
                     if (line[0] && atoi(line) > 0)
                        new_run_number = atoi(line);
                  }

                  printf("Are the above parameters correct? ([y]/n/q): ");
                  ss_gets(line, 256);

               } while (line[0] == 'n' || line[0] == 'N');
            }

            if (line[0] != 'q' && line[0] != 'Q') {
               /* start run */
               printf("Starting run #%d\n", new_run_number);

               assert(new_run_number > 0);

               status = cm_transition(TR_START, new_run_number, str,
                                      sizeof(str), mthread_flag?TR_MTHREAD|TR_SYNC:TR_SYNC, debug_flag);
               if (status != CM_SUCCESS) {
                  /* in case of error, reset run number */
                  status =
                      db_set_value(hDB, 0, "/Runinfo/Run number", &old_run_number,
                                   sizeof(old_run_number), 1, TID_INT);
                  assert(status == SUCCESS);

                  printf("Error: %s\n", str);
               }
            }
         }
      }

      /* stop */
      else if (param[0][0] == 's' && param[0][1] == 't' && param[0][2] == 'o') {
         debug_flag =   ((param[1][0] == '-' && param[1][1] == 'v') ||
                         (param[2][0] == '-' && param[2][1] == 'v') ||
                         (param[3][0] == '-' && param[3][1] == 'v'));
         mthread_flag = ((param[1][0] == '-' && param[1][1] == 'm') ||
                         (param[2][0] == '-' && param[2][1] == 'm') ||
                         (param[3][0] == '-' && param[3][1] == 'm'));

         /* check if run is stopped */
         state = STATE_STOPPED;
         size = sizeof(i);
         db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, TRUE);
         str[0] = 0;
         if (state == STATE_STOPPED && !cmd_mode) {
            printf("Run is already stopped. Stop again? (y/[n]) ");
            ss_gets(str, 256);
         }
         if (str[0] == 'y' || state != STATE_STOPPED || cmd_mode) {
            if (param[1][0] == 'n')
               status =
               cm_transition(TR_STOP | TR_DEFERRED, 0, str, sizeof(str), mthread_flag?TR_MTHREAD|TR_SYNC:TR_SYNC, debug_flag);
            else
               status = cm_transition(TR_STOP, 0, str, sizeof(str), mthread_flag?TR_MTHREAD|TR_SYNC:TR_SYNC, debug_flag);

            if (status == CM_DEFERRED_TRANSITION)
               printf("%s\n", str);
            else if (status == CM_TRANSITION_IN_PROGRESS)
               printf
                   ("Deferred stop already in progress, enter \"stop now\" to force stop\n");
            else if (status != CM_SUCCESS)
               printf("Error: %s\n", str);
         }
      }

      /* pause */
      else if (param[0][0] == 'p' && param[0][1] == 'a' && param[0][2] == 'u') {
         debug_flag = ((param[1][0] == '-' && param[1][1] == 'v') ||
                       (param[2][0] == '-' && param[2][1] == 'v'));

         /* check if run is started */
         i = STATE_STOPPED;
         size = sizeof(i);
         db_get_value(hDB, 0, "/Runinfo/State", &i, &size, TID_INT, TRUE);
         if (i != STATE_RUNNING) {
            printf("Run is not started\n");
         } else {
            if (param[1][0] == 'n')
               status = cm_transition(TR_PAUSE | TR_DEFERRED, 0, str, sizeof(str), TR_SYNC, debug_flag);
            else
               status = cm_transition(TR_PAUSE, 0, str, sizeof(str), TR_SYNC, debug_flag);

            if (status == CM_DEFERRED_TRANSITION)
               printf("%s\n", str);
            else if (status == CM_TRANSITION_IN_PROGRESS)
               printf("Deferred pause already in progress, enter \"pause now\" to force pause\n");
            else if (status != CM_SUCCESS)
               printf("Error: %s\n", str);
         }
      }

      /* resume */
      else if (param[0][0] == 'r' && param[0][1] == 'e' && param[0][2] == 's') {
         debug_flag = ((param[1][0] == '-' && param[1][1] == 'v') ||
                       (param[2][0] == '-' && param[2][1] == 'v'));

         /* check if run is paused */
         i = STATE_STOPPED;
         size = sizeof(i);
         db_get_value(hDB, 0, "/Runinfo/State", &i, &size, TID_INT, TRUE);
         if (i != STATE_PAUSED) {
            printf("Run is not paused\n");
         } else {
            status = cm_transition(TR_RESUME, 0, str, sizeof(str), TR_SYNC, debug_flag);
            if (status != CM_SUCCESS)
               printf("Error: %s\n", str);
         }
      }

      /* msg */
      else if (param[0][0] == 'm' && param[0][1] == 's') {
         // param[1]: facility, param[2]: type, param[3]: name  param[4]: message
         if (!param[4][0]) {
            printf("Error: Not enough parameters. Please use\n\n");
            printf("   msg <facility> <type> <name> <message>\n\n");
            printf("where <facility> can be \"midas\", \"chat\", ...\n");
            printf("and <type> must be \"error\", \"info\", \"debug\", \"user\", \"log\", \"talk\" or \"call\".\n");
         } else {
            int type = 0;
            if (equal_ustring(param[2], MT_ERROR_STR))
               type = MT_ERROR;
            if (equal_ustring(param[2], MT_INFO_STR))
               type = MT_INFO;
            if (equal_ustring(param[2], MT_DEBUG_STR))
               type = MT_DEBUG;
            if (equal_ustring(param[2], MT_USER_STR))
               type = MT_USER;
            if (equal_ustring(param[2], MT_LOG_STR))
               type = MT_LOG;
            if (equal_ustring(param[2], MT_TALK_STR))
               type = MT_TALK;
            if (equal_ustring(param[2], MT_CALL_STR))
               type = MT_CALL;
            if (type == 0) {
               printf("Error: inavlid type \"%s\".\n", param[2]);
               printf("<type> must be one of \"error\", \"info\", \"debug\", \"user\", \"log\", \"talk\", \"call\".\n");
            } else {
               
               cm_msg1(type, __FILE__, __LINE__, param[1], param[3], "%s", param[4]);
               last_msg_time = ss_time();
            }
         }
      }

      /* chat */
      else if (param[0][0] == 'c' && param[0][1] == 'h' && param[0][2] == 'a') {
         message[0] = 0;

         if (ss_time() - last_msg_time > 300) {
            printf("Your name> ");
            ss_gets(user_name, 80);
         }

         printf("Exit chat mode with empty line.\n");
         do {
            in_cmd_edit = TRUE;
            message[0] = 0;
            cmd_edit("> ", message, NULL, cmd_idle);
            in_cmd_edit = FALSE;

            if (message[0])
               cm_msg1(MUSER, "chat", user_name, "%s", message);

         } while (message[0]);

         last_msg_time = ss_time();
      }

      /* old */
      else if (param[0][0] == 'o' && param[0][1] == 'l') {
         i = 20;
         if (param[1][0])
            i = atoi(param[1]);

         char data[50000];
         cm_msg_retrieve(i, data, sizeof(data));
         printf("%s\n\n", data);
      }

      /* cleanup */
      else if (param[0][0] == 'c' && param[0][1] == 'l') {
         HNDLE hBuf;
         BOOL force;

         force = FALSE;
         if (param[1][0] == '-' && param[1][1] == 'f')
            force = TRUE;
         if (param[2][0] == '-' && param[2][1] == 'f')
            force = TRUE;

         bm_open_buffer(EVENT_BUFFER_NAME, DEFAULT_BUFFER_SIZE, &hBuf);

         if (param[1][0] && param[1][0] != '-')
            cm_cleanup(param[1], force);
         else
            cm_cleanup("", force);
         bm_close_buffer(hBuf);
      }

      /* shutdown */
      else if (param[0][0] == 's' && param[0][1] == 'h') {
         if (param[1][0] == 0)
            printf("Please enter client name or \"all\" to shutdown all clients.\n");
         else {
            status = cm_shutdown(param[1], TRUE);
            if (status != CM_SUCCESS) {
               if (strcmp(param[1], "all") == 0)
                  printf("No clients found\n");
               else
                  printf("Client \"%s\" not active\n", param[1]);
            }
         }
      }

      /* ver */
      else if (param[0][0] == 'v' && param[0][1] == 'e') {
         printf("MIDAS version:      %s\n", cm_get_version());
         printf("GIT revision:       %s\n", cm_get_revision());
         printf("ODB version:        %d\n", DATABASE_VERSION);
      }

      /* exec */
      else if (param[0][0] == 'e' && param[0][1] == 'x' && param[0][2] == 'e') {
         compose_name(pwd, param[1], str);
         char data[50000];
         status = db_find_key(hDB, 0, str, &hKey);
         if (status == DB_SUCCESS) {
            db_get_key(hDB, hKey, &key);
            if (key.type != TID_STRING) {
               printf("Key contains no command\n");
               continue;
            }
            size = sizeof(str);
            db_get_data(hDB, hKey, str, &size, key.type);

            cm_execute(str, data, sizeof(data));
         } else {
            cm_execute(param[1], data, sizeof(data));
         }
         puts(data);
      }

      /* wait */
      else if (param[0][0] == 'w' && param[0][1] == 'a' && param[0][2] == 'i') {
         if (param[1][0] == 0) {
            printf("Please specify key\n");
         } else {
            compose_name(pwd, param[1], str);
            
            if (equal_ustring(str, "/")) {
               status = DB_SUCCESS;
               status = db_find_link(hDB, 0, "/", &hKey);
            } else
               status = db_find_link(hDB, 0, str, &hKey);
            
            if (status == DB_SUCCESS) {
               db_get_key(hDB, hKey, &key);
               printf("Waiting for key \"%s\" to be modified, abort with any key\n", key.name);
               db_get_record_size(hDB, hKey, 0, &size);
               char* buf = (char*)malloc(size);
               key_modified = FALSE;
               db_open_record(hDB, hKey, buf, size, MODE_READ, key_update, NULL);
               
               do {
                  status = cm_yield(1000);
                  if (status == SS_ABORT || status == RPC_SHUTDOWN)
                     break;
               } while (!key_modified && !ss_kbhit());
               
               while (ss_kbhit())
                  ss_getchar(0);
               
               db_close_record(hDB, hKey);
               if (buf) {
                  free(buf);
                  buf = NULL;
               }

               if (status == SS_ABORT || status == RPC_SHUTDOWN)
                  break;
               
               if (i == '!')
                  printf("Wait aborted.\n");
               else
                  printf("Key has been modified.\n");
            } else
               printf("key \"%s\" not found\n", str);
         }
      }
      
      /* watch */
      else if (param[0][0] == 'w' && param[0][1] == 'a' && param[0][2] == 't') {
         if (param[1][0] == 0) {
            printf("Please specify key\n");
         } else {
            compose_name(pwd, param[1], str);
            
            status = db_find_link(hDB, 0, str, &hKey);
            if (status == DB_SUCCESS) {
               db_get_key(hDB, hKey, &key);
               if (key.type != TID_KEY)
                  printf("Watch key \"%s\" to be modified, abort with any key\n", str);
               else
                  printf("Watch ODB tree \"%s\" to be modified, abort with any key\n", str);
               db_watch(hDB, hKey, watch_callback, NULL);
               
               do {
                  status = cm_yield(1000);
                  if (status == SS_ABORT || status == RPC_SHUTDOWN)
                     break;
               } while (!ss_kbhit());

               while (ss_kbhit())
                  ss_getchar(0);
               
               db_unwatch(hDB, hKey);

               if (status == SS_ABORT || status == RPC_SHUTDOWN)
                  break;
            } else
               printf("key \"%s\" not found\n", str);
         }
      }

      /* test 1  */
      else if (param[0][0] == 't' && param[0][1] == '1') {
         DWORD start_time;
         INT i, size, rn;
         HNDLE hKey;

         start_time = ss_millitime();
         status = db_find_key(hDB, 0, "/runinfo/run number", &hKey);
         assert(status == SUCCESS);
         size = sizeof(rn);

         i = 0;
         do {
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            i += 10;
         } while (ss_millitime() - start_time < 5000);

         printf("%d accesses per second\n", i / 5);
      }

      /* test 2 */
      else if (param[0][0] == 't' && param[0][1] == '2') {
         do {
            do {
               i = ss_getchar(0);
               printf("%d\n", i);
            } while (i > 0 && i != 4);

            ss_sleep(1000);
         } while (i != 4);

         ss_getchar(1);
      }

      /* test 3 */
      else if (param[0][0] == 't' && param[0][1] == '3') {
#ifdef USE_ADDRESS_SANITIZER
         // test address sanitizer
         int *a = (int *)malloc(sizeof(int) * 10);
         i = a[11];
         free(a);
#else
         printf("test address sanitizer is disabled!\n");
#endif
      }

      /* exit/quit */
      else if ((param[0][0] == 'e' && param[0][1] == 'x' && param[0][2] == 'i') ||
               (param[0][0] == 'q'))
         break;

      else if (param[0][0] == 0);

      else
         printf("Unknown command %s %s %s\n", param[0], param[1], param[2]);

      /* exit after single command */
      if (cmd_mode && cmd[0] != '@')
         break;

      /* check if client connections are broken */
      status = cm_yield(0);
      if (status == SS_ABORT || status == RPC_SHUTDOWN)
         break;

   } while (TRUE);

   /* check if client connections are broken */
   status = cm_yield(0);

   return 1; /* indicate success */
}

/*------------------------------------------------------------------*/

void ctrlc_odbedit(INT i)
{
   /* reset terminal */
   ss_getchar(1);

   /* no shutdown message */
   cm_set_msg_print(MT_ERROR, 0, NULL);

   cm_disconnect_experiment();

   exit(EXIT_SUCCESS);
}

/*------------------------------------------------------------------*/

#ifdef OS_VXWORKS
int odbedit(char *ahost_name, char *aexp_name)
#else
int main(int argc, char *argv[])
#endif
{
   INT status, i, odb_size, size;
   char host_name[HOST_NAME_LENGTH], exp_name[NAME_LENGTH];
   char cmd[2000], dir[256];
   BOOL debug;
   BOOL quiet;
   BOOL corrupted;
   BOOL reload_from_file = FALSE;
   HNDLE hDB;

   cmd[0] = dir[0] = 0;
   odb_size = DEFAULT_ODB_SIZE;
   debug = corrupted = cmd_mode = quiet = FALSE;

#ifdef OS_VXWORKS
   strcpy(host_name, ahost_name);
   strcpy(exp_name, aexp_name);
#else

   /* get default from environment */
   cm_get_environment(host_name, sizeof(host_name), exp_name, sizeof(exp_name));

   /* check for MIDASSYS */
   if (!getenv("MIDASSYS")) {
      puts("Please define environment variable 'MIDASSYS'");
      puts("pointing to the midas source installation directory.");
   }

   /* parse command line parameters */
   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'g')
         debug = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 'q')
         quiet = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 'R')
         reload_from_file = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 'C')
         corrupted = TRUE;
      else if (argv[i][0] == '-') {
         if (i + 1 >= argc || argv[i + 1][0] == '-')
            goto usage;
         if (argv[i][1] == 'e')
            strcpy(exp_name, argv[++i]);
         else if (argv[i][1] == 'h')
            strcpy(host_name, argv[++i]);
         else if (argv[i][1] == 'c') {
            if (strlen(argv[i+1]) >= sizeof(cmd)) {
               printf("error: command line too long (>%d bytes)\n", (int)sizeof(cmd));
               return 0;
            }
            strlcpy(cmd, argv[++i], sizeof(cmd));
            cmd_mode = TRUE;
         } else if (argv[i][1] == 'd')
            strlcpy(dir, argv[++i], sizeof(dir));
         else if (argv[i][1] == 's')
            odb_size = atoi(argv[++i]);
         else {
          usage:
            printf("usage: odbedit [-h Hostname] [-e Experiment] [-d ODB Subtree]\n");
            printf("               [-q] [-c Command] [-c @CommandFile] [-s size]\n");
            printf("               [-g (debug)] [-C (connect to corrupted ODB)]\n");
            printf("               [-R (reload ODB from .ODB.SHM)]\n\n");
            printf("For a list of valid commands start odbedit interactively\n");
            printf("and type \"help\".\n");
            return 0;
         }
      } else
         strlcpy(host_name, argv[i], sizeof(host_name));
   }
#endif

   if (reload_from_file) {
#ifdef LOCAL_ROUTINES
      status = cm_set_experiment_local(exp_name);
      if (status != CM_SUCCESS) {
         printf("Cannot setup experiment name and path.\n");
         return 1;
      }
      status = ss_shm_delete("ODB");
      printf("MIDAS ODB shared memory was deleted, ss_shm_delete(ODB) status %d\n", status);
      printf("Please run odbedit again without \'-R\' and ODB will be reloaded from .ODB.SHM\n");
      return 1;
#else
      printf("This odbedit only works remotely, -R is not supported\n");
      return 1;
#endif
   }

   /* no startup message if called with command */
   if (cmd[0])
      cm_set_msg_print(MT_ERROR, 0, NULL);

   status = cm_connect_experiment1(host_name, exp_name, "ODBEdit", NULL,
                                   odb_size, DEFAULT_WATCHDOG_TIMEOUT);

   if (status == CM_WRONG_PASSWORD)
      return 1;

   cm_msg_flush_buffer();

   if ((status == DB_INVALID_HANDLE) && corrupted) {
      std::string s = cm_get_error(status);
      puts(s.c_str());
      printf("ODB is corrupted, connecting anyway...\n");
   } else if (status != CM_SUCCESS) {
      std::string s = cm_get_error(status);
      puts(s.c_str());
      return 1;
   }

   /* place a request for system messages */
   cm_msg_register(process_message);

   /* route local messages through print_message */
   cm_set_msg_print(MT_ALL, MT_ALL, print_message);

   /* turn off watchdog if in debug mode */
   if (debug)
      cm_set_watchdog_params(FALSE, 0);

   /* get experiment name */
   if (!exp_name[0]) {
      cm_get_experiment_database(&hDB, NULL);
      size = NAME_LENGTH;
      db_get_value(hDB, 0, "/Experiment/Name", exp_name, &size, TID_STRING, TRUE);
   }

   /* register Ctrl-C handler */
   ss_ctrlc_handler(ctrlc_odbedit);

   /* for use with the mac os profiler */
   //cmd_mode = TRUE;
   //status = command_loop(host_name, exp_name, "save xxx3.json", dir);

   /* log all commands passed via -c on command line */
   if (cmd_mode && !quiet)
      cm_msg(MINFO, "odbedit", "Execute command from command line: \"%s\"", cmd);

   /* command loop */
   status = command_loop(host_name, exp_name, cmd, dir);

   /* no shutdown message if called with command */
   if (cmd_mode)
      cm_set_msg_print(MT_ERROR, 0, NULL);

   cm_disconnect_experiment();

   if (status != 1)
      return EXIT_FAILURE;

   return EXIT_SUCCESS;
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
