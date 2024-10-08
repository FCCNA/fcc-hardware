/********************************************************************\

  Name:         json_paste.cxx
  Created by:   Konstantin Olchanski

  Contents:     JSON decoder for ODB

\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include "mjson.h"

#ifndef HAVE_STRLCPY
#include "strlcpy.h"
#endif

INT EXPRT db_load_json(HNDLE hDB, HNDLE hKey, const char *filename)
{
   int status;
   //printf("db_load_json: filename [%s], handle %d\n", filename, hKey);

   FILE* fp = fopen(filename, "r");
   if (!fp) {
      cm_msg(MERROR, "db_load_json", "file \"%s\" not found", filename);
      return DB_FILE_ERROR;
   }

   std::string data;

   while (1) {
      char buf[102400];
      int rd = read(fileno(fp), buf, sizeof(buf)-1);
      if (rd == 0)
         break; // end of file
      if (rd < 0) {
         // error
         cm_msg(MERROR, "db_load_json", "file \"%s\" read error %d (%s)", filename, errno, strerror(errno));
         fclose(fp);
         return DB_FILE_ERROR;
      }
      // make sure string is nul terminated
      buf[sizeof(buf)-1] = 0;
      data += buf;
   }

   fclose(fp);
   fp = NULL;

   //printf("file contents: [%s]\n", data.c_str());

   const char* jptr = strchr(data.c_str(), '{');

   if (!jptr) {
      cm_msg(MERROR, "db_load_json", "file \"%s\" does not look like JSON data", filename);
      fclose(fp);
      return DB_FILE_ERROR;
   }

   status = db_paste_json(hDB, hKey, jptr);

   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "db_load_json", "error loading JSON data from file \"%s\"", filename);
      return status;
   }

   return status;
}

static int tid_from_key(const MJsonNode* key)
{
   if (!key)
      return 0;
   const MJsonNode* n = key->FindObjectNode("type");
   if (!n)
      return 0;
   int tid = n->GetInt();
   if (tid > 0)
      return tid;
   return 0;
}

static int item_size_from_key(const MJsonNode* key)
{
   if (!key)
      return 0;
   const MJsonNode* n = key->FindObjectNode("item_size");
   if (!n)
      return 0;
   int item_size = n->GetInt();
   if (item_size > 0)
      return item_size;
   return 0;
}

static int guess_tid(const MJsonNode* node)
{
   switch (node->GetType()) {
   case MJSON_ARRAY:  { const MJsonNodeVector* a = node->GetArray(); if (a && a->size()>0) return guess_tid((*a)[0]); else return 0; }
   case MJSON_OBJECT: return TID_KEY;
   case MJSON_STRING: {
      std::string v = node->GetString();
      if (v == "NaN")
         return TID_DOUBLE;
      else if (v == "Infinity")
         return TID_DOUBLE;
      else if (v == "-Infinity")
         return TID_DOUBLE;
      else if (v[0]=='0' && v[1]=='x' && isxdigit(v[2]) && v.length() > 10)
         return TID_UINT64;
      else if (v[0]=='0' && v[1]=='x' && isxdigit(v[2]))
         return TID_DWORD;
      else
         return TID_STRING;
   }
   case MJSON_INT:    return TID_INT;
   case MJSON_NUMBER: return TID_DOUBLE;
   case MJSON_BOOL:   return TID_BOOL;
   default: return 0;
   }
}

static int paste_node(HNDLE hDB, HNDLE hKey, const char* path, bool is_array, int index, const MJsonNode* node, int tid, int string_length, const MJsonNode* key);

static int paste_array(HNDLE hDB, HNDLE hKey, const char* path, const MJsonNode* node, int tid, int string_length, const MJsonNode* key)
{
   int status, slength = 0;
   const MJsonNodeVector* a = node->GetArray();

   if (a==NULL) {
      cm_msg(MERROR, "db_paste_json", "invalid array at \"%s\"", path);
      return DB_FILE_ERROR;
   }

   if (string_length == 0) {
      int slength = item_size_from_key(key);
      if (slength == 0)
         slength = NAME_LENGTH;
   } else {
      slength = string_length;
   }

   KEY odbkey;
   status = db_get_key(hDB, hKey, &odbkey);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "db_paste_json", "cannot get odb key at \"%s\", status %d", path, status);
      return DB_FILE_ERROR;
   }

   if (odbkey.item_size > 0 && (INT) a->size() != odbkey.num_values) {
      status = db_set_num_values(hDB, hKey, a->size());
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot resize key \"%s\", status = %d", path, status);
         return status;
      }
   }

   bool is_array = (a->size() > 1);
   for (unsigned i=a->size(); ;) {
      if (i==0)
         break;
      i--;
      MJsonNode* n = (*a)[i];
      if (!n)
         continue;
      status = paste_node(hDB, hKey, path, is_array, i, n, tid, slength, key);
      if (status == DB_NO_ACCESS) {
         cm_msg(MERROR, "db_paste_json", "skipping write-protected array \"%s\"", path);
         return DB_SUCCESS;
      } else if (status != DB_SUCCESS)
         return status;
   }

   return DB_SUCCESS;
}

static int paste_object(HNDLE hDB, HNDLE hKey, const char* path, const MJsonNode* objnode)
{
   if (equal_ustring(path, "/system/clients")) {
      // do not reload ODB /system/clients
      return DB_SUCCESS;
   }
   int status;
   const MJsonStringVector* names = objnode->GetObjectNames();
   const MJsonNodeVector* nodes = objnode->GetObjectNodes();
   if (names==NULL||nodes==NULL||names->size()!=nodes->size()) {
      cm_msg(MERROR, "db_paste_json", "invalid object at \"%s\"", path);
      return DB_FILE_ERROR;
   }
   for(unsigned i=0; i<names->size(); i++) {
      const char* name = (*names)[i].c_str();
      const MJsonNode* node = (*nodes)[i];
      const MJsonNode* key = NULL;
      if (strchr(name, '/')) // skip special entries
         continue;
      int tid = 0;
      if (node->GetType() == MJSON_OBJECT)
         tid = TID_KEY;
      else {
         key = objnode->FindObjectNode((std::string(name) + "/key").c_str());
         if (key)
            tid = tid_from_key(key);
         if (!tid)
            tid = guess_tid(node);
         //printf("entry [%s] type %s, tid %d\n", name, MJsonNode::TypeToString(node->GetType()), tid);
      }

      status = db_create_key(hDB, hKey, name, tid);

      if (status == DB_KEY_EXIST) {
         HNDLE hSubkey;
         KEY key;
         status = db_find_link(hDB, hKey, name, &hSubkey);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "db_paste_json", "key exists, but cannot find it \"%s\" of type %d in \"%s\", db_find_link() status %d", name, tid, path, status);
            return status;
         }

         status = db_get_key(hDB, hSubkey, &key);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "db_paste_json", "cannot create \"%s\" of type %d in \"%s\", db_create_key() status %d", name, tid, path, status);
            return status;
         }

         if ((int)key.type == tid) {
            // existing item is of the same type, continue with overwriting it
            status = DB_SUCCESS;
         } else {
            // FIXME: delete wrong item, create item with correct tid
            cm_msg(MERROR, "db_paste_json", "cannot overwrite existing item \"%s\" of type %d in \"%s\" with new tid %d", name, key.type, path, tid);
            return status;
         }
      }

      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot create \"%s\" of type %d in \"%s\", db_create_key() status %d", name, tid, path, status);
         return status;
      }

      HNDLE hSubkey;
      status = db_find_link(hDB, hKey, name, &hSubkey);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot find \"%s\" of type %d in \"%s\", db_find_link() status %d", name, tid, path, status);
         return status;
      }

      std::string node_name;
      if (strcmp(path, "/") != 0) {
         node_name += path;
      }
      node_name += "/";
      node_name += name;

      status = paste_node(hDB, hSubkey, node_name.c_str(), false, 0, node, tid, 0, key);
      if (status == DB_NO_ACCESS) {
         cm_msg(MERROR, "db_paste_json", "skipping write-protected node \"%s\"", node_name.c_str());
      } else if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "paste of \"%s\" is incomplete", node_name.c_str());
         //cm_msg(MERROR, "db_paste_json", "cannot..."); // paste_node() reports it's own failures
         return status;
      }
   }
   return DB_SUCCESS;
}

static int paste_bool(HNDLE hDB, HNDLE hKey, const char* path, int index, const MJsonNode* node)
{
   int status;
   BOOL value = node->GetBool();
   int size = sizeof(value);
   status = db_set_data_index(hDB, hKey, &value, size, index, TID_BOOL);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "db_paste_json", "cannot set TID_BOOL value for \"%s\", db_set_data_index() status %d", path, status);
      return status;
   }
   return DB_SUCCESS;
}

static int GetDWORD(const MJsonNode* node, const char* path, DWORD* dw)
{
   switch (node->GetType()) {
   default:
      cm_msg(MERROR, "db_paste_json", "GetDWORD: unexpected node type %d at \"%s\"", node->GetType(), path);
      *dw = 0;
      return DB_FILE_ERROR;
   case MJSON_INT:
      *dw = node->GetInt();
      return SUCCESS;
   case MJSON_NUMBER:
      *dw = (DWORD)node->GetDouble();
      return SUCCESS;
   case MJSON_STRING:
      std::string s = node->GetString();
      errno = 0;
      if (s[0] == '0' && s[1] == 'x') { // hex encoded number
         *dw = strtoul(s.c_str(), NULL, 16);
      } else if (s.back() == 'b') { // binary number
         *dw = strtoul(s.c_str(), NULL, 2);
      } else if (isdigit(s[0]) || (s[0]=='-' && isdigit(s[1]))) { // probably a number
         *dw = strtoul(s.c_str(), NULL, 0);
      } else {
         cm_msg(MERROR, "db_paste_json", "GetDWORD: MJSON_STRING node invalid numeric value \'%s\' at \"%s\"", s.c_str(), path);
         *dw = 0;
         return DB_FILE_ERROR;
      }
      if (errno != 0) {
         cm_msg(MERROR, "db_paste_json", "GetDWORD: MJSON_STRING node invalid numeric value \'%s\', strtoul() errno %d (%s) at \"%s\"", s.c_str(), errno, strerror(errno), path);
         *dw = 0;
         return DB_FILE_ERROR;
      }
      return SUCCESS;
   }
   // NOT REACHED
}

static int GetQWORD(const MJsonNode* node, const char* path, UINT64* qw)
{
   switch (node->GetType()) {
   default:
      cm_msg(MERROR, "db_paste_json", "GetQWORD: unexpected node type %d at \"%s\"", node->GetType(), path);
      *qw = 0;
      return DB_FILE_ERROR;
   case MJSON_INT:
      *qw = node->GetLL();
      return SUCCESS;
   case MJSON_NUMBER:
      *qw = node->GetDouble();
      return SUCCESS;
   case MJSON_STRING:
      std::string s = node->GetString();
      errno = 0;
      if (s[0] == '0' && s[1] == 'x') { // hex encoded number
         *qw = strtoull(s.c_str(), NULL, 16);
      } else if (s.back() == 'b') { // binary number
         *qw = strtoull(s.c_str(), NULL, 2);
      } else if (isdigit(s[0]) || (s[0]=='-' && isdigit(s[1]))) { // probably a number
         *qw = strtoull(s.c_str(), NULL, 0);
      } else {
         cm_msg(MERROR, "db_paste_json", "GetQWORD: MJSON_STRING node invalid numeric value \'%s\' at \"%s\"", s.c_str(), path);
         *qw = 0;
         return DB_FILE_ERROR;
      }
      if (errno != 0) {
         cm_msg(MERROR, "db_paste_json", "GetQWORD: MJSON_STRING node invalid numeric value \'%s\', strtoul() errno %d (%s) at \"%s\"", s.c_str(), errno, strerror(errno), path);
         *qw = 0;
         return DB_FILE_ERROR;
      }
      return SUCCESS;
   }
   // NOT REACHED
}

static int GetDOUBLE(const MJsonNode* node, const char* path, double* dw)
{
   switch (node->GetType()) {
   default:
      cm_msg(MERROR, "db_paste_json", "GetDOUBLE: unexpected node type %d at \"%s\"", node->GetType(), path);
      *dw = 0;
      return DB_FILE_ERROR;
   case MJSON_INT:
      *dw = node->GetInt();
      return SUCCESS;
   case MJSON_NUMBER:
      *dw = node->GetDouble();
      return SUCCESS;
   case MJSON_STRING:
      std::string s = node->GetString();
      errno = 0;
      if (s == "NaN" || s == "Infinity" || s == "-Infinity") {
         *dw = node->GetDouble();
      } else if (s[0] == '0' && s[1] == 'x') { // hex encoded number
         *dw = strtoul(s.c_str(), NULL, 16);
      } else if (isdigit(s[0]) || (s[0]=='-' && isdigit(s[1]))) { // probably a number
         *dw = strtod(s.c_str(), NULL);
      } else {
         cm_msg(MERROR, "db_paste_json", "GetDOUBLE: MJSON_STRING node invalid numeric value \'%s\' at \"%s\"", s.c_str(), path);
         *dw = 0;
         return DB_FILE_ERROR;
      }
      if (errno != 0) {
         cm_msg(MERROR, "db_paste_json", "GetDOUBLE: MJSON_STRING node invalid numeric value \'%s\', strtoul() errno %d (%s) at \"%s\"", s.c_str(), errno, strerror(errno), path);
         *dw = 0;
         return DB_FILE_ERROR;
      }
      return SUCCESS;
   }
   // NOT REACHED
}

static int paste_value(HNDLE hDB, HNDLE hKey, const char* path, bool is_array, int index, const MJsonNode* node, int tid, int string_length, const MJsonNode* key)
{
   int status;
   //printf("paste_value: path [%s], index %d, tid %d, slength %d, key %p\n", path, index, tid, string_length, key);

   switch (tid) {
   default:
      cm_msg(MERROR, "db_paste_json", "do not know what to do with tid %d at \"%s\"", tid, path);
      // keep loading remaining data, ignore this error return DB_FILE_ERROR;
      return DB_SUCCESS;
   case TID_ARRAY:
      cm_msg(MERROR, "db_paste_json", "paste of TID_ARRAY is not implemented at \"%s\"", path);
      return DB_SUCCESS;
   case TID_STRUCT:
      cm_msg(MERROR, "db_paste_json", "paste of TID_STRUCT is not implemented at \"%s\"", path);
      return DB_SUCCESS;
   case TID_BITFIELD:
      cm_msg(MERROR, "db_paste_json", "paste of TID_BITFIELD is not implemented at \"%s\"", path);
      return DB_SUCCESS;
   case TID_CHAR: {
      const std::string value = node->GetString();
      int size = 1;
      status = db_set_data_index(hDB, hKey, value.c_str(), size, index, TID_CHAR);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_CHAR value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_BOOL: {
      BOOL v;
      if (node->GetType() == MJSON_STRING && node->GetString() == "true") {
         v = true;
      } else if (node->GetType() == MJSON_STRING && node->GetString() == "false") {
         v = false;
      } else if (node->GetType() == MJSON_STRING && node->GetString()[0] == 'y') {
         v = true;
      } else if (node->GetType() == MJSON_STRING && node->GetString()[0] == 'n') {
         v = false;
      } else if (node->GetType() == MJSON_STRING && node->GetString()[0] == 'Y') {
         v = true;
      } else if (node->GetType() == MJSON_STRING && node->GetString()[0] == 'N') {
         v = false;
      } else if (node->GetType() == MJSON_STRING && node->GetString()[0] == 't') {
         v = true;
      } else if (node->GetType() == MJSON_STRING && node->GetString()[0] == 'f') {
         v = false;
      } else if (node->GetType() == MJSON_STRING && node->GetString()[0] == 'T') {
         v = true;
      } else if (node->GetType() == MJSON_STRING && node->GetString()[0] == 'F') {
         v = false;
      } else {
         DWORD dw;
         status = GetDWORD(node, path, &dw);
         if (status != SUCCESS)
            return status;
         if (dw) v = TRUE;
         else v = FALSE;
      }
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, TID_BOOL);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_BOOL value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_BYTE:
   case TID_SBYTE: {
      DWORD dw;
      status = GetDWORD(node, path, &dw);
      if (status != SUCCESS)
         return status;
      BYTE b = (BYTE)dw;
      int size = sizeof(b);
      status = db_set_data_index(hDB, hKey, &b, size, index, tid);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_BYTE/TID_SBYTE value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_WORD:
   case TID_SHORT: {
      DWORD dw;
      status = GetDWORD(node, path, &dw);
      if (status != SUCCESS)
         return status;
      WORD v = (WORD)dw;
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, tid);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_WORD/TID_SHORT value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_DWORD: {
      DWORD v;
      status = GetDWORD(node, path, &v);
      if (status != SUCCESS)
         return status;
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, TID_DWORD);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_DWORD value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_INT: {
      int v = 0;
      switch (node->GetType()) {
      default:
         cm_msg(MERROR, "db_paste_json", "unexpected node type %d at \"%s\"", node->GetType(), path);
         return DB_FILE_ERROR;
      case MJSON_INT: {
         v = node->GetInt();
         break;
      }
      case MJSON_NUMBER: {
         double dv = node->GetDouble();
         if (dv > INT_MAX || dv < INT_MIN) {
            cm_msg(MERROR, "db_paste_json", "numeric value %f out of range at \"%s\"", dv, path);
            return DB_FILE_ERROR;
         }
         v = (int)dv;
         break;
      }
      case MJSON_STRING: {
         status = GetDWORD(node, path, (DWORD*)&v);
         if (status != SUCCESS)
            return status;
         break;
      }
      }
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, TID_INT);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_INT value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }

   case TID_UINT64: {
      UINT64 v;
      status = GetQWORD(node, path, &v);
      if (status != SUCCESS)
         return status;
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, TID_UINT64);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_UINT64 value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }

   case TID_INT64: {
      INT64 v;
      status = GetQWORD(node, path, (UINT64 *)&v);
      if (status != SUCCESS)
         return status;
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, TID_INT64);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_INT64 value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }

   case TID_FLOAT: {
      double dv;
      status = GetDOUBLE(node, path, &dv);
      if (status != SUCCESS)
         return status;
      float v = dv;
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, TID_FLOAT);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_FLOAT value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_DOUBLE: {
      double v;
      status = GetDOUBLE(node, path, &v);
      if (status != SUCCESS)
         return status;
      int size = sizeof(v);
      status = db_set_data_index(hDB, hKey, &v, size, index, TID_DOUBLE);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_DOUBLE value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }
      return DB_SUCCESS;
   }
   case TID_STRING: {
      char* buf = NULL;
      const char* ptr = NULL;
      int size = 0;
      const std::string value = node->GetString();
      if (string_length == 0)
         string_length = item_size_from_key(key);
      //printf("string_length %d\n", string_length);
      if (string_length) {
         buf = new char[string_length];
         strlcpy(buf, value.c_str(), string_length);
         ptr = buf;
         size = string_length;
      } else {
         ptr = value.c_str();
         size = strlen(ptr) + 1;
      }

      if (is_array) {
         KEY key;
         status = db_get_key(hDB, hKey, &key);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "db_paste_json", "cannot get key of string array for \"%s\", db_get_key() status %d", path, status);
            return status;
         }
         
         if (key.item_size < size) {
            status = db_resize_string(hDB, hKey, NULL, key.num_values, size);
            if (status != DB_SUCCESS) {
               cm_msg(MERROR, "db_paste_json", "cannot change array string length from %d to %d for \"%s\", db_resize_string() status %d", key.item_size, size, path, status);
               return status;
            }
         }
      }

      //printf("set_data_index index %d, size %d\n", index, size);

      if (string_length > 0) {
         if (is_array) {
            status = db_set_data_index(hDB, hKey, ptr, size, index, TID_STRING);
         } else {
            status = db_set_data(hDB, hKey, ptr, size, 1, TID_STRING);
         }
      } else if (index != 0) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_STRING value for \"%s\" index %d, it is not an array", path, index);
         status = DB_OUT_OF_RANGE;
      } else {
         status = db_set_data(hDB, hKey, ptr, size, 1, TID_STRING);
      }

      if (buf)
         delete[] buf;

      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_STRING value for \"%s\", db_set_data_index() status %d", path, status);
         return status;
      }

      return DB_SUCCESS;
   }
   case TID_LINK: {
      std::string value_string = node->GetString();
      const char* value = value_string.c_str();
      int size = strlen(value) + 1;

      status = db_set_data(hDB, hKey, value, size, 1, TID_LINK);

      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "db_paste_json", "cannot set TID_LINK value for \"%s\", db_set_data() status %d", path, status);
         return status;
      }

      return DB_SUCCESS;
   }
   }
   // NOT REACHED
}

static int paste_node(HNDLE hDB, HNDLE hKey, const char* path, bool is_array, int index, const MJsonNode* node, int tid, int string_length, const MJsonNode* key)
{
   //node->Dump();
   switch (node->GetType()) {
   case MJSON_ARRAY:  return paste_array(hDB, hKey, path, node, tid, string_length, key);
   case MJSON_OBJECT: return paste_object(hDB, hKey, path, node);
   case MJSON_STRING: return paste_value(hDB, hKey, path, is_array, index, node, tid, string_length, key);
   case MJSON_INT:    return paste_value(hDB, hKey, path, is_array, index, node, tid, 0, key);
   case MJSON_NUMBER: return paste_value(hDB, hKey, path, is_array, index, node, tid, 0, key);
   case MJSON_BOOL:   return paste_bool(hDB, hKey, path, index, node);
   case MJSON_ERROR:
      cm_msg(MERROR, "db_paste_json", "JSON parse error: \"%s\" at \"%s\"", node->GetError().c_str(), path);
      return DB_FILE_ERROR;
   case MJSON_NULL:
      cm_msg(MERROR, "db_paste_json", "unexpected JSON null value at \"%s\"", path);
      return DB_FILE_ERROR;
   default:
      cm_msg(MERROR, "db_paste_json", "unexpected JSON node type %d (%s) at \"%s\"", node->GetType(), MJsonNode::TypeToString(node->GetType()), path);
      return DB_FILE_ERROR;
   }
   // NOT REACHED
}

INT EXPRT db_paste_json(HNDLE hDB, HNDLE hKeyRoot, const char *buffer)
{
   int status;
   char path[MAX_ODB_PATH];

   status = db_get_path(hDB, hKeyRoot, path, sizeof(path));

   //printf("db_paste_json: handle %d, path [%s]\n", hKeyRoot, path);

   MJsonNode* node = MJsonNode::Parse(buffer);
   status = paste_node(hDB, hKeyRoot, path, false, 0, node, 0, 0, NULL);
   delete node;

   return status;
}

INT EXPRT db_paste_json_node(HNDLE hDB, HNDLE hKeyRoot, int index, const MJsonNode *node)
{
   int status;
   char path[MAX_ODB_PATH];
   KEY key;

   status = db_get_key(hDB, hKeyRoot, &key);
   if (status != DB_SUCCESS)
      return status;

   status = db_get_path(hDB, hKeyRoot, path, sizeof(path));
   if (status != DB_SUCCESS)
      return status;

   int tid = key.type;
   int string_length = 0;
   bool is_array = (key.num_values > 1);
   if (tid == TID_STRING) {
      // do not truncate strings, only extend if necessary
      if ((int)node->GetString().length()+1 > key.item_size)
         string_length = node->GetString().length()+1;
      else
         string_length = key.item_size;
   }

   status = paste_node(hDB, hKeyRoot, path, is_array, index, node, tid, string_length, NULL);

   return status;
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
