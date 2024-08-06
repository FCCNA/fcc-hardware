#include "midas_c_compat.h"
#include "midas.h"
#include "mrpc.h"
#include "msystem.h"
#include <vector>
#include <string>
#include "string.h"
#include "stdlib.h"
#include "stdarg.h"
#include "history.h"

/*
We define a simple free function to ensure that python clients can
free any memory that was allocated by midas. We define it as part
of this library (rather than importing libc directly in python) to
ensure that the same version of libc is used for the alloc and free.
*/
void c_free(void* mem) {
   free(mem);
}

void c_free_list(void** mem_list, int arr_len) {
   for (int i = 0; i < arr_len; i++) {
      free(mem_list[i]);
   }

   free(mem_list);
}

/*
Copies the content for src to dest (at most dest_size bytes).
dest should already have been allocated to the correct size.
If the destination is not large enough to hold the entire src
string, we return DB_TRUNCATED; otherwise we return SUCCESS.

In general it's preferable to accept a char** from python rather than
a buffer of a fixed size. Although python must then remember to free
the memory we allocated.
 */
INT copy_string_to_c(std::string src, char* dest, DWORD dest_size) {
   strncpy(dest, src.c_str(), dest_size);

   if (src.size() > dest_size) {
      return DB_TRUNCATED;
   }

   return SUCCESS;
}

/*
Copies the content of vec into an array of type 'T' at dest. Will malloc the
memory needed, so you must later call c_free() on dest. Fills dest_len with
the size of the vector.
 */
template <class T> INT copy_vector_to_c(std::vector<T> vec, void** dest, int& dest_len) {
   dest_len = vec.size();
   *dest = malloc(sizeof(T) * dest_len);
   std::copy(vec.begin(), vec.end(), (T*)*dest);
   return SUCCESS;
}

/*
Copies the content of vec into an array of char* at dest. Will malloc the
memory needed for each string (and for the array itself), so you must later call
c_free_list() on dest. Fills dest_len with the size of the vector.
 */
INT copy_vector_string_to_c(std::vector<std::string> vec, char*** dest, int& dest_len) {
   dest_len = vec.size();
   *dest = (char**) malloc(sizeof(char*) * dest_len);

   for (int i = 0; i < dest_len; i++) {
      (*dest)[i] = strdup(vec[i].c_str());
   }

   return SUCCESS;
}

/*
Example of how one could wrap a midas function that returns/fills a std::string.
In this version we accept a buffer of a specified size from the user.

The python code would be:
```
buffer = ctypes.create_string_buffer(64)
lib.c_example_string_c_bufsize(buffer, 64)
py_str = buffer.value.decode("utf-8")
```
 */
INT c_example_string_c_bufsize(char* buffer, DWORD buffer_size) {
   std::string retval("My string that would come from a C++ function");
   return copy_string_to_c(retval, buffer, buffer_size);
}

/*
Example of how one could wrap a midas function that returns/fills a std::string.
In this version we allocate memory for the C char array. The caller must later
free this memory themselves.

The python code would be (note the final free!):
```
buffer = ctypes.c_char_p()
lib.c_example_string_c_alloc(ctypes.byref(buffer))
py_str = buffer.value.decode("utf-8")
lib.c_free(buffer)
```
 */
INT c_example_string_c_alloc(char** dest) {
   std::string retval("My string that would come from a C++ function");
   *dest = strdup(retval.c_str());
   return SUCCESS;
}

/*
Example of how one could wrap a midas function that returns/fills a std::vector.
In this version we allocate memory for the C array. The caller must later
free this memory themselves.

The python code would be (note the final free!):
```
import ctypes
import midas.client

client = midas.client.MidasClient("pytest")
lib = client.lib

arr = ctypes.c_void_p()
arr_len = ctypes.c_int()
lib.c_example_vector(ctypes.byref(arr), ctypes.byref(arr_len))
casted = ctypes.cast(arr, ctypes.POINTER(ctypes.c_float))
py_list = casted[:arr_len.value]
lib.c_free(arr)
```
 */
INT c_example_vector(void** dest, int& dest_len) {
   std::vector<float> retvec;
   for (int i = 0; i < 10; i++) {
      retvec.push_back(i/3.);
   }

   return copy_vector_to_c(retvec, dest, dest_len);
}

/*
Example of how one could wrap a midas function that returns/fills a std::vector.
In this version we allocate memory for the C array. The caller must later
free this memory themselves.

The python code would be (note the final free!):
```
import ctypes
import midas.client
client = midas.client.MidasClient("pytest")
lib = client.lib

arr = ctypes.POINTER(ctypes.c_char_p)()
arr_len = ctypes.c_int()
lib.c_example_string_vector(ctypes.byref(arr), ctypes.byref(arr_len))
casted = ctypes.cast(arr, ctypes.POINTER(ctypes.c_char_p))
py_list = [casted[i].decode("utf-8") for i in range(arr_len.value)]
lib.c_free_list(arr, arr_len)
```
 */
INT c_example_string_vector(char*** dest, int& dest_len) {
   std::vector<std::string> retvec;
   retvec.push_back("Hello");
   retvec.push_back("world!");

   return copy_vector_string_to_c(retvec, dest, dest_len);
}

INT c_al_trigger_alarm(const char *alarm_name, const char *alarm_message, const char *default_class, const char *cond_str, INT type) {
   return al_trigger_alarm(alarm_name, alarm_message, default_class, cond_str, type);
}

INT c_al_reset_alarm(const char *alarm_name) {
   return al_reset_alarm(alarm_name);
}

INT c_al_define_odb_alarm(const char *name, const char *condition, const char *aclass, const char *message) {
   return al_define_odb_alarm(name, condition, aclass, message);
}

INT c_bm_flush_cache(INT buffer_handle, INT async_flag) {
   return bm_flush_cache(buffer_handle, async_flag);
}

INT c_bm_open_buffer(const char *buffer_name, INT buffer_size, INT * buffer_handle) {
   return bm_open_buffer(buffer_name, buffer_size, buffer_handle);
}

INT c_bm_receive_event(INT buffer_handle, void *destination, INT * buf_size, INT async_flag) {
   return bm_receive_event(buffer_handle, destination, buf_size, async_flag);
}

INT c_bm_remove_event_request(INT buffer_handle, INT request_id) {
   return bm_remove_event_request(buffer_handle, request_id);
}

INT c_bm_request_event(INT buffer_handle, short int event_id, short int trigger_mask, INT sampling_type, INT * request_id) {
   // Final argument is function pointer that python lib doesn't need.
   return bm_request_event(buffer_handle, event_id, trigger_mask, sampling_type, request_id, 0);
}

INT c_cm_check_deferred_transition(void) {
   return cm_check_deferred_transition();
}

INT c_cm_connect_client(const char *client_name, HNDLE * hConn) {
   return cm_connect_client(client_name, hConn);
}

INT c_cm_connect_experiment(const char *host_name, const char *exp_name, const char *client_name, void (*func) (char *)) {
   return cm_connect_experiment(host_name, exp_name, client_name, func);
}

INT c_cm_deregister_transition(INT transition) {
   return cm_deregister_transition(transition);
}

INT c_cm_disconnect_client(HNDLE hConn, BOOL bShutdown) {
   return cm_disconnect_client(hConn, bShutdown);
}

INT c_cm_disconnect_experiment() {
   return cm_disconnect_experiment();
}

INT c_cm_get_environment(char *host_name, int host_name_size, char *exp_name, int exp_name_size) {
   return cm_get_environment(host_name, host_name_size, exp_name, exp_name_size);
}

INT c_cm_get_experiment_database(HNDLE * hDB, HNDLE * hKeyClient) {
   return cm_get_experiment_database(hDB, hKeyClient);
}

const char* c_cm_get_revision(void) {
   // If this changes to returning a string, do:
   // return strdup(cm_get_revision().c_str());
   return cm_get_revision();
}

const char* c_cm_get_version(void) {
   // If this changes to returning a string, do:
   // return strdup(cm_get_version().c_str());
   return cm_get_version();
}

INT c_cm_msg(INT message_type, const char *filename, INT line, const char *facility, const char *routine, const char *format, ...) {
   va_list argptr;
   char message[1000];
   va_start(argptr, format);
   vsnprintf(message, 1000, (char *) format, argptr);
   va_end(argptr);
   return cm_msg1(message_type, filename, line, facility, routine, "%s", message);
}

/*
Remember to call c_free_list on the dest afterwards. E.g.:
```
import ctypes
import midas.client
lib = midas.client.MidasClient("pytest").lib

arr = ctypes.POINTER(ctypes.c_char_p)()
arr_len = ctypes.c_int()
lib.c_cm_msg_facilities(ctypes.byref(arr), ctypes.byref(arr_len))
casted = ctypes.cast(arr, ctypes.POINTER(ctypes.c_char_p))
py_list = [casted[i].decode("utf-8") for i in range(arr_len.value)]
lib.c_free_list(arr, arr_len)
```
*/
INT c_cm_msg_facilities(char*** dest, int& dest_len) {
   std::vector<std::string> retvec;
   INT retcode = cm_msg_facilities(&retvec);
   if (retcode == SUCCESS) {
      return copy_vector_string_to_c(retvec, dest, dest_len);
   } else {
      return retcode;
   }
}

INT c_cm_msg_register(EVENT_HANDLER *func) {
   return cm_msg_register(func);
}

INT c_cm_msg_retrieve2(const char *facility, uint64_t before, INT min_messages, char **messages, int *num_messages_read) {
   // Python ctypes doesn't know the size of time_t, so just accept a uint64_t and convert here.
   time_t t = before;
   INT retval = cm_msg_retrieve2(facility, t, min_messages, messages, num_messages_read);
   return retval;
}

INT c_cm_msg_open_buffer() {
   return cm_msg_open_buffer();
}

INT c_cm_msg_close_buffer() {
   return cm_msg_close_buffer();
}

INT c_cm_register_deferred_transition(INT transition, BOOL(*func) (INT, BOOL)) {
   return cm_register_deferred_transition(transition, func);
}

INT c_cm_register_function(INT id, INT(*func) (INT, void **)) {
   return cm_register_function(id, func);
}

INT c_cm_register_transition(INT transition, INT(*func) (INT, char *), int sequence_number) {
   return cm_register_transition(transition, func, sequence_number);
}

INT c_cm_set_transition_sequence(INT transition, INT sequence_number) {
   return cm_set_transition_sequence(transition, sequence_number);
}

INT c_cm_start_watchdog_thread(void) {
   return cm_start_watchdog_thread();
}

INT c_cm_stop_watchdog_thread(void) {
   return cm_stop_watchdog_thread();
}

INT c_cm_transition(INT transition, INT run_number, char *error, INT strsize, INT async_flag, INT debug_flag) {
   return cm_transition(transition, run_number, error, strsize, async_flag, debug_flag);
}

INT c_cm_yield(INT millisec) {
   return cm_yield(millisec);
}

INT c_db_close_record(HNDLE hdb, HNDLE hkey) {
   return db_close_record(hdb, hkey);
}

INT c_db_copy_json_ls(HNDLE hDB, HNDLE hKey, char **buffer, int* buffer_size, int* buffer_end) {
   return db_copy_json_ls(hDB, hKey, buffer, buffer_size, buffer_end);
}

INT c_db_copy_json_save(HNDLE hDB, HNDLE hKey, char **buffer, int* buffer_size, int* buffer_end) {
   return db_copy_json_save(hDB, hKey, buffer, buffer_size, buffer_end);
}

INT c_db_create_key(HNDLE hdb, HNDLE key_handle, const char *key_name, DWORD type) {
   return db_create_key(hdb, key_handle, key_name, type);
}

INT c_db_create_link(HNDLE hdb, HNDLE key_handle, const char *link_name, const char *destination) {
   return db_create_link(hdb, key_handle, link_name, destination);
}

INT c_db_delete_key(HNDLE database_handle, HNDLE key_handle, BOOL follow_links) {
   return db_delete_key(database_handle, key_handle, follow_links);
}

INT c_db_enum_key(HNDLE hDB, HNDLE hKey, INT idx, HNDLE * subkey_handle) {
	return db_enum_key(hDB, hKey, idx, subkey_handle);
}

INT c_db_enum_link(HNDLE hDB, HNDLE hKey, INT idx, HNDLE * subkey_handle) {
   return db_enum_link(hDB, hKey, idx, subkey_handle);
}

INT c_db_find_key(HNDLE hdb, HNDLE hkey, const char *name, HNDLE * hsubkey) {
   return db_find_key(hdb, hkey, name, hsubkey);
}

INT c_db_find_link(HNDLE hDB, HNDLE hKey, const char *key_name, HNDLE * subhKey) {
   return db_find_link(hDB, hKey, key_name, subhKey);
}

INT c_db_get_key(HNDLE hdb, HNDLE key_handle, KEY * key) {
   return db_get_key(hdb, key_handle, key);
}

INT c_db_get_link_data(HNDLE hdb, HNDLE key_handle, void *data, INT * buf_size, DWORD type) {
   return db_get_link_data(hdb, key_handle, data, buf_size, type);
}

INT c_db_get_parent(HNDLE hDB, HNDLE hKey, HNDLE * parenthKey) {
   return db_get_parent(hDB, hKey, parenthKey);
}

INT c_db_get_value(HNDLE hdb, HNDLE hKeyRoot, const char *key_name, void *data, INT * size, DWORD type, BOOL create) {
   return db_get_value(hdb, hKeyRoot, key_name, data, size, type, create);
}

INT c_db_open_record(HNDLE hdb, HNDLE hkey, void *ptr, INT rec_size, WORD access, void (*dispatcher) (INT, INT, void *), void *info) {
   return db_open_record(hdb, hkey, ptr, rec_size, access, dispatcher, info);
}

INT c_db_rename_key(HNDLE hDB, HNDLE hKey, const char *name) {
   return db_rename_key(hDB, hKey, name);
}

INT c_db_reorder_key(HNDLE hDB, HNDLE hKey, INT index) {
   return db_reorder_key(hDB, hKey, index);
}

INT c_db_resize_string(HNDLE hDB, HNDLE hKeyRoot, const char *key_name, int num_values, int max_string_size) {
   return db_resize_string(hDB, hKeyRoot, key_name, num_values, max_string_size);
}

INT c_db_set_link_data(HNDLE hdb, HNDLE key_handle, void *data, INT buf_size, int num_values, DWORD type) {
   return db_set_link_data(hdb, key_handle, data, buf_size, num_values, type);
}

INT c_db_set_num_values(HNDLE hDB, HNDLE hKey, INT num_values) {
   return db_set_num_values(hDB, hKey, num_values);
}

INT c_db_set_value(HNDLE hdb, HNDLE hKeyRoot, const char *key_name, const void *data, INT size, INT num_values, DWORD type) {
   return db_set_value(hdb, hKeyRoot, key_name, data, size, num_values, type);
}

INT c_db_set_value_index(HNDLE hDB, HNDLE hKeyRoot, const char *key_name, const void *data, INT data_size, INT index, DWORD type, BOOL truncate) {
   return db_set_value_index(hDB, hKeyRoot, key_name, data, data_size, index, type, truncate);
}

INT c_db_unwatch(HNDLE hDB, HNDLE hKey) {
   return db_unwatch(hDB, hKey);
}

INT c_db_watch(HNDLE hDB, HNDLE hKey, void (*dispatcher) (INT, INT, INT, void*), void* info) {
   return db_watch(hDB, hKey, dispatcher, info);
}

INT c_jrpc_client_call(HNDLE hconn, char* cmd, char* args, char* buf, int buf_length) {
   // Specialized version of rpc_client_call that just deals with RPC_JRPC,
   // so we don't have to worry about variable arg lists.
   // You must already have malloc'd buf to be big enough for buf_length.
   return rpc_client_call(hconn, RPC_JRPC, cmd, args, buf, buf_length);
}

INT c_rpc_flush_event(void) {
   return rpc_flush_event();
}

INT c_rpc_is_remote(void) {
	return rpc_is_remote();
}

INT c_rpc_send_event(INT buffer_handle, const EVENT_HEADER *event, INT buf_size, INT async_flag, INT mode) {
   return rpc_send_event(buffer_handle, event, buf_size, async_flag, mode);
}

INT c_ss_daemon_init(BOOL keep_stdout) {
   return ss_daemon_init(keep_stdout);
}

MidasHistoryInterface* mh = nullptr;

INT c_connect_history_if_needed(HNDLE hDB, bool debug=false) {
   INT status = SUCCESS;

   if (mh == nullptr) {
      status = hs_get_history(hDB, 0, HS_GET_DEFAULT|HS_GET_READER|HS_GET_INACTIVE, debug, &mh);
   }

   return status;
}

INT c_hs_get_events(HNDLE hDB, char*** dest, int& dest_len) {
   INT status = c_connect_history_if_needed(hDB);

   if (status != SUCCESS) {
      return status;
   }

   time_t t = 0;
   std::vector<std::string> events;

   status = mh->hs_get_events(t, &events);

   if (status != SUCCESS) {
      return status;
   }

   return copy_vector_string_to_c(events, dest, dest_len);
}
 
INT c_hs_get_tags(HNDLE hDB, char* event_name, char*** dest_names, void** dest_types, void** dest_n_data, int& dest_len) {     
   INT status = c_connect_history_if_needed(hDB);

   if (status != SUCCESS) {
      return status;
   }
   
   time_t t = 0;
   std::vector<TAG> tags;
   status = mh->hs_get_tags(event_name, t, &tags);

   if (status != SUCCESS) {
      return status;
   }

   std::vector<std::string> tag_names;
   std::vector<DWORD> tag_types;
   std::vector<DWORD> tag_n_data;

   for (auto& tag : tags) {
      tag_names.push_back(tag.name);
      tag_types.push_back(tag.type);
      tag_n_data.push_back(tag.n_data);
   }

   copy_vector_string_to_c(tag_names, dest_names, dest_len);
   copy_vector_to_c(tag_types, dest_types, dest_len);
   copy_vector_to_c(tag_n_data, dest_n_data, dest_len);
   return SUCCESS;
}

INT c_hs_read(HNDLE hDB, 
              uint32_t start_time, uint32_t end_time, uint32_t interval_secs, 
              char* event_name, char* tag_name, int idx_start, int nvars,
              void** num_entries, void** times, void** values, void** hs_status) {
   // Note this function varies from hs_read() for the times/tbuffer and values/vbuffer
   // parameters! To simplify passing between python/C, we concatenate all the data for
   // all variables into one array, rather than using a 2D array.
   // So num_entries and hs_status are length "nvars", while 
   // times and values are length "sum of num_entries".
   INT status = c_connect_history_if_needed(hDB);

   if (status != SUCCESS) {
      return status;
   }

   const char** event_names = (const char**)calloc(nvars, sizeof(const char*));
   const char** var_names = (const char**)calloc(nvars, sizeof(const char*));
   int* indices = (int*)calloc(nvars, sizeof(int));

   for (int i = 0; i < nvars; i++) {
      event_names[i] = event_name;
      var_names[i] = tag_name;
      indices[i] = idx_start + i;
   }

   *num_entries = calloc(nvars, sizeof(int));
   time_t** tbuffer_int = (time_t**)calloc(nvars, sizeof(time_t*));
   double** vbuffer_int = (double**)calloc(nvars, sizeof(double*));
   *hs_status = calloc(nvars, sizeof(int));

   status = mh->hs_read(start_time, end_time, interval_secs, nvars, event_names, var_names, indices, (int*)*num_entries, tbuffer_int, vbuffer_int, (int*)*hs_status);

   // Simplify passing back to python by concatenating timestamp buffers into
   // a single buffer rather than 2-D array. Same for value buffers.
   size_t tot_len = 0;

   for (int i = 0; i < nvars; i++) {
      tot_len += ((int*)(*num_entries))[i];
   }

   if (tot_len > 0) {
      *times = calloc(tot_len, sizeof(uint32_t));
      *values = calloc(tot_len, sizeof(double));

      uint32_t* cast_tbuffer = (uint32_t*)*times;
      double* cast_vbuffer = (double*)*values;
      size_t offset = 0;

      for (int i = 0; i < nvars; i++) {
         size_t this_n = ((int*)(*num_entries))[i];
         for (int n = 0; n < (int) this_n; n++) {
            // Deal with time_t vs uint32_t possibly being different sizes
            // by manually copying values rather than doing a memcpy.
            cast_tbuffer[offset+n] = tbuffer_int[i][n];
            cast_vbuffer[offset+n] = vbuffer_int[i][n];
         }
      
         offset += this_n;
      }
   }

   free(tbuffer_int);
   free(vbuffer_int);
   free(event_names);
   free(var_names);
   free(indices);

   return status;
}