/********************************************************************\

  Name:         odbxx.cxx
  Created by:   Stefan Ritt

  Contents:     Object oriented interface to ODB implementation file

\********************************************************************/

#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <initializer_list>
#include <cstring>
#include <bitset>
#include <functional>

#include "midas.h"
#include "odbxx.h"
#include "mexcept.h"
// #include "mleak.hxx" // un-comment for memory leak debugging

/*------------------------------------------------------------------*/

namespace midas {

   //----------------------------------------------------------------

   // initialize static variables
   HNDLE odb::s_hDB = 0;
   bool odb::s_debug = false;
   bool odb::s_connected_odb = false;
   std::vector<midas::odb *> g_watchlist = {};

   // static functions ----------------------------------------------

   // initialize s_hDB, internal use only
   void odb::init_hdb() {
      if (s_hDB == 0)
         cm_get_experiment_database(&s_hDB, nullptr);
      if (s_hDB == 0)
         mthrow("Please call cm_connect_experiment() before accessing the ODB");
      s_connected_odb = true;
   }

   // search for a key with a specific hKey, needed for callbacks
   midas::odb *odb::search_hkey(midas::odb *po, int hKey) {
      if (po->m_hKey == hKey)
         return po;
      if (po->m_tid == TID_KEY) {
         for (int i = 0; i < po->m_num_values; i++) {
            midas::odb *pot = search_hkey(po->m_data[i].get_podb(), hKey);
            if (pot != nullptr)
               return pot;
         }
      }
      return nullptr;
   }

   // check if a key exists in the ODB
   bool odb::exists(const std::string &name) {
      init_hdb();
      if (!odb::is_connected_odb())
         return false;
      HNDLE hkey;
      return db_find_key(s_hDB, 0, name.c_str(), &hkey) == DB_SUCCESS;
   }

   // delete a key in the ODB
   int odb::delete_key(const std::string &name) {
      init_hdb();
      if (!odb::is_connected_odb())
         return false;
      HNDLE hkey;
      auto status = db_find_key(s_hDB, 0, name.c_str(), &hkey);
      if (status != DB_SUCCESS) {
         if (s_debug)
            std::cout << "Delete key " + name << " not found in ODB" << std::endl;
         return status;
      }
      if (s_debug)
         std::cout << "Delete ODB key " + name << std::endl;
      return db_delete_key(s_hDB, hkey, false);
   }

   // global callback function for db_watch()
   void odb::watch_callback(int hDB, int hKey, int index, void *info) {
      midas::odb *po = static_cast<midas::odb *>(info);
      if (po->m_data == nullptr)
         mthrow("Callback received for a midas::odb object which went out of scope");
      midas::odb *poh = search_hkey(po, hKey);
      poh->m_last_index = index;
      po->m_watch_callback(*poh);
      poh->m_last_index = -1;
   }

   // create ODB key
   int odb::create(const char *name, int type) {
      init_hdb();
      int status = -1;
      if (is_connected_odb())
         status = db_create_key(s_hDB, 0, name, type);
      if (status != DB_SUCCESS && status != DB_KEY_EXIST)
         mthrow("Cannot create key " + std::string(name) + ", db_create_key() status = " + std::to_string(status));
      return status;
   }

   // private functions ---------------------------------------------

   void odb::set_flags_recursively(uint32_t f) {
      m_flags = f;
      if (m_tid == TID_KEY) {
         for (int i = 0; i < m_num_values; i++)
            m_data[i].get_odb().set_flags_recursively(f);
      }
   }

   void odb::odb_from_xml(const std::string &str) {
      init_hdb();

      HNDLE hKey;
      int status = db_find_link(s_hDB, 0, str.c_str(), &hKey);
      if (status != DB_SUCCESS)
         mthrow("ODB key \"" + str + "\" not found in ODB");

      int bsize = 1000000;
      char *buffer = (char *)malloc(bsize);
      do {
         int size = bsize;
         status = db_copy_xml(s_hDB, hKey, buffer, &size, false);
         if (status == DB_TRUNCATED) {
            bsize *= 2;
            buffer = (char *)realloc(buffer, bsize);
         }
      } while (status == DB_TRUNCATED);

      if (status != DB_SUCCESS)
         mthrow("Cannot retrieve XML data, status = " + std::to_string(status));

      if (s_debug)
         std::cout << "Retrieved XML tree for \"" + str + "\"" << std::endl;

      char error[256] = "";
      PMXML_NODE tree = mxml_parse_buffer(buffer, error, sizeof(error), NULL);
      free(buffer);
      if (error[0])
         mthrow("MXML error: " + std::string(error));

      odb_from_xml(&tree->child[2], this);
      m_hKey = hKey;

      mxml_free_tree(tree);
   }

   // resize internal m_data array, keeping old values
   void odb::resize_mdata(int size) {
      auto new_array = new u_odb[size]{};
      int i;
      for (i = 0; i < m_num_values && i < size; i++) {
         new_array[i] = m_data[i];
         if (m_tid == TID_KEY)
            m_data[i].set_odb(nullptr); // move odb*
         if (m_tid == TID_STRING || m_tid == TID_LINK)
            m_data[i].set_string_ptr(nullptr); // move std::string*
      }
      for (; i < size; i++) {
         if (m_tid == TID_STRING || m_tid == TID_LINK)
            new_array[i].set_string(""); // allocates new string
      }
      delete[] m_data;
      m_data = new_array;
      m_num_values = size;
      for (int i = 0; i < m_num_values; i++) {
         m_data[i].set_tid(m_tid);
         m_data[i].set_parent(this);
      }
   }

   // get function for strings
   void odb::get(std::string &s, bool quotes, bool refresh) {
      if (refresh && is_auto_refresh_read())
         read();

      // put value into quotes
      s = "";
      std::string sd;
      for (int i = 0; i < m_num_values; i++) {
         m_data[i].get(sd);
         if (quotes)
            s += "\"";
         s += sd;
         if (quotes)
            s += "\"";
         if (i < m_num_values - 1)
            s += ",";
      }
   }

   // public functions ----------------------------------------------

   // Deep copy constructor
   odb::odb(const odb &o) : odb() {
      m_tid = o.m_tid;
      m_name = o.m_name;
      m_num_values = o.m_num_values;
      m_hKey = o.m_hKey;
      m_watch_callback = o.m_watch_callback;
      m_data = new midas::u_odb[m_num_values]{};
      for (int i = 0; i < m_num_values; i++) {
         m_data[i].set_tid(m_tid);
         m_data[i].set_parent(this);
         if (m_tid == TID_STRING || m_tid == TID_LINK) {
            // set_string() creates a copy of our string
            m_data[i].set_string(o.m_data[i]);
         } else if (m_tid == TID_KEY) {
            // recursive call to create a copy of the odb object
            midas::odb *po = o.m_data[i].get_podb();
            midas::odb *pc = new midas::odb(*po);
            pc->set_parent(this);
            m_data[i].set(pc);
         } else {
            // simply pass basic types
            m_data[i] = o.m_data[i];
            m_data[i].set_parent(this);
         }
      }
   }

   void odb::deep_copy(odb &d, const odb &s) {
      d.m_tid = s.m_tid;
      d.m_name = s.m_name;
      d.m_num_values = s.m_num_values;
      d.m_hKey = s.m_hKey;
      d.m_watch_callback = s.m_watch_callback;
      d.m_data = new midas::u_odb[d.m_num_values]{};
      for (int i = 0; i < d.m_num_values; i++) {
         d.m_data[i].set_tid(d.m_tid);
         d.m_data[i].set_parent(&d);
         if (d.m_tid == TID_STRING || d.m_tid == TID_LINK) {
            // set_string() creates a copy of our string
            d.m_data[i].set_string(s.m_data[i]);
         } else if (d.m_tid == TID_KEY) {
            // recursive call to create a copy of the odb object
            midas::odb *po = s.m_data[i].get_podb();
            midas::odb *pc = new midas::odb(*po);
            pc->set_parent(&d);
            d.m_data[i].set(pc);
         } else {
            // simply pass basic types
            d.m_data[i] = s.m_data[i];
            d.m_data[i].set_parent(this);
         }
      }
   }

   // return full path from ODB
   std::string odb::get_full_path() {
      if (m_parent)
         return m_parent->get_full_path() + "/" + m_name;

      if (!is_connected_odb() || m_hKey == -1)
         return m_name;

      char str[256];
      db_get_path(s_hDB, m_hKey, str, sizeof(str));
      if (equal_ustring(str, "/")) // change "/" to ""
         str[0] = 0;
      return str;
   }

   // return parent object
   std::string odb::get_parent_path() {
      std::string s = get_full_path();
      std::size_t i = s.find_last_of("/");
      s = s.substr(0, i);
      return s;
   }

   // return size of ODB key
   int odb::size() {
      return m_num_values;
   }

   // Resize an ODB key
   void odb::resize(int size) {
      resize_mdata(size);
      if (this->is_auto_refresh_write()) {
         int status = db_set_num_values(s_hDB, m_hKey, size);
         if (status != DB_SUCCESS)
            mthrow("db_set_num_values for ODB key \"" + get_full_path() +
                   "\" failed with status " + std::to_string(status));
      }
   }

   std::string odb::print() {
      std::string s;
      s = "{\n";
      print(s, 1);
      s += "\n}";
      return s;
   }

   std::string odb::dump() {
      std::string s;
      s = "{\n";
      dump(s, 1);
      s += "\n}";
      return s;
   }

   // print current object with all sub-objects nicely indented
   void odb::print(std::string &s, int indent) {
      for (int i = 0; i < indent; i++)
         s += "   ";
      if (m_tid == TID_KEY) {
         s += "\"" + m_name + "\": {\n";
         for (int i = 0; i < m_num_values; i++) {
            std::string v;
            // recursive call
            m_data[i].get_odb().print(v, indent + 1);
            s += v;
            if (i < m_num_values - 1)
               s += ",\n";
            else
               s += "\n";
         }
         for (int i = 0; i < indent; i++)
            s += "   ";
         s += "}";
      } else {
         s += "\"" + m_name + "\": ";
         if (m_num_values > 1)
            s += "[";
         std::string v;
         get(v, m_tid == TID_STRING || m_tid == TID_LINK);
         if (m_tid == TID_LINK)
            s += " -> ";
         s += v;
         if (m_num_values > 1)
            s += "]";
      }
   }

   // dump current object in the same way as odbedit saves as json
   void odb::dump(std::string &s, int indent) {
      for (int i = 0; i < indent; i++)
         s += "   ";
      if (m_tid == TID_KEY) {
         s += "\"" + m_name + "\": {\n";
         for (int i = 0; i < m_num_values; i++) {
            std::string v;
            m_data[i].get_odb().dump(v, indent + 1);
            s += v;
            if (i < m_num_values - 1)
               s += ",\n";
            else
               s += "\n";
         }
         for (int i = 0; i < indent; i++)
            s += "   ";
         s += "}";
      } else {
         KEY key;
         db_get_link(s_hDB, m_hKey, &key);
         s += "\"" + m_name + "/key\": ";
         s += "{ \"type\": " + std::to_string(m_tid) + ", ";
         s += "\"access_mode\": " + std::to_string(key.access_mode) + ", ";
         s += "\"last_written\": " + std::to_string(key.last_written) + "},\n";
         for (int i = 0; i < indent; i++)
            s += "   ";
         s += "\"" + m_name + "\": ";
         if (m_num_values > 1)
            s += "[";
         std::string v;
         get(v, m_tid == TID_STRING || m_tid == TID_LINK);
         s += v;
         if (m_num_values > 1)
            s += "]";
      }
   }

   // check if key contains a certain subkey
   bool odb::is_subkey(std::string str) {
      if (m_tid != TID_KEY)
         return false;

      std::string first = str;
      std::string tail{};
      if (str.find('/') != std::string::npos) {
         first = str.substr(0, str.find('/'));
         tail = str.substr(str.find('/') + 1);
      }

      int i;
      for (i = 0; i < m_num_values; i++)
         if (m_data[i].get_odb().get_name() == first)
            break;
      if (i == m_num_values)
         return false;

      if (!tail.empty())
         return m_data[i].get_odb().is_subkey(tail);

      return true;
   }

   odb &odb::get_subkey(std::string str) {
      if (m_tid == 0) {
         if (is_auto_create()) {
            m_tid = TID_KEY;
            int status = db_create_key(s_hDB, 0, m_name.c_str(), m_tid);
            if (status != DB_SUCCESS && status != DB_CREATED && status != DB_KEY_EXIST)
               mthrow("Cannot create ODB key \"" + m_name + "\", status" + std::to_string(status));
            db_find_link(s_hDB, 0, m_name.c_str(), &m_hKey);
            if (s_debug) {
               if (m_name[0] == '/')
                  std::cout << "Created ODB key \"" + m_name + "\"" << std::endl;
               else
                  std::cout << "Created ODB key \"" + get_full_path() + "\"" << std::endl;
            }
            // strip path from name
            if (m_name.find_last_of('/') != std::string::npos)
               m_name = m_name.substr(m_name.find_last_of('/') + 1);
         } else
            mthrow("Invalid key \"" + m_name + "\" does not have subkeys");

      }
      if (m_tid != TID_KEY)
         mthrow("ODB key \"" + get_full_path() + "\" does not have subkeys");

      std::string first = str;
      std::string tail{};
      if (str.find('/') != std::string::npos) {
         first = str.substr(0, str.find('/'));
         tail = str.substr(str.find('/') + 1);
      }

      int i;
      for (i = 0; i < m_num_values; i++)
         if (m_data[i].get_odb().get_name() == first)
            break;
      if (i == m_num_values) {
         if (is_auto_create()) {
            if (m_num_values == 0) {
               m_num_values = 1;
               m_data = new u_odb[1]{};
               i = 0;
            } else {
               // resize array
               resize_mdata(m_num_values + 1);
               i = m_num_values - 1;
            }
            midas::odb *o = new midas::odb();
            m_data[i].set_tid(TID_KEY);
            m_data[i].set_parent(this);
            o->set_name(get_full_path() + "/" + str);
            o->set_tid(0); // tid is currently undefined
            o->set_flags(get_flags());
            o->set_parent(this);
            m_data[i].set(o);
         } else
            mthrow("ODB key \"" + get_full_path() + "\" does not contain subkey \"" + first + "\"");
      }
      if (!tail.empty())
         return m_data[i].get_odb().get_subkey(tail);

      return *m_data[i].get_podb();
   }

   // get number of subkeys in ODB, return number and vector of names
   int odb::get_subkeys(std::vector<std::string> &name) {
      if (m_tid != TID_KEY)
         return 0;
      if (m_hKey == 0 || m_hKey == -1)
         mthrow("get_sub-keys called with invalid m_hKey for ODB key \"" + m_name + "\"");

      // count number of subkeys in ODB
      std::vector<HNDLE> hlist;
      int n = 0;
      for (int i = 0;; i++) {
         HNDLE h;
         int status = db_enum_link(s_hDB, m_hKey, i, &h);
         if (status != DB_SUCCESS)
            break;
         KEY key;
         db_get_link(s_hDB, h, &key);
         hlist.push_back(h);
         name.push_back(key.name);
         n = i + 1;
      }

      return n;
   }

   // obtain key definition from ODB and allocate local data array
   bool odb::read_key(const std::string &path) {
      init_hdb();

      int status = db_find_link(s_hDB, 0, path.c_str(), &m_hKey);
      if (status != DB_SUCCESS)
         return false;

      KEY key;
      status = db_get_link(s_hDB, m_hKey, &key);
      if (status != DB_SUCCESS)
         mthrow("db_get_link for ODB key \"" + path +
                "\" failed with status " + std::to_string(status));

      // check for correct type if given as parameter
      if (m_tid > 0 && m_tid != (int) key.type)
         mthrow("ODB key \"" + get_full_path() +
                "\" has different type than specified");

      if (s_debug)
         std::cout << "Get definition for ODB key \"" + get_full_path() + "\"" << std::endl;

      m_tid = key.type;
      m_name = key.name;
      if (m_tid == TID_KEY) {

         // merge ODB keys with local keys
         for (int i = 0; i < m_num_values; i++) {
            std::string k(path);
            if (k.back() != '/') {
               k += "/";
            }
            k += m_data[i].get_odb().get_name();
            HNDLE h;
            status = db_find_link(s_hDB, 0, k.c_str(), &h);
            if (status != DB_SUCCESS) {
               // if key does not exist in ODB write it
               m_data[i].get_odb().write_key(k, true);
               m_data[i].get_odb().write();
            } else {
               // check key type
               KEY key;
               status = db_get_link(s_hDB, h, &key);
               if (status != DB_SUCCESS)
                  mthrow("db_get_link for ODB key \"" + get_full_path() +
                         "\" failed with status " + std::to_string(status));
               if (m_data[i].get_odb().get_tid() != (int)key.type) {
                  // write key if different
                  m_data[i].get_odb().write_key(k, true);
                  m_data[i].get_odb().write();
               }
               if (m_data[i].get_odb().get_tid() == TID_KEY) {
                  // update subkey structure
                  m_data[i].get_odb().read_key(k);
               }
            }
         }

         // read back everything from ODB
         std::vector<std::string> name;
         m_num_values = get_subkeys(name);
         delete[] m_data;
         m_data = new midas::u_odb[m_num_values]{};
         for (int i = 0; i < m_num_values; i++) {
            std::string k(path);
            if (k.back() != '/') {
               k += "/";
            }
            k += name[i];
            midas::odb *o = new midas::odb(k.c_str());
            o->set_parent(this);
            m_data[i].set_tid(TID_KEY);
            m_data[i].set_parent(this);
            m_data[i].set(o);
         }
      } else  {
         m_num_values = key.num_values;
         delete[] m_data;
         m_data = new midas::u_odb[m_num_values]{};
         for (int i = 0; i < m_num_values; i++) {
            m_data[i].set_tid(m_tid);
            m_data[i].set_parent(this);
         }
      }

      return true;
   }

   // create key in ODB if it does not exist, otherwise check key type
   bool odb::write_key(std::string &path, bool force_write) {
      int status = db_find_link(s_hDB, 0, path.c_str(), &m_hKey);
      if (status != DB_SUCCESS) {
         if (m_tid == 0) // auto-create subdir
            m_tid = TID_KEY;
         if (m_tid > 0 && m_tid < TID_LAST) {
            status = db_create_key(s_hDB, 0, path.c_str(), m_tid);
            if (status != DB_SUCCESS)
               mthrow("ODB key \"" + path + "\" cannot be created");
            status = db_find_link(s_hDB, 0, path.c_str(), &m_hKey);
            if (status != DB_SUCCESS)
               mthrow("ODB key \"" + path + "\" not found after creation");
            if (s_debug) {
               if (path[0] == '/')
                  std::cout << "Created ODB key \"" + path + "\"" << std::endl;
               else
                  std::cout << "Created ODB key \"" + get_full_path() + "\"" << std::endl;
            }
         } else
            mthrow("ODB key \"" + path + "\" cannot be found");
         return true;
      } else {
         KEY key;
         status = db_get_link(s_hDB, m_hKey, &key);
         if (status != DB_SUCCESS)
            mthrow("db_get_link for ODB key \"" + path +
                   "\" failed with status " + std::to_string(status));
         if (m_tid == 0)
            m_tid = key.type;

         // check for correct type
         if (m_tid > 0 && m_tid != (int) key.type) {
            if (force_write) {
               // delete and recreate key
               status = db_delete_key(s_hDB, m_hKey, false);
               if (status != DB_SUCCESS)
                  mthrow("db_delete_key for ODB key \"" + path +
                         "\" failed with status " + std::to_string(status));
               status = db_create_key(s_hDB, 0, path.c_str(), m_tid);
               if (status != DB_SUCCESS)
                  mthrow("ODB key \"" + path + "\" cannot be created");
               status = db_find_link(s_hDB, 0, path.c_str(), &m_hKey);
               if (status != DB_SUCCESS)
                  mthrow("ODB key \"" + path + "\" not found after creation");
               if (s_debug)
                  std::cout << "Re-created ODB key \"" + get_full_path() << "\" with different type" << std::endl;
            } else
               // abort
               mthrow("ODB key \"" + get_full_path() +
                      "\" has differnt type than specified");
         } else if (s_debug)
            std::cout << "Validated ODB key \"" + get_full_path() + "\"" << std::endl;

         return false;
      }
   }


   // retrieve data from ODB and assign it to this object
   void odb::read() {
      if (!is_connected_odb())
         return;

      // check if deleted
      if (is_deleted())
         mthrow("ODB key \"" + m_name + "\" cannot be pulled because it has been deleted");

      if (m_hKey == 0)
         return; // needed to print un-connected objects

      if (m_tid == 0)
         mthrow("Read of invalid ODB key \"" + m_name + "\"");

      if (m_hKey == -1) {
         // connect un-connected object (crated via XML)
         std::string path = get_full_path();

         int status = db_find_link(s_hDB, 0, path.c_str(), &m_hKey);
         if (status != DB_SUCCESS)
            mthrow("Cannot connect key \"" + path + "\" to ODB");
      }

      int status{};
      if (m_tid == TID_STRING || m_tid == TID_LINK) {
         KEY key;
         db_get_link(s_hDB, m_hKey, &key);
         char *str = (char *) malloc(key.total_size);
         int size = key.total_size;
         if (m_tid == TID_LINK)
            status = db_get_link_data(s_hDB, m_hKey, str, &size, m_tid);
         else
            status = db_get_data(s_hDB, m_hKey, str, &size, m_tid);
         for (int i = 0; i < m_num_values; i++)
            m_data[i].set(str + i * key.item_size);
         free(str);
      } else if (m_tid == TID_KEY) {
         std::vector<std::string> name;
         int n = get_subkeys(name);
         if (n != m_num_values) {
            // if subdirs have changed, rebuild it
            delete[] m_data;
            m_num_values = n;
            m_data = new midas::u_odb[m_num_values]{};
            for (int i = 0; i < m_num_values; i++) {
               std::string k(get_full_path());
               k += "/" + name[i];
               midas::odb *o = new midas::odb(k.c_str());
               o->set_parent(this);
               m_data[i].set_tid(TID_KEY);
               m_data[i].set_parent(this);
               m_data[i].set(o);
            }
         }
         for (int i = 0; i < m_num_values; i++)
            m_data[i].get_odb().read();
         status = DB_SUCCESS;
      } else {
         // resize local array if number of values has changed
         KEY key;
         status = db_get_link(s_hDB, m_hKey, &key);
         if (key.num_values != m_num_values) {
            delete[] m_data;
            m_num_values = key.num_values;
            m_data = new midas::u_odb[m_num_values]{};
            for (int i = 0; i < m_num_values; i++) {
               m_data[i].set_tid(m_tid);
               m_data[i].set_parent(this);
            }
         }

         int size = rpc_tid_size(m_tid) * m_num_values;
         void *buffer = malloc(size);
         void *p = buffer;
         status = db_get_data(s_hDB, m_hKey, p, &size, m_tid);
         for (int i = 0; i < m_num_values; i++) {
            if (m_tid == TID_UINT8)
               m_data[i].set(*static_cast<uint8_t *>(p));
            else if (m_tid == TID_INT8)
               m_data[i].set(*static_cast<int8_t *>(p));
            else if (m_tid == TID_UINT16)
               m_data[i].set(*static_cast<uint16_t *>(p));
            else if (m_tid == TID_INT16)
               m_data[i].set(*static_cast<int16_t *>(p));
            else if (m_tid == TID_UINT32)
               m_data[i].set(*static_cast<uint32_t *>(p));
            else if (m_tid == TID_INT32)
               m_data[i].set(*static_cast<int32_t *>(p));
            else if (m_tid == TID_UINT64)
               m_data[i].set(*static_cast<uint64_t *>(p));
            else if (m_tid == TID_INT64)
               m_data[i].set(*static_cast<int64_t *>(p));
            else if (m_tid == TID_BOOL)
               m_data[i].set(*static_cast<bool *>(p));
            else if (m_tid == TID_FLOAT)
               m_data[i].set(*static_cast<float *>(p));
            else if (m_tid == TID_DOUBLE)
               m_data[i].set(*static_cast<double *>(p));
            else if (m_tid == TID_STRING)
               m_data[i].set(std::string(static_cast<const char *>(p)));
            else if (m_tid == TID_LINK)
               m_data[i].set(std::string(static_cast<const char *>(p)));
            else
               mthrow("Invalid type ID " + std::to_string(m_tid));

            p = static_cast<char *>(p) + rpc_tid_size(m_tid);
         }
         free(buffer);
      }

      if (status != DB_SUCCESS)
         mthrow("db_get_data for ODB key \"" + get_full_path() +
                "\" failed with status " + std::to_string(status));
      if (s_debug) {
         if (m_tid == TID_KEY) {
            std::cout << "Get ODB key \"" + get_full_path() + "[0..." +
                         std::to_string(m_num_values - 1) + "]\"" << std::endl;
         } else {
            std::string s;
            get(s, false, false);
            if (m_num_values > 1) {
               if (m_tid == TID_STRING || m_tid == TID_LINK)
                  std::cout << "Get ODB key \"" + get_full_path() + "[0..." +
                               std::to_string(m_num_values - 1) + "]\": [\"" + s + "\"]" << std::endl;
               else
                  std::cout << "Get ODB key \"" + get_full_path() + "[0..." +
                               std::to_string(m_num_values - 1) + "]\": [" + s + "]" << std::endl;
            } else {
               if (m_tid == TID_STRING || m_tid == TID_LINK)
                  std::cout << "Get ODB key \"" + get_full_path() + "\": \"" + s + "\"" << std::endl;
               else
                  std::cout << "Get ODB key \"" + get_full_path() + "\": " + s << std::endl;
            }
         }
      }
   }

   // retrieve individual member of array
   void odb::read(int index) {
      if (!is_connected_odb())
         return;

      if (m_hKey == 0 || m_hKey == -1)
         return; // needed to print un-connected objects

      if (m_tid == 0)
         mthrow("Pull of invalid ODB key \"" + m_name + "\"");

      int status{};
      if (m_tid == TID_STRING || m_tid == TID_LINK) {
         KEY key;
         db_get_link(s_hDB, m_hKey, &key);
         char *str = (char *) malloc(key.item_size);
         int size = key.item_size;
         status = db_get_data_index(s_hDB, m_hKey, str, &size, index, m_tid);
         m_data[index].set(str);
         free(str);
      } else if (m_tid == TID_KEY) {
         m_data[index].get_odb().read();
         status = DB_SUCCESS;
      } else {
         int size = rpc_tid_size(m_tid);
         void *buffer = malloc(size);
         void *p = buffer;
         status = db_get_data_index(s_hDB, m_hKey, p, &size, index, m_tid);
         if (m_tid == TID_UINT8)
            m_data[index].set(*static_cast<uint8_t *>(p));
         else if (m_tid == TID_INT8)
            m_data[index].set(*static_cast<int8_t *>(p));
         else if (m_tid == TID_UINT16)
            m_data[index].set(*static_cast<uint16_t *>(p));
         else if (m_tid == TID_INT16)
            m_data[index].set(*static_cast<int16_t *>(p));
         else if (m_tid == TID_UINT32)
            m_data[index].set(*static_cast<uint32_t *>(p));
         else if (m_tid == TID_INT32)
            m_data[index].set(*static_cast<int32_t *>(p));
         else if (m_tid == TID_UINT64)
            m_data[index].set(*static_cast<uint64_t *>(p));
         else if (m_tid == TID_INT64)
            m_data[index].set(*static_cast<int64_t *>(p));
         else if (m_tid == TID_BOOL)
            m_data[index].set(*static_cast<bool *>(p));
         else if (m_tid == TID_FLOAT)
            m_data[index].set(*static_cast<float *>(p));
         else if (m_tid == TID_DOUBLE)
            m_data[index].set(*static_cast<double *>(p));
         else if (m_tid == TID_STRING)
            m_data[index].set(std::string(static_cast<const char *>(p)));
         else if (m_tid == TID_LINK)
            m_data[index].set(std::string(static_cast<const char *>(p)));
         else
            mthrow("Invalid type ID " + std::to_string(m_tid));

         free(buffer);
      }

      if (status != DB_SUCCESS)
         mthrow("db_get_data for ODB key \"" + get_full_path() +
                "\" failed with status " + std::to_string(status));
      if (s_debug) {
         std::string s;
         m_data[index].get(s);
         if (m_tid == TID_STRING || m_tid == TID_LINK)
            std::cout << "Get ODB key \"" + get_full_path() + "[" +
                         std::to_string(index) + "]\": [\"" + s + "\"]" << std::endl;
         else
            std::cout << "Get ODB key \"" + get_full_path() + "[" +
                         std::to_string(index) + "]\": [" + s + "]" << std::endl;
      }
   }

   // push individual member of an array
   void odb::write(int index, int str_size) {
      if (!is_connected_odb())
         return;

      if (m_hKey == -1) {
         // connect un-connected object (crated via XML)
         std::string path = get_full_path();

         int status = db_find_link(s_hDB, 0, path.c_str(), &m_hKey);
         if (status != DB_SUCCESS)
            mthrow("Cannot connect key \"" + path + "\" to ODB");

      } else if (m_hKey == 0) {
         if (is_auto_create()) {
            std::string to_create = m_name[0] == '/' ? m_name : get_full_path();
            int status = db_create_key(s_hDB, 0, to_create.c_str(), m_tid);
            if (status != DB_SUCCESS && status != DB_CREATED && status != DB_KEY_EXIST)
               mthrow("Cannot create ODB key \"" + to_create + "\", status =" + std::to_string(status));
            db_find_link(s_hDB, 0, to_create.c_str(), &m_hKey);
            if (s_debug) {
               std::cout << "Created ODB key \"" + to_create + "\"" << std::endl;
            }
            // strip path from name
            if (m_name.find_last_of('/') != std::string::npos)
               m_name = m_name.substr(m_name.find_last_of('/') + 1);
         } else
            mthrow("Write of un-connected ODB key \"" + m_name + "\" not possible");
      }

      // don't write keys
      if (m_tid == TID_KEY)
         return;

      int status{};
      if (m_tid == TID_STRING || m_tid == TID_LINK) {
         KEY key;
         db_get_link(s_hDB, m_hKey, &key);
         std::string s;
         m_data[index].get(s);
         if (m_num_values == 1) {
            int size = key.item_size;
            if (key.item_size == 0 || !is_preserve_string_size())
               size = s.size() + 1;
            if (str_size > 0)
               size = str_size;
            char *ss = (char *)malloc(size+1);
            strlcpy(ss, s.c_str(), size);
            if (is_trigger_hotlink())
               status = db_set_data(s_hDB, m_hKey, ss, size, 1, m_tid);
            else
               status = db_set_data1(s_hDB, m_hKey, ss, size, 1, m_tid);
            free(ss);
         } else {
            if (key.item_size == 0)
               key.item_size = s.size() + 1;
            if (str_size > 0) {
               if (key.item_size > 0 && key.item_size != str_size) {
                  std::cout << "ODB string size mismatch for \"" << get_full_path() <<
                  "\" (" << key.item_size << " vs " << str_size << "). ODB key recreated."
                            << std::endl;
                  if (is_trigger_hotlink())
                     status = db_set_data(s_hDB, m_hKey, s.c_str(), str_size, 1, m_tid);
                  else
                     status = db_set_data1(s_hDB, m_hKey, s.c_str(), str_size, 1, m_tid);
               }
               key.item_size = str_size;
            }
            status = db_set_data_index1(s_hDB, m_hKey, s.c_str(), key.item_size, index, m_tid, is_trigger_hotlink());
         }
         if (s_debug) {
            if (m_num_values > 1)
               std::cout << "Set ODB key \"" + get_full_path() + "[" + std::to_string(index) + "]\" = \"" + s
                         + "\"" << std::endl;
            else
               std::cout << "Set ODB key \"" + get_full_path() + "\" = \"" + s + "\""<< std::endl;
         }
      } else {
         u_odb u = m_data[index];
         if (m_tid == TID_BOOL) {
            u.set_parent(nullptr);
            BOOL b = static_cast<bool>(u); // "bool" is only 1 Byte, BOOL is 4 Bytes
            status = db_set_data_index1(s_hDB, m_hKey, &b, rpc_tid_size(m_tid), index, m_tid, is_trigger_hotlink());
         } else {
            status = db_set_data_index1(s_hDB, m_hKey, &u, rpc_tid_size(m_tid), index, m_tid, is_trigger_hotlink());
         }
         if (s_debug) {
            std::string s;
            u.get(s);
            if (m_num_values > 1)
               std::cout << "Set ODB key \"" + get_full_path() + "[" + std::to_string(index) + "]\" = " + s
                         << std::endl;
            else
               std::cout << "Set ODB key \"" + get_full_path() + "\" = " + s << std::endl;
         }
      }
      if (status != DB_SUCCESS)
         mthrow("db_set_data_index for ODB key \"" + get_full_path() +
                "\" failed with status " + std::to_string(status));
   }

   // write all members of an array to the ODB
   void odb::write(int str_size) {

      // check if deleted
      if (is_deleted())
         mthrow("ODB key \"" + m_name + "\" cannot be written because it has been deleted");

      // write subkeys
      if (m_tid == TID_KEY) {
         for (int i = 0; i < m_num_values; i++)
            m_data[i].get_odb().write();
         return;
      }

      if (m_tid == 0 && m_data[0].get_tid() != 0)
         m_tid = m_data[0].get_tid();

      if (m_tid < 1 || m_tid >= TID_LAST)
         mthrow("Invalid TID for ODB key \"" + get_full_path() + "\"");

      if ((m_hKey == 0  || m_hKey == -1) && !is_auto_create())
         mthrow("Writing ODB key \"" + m_name +
                "\" is not possible because of invalid key handle");

      // if index operator [] returned previously a certain index, write only this one
      if (m_last_index != -1) {
         write(m_last_index, str_size);
         m_last_index = -1;
         return;
      }

      if (m_num_values == 1) {
         write(0, str_size);
         return;
      }

      if (m_hKey == -1) {
         // connect un-connected object (crated via XML)
         std::string path = get_full_path();

         int status = db_find_link(s_hDB, 0, path.c_str(), &m_hKey);
         if (status != DB_SUCCESS)
            mthrow("Cannot connect key \"" + path + "\" to ODB");

      } else if (m_hKey == 0) {
         if (is_auto_create()) {
            std::string to_create = m_name[0] == '/' ? m_name : get_full_path();
            int status = db_create_key(s_hDB, 0, to_create.c_str(), m_tid);
            if (status != DB_SUCCESS && status != DB_CREATED && status != DB_KEY_EXIST)
               mthrow("Cannot create ODB key \"" + to_create + "\", status" + std::to_string(status));
            db_find_link(s_hDB, 0, to_create.c_str(), &m_hKey);
            if (s_debug) {
               std::cout << "Created ODB key \"" + to_create + "\"" << std::endl;
            }
            // strip path from name
            if (m_name.find_last_of('/') != std::string::npos)
               m_name = m_name.substr(m_name.find_last_of('/') + 1);
         } else
            mthrow("Write of un-connected ODB key \"" + m_name + "\" not possible");
      }

      int status{};
      if (m_tid == TID_STRING || m_tid == TID_LINK) {
         if (is_preserve_string_size() || m_num_values > 1) {
            KEY key;
            db_get_link(s_hDB, m_hKey, &key);
            if (key.item_size == 0 || key.total_size == 0) {
               int size = 1;
               for (int i = 0; i < m_num_values; i++) {
                  std::string d;
                  m_data[i].get(d);
                  if ((int) d.size() + 1 > size)
                     size = d.size() + 1;
               }
               // round up to multiples of 32
               size = (((size - 1) / 32) + 1) * 32;
               key.item_size = size;
               key.total_size = size * m_num_values;
            }
            char *str = (char *) malloc(key.item_size * m_num_values);
            for (int i = 0; i < m_num_values; i++) {
               std::string d;
               m_data[i].get(d);
               strncpy(str + i * key.item_size, d.c_str(), key.item_size);
            }
            if (is_trigger_hotlink())
               status = db_set_data(s_hDB, m_hKey, str, key.item_size * m_num_values, m_num_values, m_tid);
            else
               status = db_set_data1(s_hDB, m_hKey, str, key.item_size * m_num_values, m_num_values, m_tid);
            free(str);
            if (s_debug) {
               std::string s;
               get(s, true, false);
               std::cout << "Set ODB key \"" + get_full_path() +
                            "[0..." + std::to_string(m_num_values - 1) + "]\" = [" + s + "]" << std::endl;
            }
         } else {
            std::string s;
            m_data[0].get(s);
            if (is_trigger_hotlink())
               status = db_set_data(s_hDB, m_hKey, s.c_str(), s.length() + 1, 1, m_tid);
            else
               status = db_set_data1(s_hDB, m_hKey, s.c_str(), s.length() + 1, 1, m_tid);
            if (s_debug)
               std::cout << "Set ODB key \"" + get_full_path() + "\" = " + s << std::endl;
         }
      } else {
         int size = rpc_tid_size(m_tid) * m_num_values;
         uint8_t *buffer = (uint8_t *) malloc(size);
         uint8_t *p = buffer;
         for (int i = 0; i < m_num_values; i++) {
            if (m_tid == TID_BOOL) {
               // bool has 1 Byte, BOOL has 4 Bytes
               BOOL b = static_cast<bool>(m_data[i]);
               memcpy(p, &b, rpc_tid_size(m_tid));
            } else {
               memcpy(p, (void*)&m_data[i], rpc_tid_size(m_tid));
            }
            p += rpc_tid_size(m_tid);
         }
         if (is_trigger_hotlink())
            status = db_set_data(s_hDB, m_hKey, buffer, size, m_num_values, m_tid);
         else
            status = db_set_data1(s_hDB, m_hKey, buffer, size, m_num_values, m_tid);
         free(buffer);
         if (s_debug) {
            std::string s;
            get(s, false, false);
            if (m_num_values > 1)
               std::cout << "Set ODB key \"" + get_full_path() + "[0..." + std::to_string(m_num_values - 1) +
                            "]\" = [" + s + "]" << std::endl;
            else
               std::cout << "Set ODB key \"" + get_full_path() + "\" = " + s << std::endl;
         }
      }

      if (status != DB_SUCCESS)
         mthrow("db_set_data for ODB key \"" + get_full_path() +
                "\" failed with status " + std::to_string(status));
   }

   void recurse_del_keys_not_in_defaults(std::string path, HNDLE hDB, HNDLE hKey, midas::odb& default_odb) {
      // Delete any subkeys that are not in the list of defaults.
      KEY key;
      db_get_link(hDB, hKey, &key);

      if (key.type == TID_KEY) {
         std::vector<std::string> to_delete;

         for (int i = 0;; i++) {
            HNDLE hSubKey;
            int status = db_enum_link(hDB, hKey, i, &hSubKey);
            if (status != DB_SUCCESS)
               break;

            KEY subKey;
            db_get_link(hDB, hSubKey, &subKey);
            std::string full_path = path + "/" + subKey.name;

            if (!default_odb.is_subkey(subKey.name)) {
               to_delete.push_back(subKey.name);

               if (default_odb.get_debug()) {
                  std::cout << "Deleting " << full_path << " as not in list of defaults" << std::endl;
               }
            } else if (key.type == TID_KEY) {
               recurse_del_keys_not_in_defaults(full_path, hDB, hSubKey, default_odb[subKey.name]);
            }
         }

         for (auto name : to_delete) {
            HNDLE hSubKey;
            db_find_link(hDB, hKey, name.c_str(), &hSubKey);
            db_delete_key(hDB, hSubKey, FALSE);
         }
      }
   }

   void recurse_get_defaults_order(std::string path, midas::odb& default_odb, std::map<std::string, std::vector<std::string> >& retval) {
      for (midas::odb& sub : default_odb) {
         if (sub.get_tid() == TID_KEY) {
            recurse_get_defaults_order(path + "/" + sub.get_name(), sub, retval);
         }

         retval[path].push_back(sub.get_name());
      }
   }

   void recurse_fix_order(midas::odb& default_odb, std::map<std::string, std::vector<std::string> >& user_order) {
      std::string path = default_odb.get_full_path();

      if (user_order.find(path) != user_order.end()) {
         default_odb.fix_order(user_order[path]);
      }

      for (midas::odb& it : default_odb) {
         if (it.get_tid() == TID_KEY) {
            recurse_fix_order(it, user_order);
         }
      }
   }

   void odb::fix_order(std::vector<std::string> target_order) {
      // Fix the order of ODB keys to match that specified in target_order.
      // The in-ODB representation is simple, as we can just use db_reorder_key()
      // on anything that's in the wrong place.
      // The in-memory representation is a little trickier, but we just copy raw
      // memory into a temporary array, so we don't have to delete/recreate the
      // u_odb objects.
      std::vector<std::string> curr_order;

      if (get_subkeys(curr_order) <= 0) {
         // Not a TID_KEY (or no keys)
         return;
      }

      if (target_order.size() != curr_order.size() || (int)target_order.size() != m_num_values) {
         return;
      }

      HNDLE hKey = get_hkey();
      bool force_order = false;

      // Temporary location where we'll store in-memory u_odb objects in th
      // correct order.
      u_odb* new_m_data = new u_odb[m_num_values];

      for (int i = 0; i < m_num_values; i++) {
         if (force_order || curr_order[i] != target_order[i]) {
            force_order = true;
            HNDLE hSubKey;

            // Fix the order in the ODB
            db_find_key(s_hDB, hKey, target_order[i].c_str(), &hSubKey);
            db_reorder_key(s_hDB, hSubKey, i);
         }

         // Fix the order in memory
         auto curr_it = std::find(curr_order.begin(), curr_order.end(), target_order[i]);

         if (curr_it == curr_order.end()) {
            // Logic error - bail to avoid doing any damage to the in-memory version.
            delete[] new_m_data;
            return;
         }

         int curr_idx = curr_it - curr_order.begin();
         new_m_data[i] = m_data[curr_idx];
      }

      // Final update of the in-memory version so they are in the correct order
      for (int i = 0; i < m_num_values; i++) {
         m_data[i] = new_m_data[i];

         // Nullify pointers that point to the same object in
         // m_data and new_m_data, so the underlying object doesn't
         // get destroyed when we delete new_m_data.
         new_m_data[i].set_string_ptr(nullptr);
         new_m_data[i].set_odb(nullptr);
      }

      delete[] new_m_data;
   }

   // connect function with separated path and key name
   void odb::connect(const std::string &p, const std::string &name, bool write_defaults, bool delete_keys_not_in_defaults) {
      init_hdb();

      if (!name.empty())
         m_name = name;
      std::string path(p);

      if (path.empty())
         mthrow("odb::connect() cannot be called with an empty ODB path");

      if (path[0] != '/')
         mthrow("odb::connect(\"" + path + "\"): path must start with leading \"/\"");

      if (path.back() != '/')
         path += "/";

      path += m_name;

      HNDLE hKey;
      int status = db_find_link(s_hDB, 0, path.c_str(), &hKey);
      bool key_exists = (status == DB_SUCCESS);
      bool created = false;

      if (key_exists && delete_keys_not_in_defaults) {
         // Recurse down to delete keys as needed.
         // We need to do this recursively BEFORE calling read/read_key for the first time
         // to ensure that subdirectories get handled correctly.
         recurse_del_keys_not_in_defaults(path, s_hDB, hKey, *this);
      }

      if (!key_exists || write_defaults) {
         created = write_key(path, write_defaults);
      } else {
         read_key(path);
      }

      // correct wrong parent ODB from initializer_list
      for (int i = 0; i < m_num_values; i++)
         m_data[i].set_parent(this);

      if (m_tid == TID_KEY) {
         for (int i = 0; i < m_num_values; i++)
            m_data[i].get_odb().connect(get_full_path(), m_data[i].get_odb().get_name(), write_defaults);
      } else if (created || write_defaults) {
         write();
      } else {
         read();
      }
   }

   // send key definitions and data with optional subkeys to certain path in ODB
   void odb::connect(std::string str, bool write_defaults, bool delete_keys_not_in_defaults) {

      if (str.empty())
         mthrow("odb::connect() cannot be called with an empty ODB path");

      if (str[0] != '/')
         mthrow("odb::connect(\"" + str + "\"): path must start with leading \"/\"");

      if (str == "/")
         mthrow("odb::connect(\"" + str + "\"): root ODB tree is not allowed");

      if (str.back() == '/')
         str = str.substr(0, str.size()-1);

      // separate ODB path and key nam
      std::string name;
      std::string path;
      name = str.substr(str.find_last_of('/') + 1);
      path = str.substr(0, str.find_last_of('/') + 1);

      connect(path, name, write_defaults, delete_keys_not_in_defaults);
   }

   // shorthand for the same behavior as db_check_record:
   // - keep values of keys that already exist with the correct type
   // - add keys that user provided but aren't in ODB already
   // - delete keys that are in ODB but not in user's settings
   // - re-order ODB keys to match user's order
   void odb::connect_and_fix_structure(std::string path) {
      // Store the order the user specified.
      // Need to do this recursively before calling connect(), as the first
      // read() in that function merges user keys and existing keys.
      std::map<std::string, std::vector<std::string> > user_order;
      recurse_get_defaults_order(path, *this, user_order);

      // Main connect() that adds/deletes/updates keys as needed.
      connect(path, false, true);

      // Fix order in ODB (and memory)
      recurse_fix_order(*this, user_order);
   }

   void odb::delete_key() {
      init_hdb();

      if (this->is_write_protect())
         mthrow("Cannot modify write protected key \"" + get_full_path() + "\"");

      // delete key in ODB
      int status = db_delete_key(s_hDB, m_hKey, FALSE);
      if (status != DB_SUCCESS && status != DB_INVALID_HANDLE)
         mthrow("db_delete_key for ODB key \"" + get_full_path() +
                "\" returnd error code " + std::to_string(status));

      if (s_debug)
         std::cout << "Deleted ODB key \"" + get_full_path() + "\"" << std::endl;

      // invalidate this object
      delete[] m_data;
      m_data = nullptr;
      m_num_values = 0;
      m_tid = 0;
      m_hKey = 0;

      // set flag that this object has been deleted
      set_deleted(true);
   }

   void odb::set_mode(int mode) {
      // set mode of ODB key
      // default is MODE_READ | MODE_WRITE | MODE_DELETE

      init_hdb();

      // set mode in ODB
      int status = db_set_mode(s_hDB, m_hKey, mode, TRUE);

      if (status != DB_SUCCESS && status != DB_INVALID_HANDLE)
         mthrow("db_set_mode for ODB key \"" + get_full_path() +
                "\" returnd error code " + std::to_string(status));

      if (s_debug)
         std::cout << "Set mode of ODB key \"" + get_full_path() + "\" to " << mode << std::endl;
   }

   int odb::get_mode() {
      init_hdb();

      // set mode in ODB
      KEY key;
      int status = db_get_key(s_hDB, m_hKey, &key);

      if (status != DB_SUCCESS && status != DB_INVALID_HANDLE)
         mthrow("db_get_key for ODB key \"" + get_full_path() +
                "\" returnd error code " + std::to_string(status));

      return key.access_mode;
   }

   unsigned int odb::get_last_written() {
      init_hdb();

      // set mode in ODB
      KEY key;
      int status = db_get_key(s_hDB, m_hKey, &key);

      if (status != DB_SUCCESS && status != DB_INVALID_HANDLE)
         mthrow("db_get_key for ODB key \"" + get_full_path() +
                "\" returnd error code " + std::to_string(status));

      return (unsigned int) (key.last_written);
   }

   void odb::watch(std::function<void(midas::odb &)> f) {
      if (m_hKey == 0 || m_hKey == -1)
         mthrow("watch() called for ODB key \"" + m_name +
                "\" which is not connected to ODB");

      // create a deep copy of current object in case it
      // goes out of scope
      midas::odb* ow = new midas::odb(*this);

      ow->m_watch_callback = f;
      db_watch(s_hDB, m_hKey, midas::odb::watch_callback, ow);

      // put object into watchlist
      g_watchlist.push_back(ow);
   }

   void odb::unwatch()
   {
      for (int i=0 ; i<(int) g_watchlist.size() ; i++) {
         if (g_watchlist[i]->get_hkey() == this->get_hkey()) {
            db_unwatch(s_hDB, g_watchlist[i]->get_hkey());
            delete g_watchlist[i];
            g_watchlist.erase(g_watchlist.begin() + i);
            i--;
         }
      }
   }

   void odb::unwatch_all()
   {
      for (int i=0 ; i<(int) g_watchlist.size() ; i++) {
         db_unwatch(s_hDB, g_watchlist[i]->get_hkey());
         delete g_watchlist[i];
      }
      g_watchlist.clear();
   }

   void odb::set(std::string s)
   {
      if (this->is_write_protect())
         mthrow("Cannot modify write protected key \"" + get_full_path() + "\"");

      if (m_tid == TID_BOOL)
         s = (s == "y" || s == "1") ? "1" : "0";

      m_num_values = 1;
      m_data = new u_odb[1];
      m_data[0].set_parent(this);
      m_data[0].set_tid(m_tid);
      m_data[0].set(s);
   }

   void odb::set(std::string s, int i)
   {
      if (this->is_write_protect())
         mthrow("Cannot modify write protected key \"" + get_full_path() + "\"");

      if (m_tid == TID_BOOL)
         s = (s == "y" || s == "1") ? "1" : "0";

      if (m_data == nullptr)
         m_data = new u_odb[m_num_values];
      m_data[i].set_parent(this);
      m_data[i].set_tid(m_tid);
      m_data[i].set(s);
   }

   void odb::set_string_size(std::string s, int size)
   {
      if (this->is_write_protect())
         mthrow("Cannot modify write protected key \"" + get_full_path() + "\"");

      m_num_values = 1;
      m_tid = TID_STRING;
      m_data = new u_odb[1];
      m_data[0].set_parent(this);
      m_data[0].set_tid(m_tid);
      m_data[0].set_string_size(s, size);
      set_preserve_string_size(true);
   }

   void odb::set_odb(odb *o, int i)
   {
      if (this->is_write_protect())
         mthrow("Cannot modify write protected key \"" + get_full_path() + "\"");

      if (m_data == nullptr)
         m_data = new u_odb[m_num_values];
      m_data[i].set_parent(this);
      m_data[i].set_tid(m_tid);
      m_data[i].set_odb(o);
   }

   //-----------------------------------------------

   //---- u_odb implementations calling functions from odb

   u_odb::~u_odb() {
      if (m_tid == TID_STRING || m_tid == TID_LINK)
         delete m_string;
      else if (m_tid == TID_KEY)
         delete m_odb;
   }

   // get function for strings
   void u_odb::get(std::string &s) {
      if (m_tid == TID_UINT8)
         s = std::to_string(m_uint8);
      else if (m_tid == TID_INT8)
         s = std::to_string(m_int8);
      else if (m_tid == TID_UINT16)
         s = std::to_string(m_uint16);
      else if (m_tid == TID_INT16)
         s = std::to_string(m_int16);
      else if (m_tid == TID_UINT32)
         s = std::to_string(m_uint32);
      else if (m_tid == TID_INT32)
         s = std::to_string(m_int32);
      else if (m_tid == TID_UINT64)
         s = std::to_string(m_uint64);
      else if (m_tid == TID_INT64)
         s = std::to_string(m_int64);
      else if (m_tid == TID_BOOL)
         s = std::string(m_bool ? "true" : "false");
      else if (m_tid == TID_FLOAT)
         s = std::to_string(m_float);
      else if (m_tid == TID_DOUBLE)
         s = std::to_string(m_double);
      else if (m_tid == TID_STRING)
         s = *m_string;
      else if (m_tid == TID_LINK)
         s = *m_string;
      else if (m_tid == TID_KEY)
         m_odb->print(s, 0);
      else if (m_tid == 0)
         mthrow("Subkey \"" + m_parent_odb->get_name() + "\" not found");
      else
         mthrow("Invalid type ID " + std::to_string(m_tid));
   }

   //---- u_odb assignment and arithmetic operators overloads which call odb::write()

   // overload assignment operators
   uint8_t u_odb::operator=(uint8_t v) {
      if (m_tid == 0)
         m_tid = TID_UINT8;
      set(v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
      return v;
   }
   int8_t u_odb::operator=(int8_t v) {
      if (m_tid == 0)
         m_tid = TID_INT8;
      set(v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
      return v;
   }
   uint16_t u_odb::operator=(uint16_t v) {
      if (m_tid == 0)
         m_tid = TID_UINT16;
      set(v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
      return v;
   }
   int16_t u_odb::operator=(int16_t v) {
      if (m_tid == 0)
         m_tid = TID_INT16;
      set(v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
      return v;
   }
   uint32_t u_odb::operator=(uint32_t v) {
      if (m_tid == 0)
         m_tid = TID_UINT32;
      set(v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
      return v;
   }
   int32_t u_odb::operator=(int32_t v) {
      if (m_tid == 0)
         m_tid = TID_INT32;
      set(v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
      return v;
   }
   uint64_t u_odb::operator=(uint64_t v) {
      if (m_tid == 0)
         m_tid = TID_UINT64;
      set(v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
      return v;
   }
   int64_t u_odb::operator=(int64_t v) {
      if (m_tid == 0)
         m_tid = TID_INT64;
      set(v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
      return v;
   }
   bool u_odb::operator=(bool v) {
      if (m_tid == 0)
         m_tid = TID_BOOL;
      set(v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
      return v;
   }
   float u_odb::operator=(float v) {
      if (m_tid == 0)
         m_tid = TID_FLOAT;
      set(v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
      return v;
   }
   double u_odb::operator=(double v) {
      if (m_tid == 0)
         m_tid = TID_DOUBLE;
      set(v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
      return v;
   }
   const char * u_odb::operator=(const char * v) {
      if (m_tid == 0)
         m_tid = TID_STRING;
      set(v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
      return v;
   }

   std::string * u_odb::operator=(std::string * v){
      if (m_tid == 0)
         m_tid = TID_STRING;
       set(*v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
          m_parent_odb->write();
       return v;
   }

   std::string u_odb::operator=(std::string v){
      if (m_tid == 0)
         m_tid = TID_STRING;
      set(v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
      return v;
   }

   void u_odb::set_string_size(std::string v, int size) {
      m_tid = TID_STRING;
      set(v);
      if (m_parent_odb && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write(size);
   }

   // overload all standard conversion operators
   u_odb::operator uint8_t() {
      if (m_parent_odb)
         m_parent_odb->set_last_index(-1);
      return get<uint8_t>();
   }
   u_odb::operator int8_t() {
      if (m_parent_odb)
         m_parent_odb->set_last_index(-1);
      return get<int8_t>();
   }
   u_odb::operator uint16_t() {
      if (m_parent_odb)
         m_parent_odb->set_last_index(-1);
      return get<uint16_t>();
   }
   u_odb::operator int16_t() {
      if (m_parent_odb)
         m_parent_odb->set_last_index(-1);
      return get<int16_t>();
   }
   u_odb::operator uint32_t() {
      if (m_parent_odb)
         m_parent_odb->set_last_index(-1);
      return get<uint32_t>();
   }
   u_odb::operator int32_t() {
      if (m_parent_odb)
         m_parent_odb->set_last_index(-1);
      return get<int32_t>();
   }
   u_odb::operator uint64_t() {
      if (m_parent_odb)
         m_parent_odb->set_last_index(-1);
      return get<uint64_t>();
   }
   u_odb::operator int64_t() {
      if (m_parent_odb)
         m_parent_odb->set_last_index(-1);
      return get<int64_t>();
   }
   u_odb::operator bool() {
      if (m_parent_odb)
         m_parent_odb->set_last_index(-1);
      return get<bool>();
   }
   u_odb::operator float() {
      if (m_parent_odb)
         m_parent_odb->set_last_index(-1);
      return get<float>();
   }
   u_odb::operator double() {
      if (m_parent_odb)
         m_parent_odb->set_last_index(-1);
      return get<double>();
   }
   u_odb::operator std::string() {
      if (m_parent_odb)
         m_parent_odb->set_last_index(-1);
      std::string s;
      get(s);
      return s;
   }
   u_odb::operator const char *() {
      if (m_parent_odb)
         m_parent_odb->set_last_index(-1);
      if (m_tid != TID_STRING && m_tid != TID_LINK)
         mthrow("Only ODB string keys can be converted to \"const char *\"");
      return m_string->c_str();
   }
   u_odb::operator midas::odb&() {
      if (m_parent_odb)
         m_parent_odb->set_last_index(-1);
      if (m_tid != TID_KEY)
         mthrow("Only ODB directories can be converted to \"midas::odb &\"");
      return *m_odb;
   }


   void u_odb::add(double inc, bool write) {
      if (m_tid == TID_UINT8)
         m_uint8 += inc;
      else if (m_tid == TID_INT8)
         m_int8 += inc;
      else if (m_tid == TID_UINT16)
         m_uint16 += inc;
      else if (m_tid == TID_INT16)
         m_int16 += inc;
      else if (m_tid == TID_UINT32)
         m_uint32 += inc;
      else if (m_tid == TID_INT32)
         m_int32 += inc;
      else if (m_tid == TID_FLOAT)
         m_float += static_cast<float>(inc);
      else if (m_tid == TID_DOUBLE)
         m_double += inc;
      else
         mthrow("Invalid arithmetic operation for ODB key \"" +
                m_parent_odb->get_full_path() + "\"");
      if (write && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
   }

   void u_odb::mult(double f, bool write) {
      int tid = m_parent_odb->get_tid();
      if (tid == TID_UINT8)
         m_uint8 *= f;
      else if (tid == TID_INT8)
         m_int8 *= f;
      else if (tid == TID_UINT16)
         m_uint16 *= f;
      else if (tid == TID_INT16)
         m_int16 *= f;
      else if (tid == TID_UINT32)
         m_uint32 *= f;
      else if (tid == TID_INT32)
         m_int32 *= f;
      else if (tid == TID_FLOAT)
         m_float *= f;
      else if (tid == TID_DOUBLE)
         m_double *= f;
      else
         mthrow("Invalid operation for ODB key \"" +
                m_parent_odb->get_full_path() + "\"");
      if (write && m_parent_odb->is_auto_refresh_write())
         m_parent_odb->write();
   }

}; // namespace midas