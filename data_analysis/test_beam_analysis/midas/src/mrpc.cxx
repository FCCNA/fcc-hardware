/********************************************************************\

  Name:         MRPC.C
  Created by:   Stefan Ritt

  Contents:     List of MIDAS RPC functions with parameters

  $Id$

\********************************************************************/

/**dox***************************************************************/
/** @file mrpc.c
The Midas RPC file
*/

/** @defgroup mrpcstructc Midas RPC_LIST 
 */

/**dox***************************************************************/
/** @addtogroup mrpcincludecode
 *  
 *  @{  */

/**dox***************************************************************/
/** @addtogroup mrpcstructc
 *  
 *  @{  */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "midas.h"
#include "msystem.h"

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/**dox***************************************************************/
/**
rpc_list_library contains all MIDAS library functions and gets
registerd whenever a connection to the MIDAS server is established 
*/
static RPC_LIST rpc_list_library[] = {

   /* common functions */
   {RPC_CM_SET_CLIENT_INFO, "cm_set_client_info",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_OUT},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_INT32, RPC_IN},
     {0}}},

   {RPC_CM_CHECK_CLIENT, "cm_check_client",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {0}}},

   {RPC_CM_SET_WATCHDOG_PARAMS, "cm_set_watchdog_params",
    {{TID_BOOL, RPC_IN},
     {TID_INT32, RPC_IN},
     {0}}},

   {RPC_CM_CLEANUP, "cm_cleanup",
    {{TID_STRING, RPC_IN},
     {TID_BOOL, RPC_IN},
     {0}}},

   {RPC_CM_GET_WATCHDOG_INFO, "cm_get_watchdog_info",
    {{TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_UINT32, RPC_OUT},
     {TID_UINT32, RPC_OUT},
     {0}}},

   {RPC_CM_MSG, "cm_msg",
    {{TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {0}}},

   {RPC_CM_MSG_LOG, "cm_msg_log",
    {{TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {0}}},

   {RPC_CM_EXECUTE, "cm_execute",
    {{TID_STRING, RPC_IN},
     {TID_STRING, RPC_OUT},
     {TID_INT32, RPC_IN},
     {0}}},

   {RPC_CM_EXIST, "cm_exist",
    {{TID_STRING, RPC_IN},
     {TID_BOOL, RPC_IN},
     {0}}},

   {RPC_CM_SYNCHRONIZE, "cm_synchronize",
    {{TID_UINT32, RPC_OUT},
     {0}}},

   {RPC_CM_ASCTIME, "cm_asctime",
    {{TID_STRING, RPC_OUT},
     {TID_INT32, RPC_IN},
     {0}}},

   {RPC_CM_TIME, "cm_time",
    {{TID_UINT32, RPC_OUT},
     {0}}},

   {RPC_CM_MSG_RETRIEVE, "cm_msg_retrieve",
    {{TID_INT32, RPC_IN},
     {TID_STRING, RPC_OUT},
     {TID_INT32, RPC_IN},
     {0}}},

   /* buffer manager functions */

   {RPC_BM_OPEN_BUFFER, "bm_open_buffer",
    {{TID_STRING, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_OUT},
     {0}}},

   {RPC_BM_CLOSE_BUFFER, "bm_close_buffer",
    {{TID_INT32, RPC_IN},
     {0}}},

   {RPC_BM_CLOSE_ALL_BUFFERS, "bm_close_all_buffers",
    {{0}}},

   {RPC_BM_GET_BUFFER_INFO, "bm_get_buffer_info",
    {{TID_INT32, RPC_IN},
     {TID_STRUCT, RPC_OUT, sizeof(BUFFER_HEADER)},
     {0}}},

   {RPC_BM_GET_BUFFER_LEVEL, "bm_get_buffer_level",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_OUT},
     {0}}},

   {RPC_BM_INIT_BUFFER_COUNTERS, "bm_init_buffer_counters",
    {{TID_INT32, RPC_IN},
     {0}}},

   {RPC_BM_SET_CACHE_SIZE, "bm_set_cache_size",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {0}}},
   
   {RPC_BM_ADD_EVENT_REQUEST, "bm_add_event_request",
    {{TID_INT32, RPC_IN},
     {TID_INT16, RPC_IN},
     {TID_INT16, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {0}}},
   
   {RPC_BM_REMOVE_EVENT_REQUEST, "bm_remove_event_request",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {0}}},

   {RPC_BM_SEND_EVENT, "bm_send_event",
    {{TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {0}}},


   {RPC_BM_FLUSH_CACHE, "bm_flush_cache",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {0}}},


   {RPC_BM_RECEIVE_EVENT, "bm_receive_event",
    {{TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_OUT | RPC_VARARRAY},
     {TID_INT32, RPC_IN | RPC_OUT},
     {TID_INT32, RPC_IN},
     {0}}},


   {RPC_BM_SKIP_EVENT, "bm_skip_event",
    {{TID_INT32, RPC_IN},
     {0}}},

   {RPC_BM_MARK_READ_WAITING, "bm_mark_read_waiting",
    {{TID_BOOL, RPC_IN},
     {0}}},

   {RPC_BM_EMPTY_BUFFERS, "bm_empty_buffers",
    {{0}}},

   /* online database functions */

   {RPC_DB_OPEN_DATABASE, "db_open_database",
    {{TID_STRING, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_OUT},
     {TID_STRING, RPC_IN},
     {0}}},

   {RPC_DB_CLOSE_DATABASE, "db_close_database",
    {{TID_INT32, RPC_IN},
     {0}}},

   {RPC_DB_FLUSH_DATABASE, "db_flush_database",
    {{TID_INT32, RPC_IN},
     {0}}},

   {RPC_DB_CLOSE_ALL_DATABASES, "db_close_all_databases",
    {{0}
     }
    }
   ,

   {RPC_DB_CREATE_KEY, "db_create_key",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_UINT32, RPC_IN},
     {0}}},

   {RPC_DB_CREATE_LINK, "db_create_link",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {0}}},

   {RPC_DB_SET_VALUE, "db_set_value",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_UINT32, RPC_IN},
     {0}}},

   {RPC_DB_GET_VALUE, "db_get_value",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_OUT | RPC_VARARRAY},
     {TID_INT32, RPC_IN | RPC_OUT},
     {TID_UINT32, RPC_IN},
     {TID_BOOL, RPC_IN},
     {0}}},

   {RPC_DB_FIND_KEY, "db_find_key",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_INT32, RPC_OUT},
     {0}}},

   {RPC_DB_FIND_LINK, "db_fink_link",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_INT32, RPC_OUT},
     {0}}},

   {RPC_DB_GET_PARENT, "db_get_parent",
     {{TID_INT32, RPC_IN},
      {TID_INT32, RPC_IN},
      {TID_INT32, RPC_OUT},
      {0}}},

   {RPC_DB_COPY_XML, "db_copy_xml",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_OUT | RPC_VARARRAY},
     {TID_INT32, RPC_IN | RPC_OUT},
     {TID_BOOL, RPC_IN},
     {0}}},

   {RPC_DB_GET_PATH, "db_get_path",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_OUT},
     {TID_INT32, RPC_IN},
     {0}}},

   {RPC_DB_DELETE_KEY, "db_delete_key",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_BOOL, RPC_IN},
     {0}}},

   {RPC_DB_ENUM_KEY, "db_enum_key",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_OUT},
     {0}}},

   {RPC_DB_ENUM_LINK, "db_enum_link",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_OUT},
     {0}}},

   {RPC_DB_GET_KEY, "db_get_key",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRUCT, RPC_OUT, sizeof(KEY)},
     {0}}},

   {RPC_DB_GET_LINK, "db_get_link",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRUCT, RPC_OUT, sizeof(KEY)},
     {0}}},

   {RPC_DB_GET_KEY_INFO, "db_get_key_info",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_OUT},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_OUT},
     {TID_INT32, RPC_OUT},
     {TID_INT32, RPC_OUT},
     {0}}},

   {RPC_DB_GET_KEY_TIME, "db_get_key_time",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_UINT32, RPC_OUT},
     {0}}},

   {RPC_DB_RENAME_KEY, "db_rename_key",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {0}}},

   {RPC_DB_REORDER_KEY, "db_reorder_key",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {0}}},

   {RPC_DB_GET_DATA, "db_get_data",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_OUT | RPC_VARARRAY},
     {TID_INT32, RPC_IN | RPC_OUT},
     {TID_UINT32, RPC_IN},
     {0}}},

   {RPC_DB_GET_LINK_DATA, "db_get_data",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_OUT | RPC_VARARRAY},
     {TID_INT32, RPC_IN | RPC_OUT},
     {TID_UINT32, RPC_IN},
     {0}}},

   {RPC_DB_GET_DATA1, "db_get_data1",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_OUT | RPC_VARARRAY},
     {TID_INT32, RPC_IN | RPC_OUT},
     {TID_UINT32, RPC_IN},
     {TID_INT32, RPC_OUT},
     {0}}},

   {RPC_DB_GET_DATA_INDEX, "db_get_data_index",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_OUT | RPC_VARARRAY},
     {TID_INT32, RPC_IN | RPC_OUT},
     {TID_INT32, RPC_IN},
     {TID_UINT32, RPC_IN},
     {0}}},

   {RPC_DB_SET_DATA, "db_set_data",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_UINT32, RPC_IN},
     {0}}},

   {RPC_DB_SET_DATA1, "db_set_data1",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_UINT32, RPC_IN},
     {0}}},

   {RPC_DB_NOTIFY_CLIENTS_ARRAY, "db_notify_clients_array",
    {{TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT32, RPC_IN},
     {0}}},

   {RPC_DB_SET_LINK_DATA, "db_set_link_data",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_UINT32, RPC_IN},
     {0}}},

   {RPC_DB_SET_DATA_INDEX, "db_set_data_index",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_UINT32, RPC_IN},
     {0}}},

   {RPC_DB_SET_LINK_DATA_INDEX, "db_set_link_data_index",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_UINT32, RPC_IN},
     {0}}},

   {RPC_DB_SET_DATA_INDEX1, "db_set_data_index1",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_UINT32, RPC_IN},
     {TID_BOOL, RPC_IN},
     {0}}},

   {RPC_DB_SET_NUM_VALUES, "db_set_num_values",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {0}}},

   {RPC_DB_SET_MODE, "db_set_mode",
     {{TID_INT32, RPC_IN},
      {TID_INT32, RPC_IN},
      {TID_UINT16, RPC_IN},
      {TID_BOOL, RPC_IN},
      {0}}},

   {RPC_DB_CREATE_RECORD, "db_create_record",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {0}}},

   {RPC_DB_CHECK_RECORD, "db_check_record",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_BOOL, RPC_IN},
     {0}}},

   {RPC_DB_GET_RECORD, "db_get_record",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_OUT | RPC_VARARRAY},
     {TID_INT32, RPC_IN | RPC_OUT},
     {TID_INT32, RPC_IN},
     {0}}},

   {RPC_DB_GET_RECORD_SIZE, "db_get_record_size",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_OUT},
     {0}}},

   {RPC_DB_SET_RECORD, "db_set_record",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {0}}},

   {RPC_DB_ADD_OPEN_RECORD, "db_add_open_record",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_UINT16, RPC_IN},
     {0}}},

   {RPC_DB_REMOVE_OPEN_RECORD, "db_remove_open_record",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_BOOL, RPC_IN},
     {0}}},

   {RPC_DB_LOAD, "db_load",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_BOOL, RPC_IN},
     {0}}},

   {RPC_DB_SAVE, "db_save",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_BOOL, RPC_IN},
     {0}}},

   {RPC_DB_SET_CLIENT_NAME, "db_set_client_name",
    {{TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {0}}},

   {RPC_DB_GET_OPEN_RECORDS, "db_get_open_records",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_OUT},
     {TID_INT32, RPC_IN},
     {TID_BOOL, RPC_IN},
     {0}}},

   /* elog funcions */

   {RPC_EL_SUBMIT, "el_submit",
    {{TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_IN, RPC_OUT},
     {TID_INT32, RPC_IN},
     {0}}},

   /* alarm  funcions */

   {RPC_AL_CHECK, "al_check",
    {{0}}},

   {RPC_AL_TRIGGER_ALARM, "al_trigger_alarm",
    {{TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_IN},
     {TID_INT32, RPC_IN},
     {0}}},

   /* run control */

   {RPC_RC_TRANSITION, "rc_transition",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_STRING, RPC_OUT},
     {TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {0}}},

   /* analyzer control */

   {RPC_ANA_CLEAR_HISTOS, "ana_clear_histos",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_IN},
     {0}}},

   /* logger control */

   {RPC_LOG_REWIND, "log_rewind",
    {{TID_INT32, RPC_IN},
     {0}}},

   /* test functions */

   {RPC_TEST, "test",
    {{TID_UINT8, RPC_IN},
     {TID_UINT16, RPC_IN},
     {TID_INT32, RPC_IN},
     {TID_FLOAT, RPC_IN},
     {TID_DOUBLE, RPC_IN},
     {TID_UINT8, RPC_OUT},
     {TID_UINT16, RPC_OUT},
     {TID_INT32, RPC_OUT},
     {TID_FLOAT, RPC_OUT},
     {TID_DOUBLE, RPC_OUT},
     {0}}},

   {RPC_TEST2, "test2",
    {{TID_INT32, RPC_IN},
     {TID_INT32, RPC_OUT},
     {TID_INT32, RPC_IN | RPC_OUT},
     {TID_STRING, RPC_IN},
     {TID_STRING, RPC_OUT},
     {TID_INT32, RPC_IN}, // string size
     {TID_STRING, RPC_OUT},
     {TID_INT32, RPC_IN}, // string size
     {TID_STRING,RPC_IN | RPC_OUT},
     {TID_INT32, RPC_IN}, // string size
     {TID_STRUCT, RPC_IN, sizeof(KEY)},
     {TID_STRUCT, RPC_OUT, sizeof(KEY)},
     {TID_STRUCT, RPC_IN | RPC_OUT, sizeof(KEY)},
     {TID_UINT32, RPC_IN | RPC_OUT | RPC_VARARRAY},
     {TID_INT32, RPC_IN | RPC_OUT}, // RPC_VARARRAY size
     {TID_ARRAY, RPC_IN | RPC_VARARRAY},
     {TID_INT32, RPC_IN}, // RPC_VARARRAY size
     {TID_ARRAY, RPC_OUT | RPC_VARARRAY},
     {TID_INT32, RPC_IN | RPC_OUT}, // RPC_VARARRAY size
     {0}}},

   /* CAMAC server */

   {RPC_CNAF16, "cnaf16",
    {{TID_UINT32, RPC_IN},                          /* command */
     {TID_UINT32, RPC_IN},                          /* branch */
     {TID_UINT32, RPC_IN},                          /* crate */
     {TID_UINT32, RPC_IN},                          /* station */
     {TID_UINT32, RPC_IN},                          /* subaddress */
     {TID_UINT32, RPC_IN},                          /* function */
     {TID_UINT16, RPC_IN | RPC_OUT | RPC_VARARRAY},                          /* data */
     {TID_UINT32, RPC_IN | RPC_OUT},                          /* array size */
     {TID_UINT32, RPC_OUT},                          /* x */
     {TID_UINT32, RPC_OUT},                          /* q */
     {0}}},

   {RPC_CNAF24, "cnaf24",
    {{TID_UINT32, RPC_IN},                                    /**< command */
     {TID_UINT32, RPC_IN},                                    /**< branch */
     {TID_UINT32, RPC_IN},                                    /**< crate */
     {TID_UINT32, RPC_IN},                                    /**< station */
     {TID_UINT32, RPC_IN},                                    /**< subaddress */
     {TID_UINT32, RPC_IN},                                    /**< function */
     {TID_UINT32, RPC_IN | RPC_OUT | RPC_VARARRAY},                                                  /**< data */
     {TID_UINT32, RPC_IN | RPC_OUT},                                    /**< array size */
     {TID_UINT32, RPC_OUT},                                    /**< x */
     {TID_UINT32, RPC_OUT},                                    /**< q */
     {0}}},

   /* manual triggered equipment */

   {RPC_MANUAL_TRIG, "manual_trig",
    {{TID_UINT16, RPC_IN},                          /* event id */
     {0}}},

   /* json rpc */

   {RPC_JRPC, "json_rpc",
    {{TID_STRING, RPC_IN}, // command
     {TID_STRING, RPC_IN}, // arguments (JSON-encoded)
     {TID_STRING, RPC_OUT}, // return string (JSON-encoded)
     {TID_INT32,    RPC_IN},  // maximum length of the return string
     {0}}},

   /* binary rpc */

   {RPC_BRPC, "binary_rpc",
    {{TID_STRING, RPC_IN}, // command
     {TID_STRING, RPC_IN}, // arguments (JSON-encoded)
     {TID_ARRAY,  RPC_OUT | RPC_VARARRAY}, // return binary data
     {TID_INT32,  RPC_IN | RPC_OUT},  // maximum length of the return string
     {0}}},

   {0}

};

/**dox***************************************************************/
/** 
rpc_list_system contains MIDAS system functions and gets
registerd whenever a RPC server is registered
*/
static RPC_LIST rpc_list_system[] = {

   /* system  functions */
   {RPC_ID_WATCHDOG, "id_watchdog",
    {{0}}},

   {RPC_ID_SHUTDOWN, "id_shutdown",
    {{0}}},

   {RPC_ID_EXIT, "id_exit",
    {{0}}},

   {0}

};

RPC_LIST *rpc_get_internal_list(INT flag)
{
   if (flag)
      return rpc_list_library;
   else
      return rpc_list_system;
}

/*------------------------------------------------------------------*/

/**dox***************************************************************/
/** @} */ /* end of mrpcstructc */

/**dox***************************************************************/
/** @} */ /* end of mrpcincludecode */
