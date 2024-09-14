/********************************************************************\

  Name:         odbxx_test.cxx
  Created by:   Stefan Ritt

  Contents:     Test and Demo of Object oriented interface to ODB

\********************************************************************/

#include <string>
#include <iostream>
#include <array>
#include <functional>

#include "midas.h"
#include "odbxx.h"

/*------------------------------------------------------------------*/

int main() {

   cm_connect_experiment(NULL, NULL, "test", NULL);
   midas::odb::set_debug(true);

   // delete /Test/Settings to start from scratch
   midas::odb::delete_key("/Test/Settings");

   // create ODB structure...
   midas::odb o = {
      {"Int32 Key", 42},
      {"Bool Key", true},
      {"Subdir", {
         {"Int32 key", 123 },
         {"Double Key", 1.2},
         {"Subsub", {
            {"Float key", 1.2f},     // floats must be explicitly specified
            {"String Key", "Hello"},
         }}
      }},
      {"Int Array", {1, 2, 3}},
      {"Double Array", {1.2, 2.3, 3.4}},
      {"String Array", {"Hello1", "Hello2", "Hello3"}},
      {"Large Array", std::array<int, 10>{} },            // array with explicit size
      {"Large String", std::string(63, '\0') },           // string with explicit size
      {"String Array 10", std::array<std::string, 10>{}}, // string array with explicit size
      // string array with 10 strings of each 63 chars
      {"Large String Array 10", std::array<std::string, 10>{std::string(63, '\0')}}
   };

   // ...and push it to ODB. If keys are present in the
   // ODB, their value is kept. If not, the default values
   // from above are copied to the ODB
   o.connect("/Test/Settings");

   // alternatively, a structure can be created from an existing ODB subtree
   midas::odb o2("/Test/Settings");
   std::cout << o2 << std::endl;

   // set, retrieve, and change ODB value
   o["Int32 Key"] = 42;
   int i = o["Int32 Key"];
   o["Int32 Key"] = i+1;
   o["Int32 Key"]++;
   o["Int32 Key"] *= 1.3;
   std::cout << "Should be 57: " << o["Int32 Key"] << std::endl;

   // test with 64-bit variables
   o["Int64 Key"] = -1LL;
   int64_t i64 = o["Int64 Key"];
   std::cout << std::hex << "0x" << i64 << std::dec << std::endl;

   o["UInt64 Key"] = 0x1234567812345678u;
   uint64_t ui64 = o["UInt64 Key"];
   std::cout << std::hex << "0x" << ui64 << std::dec << std::endl;

   // test with bool
   o["Bool Key"] = false;
   o["Bool Key"] = !o["Bool Key"];

   // test with std::string
   o["Subdir"]["Subsub"]["String Key"] = "Hello";
   std::string s = o["Subdir"]["Subsub"]["String Key"];
   s += " world!";
   o["Subdir"]["Subsub"]["String Key"] = s;
   s = s + o["Subdir"]["Subsub"]["String Key"].s(); // need .s() for concatenations

   // use std::string as index
   std::string substr("Int32 Key");
   o[substr] = 43;

   // test with a vector
   std::vector<int> v = o["Int Array"]; // read vector
   std::fill(v.begin(), v.end(), 10);
   o["Int Array"] = v;        // assign vector to ODB array
   o["Int Array"][1] = 2;     // modify array element
   i = o["Int Array"][1];     // read from array element
   o["Int Array"].resize(5);  // resize array
   o["Int Array"]++;          // increment all values of array
   std::cout << "Arrays size is " << o["Int Array"].size() << std::endl;

   // auto-enlarge arrays
   o["Int Array"][10] = 10;

   // test with a string vector
   std::vector<std::string> sv;
   sv = o["String Array"];
   sv[1] = "New String";
   o["String Array"] = sv;
   o["String Array"][2] = "Another String";
   o["String Array"][3] = std::string("One more");
   s = o["String Array"][1].s(); // need .s() to explicitly convert to std::string

   // test with strings with given size
   o["String Array 2"][0].set_string_size("Hello", 64);
   o["String Array 2"][1] = "Second string";
   o["String Array 2"][2] = "Third string";

   // test with bool arrays/vectors
   o["Bool Array"] = std::array<bool, 3>{true, false, true};
   o["Bool Array from Vector"] = std::vector<bool>{true, false, true};

   // iterate over array
   int sum = 0;
   for (int e : o["Int Array"])
      sum += e;
   std::cout << "Sum should be 37: " << sum << std::endl;

   // create key from other key
   midas::odb oi(o["Int32 Key"]);
   oi = 123;

   // test auto refresh read
   std::cout << oi << std::endl;     // each read access reads value from ODB
   oi.set_auto_refresh_read(false);  // turn off auto refresh
   std::cout << oi << std::endl;     // this does not read value from ODB
   oi.read();                        // this forces a manual read
   std::cout << oi << std::endl;

   // test auto refresh write
   oi.set_auto_refresh_write(false); // turn off auto refresh write
   oi = 321;                         // this does not write a value to the ODB
   oi.write();                       // this forces a manual write

   // create ODB entries on-the-fly
   midas::odb ot;
   ot.connect("/Test/Settings/OTF");// this forces /Test/OTF to be created if not already there
   ot["Int32 Key"] = 1;             // create all these keys with different types
   ot["Double Key"] = 1.23;
   ot["String Key"] = "Hello";
   ot["Int Array"] = std::array<int, 10>{};
   ot["Subdir"]["Int32 Key"] = 42;
   ot["String Array"] = std::vector<std::string>{"S1", "S2", "S3"};
   ot["Other String Array"][0] = "OSA0";
   ot["Other String Array"][1] = "OSA1";

   // create key with default value
   i = ot["Int32 Key"](123);        // key exists already (created above) -> use key value i=1
   i = ot["New Int32 Key"](123);    // key does not exist -> set it to default value 123 i=123
   //  std::string s1 = ot["New String Key"]("Hi"); // same for strings
   std::cout << ot << std::endl;

   o.read();                        // re-read the underlying ODB tree which got changed by above OTF code
   std::cout << o.print() << std::endl;

   // iterate over sub-keys
   for (midas::odb& oit : o)
      std::cout << oit.get_name() << std::endl;

   // print whole sub-tree
   std::cout << o.print() << std::endl;

   // print whole subtree
   std::cout << o.dump() << std::endl;

   // update structure - create keys if needed, keep existing values if key already exists,
   // delete keys that are in ODB but not the list of defaults.
   midas::odb o3 = {
      {"Int32 Key", 456},
      {"New Bool Key", true},
      {"String Array", {"Hello1", "Hello2", "Hello3"}},
      {"Bool Key", true},
      {"Subdir", {
               {"Int32 key", 135 },
               {"New Sub Bool Key", false},
               {"Double Key", 1.5}
      }}
   };
   o3.connect_and_fix_structure("/Test/Settings");

   // Print new structure
   std::cout << "After changing structure with o3:" << std::endl;
   std::cout << o3.print() << std::endl;

   // delete test key from ODB
   o.delete_key();

   // don't clutter watch callbacks
   midas::odb::set_debug(false);

   // watch ODB key for any change with lambda function
   midas::odb ow("/Experiment");
   ow.watch([](midas::odb &o) {
      std::cout << "Value of key \"" + o.get_full_path() + "\" changed to " << o << std::endl;
   });

   do {
      int status = cm_yield(100);
      if (status == SS_ABORT || status == RPC_SHUTDOWN)
         break;
   } while (!ss_kbhit());

   cm_disconnect_experiment();
   return 1;
}