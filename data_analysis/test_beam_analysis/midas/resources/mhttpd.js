/********************************************************************\

 Name:         mhttpd.js
 Created by:   Stefan Ritt

 Contents:     JavaScript midas library used by mhttpd

 Note: please load midas.js before loading mhttpd.js

 \********************************************************************/

let run_state_names = {1: "Stopped", 2: "Paused", 3: "Running"};

let serverTimezoneOffset = undefined;

let transition_names = {
   1: "Starting run...",
   2: "Stopping run...",
   4: "Pausing run...",
   8: "Resuming run...",
   16: "Start abort",
   4096: "Deferred"
};

// extend 2d canvas object
CanvasRenderingContext2D.prototype.drawLine = function (x1, y1, x2, y2) {
   this.beginPath();
   this.moveTo(x1, y1);
   this.lineTo(x2, y2);
   this.stroke();
};

//
// convert json dom values to text for display and editing
// this is similar to db_sprintf()
//
function mie_to_string(tid, jvalue, format) {
   if (tid === TID_BOOL) {
      if (jvalue)
         return "y";
      else
         return "n";
   }

   if (tid === TID_FLOAT || tid === TID_DOUBLE) {
      if (jvalue === "NaN" || jvalue === "Infinity" || jvalue === "-Infinity")
         return jvalue;

      if (format && format.indexOf("%") !== -1) {
         // new format
         let i = format.indexOf('%');
         let s1 = format.substring(0, format.indexOf('%'));
         let s2, s3;
         let p = parseInt(format[i+2]);
         if (format[i+1] === "e")
            s2 = jvalue.toExponential(p);
         if (format[i+1] === "p")
            s2 = jvalue.toPrecision(p);
         if (format[i+1] === "f")
            s2 = jvalue.toFixed(p);
         if (format[i+1] === "t")
            s2 = jvalue.toLocaleString();
         if (format[i+1] === "%")
            s1 += '%';

         i += 2;
         while (format[i] >= '0' && format[i] <= '9')
            i++;
         s3 = format.substring(i);
         return s1+s2+s3;

      } else {

         // old format
         if (format && format.indexOf("e") !== -1) {
            let p = parseInt(format.substr(format.indexOf("e") + 1));
            return jvalue.toExponential(p);
         }
         if (format && format.indexOf("p") !== -1) {
            let p = parseInt(format.substr(format.indexOf("p") + 1));
            return jvalue.toPrecision(p);
         }
         if (format && format.indexOf("f") !== -1) {
            let p = parseInt(format.substr(format.indexOf("f") + 1));
            return Number(jvalue).toFixed(p);
         }
         if (format && format.indexOf("t") !== -1) {
            return jvalue.toLocaleString();
         }
         return jvalue;
      }
   }

   let t = typeof jvalue;

   if (t === 'number') {
      jvalue = "" + jvalue;
   }

   if (tid === TID_UINT64 || tid === TID_INT64 ||
      tid === TID_DWORD || tid === TID_INT ||
      tid === TID_WORD || tid === TID_SHORT || tid === TID_BYTE) {

      if (jvalue.substring(0, 2) === '0x')
         jvalue = "" + parseInt(jvalue, 16);
      
      if (format && format.indexOf("%") !== -1) {
         // new format
         if (format.length === 1)
            format = "%d";
         let str = "";
         for (let i = 0; i < format.length; i++) {
            if (format[i] === '%') {
               i++;

               if (format[i] === 'd')
                  str += jvalue;
               if (format[i] === 'f')
                  str += jvalue;
               if (format[i] === 'x')
                  str += "0x" + parseInt(jvalue).toString(16).toUpperCase();
               if (format[i] === 'b')
                  str += parseInt(jvalue).toString(2) + "b";
               if (format[i] === 't')
                  str += parseInt(jvalue).toLocaleString();
               if (format[i] === '%')
                  str += '%';

               i++;
               while (format[i] >= '0' && format[i] <= '9')
                  i++;

            } else
               str += format[i];
         }

         return str;

      } else {

         //old format
         if (format === undefined)
            format = "d";
         let str = "";
         for (let i = 0; i < format.length; i++) {
            if (format[i] === "d") {
               if (str.length > 0)
                  str += " / " + jvalue;
               else
                  str = jvalue;
            }
            if (format[i] === "x") {
               let hex = parseInt(jvalue).toString(16).toUpperCase();
               if (str.length > 0)
                  str += " / 0x" + hex;
               else
                  str = "0x" + hex;
            }
            if (format[i] === "b") {
               let bin = parseInt(jvalue).toString(2);
               if (str.length > 0)
                  str += " / " + bin + "b";
               else
                  str = bin + "b";
            }
            if (format[i] === "t") {
               let loc = parseInt(jvalue).toLocaleString();
               if (str.length > 0)
                  str += " / " + loc;
               else
                  str = loc;
            }
         }

         return str;
      }
   }

   if (t === 'string') {
      return jvalue;
   }

   return jvalue + " (" + t + ")";
}

//
// stupid javascript does not have a function
// to escape javascript and html characters
// to make it safe to assign a json string
// to p.innerHTML. What gives? K.O.
//
function mhttpd_escape(s) {
   let ss = s;

   if (typeof s !== 'string')
      return ss;

   while (ss.indexOf('"') >= 0)
      ss = ss.replace('"', '&quot;');

   while (ss.indexOf('>') >= 0)
      ss = ss.replace('>', '&gt;');

   while (ss.indexOf('<') >= 0)
      ss = ss.replace('<', '&lt;');

   //console.log("mhttpd_escape: [" + s + "] becomes [" + ss + "]");
   return ss;
}

//
// odb inline edit - make element a link to inline editor
//
function mie_back_to_link(p, path) {
   let link = document.createElement('a');
   link.href = path + "?cmd=Set";
   link.innerHTML = "(loading...)";

   mjsonrpc_db_get_values([path]).then(function (rpc) {
      let value = rpc.result.data[0];
      let tid = rpc.result.tid[0];
      let mvalue = mie_to_string(tid, value, p.dataset.format);
      if (mvalue === "")
         mvalue = "(empty)";
      link.innerHTML = mhttpd_escape(mvalue);
      link.onclick = function () {
         ODBInlineEdit(p, path);
         return false;
      };
      link.onfocus = function () {
         ODBInlineEdit(p, path);
      };
      link.style.color = p.oldStyle.color;

      if (p.childNodes[1] !== undefined &&
         p.childNodes[1].value !== undefined) //two values means it was editing an array
         setTimeout(function () {
            p.appendChild(link);
            p.removeChild(p.childNodes[1])
         }, 10);
      else
         setTimeout(function () {
            p.appendChild(link);
            p.removeChild(p.childNodes[0])
         }, 10);


      if (p.value !== value) {
         p.value = value;

         if (p.onchange !== null) {
            p.onchange();
         }
      }
   }).catch(function (error) {
      mjsonrpc_error_alert(error);
   });
}

//
// called from ODBFinishInlineEdit if element has 'confirm' flag
//
function ODBConfirmInlineEdit(flag, p) {

   let value;
   if (p.childNodes[1] !== undefined && p.childNodes[1].value !== undefined)
      value = p.childNodes[1].value;
   else
      value = p.childNodes[0].value;

   if (flag === true) {
      mjsonrpc_db_set_value(p.odbPath, value).then(function (rpc) {
         if (parseInt(p.dataset.input) !== 1) {
            p.ODBsent = true;
            mie_back_to_link(p, p.odbPath);
         }
      }).catch(function (error) {
         mjsonrpc_error_alert(error);
      });
   } else {
      if (parseInt(p.dataset.input) !== 1) {
         p.ODBsent = true;
         mie_back_to_link(p, p.odbPath);
      }
   }
}


//
// odb inline edit - write new value to odb
//
function ODBFinishInlineEdit(p, path) {
   let value;

   if (p.ODBsent === true)
      return;

   if (!p.inEdit)
      return;

   if (p.childNodes[1] !== undefined && p.childNodes[1].value !== undefined)
      value = p.childNodes[1].value;
   else
      value = p.childNodes[0].value;

   //console.log("mie_write odb [" + path + "] value [" + value + "]");

   // check for validation
    if (p.dataset.validate !== undefined) {
      let flag = eval(p.dataset.validate)(value, p);
      if (!flag) {
         if (parseInt(p.dataset.input) !== 1) {
            p.ODBsent = true;
            p.inEdit = false;
            mie_back_to_link(p, path);
         }
         return;
      }
   }

   // validator might have changed the value
   if (p.childNodes[1] !== undefined && p.childNodes[1].value !== undefined)
      value = p.childNodes[1].value;
   else
      value = p.childNodes[0].value;

   // check for confirm
   if (p.dataset.confirm !== undefined) {
      p.inEdit = false;
      dlgConfirm(p.dataset.confirm, ODBConfirmInlineEdit, p);
      return;
   }

   mjsonrpc_db_set_value(path, value).then(function (rpc) {
      if (parseInt(p.dataset.input) !== 1) {
         p.ODBsent = true;
         p.inEdit = false;
         mie_back_to_link(p, path);
      }
   }).catch(function (error) {
      mjsonrpc_error_alert(error);
   });
}

//
// odb inline edit - key-press handler
//
function ODBInlineEditKeydown(event, p, path) {
   let keyCode = ('which' in event) ? event.which : event.keyCode;

   if (keyCode === 27) {
      /* cancel editing */
      if (parseInt(p.dataset.input) !== 1) {
         p.ODBsent = true;
         p.inEdit = false;
         mie_back_to_link(p, path);
      }
      return false;
   }

   if (keyCode === 13) {
      ODBFinishInlineEdit(p, path);
      return false;
   }

   return true;
}

//
// odb inline edit - convert link to edit field
//
function mie_link_to_edit(p, cur_val, size) {
   let string_val = String(cur_val);

   p.ODBsent = false;
   if (p.tagName === 'A')
      p.oldStyle = p.style;
   else if (p.getElementsByTagName('a').length > 0)
      p.oldStyle = p.getElementsByTagName('a')[0].style;

   if (size === undefined)
      size = 20;
   let str = mhttpd_escape(string_val);

   let index = "";
   if (p.innerHTML[0] === '[' && p.innerHTML.indexOf(']') !== 0)
      index = p.innerHTML.substring(0, p.innerHTML.indexOf(']')+1) + "&nbsp;";

   p.innerHTML = index + "<input type='text' size='" + size + "' value='" + str +
      "' onKeydown='return ODBInlineEditKeydown(event, this.parentNode,&quot;" +
       p.odbPath + "&quot;," + ");' onBlur='ODBFinishInlineEdit(this.parentNode,&quot;" +
       p.odbPath + "&quot;," + ");' >";

   // needed for Firefox
   if (parseInt(p.dataset.input) !== 1)
      setTimeout(function () {
         if (index === "") {
            p.childNodes[0].focus();
            p.childNodes[0].select();
         } else {
            p.childNodes[1].focus();
            p.childNodes[1].select();
         }
      }, 10);
}

//
// odb inline edit - start editing
//
function ODBInlineEdit(p, odb_path) {
   if (p.inEdit)
      return;
   p.inEdit = true;
   p.odbPath = odb_path;

   mjsonrpc_db_get_values([odb_path]).then(function (rpc) {
      let value = rpc.result.data[0];
      if (Array.isArray(value))
         value = value[0];
      let tid = rpc.result.tid[0];
      let format = p.dataset.format;
      let size = p.dataset.size;
      if (size === undefined) {
        size = value.length;
        if (size === undefined || size < 20)
           size = 20;
      }
      if (format) {
         if (format.length > 1) {
            if (format[0] === 'd' || format[0] === 'x' || format[0] === 'b') {
               //when going to edit consider only the first format specifier for integers
               format = String(format[0]);
            }
         }
         if (format == 't') {
            // JSON parser can't cope with locale-based strings (thousands separators), so
            // force the user to edit the value as a regular number.
            format = undefined;
         }
      }
      let mvalue = mie_to_string(tid, value, format);
      mie_link_to_edit(p, mvalue, size);
   }).catch(function (error) {
      mjsonrpc_error_alert(error);
   });
}

function dlgOdbEditKeydown(event, input) {
   let keyCode = ('which' in event) ? event.which : event.keyCode;

   if (keyCode === 27) {
      // cancel editing
      dlgMessageDestroy(input.parentElement.parentElement);
      return false;
   }

   if (keyCode === 13) {
      dlgOdbEditSend(input.parentElement);
      dlgMessageDestroy(input.parentElement.parentElement);
      return false;
   }

   return true;
}

function dlgOdbEditSend(b) {
   let path = b.parentElement.parentElement.parentElement.odbPath;
   let value = b.parentElement.parentElement.elements[0].value;

   mjsonrpc_db_set_value(path, value).then(function (rpc) {
      //mjsonrpc_debug_alert(rpc);
   }).catch(function (error) {
      mjsonrpc_error_alert(error);
   });
}

//
// odb edit single value dialog box
//
function dlgOdbEdit(path) {

   mjsonrpc_db_get_value(path).then(function (rpc) {
      let value = rpc.result.data[0];
      let tid = rpc.result.tid[0];
      value = mie_to_string(tid, value);

      d = document.createElement("div");
      d.className = "dlgFrame";
      d.style.zIndex = 20;
      d.odbPath = path;

      d.innerHTML = "<div class=\"dlgTitlebar\" id=\"dlgMessageTitle\">Change ODB value</div>" +
         "<form>" +
         "<div class=\"dlgPanel\" style=\"padding: 30px;\">" +
         "<div id=\"dlgMessageString\">" +
         path + "&nbsp;:&nbsp;&nbsp;" +
         "<input type='text' size='16' value='" + value + "' onkeydown='return dlgOdbEditKeydown(event, this);'>" +
         "</div>" +
         "<br /><br />" +
         "<button class=\"dlgButton\" id=\"dlgMessageButton\" style=\"background-color:#F8F8F8\" type=\"button\" " +
         " onClick=\"dlgOdbEditSend(this);dlgMessageDestroy(this.parentElement);\">Ok</button>" +
         "<button class=\"dlgButton\" id=\"dlgMessageButton\" style=\"background-color:#F8F8F8\" type=\"button\" " +
         " onClick=\"dlgMessageDestroy(this.parentElement)\">Cancel</button>" +
         "</div>" +
         "</form>";

      document.body.appendChild(d);

      dlgShow(d, false);

      // needed for Firefox
      setTimeout(function () {
         d.childNodes[1].elements[0].focus();
         d.childNodes[1].elements[0].select();
      }, 10);


   }).catch(function (error) {
      mjsonrpc_error_alert(error);
   });
}

/*---- mhttpd functions -------------------------------------*/

//
// Returns time to display in status bar and history.
// By default show local time, but through the config page the
// user can select the server time zone or any other time zone
//
function mhttpd_get_display_time(sec) {
   // retrieve timezone of server on the first call
   if (serverTimezoneOffset === undefined) {
      mjsonrpc_get_timezone().then(function (rpc) {
         serverTimezoneOffset = rpc.result;
      }).catch(function (error) {
         mjsonrpc_error_alert(error);
      });
   }

   let d;
   if (sec !== undefined)
      d = new Date(sec * 1000);
   else
      d = new Date();
   let o = 0;

   if (mhttpdConfig().timezone === undefined)
      mhttpdConfigSet('timezone', 'local');

   if (mhttpdConfig().timezone === 'local') {
      o = -d.getTimezoneOffset() / 60;
   } else if (mhttpdConfig().timezone === 'server') {
      if (serverTimezoneOffset === undefined)
         return {
            date: undefined,
            string: "",
            offset: 0
         };
      o = serverTimezoneOffset / 3600;
   } else {
      o = parseInt(mhttpdConfig().timezone);
   }

   d = new Date(d.getTime() + o * 3600 * 1000);

   let s = d.toLocaleString("en-gb", {
      timeZone: 'UTC',
      hour12: false, day: 'numeric', month: 'short', year: 'numeric',
      hour: 'numeric', minute: 'numeric', second: 'numeric'
   });

   if (o === 0)
      s += " UTC";
   else if (o > 0)
      s += " UTC+"+o;
   else
      s += " UTC"+o;

   return {
      date: d,
      string: s,
      offset: o
   };
}

function mhttpd_disable_button(button) {
   if (!button.disabled) {
      button.disabled = true;
   }
}

function mhttpd_enable_button(button) {
   if (button.disabled) {
      button.disabled = false;
   }
}

function mhttpd_set_style_visibility(e, v) {
   if (e) {
      if (e.style.visibility !== v) {
         e.style.visibility = v;
      }
   }
}

function mhttpd_set_style_display(e, v) {
   if (e) {
      if (e.style.display !== v) {
         e.style.display = v;
      }
   }
}

function mhttpd_set_firstChild_data(e, v) {
   if (e) {
      if (e.firstChild.data !== v) {
         //console.log("mhttpd_set_firstChild_data for " + e + " from " + e.firstChild.data + " to " + v);
         e.firstChild.data = v;
      }
   }
}

function mhttpd_set_innerHTML(e, v) {
   if (e) {
      if (e.innerHTML !== v) {
         //console.log("mhttpd_set_innerHTML for " + e + " from " + e.innerHTML + " to " + v);
         e.innerHTML = v;
      }
   }
}

function mhttpd_set_className(e, v) {
   if (e) {
      if (e.className !== v) {
         //console.log("mhttpd_set_className for " + e + " from " + e.className + " to " + v);
         e.className = v;
      }
   }
}

function mhttpd_hide_button(button) {
   mhttpd_set_style_visibility(button, "hidden");
   mhttpd_set_style_display(button, "none");
}

function mhttpd_unhide_button(button) {
   mhttpd_set_style_visibility(button, "visible");
   mhttpd_set_style_display(button, "");
}

function mhttpd_hide(id) {
   let e = document.getElementById(id);
   if (e) {
      mhttpd_set_style_visibility(e, "hidden");
      mhttpd_set_style_display(e, "none");
   }
}

function mhttpd_unhide(id) {
   let e = document.getElementById(id);
   if (e) {
      mhttpd_set_style_visibility(e, "visible");
      mhttpd_set_style_display(e, "");
   }
}

function mhttpd_enable(id) {
   let e = document.getElementById(id);
   if (e)
      e.disabled = false;
}

function mhttpd_disable(id) {
   let e = document.getElementById(id);
   if (e)
      e.disabled = true;
}

function mhttpd_init_overlay(overlay) {
   mhttpd_hide_overlay(overlay);

   // this element will hide the underlaying web page

   overlay.style.zIndex = 10;
   //overlay.style.backgroundColor = "rgba(0,0,0,0.5)"; /*dim the background*/
   overlay.style.backgroundColor = "white";
   overlay.style.position = "fixed";
   overlay.style.top = "0%";
   overlay.style.left = "0%";
   overlay.style.width = "100%";
   overlay.style.height = "100%";

   return overlay.children[0];
}

function mhttpd_hide_overlay(overlay) {
   overlay.style.visibility = "hidden";
   overlay.style.display = "none";
}

function mhttpd_unhide_overlay(overlay) {
   overlay.style.visibility = "visible";
   overlay.style.display = "";
}

function mhttpd_getParameterByName(name) {
   let match = RegExp('[?&]' + name + '=([^&]*)').exec(window.location.search);
   return match && decodeURIComponent(match[1].replace(/\+/g, ' '));
}

function mhttpd_goto_page(page, param) {
   if (!param) param = "";
   window.location.href = '?cmd=' + page + param; // reloads the page from new URL
   // DOES NOT RETURN
}

function mhttpd_navigation_bar(current_page, path) {
   document.write("<div id='customHeader'>\n");
   document.write("</div>\n");

   document.write("<div class='mnav'>\n");
   document.write("  <table>\n");
   document.write("    <tr><td id='navigationTableButtons'></td></tr>\n");
   document.write("  </table>\n\n");
   document.write("</div>\n");

   if (!path)
      path = "";

   if (localStorage.mNavigationButtons !== undefined) {
      document.getElementById("navigationTableButtons").innerHTML = localStorage.mNavigationButtons;
      let button = document.getElementById("navigationTableButtons").children;
      for (let i = 0; i < button.length; i++)
         if (button[i].value.toLowerCase() === current_page.toLowerCase())
            button[i].className = "mnav mnavsel navButtonSel";
         else
            button[i].className = "mnav navButton";
      return;
   }

   mjsonrpc_db_get_values(["/Custom/Header", "/Experiment/Menu", "/Experiment/Menu Buttons"]).then(function (rpc) {
      let custom_header = rpc.result.data[0];

      if (custom_header && custom_header.length > 0)
         document.getElementById("customHeader").innerHTML = custom_header;

      let menu = rpc.result.data[1];
      let buttons = rpc.result.data[2];
      let b = [];

      if (menu) {
         for (let k in menu) {
            let kk = k + "/name";
            if (kk in menu) {
               if (menu[k]) {
                  b.push(menu[kk]);
               }
            }
         }
      } else if (buttons && buttons.length > 0) {
         b = buttons.split(",");
      }

      if (!b || b.length < 1) {
         b = ["Status", "ODB", "Messages", "Chat", "ELog", "Alarms", "Programs", "History", "MSCB", "Sequencer", "Config", "Help"];
      }

      let html = "";

      for (let i = 0; i < b.length; i++) {
         let bb = b[i].trim();
         let cc = "mnav navButton";
         if (bb.toLowerCase() === current_page.toLowerCase()) {
            cc = "mnav mnavsel navButtonSel";
         }
         html += "<input type=button name=cmd value='" + bb + "' class='" + cc + "' onclick='window.location.href=\'" + path + "?cmd=" + bb + "\';return false;'>\n";
      }
      document.getElementById("navigationTableButtons").innerHTML = html;

      // cache navigation buttons in browser local storage
      localStorage.setItem("mNavigationButtons", html);

   }).catch(function (error) {
      mjsonrpc_error_alert(error);
   });
}

function mhttpd_show_menu(flag) {
   let m = document.getElementById("msidenav");

   if (m.initialWidth === undefined)
      m.initialWidth = m.clientWidth;

   if (flag) {
      m.style.width = m.initialWidth + "px";
      document.getElementById("mmain").style.marginLeft = m.initialWidth + "px";
      mhttpdConfigSet('hideMenu', false);
   } else {
      m.style.width = "0";
      document.getElementById("mmain").style.marginLeft = "0";
   }

   mhttpdConfigSet('showMenu', flag);
}

function mhttpd_toggle_menu() {
   let flag = mhttpdConfig().showMenu;
   flag = !flag;
   mhttpd_show_menu(flag);
}

function mhttpd_exec_script(name) {
   //console.log("exec_script: " + name);
   let params = new Object;
   params.script = name;
   mjsonrpc_call("exec_script", params).then(function (rpc) {
      let status = rpc.result.status;
      if (status != 1) {
         dlgAlert("Exec script \"" + name + "\" status " + status);
      }
   }).catch(function (error) {
      mjsonrpc_error_alert(error);
   });
}

let mhttpd_refresh_id;
let mhttpd_refresh_history_id;
let mhttpd_refresh_interval;
let mhttpd_refresh_paused;
let mhttpd_refresh_history_interval;
let mhttpd_spinning_wheel;
let mhttpd_initialized = false;

function mhttpd_init(current_page, interval, callback) {
   /*
    This function should be called from custom pages to initialize all ODB tags and refresh
    them periodically every "interval" in ms

    For possible ODB tags please refer to

    https://midas.triumf.ca/MidasWiki/index.php/New_Custom_Pages_(2017)
   */

   if (mhttpd_initialized) {
      dlgAlert("mhttpd_init() is called more than once from user code");
      return;
   }

   // initialize URL
   url = mhttpd_getParameterByName("URL");
   if (url)
      mjsonrpc_set_url(url);

   // retrieve current page if not given
   if (current_page === undefined) {
      current_page = mhttpd_getParameterByName("page");
   }
   if (current_page === undefined || current_page === null) {
      dlgAlert("Please specify name of page via mhttpd_init('&lt;name&gt;')<br>or make sure to have '&amp;page=&lt;name&gt;' in URL");
      return;
   }

   // create header
   let h = document.getElementById("mheader");
   if (h === null) {
      dlgAlert('Web page does not contain "mheader" element');
      return;
   }
   let s = document.getElementById("msidenav");
   if (s === null) {
      dlgAlert('Web page does not contain "msidenav" element');
      return;
   }

   mhttpd_initialized = true;

   // set global font size
   let config = mhttpdConfig();
   document.body.style.fontSize = config.fontSize + "pt";

   h.style.display = "flex";
   h.innerHTML =
      "<div style='display:inline-block; flex:none;'>" +
      "<span class='mmenuitem' style='padding: 10px;margin-right: 20px;' onclick='mhttpd_toggle_menu()'>&#9776;</span>" +
      "<span id='mheader_expt_name'></span>" +
      "</div>" +

      "<div style='flex:auto;'>" +
      "  <div id='mheader_message'></div>" +
      "</div>" +

      "<div style='display: inline; flex:none;'>" +
      "  <div id='mheader_alarm'>&nbsp;</div>" +
      "  <div style='display: inline; font-size: 75%; margin-right: 10px' id='mheader_last_updated'>mheader_last_updated</div>" +
      "</div>";

   mhttpd_resize_sidenav();
   window.addEventListener('resize', mhttpd_resize_sidenav);

   // put error header in front of header
   let d = document.createElement('div');
   d.id = 'mheader_error';
   h.parentNode.insertBefore(d, h);

   // update header and menu
   if (document.getElementById("msidenav") !== undefined) {

      // get it from session storage cache if present
      if (sessionStorage.msidenav !== undefined && sessionStorage.mexpname !== undefined) {
         let menu = document.getElementById("msidenav");
         menu.innerHTML = sessionStorage.msidenav;
         let item = menu.children;
         for (let i = 0; i < item.length; i++) {
            if (item[i].className !== "mseparator") {
               if (current_page.search(item[i].innerHTML) !== -1)
                  item[i].className = "mmenuitem mmenuitemsel";
               else
                  item[i].className = "mmenuitem";
            }
         }
         document.getElementById("mheader_expt_name").innerHTML = sessionStorage.mexpname;

         // now the side navigation has its full width, adjust the main body and make it visible
         let m = document.getElementById("mmain");
         if (m !== undefined) {
            m.style.marginLeft = document.getElementById("msidenav").clientWidth + "px";
            m.style.opacity = "1";
         }
      }

      // request it from server, since it might have changed
      mjsonrpc_db_get_values(["/Experiment/Name", "/Experiment/Menu", "/Experiment/Menu Buttons",
         "/Custom", "/Script", "/Alias"]).then(function (rpc) {

         let expt_name = rpc.result.data[0];
         let menu = rpc.result.data[1];
         let buttons = rpc.result.data[2];
         let custom = rpc.result.data[3];
         let script = rpc.result.data[4];
         let alias = rpc.result.data[5];

         document.getElementById("mheader_expt_name").innerHTML = expt_name;
         sessionStorage.setItem("mexpname", expt_name);

         // preload spinning wheel for later use
         if (mhttpd_spinning_wheel === undefined) {
            mhttpd_spinning_wheel = new Image();
            mhttpd_spinning_wheel.src = "spinning-wheel.gif";
         }

         // menu buttons
         let b = [];
         if (menu) {
            for (let k in menu) {
               if (k.indexOf('/') >= 0) // skip <key>/last_written and <key>/name
                  continue;
               if (menu[k]) // show button if not disabled
                  b.push(menu[k + "/name"]);
            }
         } else if (buttons && buttons.length > 0) {
            b = buttons.split(",");
         }

         if (!b || b.length < 1) {
            b = ["Status", "ODB", "Messages", "Chat", "ELog", "Alarms", "Programs", "History", "MSCB", "Sequencer", "Config", "Help"];
         }

         let html = "";

         for (let i = 0; i < b.length; i++) {
            let bb = b[i].trim();
            let cc = "mmenuitem";
            if (bb === current_page) {
               cc += " mmenuitemsel";
            }
            html += "<div class='" + cc + "'><a href='?cmd=" + bb + "' class='mmenulink'>" + bb + "</a></div>\n";
         }

         // custom
         if (custom !== null && Object.keys(custom).length > 0) {
            // add separator
            html += "<div class='mseparator'></div>\n";
            // add menu items recursively
            html = mhttpd_add_menu_items(html, custom, current_page, "", 0);
         }

         // script
         if (script !== null && Object.keys(script).length > 0) {
            // add separator
            html += "<div class='mseparator'></div>\n";

            for (let b in script) {
               if (b.indexOf('/') >= 0) // skip <key>/last_written and <key>/name
                  continue;
               let n = script[b + "/name"];
               //html += "<div class='mmenuitem'><a href='?script=" + b + "' class='mmenulink'>" + n + "</a></div>\n";
               html += "<div class='mmenuitem'><button class='mbutton' onclick='mhttpd_exec_script(\"" + n + "\")'>" + n + "</button></div>\n";
            }

         }

         // alias
         if (alias !== null && Object.keys(alias).length > 0) {
            // add separator
            html += "<div class='mseparator'></div>\n";

            for (b in alias) {
               if (b.indexOf('/') >= 0) // skip <key>/last_written and <key>/name
                  continue;
               n = alias[b + "/name"];
               if (n.substr(n.length - 1) === "&") {
                  n = n.substr(0, n.length - 1);
                  html += "<div class='mmenuitem'><a href='" + alias[b] + "' class='mmenulink' target='_blank'>" + n + "&#8599;</a></div>\n";
               } else {
                  html += "<div class='mmenuitem'><a href='" + alias[b] + "' class='mmenulink'>" + n + "</a></div>\n";
               }
            }

         }

         // dummy spacer to fix scrolling to the bottom, must be at least header height
         html += "<div style='height: 64px;'></div>\n";

         document.getElementById("msidenav").innerHTML = html;

         // re-adjust size of mmain element if menu has changed
         let m = document.getElementById("mmain");
         if (m !== undefined) {
            m.style.marginLeft = document.getElementById("msidenav").clientWidth + "px";
            m.style.opacity = "1";
         }

         // cache navigation buttons in browser local storage
         sessionStorage.setItem("msidenav", html);

         // show/hide sidenav according to local storage settings
         mhttpd_show_menu(mhttpdConfig().showMenu);

         // hide all invisible menu items after layout has been finished
         let mi = document.getElementsByClassName('mmenuitem');
         for (m of mi) {
            let p = m.parentElement;
            if (p.style.visibility === "hidden") {
               p.style.visibility = "visible";
               p.style.display = "none";
            }
         }
         mi = document.getElementsByClassName('msubmenuitem');
         for (m of mi) {
            let p = m.parentElement;
            if (p.style.visibility === "hidden") {
               p.style.visibility = "visible";
               p.style.display = "none";
            }
         }

      }).then(function () {
         if (callback !== undefined)
            callback();
      }).catch(function (error) {
         mjsonrpc_error_alert(error);
      });
   }

   // store refresh interval and do initial refresh
   if (interval === undefined)
      interval = 1000;
   mhttpd_refresh_interval = interval;
   mhttpd_refresh_paused = false;

   // history interval is static for now
   mhttpd_refresh_history_interval = 30000;

   // trigger first refresh
   mhttpd_refresh();
   mhttpd_refresh_history();
}

function mhttpd_find_menuitem(current_page, submenu) {
   for (let b in submenu) {

      if (typeof submenu[b] === "object") { // recurse into submenu
         let flag = mhttpd_find_menuitem(current_page, submenu[b]);
         if (flag)
            return true;
      }

      if (typeof submenu[b] === "string") {
         let s = submenu[b];
         if (s[s.length-1] === '&')
            s = s.slice(0, -1);

         if (b.indexOf('/name') !== -1 && s.toLowerCase() === current_page.toLowerCase())
            return true;

      }
   }
   
   return false;
}

function mhttpd_add_menu_items(html, custom, current_page, path, level) {
   for (let b in custom) {
      if (b.indexOf('/') >= 0) // skip <key>/last_written and <key>/name
         continue;
      if (b === "path" || b === "images" || b === "default") // skip "path", "Images" and "default"
         continue;

      if (typeof custom[b] === "object") { // submenu created by subdirectory in ODB
         let cc = "msubmenuitem";
         let l = "";
         let expand = mhttpd_find_menuitem(current_page, custom[b]);

         // add blanks according to nesting level
         for (let i=0 ; i<level ; i++)
            l += "&nbsp;&nbsp;";

         if (expand)
            l += "&#9662;&nbsp;"; // caret down
         else
            l += "&#9656;&nbsp;"; // caret right

         l += "&nbsp;";
         l += custom[b + "/name"];
         let p = path;
         if (p !== "")
            p += "/";
         p += b;

         html += "<div class='" + cc + "' onclick='mhttpd_submenu(this)'><div class='mmenulink'>" + l + "</div></div>\n";

         if (expand)
            html += "<div>"; // do not hide submenu if current page is under it
         else
            html += "<div style='visibility: hidden'>";
         html = mhttpd_add_menu_items(html, custom[b], current_page, p, level+1);
         html += "</div>";
      } else if (typeof custom[b] === "string") { // skip any items that don't have type of string, since can't be valid links
         let cc = "mmenuitem";
         let mitem = custom[b + "/name"];
         if (mitem.slice(-1) === '&')
            mitem = mitem.slice(0, -1);
         if (current_page.search(mitem) !== -1 ||
             current_page.search(mitem.toLowerCase()) !== -1)
            cc += " mmenuitemsel";
         let ln = "";
         for (let i=0 ; i<level ; i++)
            ln += "&nbsp;&nbsp;";
         ln += custom[b + "/name"];
         let lt = path;
         if (lt !== "")
            lt += "/";
         lt += custom[b + "/name"];
         if (ln.slice(-1) === '!')
            continue;
         let target = "";
         let style = "";
         // if (level > 0)
         //    style = "style='display: none'";
         if (ln.slice(-1) === '&') {
            ln = ln.slice(0, -1);
            lt = lt.slice(0, -1);
            target += "target='_blank' ";
         }

         html += "<div class='" + cc + "' " + style + "><a " + target + " href='?cmd=custom&page=" + lt + "' class='mmenulink'>" + ln + "</a></div>\n";
      }
   }

   return html;
}

function mhttpd_submenu(o) {
   let i;
   for (i=0 ; o.firstChild.innerHTML.charCodeAt(i) !== 9656 && o.firstChild.innerHTML.charCodeAt(i) !== 9662 ; i++);
   if (o.firstChild.innerHTML.charCodeAt(i) === 9656) {
      // expand
      o.firstChild.innerHTML =
         o.firstChild.innerHTML.substring(0, i-1) +
         String.fromCharCode(9662) +
         o.firstChild.innerHTML.substring(i+1);
      o.nextElementSibling.style.display = "inline";
      o.nextElementSibling.style.visibility = "visible";
   } else {
      // collapse
      o.firstChild.innerHTML =
         o.firstChild.innerHTML.substring(0, i-1) +
         String.fromCharCode(9656) +
         o.firstChild.innerHTML.substring(i+1);
      o.nextElementSibling.style.display = "none";
   }
}

function mhttpd_refresh_pause(flag) {
   mhttpd_refresh_paused = flag;
}

function mhttpd_set_refresh_interval(interval) {
   mhttpd_refresh_interval = interval;
}

function getMElements(name) {
   // collect all <div name=[name] >
   let e = [];
   e.push(...document.getElementsByName(name));

   // collect all <div class=[name] >
   e.push(...document.getElementsByClassName(name));

   return e;
}

function mhttpd_thermo_draw() {
   let ctx = this.firstChild.getContext("2d");
   ctx.save();
   let w = this.firstChild.width;
   let h = this.firstChild.height;
   ctx.clearRect(0, 0, w, h);
   w = w - 1; // space for full circles
   h = h - 1;

   if (this.dataset.scale === "1") {
      w = w / 2;
      w = Math.floor(w / 4) * 4;
   }

   if (this.dataset.printValue === "1") {
      h = h - 14;
   }

   let x0 = Math.round(w / 4 * 0);
   let x1 = Math.round(w / 4 * 1);
   let x2 = Math.round(w / 4 * 2);
   let x3 = Math.round(w / 4 * 3);

   let v = this.value;
   if (v < this.dataset.minValue)
      v = this.dataset.minValue;
   if (v > this.dataset.maxValue)
      v = this.dataset.maxValue;
   let yt = (h - 4 * x1) - (h - 5 * x1) * (v - this.dataset.minValue) / (this.dataset.maxValue - this.dataset.minValue);

   ctx.translate(0.5, 0.5);
   ctx.strokeStyle = "#000000";
   ctx.fillStyle = "#FFFFFF";
   ctx.lineWidth = 1;

   // outer "glass"
   ctx.beginPath();
   ctx.arc(x2, x1, x1, Math.PI, 0);
   ctx.lineTo(x3, h - x1 * 4);
   ctx.lineTo(x3, h - x1 * 2 * (1 + Math.sin(60 / 360 * 2 * Math.PI)));
   ctx.arc(x2, h - x2, x2, 300 / 360 * 2 * Math.PI, 240 / 360 * 2 * Math.PI);
   ctx.lineTo(x1, h - x1 * 2 * (1 + Math.sin(60 / 360 * 2 * Math.PI)));
   ctx.lineTo(x1, x1);
   ctx.stroke();
   if (this.dataset.backgroundColor !== undefined) {
      ctx.fillStyle = this.dataset.backgroundColor;
      ctx.fill();
   }

   // inner "fluid"
   if (this.dataset.color === undefined) {
      ctx.strokeStyle = "#000000";
      ctx.fillStyle = "#000000";
   } else {
      ctx.strokeStyle = this.dataset.color;
      ctx.fillStyle = this.dataset.color;
   }

   ctx.beginPath();
   ctx.moveTo(x1 + 3, yt);
   ctx.lineTo(x3 - 3, yt);
   ctx.lineTo(x3 - 3, h - x2);
   ctx.lineTo(x1 + 3, h - x2);
   ctx.lineTo(x1 + 3, yt);
   ctx.stroke();
   ctx.fill();

   ctx.beginPath();
   ctx.arc(x2, h - x2, x2 - 4, 0, 2 * Math.PI);
   ctx.stroke();
   ctx.fill();

   // re-draw outer "glass"
   ctx.strokeStyle = "#000000";
   ctx.fillStyle = "#FFFFFF";
   ctx.lineWidth = 1;
   ctx.beginPath();
   ctx.arc(x2, x1, x1, Math.PI, 0);
   ctx.lineTo(x3, h - x1 * 4);
   ctx.lineTo(x3, h - x1 * 2 * (1 + Math.sin(60 / 360 * 2 * Math.PI)));
   ctx.arc(x2, h - x2, x2, 300 / 360 * 2 * Math.PI, 240 / 360 * 2 * Math.PI);
   ctx.lineTo(x1, h - x1 * 2 * (1 + Math.sin(60 / 360 * 2 * Math.PI)));
   ctx.lineTo(x1, x1);
   ctx.stroke();

   // optional scale
   if (this.dataset.scale === "1") {
      ctx.beginPath();
      ctx.moveTo(x3 + x1 / 2, x1);
      ctx.lineTo(x3 + x1, x1);
      ctx.moveTo(x3 + x1 / 2, h - 4 * x1);
      ctx.lineTo(x3 + x1, h - 4 * x1);
      ctx.stroke();

      ctx.font = "12px sans-serif";
      ctx.fillStyle = "#000000";
      ctx.strokeStyle = "#000000";
      ctx.textBaseline = "middle";

      let mintext = this.dataset.minValue;
      let maxtext = this.dataset.maxValue;
      
      if (this.dataset.format !== undefined) {
         mintext = mie_to_string(parseInt(this.dataset.tid), this.dataset.minValue, this.dataset.format);
         maxtext = mie_to_string(parseInt(this.dataset.tid), this.dataset.maxValue, this.dataset.format);
      }
         
      ctx.fillText(mintext, 4.5 * x1, h - 4 * x1, 3.5 * x1);
      ctx.fillText(maxtext, 4.5 * x1, x1, 3.5 * x1);
   }

   // optional value display
   if (this.dataset.printValue === "1") {
      ctx.font = "12px sans-serif";
      ctx.fillStyle = "#000000";
      ctx.strokeStyle = "#000000";
      ctx.textBaseline = "bottom";
      ctx.textAlign = "center";

      let valuetext = this.value;
      
      if (this.dataset.format !== undefined) {
         valuetext = mie_to_string(parseInt(this.dataset.tid), this.value, this.dataset.format);
      }

      ctx.fillText(valuetext, x2, this.firstChild.height, 4 * x1);
   }

   ctx.restore();
}

function mhttpd_gauge_draw() {
   let ctx = this.firstChild.getContext("2d");
   ctx.save();
   let w = this.firstChild.width;
   let h = this.firstChild.height;
   let y = h;
   if (this.dataset.scale === "1")
      y -= 15;
   else
      y -= 1;
   ctx.clearRect(0, 0, w, h);

   let v = this.value;
   if (v < this.dataset.minValue)
      v = this.dataset.minValue;
   if (v > this.dataset.maxValue)
      v = this.dataset.maxValue;
   v = (v - this.dataset.minValue) / (this.dataset.maxValue - this.dataset.minValue);

   ctx.translate(0.5, 0.5);
   ctx.strokeStyle = "#000000";
   ctx.fillStyle = "#FFFFFF";
   ctx.lineWidth = 1;

   // outer ring
   ctx.beginPath();
   ctx.arc(w / 2, y, w / 2 - 1, Math.PI, 0);
   ctx.lineTo(w - 4, y);
   ctx.arc(w / 2, y, w / 2 - 4, 0, Math.PI, true);
   ctx.lineTo(4, y);
   ctx.fillStyle = this.dataset.color;
   ctx.fill();

   // value bar
   ctx.beginPath();
   ctx.fillStyle = this.dataset.color;
   ctx.strokeStyle = this.dataset.color;
   ctx.arc(w / 2, y, w / 2 - 6, Math.PI, (1 + v) * Math.PI);
   ctx.arc(w / 2, y, w / 2 - w / 5, (1 + v) * Math.PI, Math.PI, true);
   ctx.fill();

   // black frame
   if (this.dataset.frame) {
      ctx.strokeStyle = "#000000";
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.arc(w / 2, y, w / 2 - 6, Math.PI, 0);
      ctx.lineTo(w - w / 5, y);
      ctx.arc(w / 2, y, w / 2 - w / 5, 0, Math.PI, true);
      ctx.lineTo(6, y);
      ctx.stroke();
   }

   // optional value display
   if (this.dataset.printValue === "1") {
      ctx.font = "12px sans-serif";
      ctx.fillStyle = "#000000";
      ctx.strokeStyle = "#000000";
      ctx.textBaseline = "bottom";
      ctx.textAlign = "center";

      let valtext = this.value;

      if (this.dataset.format !== undefined) {
         valtext = mie_to_string(parseInt(this.dataset.tid), this.value, this.dataset.format);
      }

      ctx.fillText(valtext, w / 2, y, w);
   }

   // optional scale display
   if (this.dataset.scale === "1") {
      ctx.font = "12px sans-serif";
      ctx.fillStyle = "#000000";
      ctx.strokeStyle = "#000000";
      ctx.textBaseline = "bottom";
      ctx.textAlign = "center";

      let mintext = this.dataset.minValue;
      let maxtext = this.dataset.maxValue;
      
      if (this.dataset.format !== undefined) {
         mintext = mie_to_string(parseInt(this.dataset.tid), this.dataset.minValue, this.dataset.format);
         maxtext = mie_to_string(parseInt(this.dataset.tid), this.dataset.maxValue, this.dataset.format);
      }

      ctx.fillText(mintext, 0.1 * w, h, 0.2 * w);
      ctx.fillText(maxtext, 0.9 * w, h, 0.2 * w);
   }

   ctx.restore();
}

function mhttpd_vaxis_draw() {
   let ctx = this.firstChild.getContext("2d");
   ctx.save();
   let w = this.firstChild.width;
   let h = this.firstChild.height;
   ctx.clearRect(0, 0, w, h);

   let line = true;
   if (this.dataset.line === "0")
      line = false;
   let log = false;
   if (this.dataset.log === "1")
      log = true;

   let scaleMin = 0;
   let scaleMax = 1;
   if (this.dataset.minValue !== undefined)
      scaleMin = parseFloat(this.dataset.minValue);
   if (log && scaleMin === 0)
      scaleMin = 1E-3;
   if (this.dataset.maxValue !== undefined)
      scaleMax = parseFloat(this.dataset.maxValue);
   if (scaleMin === scaleMax)
      scaleMax += 1;

   ctx.translate(0.5, 0.5);
   ctx.strokeStyle = "#000000";
   ctx.fillStyle = "#FFFFFF";
   ctx.lineWidth = 1;

   if (this.style.textAlign === "left") {
      ctx.translate(-0.5, -0.5);
      vaxisDraw(ctx, 0, h - 1, h - 2, line, 4, 8, 10, 12, 0, scaleMin, scaleMax, log);
   } else {
      ctx.translate(-0.5, -0.5);
      vaxisDraw(ctx, w, h - 1, h - 2, line, 4, 8, 10, 12, 0, scaleMin, scaleMax, log);
   }

   ctx.restore();
}

function mhttpd_haxis_draw() {
   let ctx = this.firstChild.getContext("2d");
   ctx.save();
   let w = this.firstChild.width;
   let h = this.firstChild.height;
   ctx.clearRect(0, 0, w, h);

   let line = true;
   if (this.dataset.line === "0")
      line = false;
   let log = false;
   if (this.dataset.log === "1")
      log = true;

   let scaleMin = 0;
   let scaleMax = 1;
   if (this.dataset.minValue !== undefined)
      scaleMin = parseFloat(this.dataset.minValue);
   if (log && scaleMin === 0)
      scaleMin = 1E-3;
   if (this.dataset.maxValue !== undefined)
      scaleMax = parseFloat(this.dataset.maxValue);
   if (scaleMin === scaleMax)
      scaleMax += 1;

   ctx.translate(-0.5, 0);
   ctx.strokeStyle = "#000000";
   ctx.fillStyle = "#FFFFFF";
   ctx.lineWidth = 1;

   if (this.style.verticalAlign === "top") {
      ctx.translate(0.5, 0.5);
      haxisDraw(ctx, 1, 0, w - 2, line, 4, 8, 10, 10, 0, scaleMin, scaleMax, log);
   } else {
      ctx.translate(0.5, -0.5);
      haxisDraw(ctx, 1, h, w - 2, line, 4, 8, 10, 20, 0, scaleMin, scaleMax, log);
   }
   ctx.restore();
}

String.prototype.stripZeros = function () {
   let s = this.trim();
   if (s.search("[.]") >= 0) {
      let i = s.search("[e]");
      if (i >= 0) {
         while (s.charAt(i - 1) === "0") {
            s = s.substring(0, i - 1) + s.substring(i);
            i--;
         }
         if (s.charAt(i - 1) === ".")
            s = s.substring(0, i - 1) + s.substring(i);
      } else {
         while (s.charAt(s.length - 1) === "0")
            s = s.substring(0, s.length - 1);
         if (s.charAt(s.length - 1) === ".")
            s = s.substring(0, s.length - 1);
      }
   }
   return s;
};

function haxisDraw(ctx, x1, y1, width, line, minor, major, text, label, grid, xmin, xmax, logaxis) {
   let dx, int_dx, frac_dx, x_act, label_dx, major_dx, x_screen, maxwidth;
   let tick_base, major_base, label_base, n_sig1, n_sig2, xs;
   let base = [1, 2, 5, 10, 20, 50, 100, 200, 500, 1000];

   ctx.textAlign = "center";
   ctx.textBaseline = "top";

   if (xmax <= xmin || width <= 0)
      return;

   if (logaxis) {
      dx = Math.pow(10, Math.floor(Math.log(xmin) / Math.log(10)));
      if (dx === 0)
         dx = 1E-10;
      label_dx = dx;
      major_dx = dx * 10;
      n_sig1 = 4;
   } else {
      /* use 6 as min tick distance */
      dx = (xmax - xmin) / (width / 6);

      int_dx = Math.floor(Math.log(dx) / Math.log(10));
      frac_dx = Math.log(dx) / Math.log(10) - int_dx;

      if (frac_dx < 0) {
         frac_dx += 1;
         int_dx -= 1;
      }

      tick_base = frac_dx < (Math.log(2) / Math.log(10)) ? 1 : frac_dx < (Math.log(5) / Math.log(10)) ? 2 : 3;
      major_base = label_base = tick_base + 1;

      /* rounding up of dx, label_dx */
      dx = Math.pow(10, int_dx) * base[tick_base];
      major_dx = Math.pow(10, int_dx) * base[major_base];
      label_dx = major_dx;

      do {
         /* number of significant digits */
         if (xmin === 0)
            n_sig1 = 0;
         else
            n_sig1 = Math.floor(Math.log(Math.abs(xmin)) / Math.log(10)) - Math.floor(Math.log(Math.abs(label_dx)) / Math.log(10)) + 1;

         if (xmax === 0)
            n_sig2 = 0;
         else
            n_sig2 = Math.floor(Math.log(Math.abs(xmax)) / Math.log(10)) - Math.floor(Math.log(Math.abs(label_dx)) / Math.log(10)) + 1;

         n_sig1 = Math.max(n_sig1, n_sig2);

         // toPrecision displays 1050 with 3 digits as 1.05e+3, so increase presicion to number of digits
         if (Math.abs(xmin) < 100000)
            n_sig1 = Math.max(n_sig1, Math.floor(Math.log(Math.abs(xmin)) / Math.log(10)) + 1);
         if (Math.abs(xmax) < 100000)
            n_sig1 = Math.max(n_sig1, Math.floor(Math.log(Math.abs(xmax)) / Math.log(10)) + 1);

         /* determination of maximal width of labels */
         let str = (Math.floor(xmin / dx) * dx).toPrecision(n_sig1);
         let ext = ctx.measureText(str);
         maxwidth = ext.width;

         str = (Math.floor(xmax / dx) * dx).toPrecision(n_sig1).stripZeros();
         ext = ctx.measureText(str);
         maxwidth = Math.max(maxwidth, ext.width);
         str = (Math.floor(xmax / dx) * dx + label_dx).toPrecision(n_sig1).stripZeros();
         maxwidth = Math.max(maxwidth, ext.width);

         /* increasing label_dx, if labels would overlap */
         if (maxwidth > 0.5 * label_dx / (xmax - xmin) * width) {
            label_base++;
            label_dx = Math.pow(10, int_dx) * base[label_base];
            if (label_base % 3 === 2 && major_base % 3 === 1) {
               major_base++;
               major_dx = Math.pow(10, int_dx) * base[major_base];
            }
         } else
            break;

      } while (true);
   }

   if (y1 > 0) {
      minor = -minor;
      major = -major;
      text = -text;
      label = -label;
   }

   x_act = Math.floor(xmin / dx) * dx;

   let last_label_x = x1;

   if (line === true)
      ctx.drawLine(x1, y1, x1 + width, y1);

   do {
      if (logaxis)
         x_screen = (Math.log(x_act) - Math.log(xmin)) / (Math.log(xmax) - Math.log(xmin)) * width + x1;
      else
         x_screen = (x_act - xmin) / (xmax - xmin) * width + x1;
      xs = Math.floor(x_screen + 0.5);

      if (x_screen > x1 + width + 0.001)
         break;

      if (x_screen >= x1) {
         if (Math.abs(Math.floor(x_act / major_dx + 0.5) - x_act / major_dx) <
            dx / major_dx / 10.0) {

            if (Math.abs(Math.floor(x_act / label_dx + 0.5) - x_act / label_dx) <
               dx / label_dx / 10.0) {
               /* label tick mark */
               ctx.drawLine(xs, y1, xs, y1 + text);

               /* grid line */
               if (grid != 0 && xs > x1 && xs < x1 + width)
                  ctx.drawLine(xs, y1, xs, y1 + grid);

               /* label */
               if (label != 0) {
                  str = x_act.toPrecision(n_sig1).stripZeros();
                  ext = ctx.measureText(str);
                  ctx.save();
                  ctx.fillStyle = "black";
                  if (xs - ext.width / 2 > x1 &&
                     xs + ext.width / 2 < x1 + width)
                     ctx.fillText(str, xs, y1 + label);
                  ctx.restore();
                  last_label_x = xs + ext.width / 2;
               }
            } else {
               /* major tick mark */
               ctx.drawLine(xs, y1, xs, y1 + major);

               /* grid line */
               if (grid != 0 && xs > x1 && xs < x1 + width)
                  ctx.drawLine(xs, y1 - 1, xs, y1 + grid);
            }

            if (logaxis) {
               dx *= 10;
               major_dx *= 10;
               label_dx *= 10;
            }
         } else
         /* minor tick mark */
            ctx.drawLine(xs, y1, xs, y1 + minor);

         /* for logaxis, also put labes on minor tick marks */
         if (logaxis) {
            if (label != 0) {
               str = x_act.toPrecision(n_sig1).stripZeros();
               ext = ctx.measureText(str);
               ctx.save();
               ctx.fillStyle = "black";
               if (xs - ext.width / 2 > x1 &&
                  xs + ext.width / 2 < x1 + width &&
                  xs - ext.width / 2 > last_label_x + 2)
                  ctx.fillText(str, xs, y1 + label);
               ctx.restore();

               last_label_x = xs + ext.width / 2;
            }
         }
      }

      x_act += dx;

      /* supress 1.23E-17 ... */
      if (Math.abs(x_act) < dx / 100)
         x_act = 0;

   } while (1);
}


function vaxisDraw(ctx, x1, y1, height, line, minor, major, text, label, grid, ymin, ymax, logaxis) {
   let dy, int_dy, frac_dy, y_act, label_dy, major_dy, y_screen, maxwidth;
   let tick_base, major_base, label_base, n_sig1, n_sig2, ys;
   let base = [1, 2, 5, 10, 20, 50, 100, 200, 500, 1000];

   if (x1 > 0)
      ctx.textAlign = "right";
   else
      ctx.textAlign = "left";
   ctx.textBaseline = "middle";
   let textHeight = parseInt(ctx.font.match(/\d+/)[0]);

   if (ymax <= ymin || height <= 0)
      return;

   if (logaxis) {
      dy = Math.pow(10, Math.floor(Math.log(ymin) / Math.log(10)));
      if (dy === 0)
         dy = 1E-10;
      label_dy = dy;
      major_dy = dy * 10;
      n_sig1 = 4;
   } else {
      /* use 6 as min tick distance */
      dy = (ymax - ymin) / (height / 6);

      int_dy = Math.floor(Math.log(dy) / Math.log(10));
      frac_dy = Math.log(dy) / Math.log(10) - int_dy;

      if (frac_dy < 0) {
         frac_dy += 1;
         int_dy -= 1;
      }

      tick_base = frac_dy < (Math.log(2) / Math.log(10)) ? 1 : frac_dy < (Math.log(5) / Math.log(10)) ? 2 : 3;
      major_base = label_base = tick_base + 1;

      /* rounding up of dy, label_dy */
      dy = Math.pow(10, int_dy) * base[tick_base];
      major_dy = Math.pow(10, int_dy) * base[major_base];
      label_dy = major_dy;

      /* number of significant digits */
      if (ymin === 0)
         n_sig1 = 0;
      else
         n_sig1 = Math.floor(Math.log(Math.abs(xmin)) / Math.log(10)) - Math.floor(Math.log(Math.abs(label_dy)) / Math.log(10)) + 1;

      if (ymax === 0)
         n_sig2 = 0;
      else
         n_sig2 = Math.floor(Math.log(Math.abs(ymax)) / Math.log(10)) - Math.floor(Math.log(Math.abs(label_dy)) / Math.log(10)) + 1;

      n_sig1 = Math.max(n_sig1, n_sig2);

      // toPrecision displays 1050 with 3 digits as 1.05e+3, so increase presicion to number of digits
      if (Math.abs(ymin) < 100000)
         n_sig1 = Math.max(n_sig1, Math.floor(Math.log(Math.abs(ymin)) / Math.log(10)) + 1);
      if (Math.abs(ymax) < 100000)
         n_sig1 = Math.max(n_sig1, Math.floor(Math.log(Math.abs(ymax)) / Math.log(10)) + 1);

      /* increase label_dy if labels would overlap */
      while (label_dy / (ymax - ymin) * height < 1.5 * textHeight) {
         label_base++;
         label_dy = Math.pow(10, int_dy) * base[label_base];
         if (label_base % 3 === 2 && major_base % 3 === 1) {
            major_base++;
            major_dy = Math.pow(10, int_dy) * base[major_base];
         }
      }
   }

   if (x1 > 0) {
      minor = -minor;
      major = -major;
      text = -text;
      label = -label;
   }

   y_act = Math.floor(ymin / dy) * dy;

   let last_label_y = y1;

   if (line === true)
      ctx.drawLine(x1, y1, x1, y1 - height);

   do {
      if (logaxis)
         y_screen = y1 - (Math.log(y_act) - Math.log(ymin)) / (Math.log(ymax) - Math.log(ymin)) * height;
      else
         y_screen = y1 - (y_act - ymin) / (ymax - ymin) * height;
      ys = Math.floor(y_screen + 0.5);

      if (y_screen < y1 - height - 0.001)
         break;

      if (y_screen <= y1 + 0.001) {
         if (Math.abs(Math.floor(y_act / major_dy + 0.5) - y_act / major_dy) <
            dy / major_dy / 10.0) {

            if (Math.abs(Math.floor(y_act / label_dy + 0.5) - y_act / label_dy) <
               dy / label_dy / 10.0) {
               /* label tick mark */
               ctx.drawLine(x1, ys, x1 + text, ys);

               /* grid line */
               if (grid != 0 && ys < y1 && ys > y1 - height)
                  ctx.drawLine(x1, ys, x1 + grid, ys);

               /* label */
               if (label != 0) {
                  str = y_act.toPrecision(n_sig1).stripZeros();
                  ctx.save();
                  ctx.fillStyle = "black";
                  if (ys - textHeight / 2 > y1 - height &&
                     ys + textHeight / 2 < y1)
                     ctx.fillText(str, x1 + label, ys);
                  ctx.restore();
                  last_label_y = ys - textHeight / 2;
               }
            } else {
               /* major tick mark */
               cts.drawLine(x1, ys, x1 + major, ys);

               /* grid line */
               if (grid != 0 && ys < y1 && ys > y1 - height)
                  ctx.drawLine(x1, ys, x1 + grid, ys);
            }

            if (logaxis) {
               dy *= 10;
               major_dy *= 10;
               label_dy *= 10;
            }

         } else
         /* minor tick mark */
            ctx.drawLine(x1, ys, x1 + minor, ys);

         /* for logaxis, also put labels on minor tick marks */
         if (logaxis) {
            if (label != 0) {
               str = y_act.toPrecision(n_sig1).stripZeros();
               ctx.save();
               ctx.fillStyle = "black";
               if (ys - textHeight / 2 > y1 - height &&
                  ys + textHeight / 2 < y1 &&
                  ys + textHeight < last_label_y + 2)
                  ctx.fillText(str, x1 + label, ys);
               ctx.restore();

               last_label_y = ys;
            }
         }
      }

      y_act += dy;

      /* supress 1.23E-17 ... */
      if (Math.abs(y_act) < dy / 100)
         y_act = 0;

   } while (1);
}

function mhttpd_resize_sidenav() {
   //console.log("mhttpd_resize_sidenav!");
   let h = document.getElementById('mheader');
   let s = document.getElementById('msidenav');
   let top = h.clientHeight + 1 + "px";
   if (s.style.top !== top) {
      //console.log("httpd_resize_sidenav: top changed from " + s.style.top + " to " + top);
      s.style.top = top;
   }
   let m = document.getElementById('mmain');
   let paddingTop = h.clientHeight + 1 + "px";
   if (m.style.paddingTop !== paddingTop) {
      //console.log("httpd_resize_sidenav: paddingTop changed from " + m.style.paddingTop + " to " + paddingTop);
      m.style.paddingTop = paddingTop;
   }
}

function mhttpd_resize_header() {

}

let mhttpd_last_alarm = 0;

function get_options_path(odb_path) {
   // Convert "/Path/to/something" to "/Path/to/Options something"
   let bits = odb_path.split("/");
   bits[bits.length - 1] = "Options " + bits[bits.length - 1];
   return bits.join("/");
}

function mhttpd_refresh() {

   if (!mhttpd_initialized)
      return;

   if (mhttpd_refresh_id !== undefined)
      window.clearTimeout(mhttpd_refresh_id);

   // don't do a refresh if we are paused
   if (mhttpd_refresh_paused) {
      mhttpd_refresh_id = window.setTimeout(mhttpd_refresh, mhttpd_refresh_interval);
      return;
   }

   // don't update page if document is hidden (minimized, covered tab etc), only play alarms
   if (document.hidden) {

      // only check every 10 seconds for alarm
      if (new Date().getTime() > mhttpd_last_alarm + 10000) {

         // request current alarms
         let req = mjsonrpc_make_request("get_alarms");
         mjsonrpc_send_request([req]).then(function (rpc) {

            let alarms = rpc[0].result;

            // update alarm display
            if (alarms.alarm_system_active) {
               let n = 0;
               for (let a in alarms.alarms)
                  n++;
               if (n > 0)
                  mhttpd_alarm_play();
            }

         }).catch(function (error) {
            if (error.xhr && (error.xhr.readyState === 4) && ((error.xhr.status === 0) || (error.xhr.status === 503))) {
               mhttpd_error('Connection to server broken. Trying to reconnect&nbsp;&nbsp;');
               document.getElementById("mheader_error").appendChild(mhttpd_spinning_wheel);
               mhttpd_reconnect_id = window.setTimeout(mhttpd_reconnect, 1000);
            } else {
               mjsonrpc_error_alert(error);
            }
         });

         mhttpd_last_alarm = new Date().getTime();
      }

      mhttpd_refresh_id = window.setTimeout(mhttpd_refresh, 500);
      return;
   }

   // go through all modb* tags and refresh them -------------------------------------

   let modb = getMElements("modb");

   let modbvalue = getMElements("modbvalue");
   for (let i = 0; i < modbvalue.length; i++) {

      let o = modbvalue[i];
      if (parseInt(o.dataset.input)) {
         if (o.childNodes[0] === undefined) {
            ODBInlineEdit(o, o.dataset.odbPath, 0);
         }
      } else {
         if (parseInt(o.dataset.odbEditable)) {
            if (o.childNodes[0] === undefined || o.childNodes[0].tagName === undefined) {

               // add link and event handler if tag is editable
               let link = document.createElement('a');
               link.href = "#";
               link.onclick = function () {
                  ODBInlineEdit(this.parentElement, this.parentElement.dataset.odbPath, 0);
               };
               link.onfocus = function () {
                  ODBInlineEdit(this.parentElement, this.parentElement.dataset.odbPath, 0);
               };

               o.innerHTML = "";
               o.appendChild(link);
            }
         } else {

            // remove link if present
            if (o.childNodes[0] && o.childNodes[0].tagName === 'A')
               o.innerHTML = o.childNodes[0].innerHTML;
         }
      }

      modbvalue[i].setValue = function (x, tid) {
         if (this.dataset.formula !== undefined)
            x = eval(this.dataset.formula);
         if (tid === undefined)
            tid = TID_DOUBLE;
         this.value = x;
         this.dataset.tid = tid;

         if (this.onchange !== null)
            this.onchange();
         if (this.dataset.odbLoaded === undefined && this.onload !== null) {
            this.onload();
            this.dataset.odbLoaded = "1";
         }
      }

      modbvalue[i].sendValueToOdb = function () {
         mhttpd_refresh_pause(true);
         mjsonrpc_db_set_value(this.dataset.odbPath, this.value).then(() =>
            mhttpd_refresh_pause(false));
      }
   }

   let modbcheckbox = getMElements("modbcheckbox");
   for (let i = 0; i < modbcheckbox.length; i++) {

      // add event listener if missing
      if (!modbcheckbox[i].mEventListener) {
         modbcheckbox[i].addEventListener("click", function () {
            if (this.dataset.validate !== undefined) {
               let flag = eval(this.dataset.validate)(this.checked, this);
               if (!flag) {
                  mhttpd_refresh();
                  return;
               }
            }
            mjsonrpc_db_set_value(this.dataset.odbPath, this.checked ? 1 : 0).then(rpc =>
               mhttpd_refresh());
         });
         modbcheckbox[i].mEventListener = true;
      }
   }

   let modbselect = getMElements("modbselect");
   for (let i = 0; i < modbselect.length; i++) {
      if (modbselect[i].dataset.odbPath) {

         // add event listener if missing
         if (!modbselect[i].mEventListener) {
            modbselect[i].addEventListener("change", function (flag) {

               if (flag !== true) { // flag === true if control changed by ODB update
                  if (this.dataset.validate !== undefined) {
                     let flag = eval(this.dataset.validate)(this.value, this);
                     if (!flag) {
                        mhttpd_refresh();
                        return;
                     }
                  }
                  mjsonrpc_db_set_value(this.dataset.odbPath, this.value).then(rpc =>
                     mhttpd_refresh());
               }
            });
            modbselect[i].mEventListener = true;
         }
      }
   }

   let modbbox = getMElements("modbbox");
   for (let i = 0; i < modbbox.length; i++) {
      modbbox[i].style.border = "1px solid #808080";
   }

   // attach "set" function to all ODB buttons
   let modbbutton = getMElements("modbbutton");
   for (let i = 0; i < modbbutton.length; i++) {
      if (!modbbutton[i].mEventListener) {
         modbbutton[i].addEventListener("click", function () {

            if (this.dataset.validate !== undefined) {
               let flag = eval(this.dataset.validate)(this);
               if (!flag) {
                  mhttpd_refresh();
                  return;
               }
            }

            mjsonrpc_db_set_value(this.dataset.odbPath, this.dataset.odbValue).then(rpc =>
               mhttpd_refresh());
         });
         modbbutton[i].mEventListener = true;
      }
   }

   // replace all horizontal bars with proper <div>'s
   let modbhbar = getMElements("modbhbar");
   for (let i = 0; i < modbhbar.length; i++) {
      if (modbhbar[i].childNodes === undefined || modbhbar[i].childNodes[0] === undefined ||
         modbhbar[i].childNodes[0].tagName !== 'DIV') {
          modbhbar[i].style.display = "block";
          if (modbhbar[i].style.position === "")
              modbhbar[i].style.position = "relative";
          modbhbar[i].style.border = "1px solid #808080";
          let color = modbhbar[i].style.color;
          modbhbar[i].innerHTML = "<div style='background-color:" + color + ";" + "color:black;" +
              "width:0;height:" + modbhbar[i].clientHeight + "px;" +
              "position:relative; display:inline-block;border-right:1px solid #808080'>&nbsp;</div>";

          modbhbar[i].setValue = function (x, tid) {
              if (this.dataset.formula !== undefined)
                  x = eval(this.dataset.formula);
              if (tid === undefined)
                  tid = TID_DOUBLE;
              let mvalue = mie_to_string(tid, x, this.dataset.format);
              if (mvalue === "")
                  mvalue = "(empty)";
              let html = mhttpd_escape("&nbsp;" + mvalue);
              this.value = x;
              if (this.dataset.printValue === "1")
                  this.children[0].innerHTML = html;
              let minValue = parseFloat(this.dataset.minValue);
              let maxValue = parseFloat(this.dataset.maxValue);
              if (isNaN(minValue))
                  minValue = 0;
              if (this.dataset.log === "1" &&
                  minValue === 0)
                  minValue = 1E-3;
              if (isNaN(maxValue))
                  maxValue = 1;
              let percent;
              if (this.dataset.log === "1")
                  percent = Math.round(100 * (Math.log(x) - Math.log(minValue)) /
                      (Math.log(maxValue) - Math.log(minValue)));
              else
                  percent = Math.round(100 * (x - minValue) /
                      (maxValue - minValue));
              if (percent < 0)
                  percent = 0;
              if (percent > 100)
                  percent = 100;
              this.children[0].style.width = percent + "%";
              if (this.onchange !== null)
                  this.onchange();
              if (this.dataset.odbLoaded === undefined && this.onload !== null) {
                  this.onload();
                  this.dataset.odbLoaded = "1";
              }
              this.children[0].style.backgroundColor = this.style.color;
          }
      }
   }

   // replace all vertical bars with proper <div>'s
   let modbvbar = getMElements("modbvbar");
   for (let i = 0; i < modbvbar.length; i++) {
      if (modbvbar[i].childNodes === undefined || modbvbar[i].childNodes[0] === undefined ||
         modbvbar[i].childNodes[0].tagName !== 'DIV') {
         modbvbar[i].style.display = "inline-block";
         if (modbvbar[i].style.position === "")
            modbvbar[i].style.position = "relative";
         modbvbar[i].style.border = "1px solid #808080";
         let color = modbvbar[i].style.color;
         modbvbar[i].innerHTML = "<div style='background-color:" + color + "; height:0; width:100%; position:absolute; bottom:0; left:0; display:inline-block; border-top:1px solid #808080'>&nbsp;</div>";

         modbvbar[i].setValue = function (x, tid) {
            if (this.dataset.formula !== undefined)
               x = eval(this.dataset.formula);
            if (tid === undefined)
               tid = TID_DOUBLE;
            let mvalue = mie_to_string(tid, x, this.dataset.format);
            if (mvalue === "")
               mvalue = "(empty)";
            let html = mhttpd_escape("&nbsp;" + mvalue);
            this.value = x;
            if (this.dataset.printValue === "1")
               this.children[0].innerHTML = html;
            let minValue = parseFloat(this.dataset.minValue);
            let maxValue = parseFloat(this.dataset.maxValue);
            if (isNaN(minValue))
               minValue = 0;
            if (this.dataset.log === "1" &&
               minValue === 0)
               minValue = 1E-3;
            if (isNaN(maxValue))
               maxValue = 1;
            let percent;
            if (this.dataset.log === "1")
               percent = Math.round(100 * (Math.log(x) - Math.log(minValue)) /
                  (Math.log(maxValue) - Math.log(minValue)));
            else
               percent = Math.round(100 * (x - minValue) /
                  (maxValue - minValue));
            if (percent < 0)
               percent = 0;
            if (percent > 100)
               percent = 100;
            this.children[0].style.height = percent + "%";
            if (this.onchange !== null)
               this.onchange();
            if (this.dataset.odbLoaded === undefined && this.onload !== null) {
               this.onload();
               this.dataset.odbLoaded = "1";
            }
            this.children[0].style.backgroundColor = this.style.color;
         }
      }
   }

   // replace all thermometers with canvas
   let modbthermo = getMElements("modbthermo");
   for (let i = 0; i < modbthermo.length; i++) {
       if (modbthermo[i].childNodes === undefined || modbthermo[i].childNodes[0] === undefined ||
          modbthermo[i].childNodes[0].tagName !== 'CANVAS') {
         modbthermo[i].style.display = "inline-block";
         if (modbthermo[i].style.position === "")
            modbthermo[i].style.position = "relative";

         cvs = document.createElement("canvas");
         let w = modbthermo[i].clientWidth;
         let h = modbthermo[i].clientHeight;
         w = Math.floor(w / 4) * 4; // 2 must be devidable by 4
         cvs.width = w + 1;
         cvs.height = h;
         modbthermo[i].appendChild(cvs);
         modbthermo[i].draw = mhttpd_thermo_draw;

         modbthermo[i].setValue = function (x, tid) {
            if (this.dataset.formula !== undefined)
               x = eval(this.dataset.formula);
            if (tid === undefined)
               tid = TID_DOUBLE;
            this.value = x;
            this.dataset.tid = tid;

            if (this.onchange !== null)
               this.onchange();
            if (this.dataset.odbLoaded === undefined && this.onload !== null) {
               this.onload();
               this.dataset.odbLoaded = "1";
            }

            this.draw();
         }

         modbthermo[i].draw();
      }
   }

   // replace all gauges with canvas
   let modbgauge = getMElements("modbgauge");
   for (let i = 0; i < modbgauge.length; i++) {
      if (modbgauge[i].childNodes === undefined || modbgauge[i].childNodes[0] === undefined ||
         modbgauge[i].childNodes[0].tagName !== 'CANVAS') {
         modbgauge[i].style.display = "inline-block";
         if (modbgauge[i].style.position === "")
            modbgauge[i].style.position = "relative";

         let cvs = document.createElement("canvas");
         cvs.width = modbgauge[i].clientWidth;
         cvs.height = modbgauge[i].clientHeight;
         modbgauge[i].appendChild(cvs);
         modbgauge[i].draw = mhttpd_gauge_draw;

         modbgauge[i].setValue = function (x, tid) {
            if (this.dataset.formula !== undefined)
               x = eval(this.dataset.formula);
            if (tid === undefined)
               tid = TID_DOUBLE;
            this.value = x;
            this.dataset.tid = tid;

            if (this.onchange !== null)
               this.onchange();
            if (this.dataset.odbLoaded === undefined && this.onload !== null) {
               this.onload();
               this.dataset.odbLoaded = "1";
            }

            this.draw();
         }

         modbgauge[i].draw();
      }
   }

   // replace all haxis with canvas
   let mhaxis = getMElements("mhaxis");
   for (let i = 0; i < mhaxis.length; i++) {
      if (mhaxis[i].childNodes === undefined || mhaxis[i].childNodes[0] === undefined ||
         mhaxis[i].childNodes[0].tagName !== 'CANVAS') {
         mhaxis[i].style.display = "block";
         if (mhaxis[i].style.position === "")
            mhaxis[i].style.position = "relative";

         let cvs = document.createElement("canvas");
         cvs.width = mhaxis[i].clientWidth + 2;
         cvs.height = mhaxis[i].clientHeight;
         mhaxis[i].appendChild(cvs);
         mhaxis[i].draw = mhttpd_haxis_draw;
         mhaxis[i].draw();
      }
   }

   // replace all vaxis with canvas
   let mvaxis = getMElements("mvaxis");
   for (let i = 0; i < mvaxis.length; i++) {
      if (mvaxis[i].childNodes === undefined || mvaxis[i].childNodes[0] === undefined ||
         mvaxis[i].childNodes[0].tagName !== 'CANVAS') {
         mvaxis[i].style.display = "inline-block";
         if (mvaxis[i].style.position === "")
            mvaxis[i].style.position = "relative";

         let cvs = document.createElement("canvas");
         cvs.width = mvaxis[i].clientWidth;
         cvs.height = mvaxis[i].clientHeight + 2; // leave space for vbar border
         mvaxis[i].appendChild(cvs);
         mvaxis[i].draw = mhttpd_vaxis_draw;
         mvaxis[i].draw();
      }
   }

   // replace all mhistory tags with history plots
   let mhist = getMElements("mhistory");
   for (let i = 0; i < mhist.length; i++) {
      if (mhist[i].childNodes === undefined || mhist[i].childNodes[0] === undefined ||
         mhist[i].childNodes[0].tagName !== 'IMG') {
         let w = mhist[i].style.width;
         if (w === "")
            w = 320;
         else
            w = parseInt(w);
         let h = mhist[i].style.height;
         if (h === "")
            h = 200;
         else
            h = parseInt(h);
         mhist[i].innerHTML = "<img src=\"graph.gif?cmd=oldhistory&group=" +
            mhist[i].dataset.group +
            "&panel=" + mhist[i].dataset.panel +
            "&scale=" + mhist[i].dataset.scale +
            "&width=" + w +
            "&height=" + h +
            "&rnd=" + (new Date().getTime()) +
            "\">";
      }
   }

   // request all modb* elements
   let paths = [];
   let modbs = ["modb", "modbvalue", "modbcheckbox", "modbselect", "modbbox", "modbhbar",
      "modbvbar", "modbthermo", "modbgauge"];

   modbs.forEach((mstr) => {
      getMElements(mstr).forEach((m) => {
         if (m.dataset.odbPath)
            paths.push(m.dataset.odbPath);

         if (mstr === "modbselect" && m.dataset.autoOptions === "1")
            paths.push(get_options_path(m.dataset.odbPath));

      })
   });

   // request ODB contents for all variables
   let req1 = mjsonrpc_make_request("db_get_values", {"paths": paths});

   // request current alarms
   let req2 = mjsonrpc_make_request("get_alarms");

   // request new messages
   let req3 = mjsonrpc_make_request("cm_msg_retrieve", {
      "facility": "midas",
      "time": 0,
      "min_messages": 1
   });

   // request new char messages
   let req4 = mjsonrpc_make_request("cm_msg_retrieve", {
      "facility": "chat",
      "time": 0,
      "min_messages": 1
   });

   mjsonrpc_send_request([req1, req2, req3, req4]).then(function (rpc) {

      // update time in header
      let dstr = mhttpd_get_display_time().string;

      if (document.getElementById("mheader_last_updated") !== undefined) {
         //mhttpd_set_innerHTML(document.getElementById("mheader_last_updated"), dstr);
         mhttpd_set_firstChild_data(document.getElementById("mheader_last_updated"), dstr);
      }

      let idata = 0;

      for (let i = 0; i < modb.length; i++, idata++) {
         if (modb[i].dataset.odbPath === undefined) {
            idata--;
            continue;
         }
         let x = rpc[0].result.data[idata];
         if (typeof x === 'object' && x !== null) {
            // subdircectory
            if (modb[i].value === undefined)
               modb[i].value = JSON.stringify(x);
            if (modb[i].onchange !== null && JSON.stringify(x) !== modb[i].value) {
               modb[i].value = JSON.stringify(x);
               modb[i].onchange();
            }
            if (modb[i].dataset.odbLoaded === undefined && modb[i].onload !== null) {
               modb[i].onload();
               modb[i].dataset.odbLoaded = "1";
            }
         } else {
            // individual value
            if (modb[i].value === undefined)
               modb[i].value = x;
            if (modb[i].onchange !== null && x !== modb[i].value) {
               modb[i].value = x;
               modb[i].onchange();
            }
            if (modb[i].dataset.odbLoaded === undefined && modb[i].onload !== null) {
               modb[i].onload();
               modb[i].dataset.odbLoaded = "1";
            }
         }
      }

      for (let i = 0; i < modbvalue.length; i++, idata++) {
         if (modbvalue[i].dataset.odbPath === undefined) {
            idata--;
            continue;
         }

         if (rpc[0].result.status[idata] === 312) {
            modbvalue[i].innerHTML = "ODB key \"" + modbvalue[i].dataset.odbPath + "\" not found";
         } else {
            let x = rpc[0].result.data[idata];
            let tid = rpc[0].result.tid[idata];

            modbvalue[i].tid = tid;
            if (modbvalue[i].dataset.formula !== undefined)
               x = eval(modbvalue[i].dataset.formula);
            let mvalue = mie_to_string(tid, x, modbvalue[i].dataset.format);
            if (mvalue === "")
               mvalue = "(empty)";
            else if (typeof mvalue === 'string' && mvalue.trim() === "")
               mvalue = "(spaces)";
            let html = mhttpd_escape(mvalue);
            if (parseInt(modbvalue[i].dataset.odbEditable) || parseInt(modbvalue[i].dataset.input)) {
               if (modbvalue[i].childNodes[1] !== undefined &&
                  modbvalue[i].childNodes[1].innerHTML !== undefined) {
                  modbvalue[i].childNodes[1].innerHTML = html;
               } else {
                  if (modbvalue[i].childNodes[0]) // dataset.input could not yet be initialized (AJAX)
                     modbvalue[i].childNodes[0].innerHTML = html;
               }
            } else
               modbvalue[i].innerHTML = html;

            if (modbvalue[i].value !== x && modbvalue[i].onchange !== null) {
               modbvalue[i].value = x;
               if (modbvalue[i].value !== undefined)
                  modbvalue[i].onchange();
            }

            modbvalue[i].value = x;

            if (modbvalue[i].dataset.odbLoaded === undefined && modbvalue[i].onload !== null) {
               modbvalue[i].onload();
               modbvalue[i].dataset.odbLoaded = "1";
            }
         }
      }

      for (let i = 0; i < modbcheckbox.length; i++, idata++) {
         if (modbcheckbox[i].dataset.odbPath === undefined) {
            idata--;
            continue;
         }

         let x = rpc[0].result.data[idata];
         let tid = rpc[0].result.tid[idata];
         let mvalue = mie_to_string(tid, x);
         if (typeof x === "boolean")
            modbcheckbox[i].checked = x;
         else
            modbcheckbox[i].checked = (mvalue !== "0");
         if (modbcheckbox[i].onchange !== null)
            modbcheckbox[i].onchange();
         if (modbcheckbox[i].dataset.odbLoaded === undefined && modbcheckbox[i].onload !== null) {
            modbcheckbox[i].onload();
            modbcheckbox[i].dataset.odbLoaded = "1";
        }
      }

      for (let i = 0; i < modbselect.length; i++, idata++) {
         if (modbselect[i].dataset.odbPath === undefined) {
            idata--;
            continue;
         }

         let x = rpc[0].result.data[idata];
         let auto_options = [];

         if (modbselect[i].dataset.autoOptions == "1") {
            idata++;
            auto_options = rpc[0].result.data[idata];

            if (auto_options !== null && modbselect[i].options.length == 0) {
               for (let opt = 0; opt < auto_options.length; opt++) {
                  modbselect[i].add(new Option(auto_options[opt], auto_options[opt]));
               }
            }
         }

         if (rpc[0].result.status[idata] === 312) {
            modbselect[i].parentElement.innerHTML = "ODB key \"" + modbselect[i].dataset.odbPath + "\" not found";
         } else {
            if (x !== null && modbselect[i].value !== x.toString()) {

               if (modbselect[i].value !== undefined) {
                  // check if option is available
                  let j;
                  for (j = 0; j < modbselect[i].options.length; j++)
                     if (modbselect[i].options[j].value === x.toString())
                        break;

                  // if option is available, set it
                  if (j < modbselect[i].options.length) {
                     modbselect[i].value = x.toString();
                     if (modbselect[i].onchange !== null)
                        modbselect[i].onchange(true);
                  } else {
                     // if not, set blank
                     if (modbselect[i].value !== "")
                        modbselect[i].value = "";
                  }
               }
            }
            if (modbselect[i].dataset.odbLoaded === undefined && modbselect[i].onload !== null) {
               modbselect[i].onload();
               modbselect[i].dataset.odbLoaded = "1";
            }
         }
      }

      for (let i = 0; i < modbbox.length; i++, idata++) {
         if (modbbox[i].dataset.odbPath === undefined) {
            idata--;
            continue;
         }

         let x = rpc[0].result.data[idata];
         if (modbbox[i].dataset.formula !== undefined)
            x = eval(modbbox[i].dataset.formula);
         if (x > 0 || x === true) {
            modbbox[i].style.backgroundColor = modbbox[i].dataset.color;
         } else {
            if (modbbox[i].dataset.backgroundColor !== undefined)
               modbbox[i].style.backgroundColor = modbbox[i].dataset.backgroundColor;
            else
               modbbox[i].style.backgroundColor = "";
         }
         if (modbbox[i].onchange !== null)
            modbbox[i].onchange();
         if (modbbox[i].dataset.odbLoaded === undefined && modbbox[i].onload !== null) {
            modbbox[i].onload();
            modbbox[i].dataset.odbLoaded = "1";
         }
      }

      for (let i = 0; i < modbhbar.length; i++, idata++) {
         if (modbhbar[i].dataset.odbPath === undefined) {
            idata--;
            continue;
         }

         let x = rpc[0].result.data[idata];
         let tid = rpc[0].result.tid[idata];

         modbhbar[i].setValue(x, tid);
      }

      for (let i = 0; i < modbvbar.length; i++, idata++) {
         if (modbvbar[i].dataset.odbPath === undefined) {
            idata--;
            continue;
         }

         let x = rpc[0].result.data[idata];
         let tid = rpc[0].result.tid[idata];

         modbvbar[i].setValue(x, tid);
      }

      for (let i = 0; i < modbthermo.length; i++, idata++) {
         if (modbthermo[i].dataset.odbPath === undefined) {
            idata--;
            continue;
         }

         let x = rpc[0].result.data[idata];
         let tid = rpc[0].result.tid[idata];

         modbthermo[i].setValue(x, tid);
      }

      for (let i = 0; i < modbgauge.length; i++, idata++) {
         if (modbgauge[i].dataset.odbPath === undefined) {
            idata--;
            continue;
         }

         let x = rpc[0].result.data[idata];
         let tid = rpc[0].result.tid[idata];

         modbgauge[i].setValue(x, tid);
      }

      let alarms = rpc[1].result;

      // update alarm display
      let e = document.getElementById('mheader_alarm');
      if (!alarms.alarm_system_active) {
         mhttpd_set_innerHTML(e, "<a href=\"?cmd=Alarms\">Alarms: Off</a>");
         mhttpd_set_className(e, "mgray mbox");
      } else {
         let s = "";
         let n = 0;
         for (let a in alarms.alarms) {
            s += a + ", ";
            n++;
         }
         if (n < 1) {
            mhttpd_set_innerHTML(e, "<a href=\"?cmd=Alarms\">Alarms: None</a>");
            mhttpd_set_className(e, "mgreen mbox");
         } else {
            s = s.slice(0, -2);
            if (n > 1)
               mhttpd_set_innerHTML(e, "<a href=\"?cmd=Alarms\">Alarms: " + s + "</a>");
            else
               mhttpd_set_innerHTML(e, "<a href=\"?cmd=Alarms\">Alarm: " + s + "</a>");
            mhttpd_set_className(e, "mred mbox");

            mhttpd_alarm_play();
         }
      }

      // update messages
      let msg;
      if (rpc[2].result.messages !== undefined) {
         msg = rpc[2].result.messages.split("\n");
         if (msg[msg.length - 1] === "")
            msg = msg.slice(0, -1);
      } else
         msg = undefined;

      // update chat messages
      let chat;
      if (rpc[3].result.messages !== undefined) {
         chat = rpc[3].result.messages.split("\n");
         if (chat[chat.length - 1] === "")
            chat = chat.slice(0, -1);
      } else
         chat = undefined;

      mhttpd_message(msg, chat);
      mhttpd_resize_sidenav();

      if (mhttpd_refresh_interval !== undefined && mhttpd_refresh_interval > 0)
         mhttpd_refresh_id = window.setTimeout(mhttpd_refresh, mhttpd_refresh_interval);

   }).catch(function (error) {

      if (error.xhr && (error.xhr.readyState === 4) && ((error.xhr.status === 0) || (error.xhr.status === 503))) {
         mhttpd_error('Connection to server broken. Trying to reconnect&nbsp;&nbsp;');
         document.getElementById("mheader_error").appendChild(mhttpd_spinning_wheel);
         mhttpd_reconnect_id = window.setTimeout(mhttpd_reconnect, 1000);
      } else {
         mjsonrpc_error_alert(error);
      }
   });
}

function mhttpd_refresh_history() {
   if (mhttpd_refresh_history_id !== undefined)
      window.clearTimeout(mhttpd_refresh_history_id);

   /* this fuction gets called by mhttpd_init to periodically refresh all history panels */

   let mhist = document.getElementsByName("mhistory");
   for (let i = 0; i < mhist.length; i++) {
      let s = mhist[i].childNodes[0].src;

      if (s.lastIndexOf("&rnd=") !== -1) {
         s = s.substr(0, s.lastIndexOf('&rnd='));
         s += "&rnd=" + new Date().getTime();
      }
      mhist[i].childNodes[0].src = s;
   }

   if (mhttpd_refresh_history_interval !== undefined && mhttpd_refresh_history_interval > 0)
      mhttpd_refresh_history_id = window.setTimeout(mhttpd_refresh_history, mhttpd_refresh_history_interval);
}

function mhttpd_reconnect() {
   mjsonrpc_db_ls(["/"]).then(function (rpc) {
      // on successful connection remove error and schedule refresh
      mhttpd_error_clear();
      if (mhttpd_refresh_id !== undefined)
         window.clearTimeout(mhttpd_refresh_id);
      if (mhttpd_refresh_history_id !== undefined)
         window.clearTimeout(mhttpd_refresh_history_id);
      mhttpd_refresh_id = window.setTimeout(mhttpd_refresh, mhttpd_refresh_interval);
      mhttpd_refresh_history_id = window.setTimeout(mhttpd_refresh_history, mhttpd_refresh_history_interval);
   }).catch(function (error) {
      mhttpd_reconnect_id = window.setTimeout(mhttpd_reconnect, 1000);
   });
}

window.addEventListener('resize', mhttpd_resize_message);

function mhttpd_resize_message() {
   //console.log("mhttpd_resize_message() via resize event listener");
   let d = document.getElementById("mheader_message");
   if (d.currentMessage !== undefined && d.style.display !== 'none')
      mhttpd_fit_message(d.currentMessage);
}

function mhttpd_close_message() {
   let d = document.getElementById("mheader_message");

   // remember time of messages to suppress
   mhttpdConfigSet('suppressMessageBefore', d.currentMessageT);

   d.style.display = "none";
   mhttpd_resize_sidenav();
}

function mhttpd_fit_message(m) {
   let d = document.getElementById("mheader_message");
   let cross = "&nbsp;&nbsp;&nbsp;<span style=\"cursor: pointer;\" onclick=\"mhttpd_close_message();\">&#9587;</span>";
   let link1 = "<span style=\"cursor: pointer;\" onclick=\"window.location.href='?cmd=Messages'\">";
   let link2 = "</span>";
   d.style.display = "inline-block";

   // limit message to fit parent element

   let parentWidth = d.parentNode.offsetWidth;
   let maxWidth = parentWidth - 30;

   // check if the full message fits

   d.innerHTML = link1 + m + link2 + cross;
   //console.log("mhttpd_fit_message: len: " + d.offsetWidth + ", max: " + maxWidth + ", message: " + m);
   if (d.offsetWidth <= maxWidth) {
      return;
   }

   // check if the message minus timestamp and type fits

   m = m.substr(m.indexOf(']')+1);
   d.innerHTML = link1 + m + link2 + cross;
   let w = d.offsetWidth;
   //console.log("mhttpd_fit_message: len: " + w + ", max: " + maxWidth + ", message: " + m);
   if (w <= maxWidth) {
      return;
   }

   // guess the length assuming fix pixels per char

   let charWidth = w/m.length;
   let guessLength = maxWidth/charWidth - 3; // 3 chars of "..."

   let g = m.substr(0, guessLength);
   d.innerHTML = link1 + g + "..." + link2 + cross;
   w = d.offsetWidth;
   //console.log("mhttpd_fit_message: char: " + charWidth + ", guess: " + guessLength + ", len: " + w + ", max: " + maxWidth);

   // grow or shrink our guess
   
   if (w < maxWidth) {
      //console.log("mhttpd_fit_message: too short, grow");
      for (let i=guessLength+1; i<=m.length; i++) {
         let s = m.substr(0, i);
         d.innerHTML = link1 + s + "..." + link2 + cross;
         w = d.offsetWidth;
         //console.log("mhttpd_fit_message: len: " + w + ", max: " + maxWidth + ", message: " + s);
         if (w <= maxWidth)
            break;
      }
   } else {
      //console.log("mhttpd_fit_message: too long, shrink");
      while (g.length > 0) {
         g = g.substr(0, g.length-1);
         d.innerHTML = link1 + g + "..." + link2 + cross;
         w = d.offsetWidth;
         //console.log("mhttpd_fit_message: len: " + w + ", max: " + maxWidth + ", message: " + g);
         if (w <= maxWidth)
            break;
      }
   }
}

function mhttpd_message(msg, chat) {

   let mTalk = "";
   let mType = "";
   let chatName = "";
   let talkTime = 0;
   let notifyTime = 0;
   let lastMsg = "";
   let lastMsgT = 0;
   let lastChat = "";
   let lastChatT = 0;
   let lastT = 0;
   let m = "";
   let c = "";

   if (msg !== undefined) {
      lastMsg = msg[0].substr(msg[0].indexOf(" ") + 1);
      lastMsgT = parseInt(msg[0]);
   }

   if (chat !== undefined) {
      lastChat = chat[0].substr(chat[0].indexOf(" ") + 1);
      lastChatT = parseInt(chat[0]);
      if (chat[0].length > 0)
         mTalk = chat[0].substr(chat[0].indexOf("]") + 2);

      chatName = lastChat.substring(lastChat.indexOf("[") + 1, lastChat.indexOf(","));
      lastChat = lastChat.substr(0, lastChat.indexOf("[")) +
         "<b>" + chatName + ":</b>" +
         lastChat.substr(lastChat.indexOf("]") + 1);
   }

   if (lastChatT > lastMsgT) {
      m = lastChat;
      c = "var(--mblue)";
      mType = "USER";
      talkTime = lastChatT;
      notifyTime = lastChatT;
      lastT = lastChatT;
   } else {
      m = lastMsg;
      c = "var(--myellow)";
      mTalk = lastMsg.substr(lastMsg.indexOf("]") + 1);
      mType = m.substring(m.indexOf(",") + 1, m.indexOf("]"));
      talkTime = lastMsgT;
      notifyTime = lastMsgT;
      lastT = lastMsgT;
   }

   if (m !== "") {
      let d = document.getElementById("mheader_message");
      if (d !== undefined && d !== null && d.currentMessage !== m &&
         (mhttpdConfig().suppressMessageBefore === undefined || lastT > mhttpdConfig().suppressMessageBefore)) {

         d.style.removeProperty("-webkit-transition");
         d.style.removeProperty("transition");

         if (mType === "USER" && mhttpdConfig().displayChat  ||
             mType === "TALK" && mhttpdConfig().displayTalk  ||
             mType === "ERROR" && mhttpdConfig().displayError ||
             mType === "INFO" && mhttpdConfig().displayInfo ||
             mType === "LOG" && mhttpdConfig().displayLog) {

            let first = (d.currentMessage === undefined);
            d.currentMessage = m; // store full message in user-defined attribute
            d.currentMessageT = lastMsgT; // store message time in user-defined attribute

            mhttpd_fit_message(m);
            d.age = new Date() / 1000;

            if (first) {
               if (m.search("ERROR]") > 0) {
                  d.style.backgroundColor = "var(--mred)";
                  d.style.color = "white";
               }
            } else {

               // manage backgroud color (red for errors, fading yellow for others)
               if (m.search("ERROR]") > 0) {
                  d.style.removeProperty("-webkit-transition");
                  d.style.removeProperty("transition");
                  d.style.backgroundColor = "var(--mred)";
               } else {
                  d.age = new Date() / 1000;
                  d.style.removeProperty("-webkit-transition");
                  d.style.removeProperty("transition");
                  d.style.backgroundColor = c;
               }
            }
         }

         if (mTalk !== "") {
            if (mType === "USER" && mhttpdConfig().speakChat) {
               // do not speak own message
               if (document.getElementById("chatName") === undefined || 
                   document.getElementById("chatName") === null || 
                   document.getElementById("chatName").value !== chatName) {
                  mhttpd_speak(talkTime, mTalk);
               }
            } else if (mType === "TALK" && mhttpdConfig().speakTalk) {
               mhttpd_speak(talkTime, mTalk);
            } else if (mType === "ERROR" && mhttpdConfig().speakError) {
               mhttpd_speak(talkTime, mTalk);
            } else if (mType === "INFO" && mhttpdConfig().speakInfo) {
               mhttpd_speak(talkTime, mTalk);
            } else if (mType === "LOG" && mhttpdConfig().speakLog) {
               mhttpd_speak(talkTime, mTalk);
            }
         }

         if (mType === "USER" && mhttpdConfig().notifyChat  ||
            mType === "TALK" && mhttpdConfig().notifyTalk  ||
            mType === "ERROR" && mhttpdConfig().notifyError ||
            mType === "INFO" && mhttpdConfig().notifyInfo ||
            mType === "LOG" && mhttpdConfig().notifyLog) {

            // avoid double notification if page gets reloaded
            // console.log("notifyTime: " + notifyTime + " last: " + mhttpdConfig().var.lastNotify);
            if (mhttpdConfig().var.lastNotify === undefined || notifyTime > mhttpdConfig().var.lastNotify) {
               mhttpdConfigSet("var.lastNotify", notifyTime);

               if (("Notification" in window)) {
                  if (Notification.permission === "granted") {
                     const n = new Notification("MIDAS", {
                        body: mTalk,
                        icon: "./apple-touch-icon.png"
                     });
                  } else if (Notification.permission !== "denied") {
                     Notification.requestPermission().then((p) => {
                        if (p === "granted") {
                           const n = new Notification("MIDAS", {
                              body: mTalk,
                              icon: "./apple-touch-icon.png"
                           });
                        }
                     });
                  }
               }
            }
         }


      }
      let t = new Date() / 1000;
      if (t > d.age + 5 && d.style.backgroundColor === "var(--myellow)") {
         let backgroundColor = "var(--mgray)";
         d.style.setProperty("-webkit-transition", "background-color 3s", "");
         d.style.setProperty("transition", "background-color 3s", "");
         d.style.backgroundColor = backgroundColor;
      }
   }
}

function mhttpd_error(error) {
   let d = document.getElementById("mheader_error");
   if (d !== undefined) {
      error += "<div style=\"display: inline; float: right; padding-right: 10px; cursor: pointer;\"" +
         " onclick=\"document.getElementById(&quot;mheader_error&quot;).style.zIndex = 0;\">&#9587;</div>";
      d.innerHTML = error;
      d.style.zIndex = 3; // above header
   }
}

function mhttpd_error_clear() {
   if (document.getElementById("mheader_error")) {
      document.getElementById("mheader_error").innerHTML = "";
      document.getElementById("mheader_error").style.zIndex = 0; // below header
   }
}

function mhttpd_create_page_handle_create(mouseEvent) {
   let path = "";
   let type = "";
   let name = "";
   let arraylength = "";
   let stringlength = "";

   let form = document.getElementsByTagName('form')[0];

   if (form) {
      path = form.elements['odb'].value;
      type = form.elements['type'].value;
      name = form.elements['value'].value;
      arraylength = form.elements['index'].value;
      stringlength = form.elements['strlen'].value;
   } else {
      let e = document.getElementById("odbpath");
      path = JSON.parse(e.innerHTML);
      if (path === "/") path = "";

      type = document.getElementById("create_tid").value;
      name = document.getElementById("create_name").value;
      arraylength = document.getElementById("create_array_length").value;
      stringlength = document.getElementById("create_strlen").value;

      //alert("Path: " + path + " Name: " + name);
   }

   if (path === "/") path = "";

   if (name.length < 1) {
      dlgAlert("Name is too short");
      return false;
   }

   let int_array_length = parseInt(arraylength);

   //alert("int_array_length: " + int_array_length);

   if (!int_array_length || int_array_length < 1) {
      dlgAlert("Bad array length: " + arraylength);
      return false;
   }

   let int_string_length = parseInt(stringlength);

   if (!int_string_length || int_string_length < 1) {
      dlgAlert("Bad string length " + stringlength);
      return false;
   }

   let param = {};
   param.path = path + "/" + name;
   param.type = parseInt(type);
   if (int_array_length > 1)
      param.array_length = int_array_length;
   if (int_string_length > 0)
      param.string_length = int_string_length;

   mjsonrpc_db_create([param]).then(function (rpc) {
      let status = rpc.result.status[0];
      if (status === 311) {
         dlgMessage("Error", "ODB entry with this name already exists.", true, true, function () {
            location.search = "?cmd=odb&odb_path=" + path; // reloads the document
         });
      } else if (status !== 1) {
         dlgMessage("Error", "db_create_key() error " + status + ", see MIDAS messages.", true, true, function () {
            location.search = "?cmd=odb&odb_path=" + path; // reloads the document
         });
      } else {
         location.search = "?cmd=odb&odb_path=" + path; // reloads the document
      }
   }).catch(function (error) {
      mjsonrpc_error_alert(error);
      //location.search = "?cmd=odb&odb_path="+path; // reloads the document
   });

   return false;
}

function mhttpd_create_page_handle_cancel(mouseEvent) {
   dlgHide('dlgCreate');
   return false;
}

function mhttpd_link_page_handle_link(mouseEvent) {
   let e = document.getElementById("link_odbpath");
   let path = JSON.parse(e.innerHTML);
   if (path === "/") path = "";
   //let path   = document.getElementById("odb_path").value;
   let name = document.getElementById("link_name").value;
   let target = document.getElementById("link_target").value;

   //console.log("Path: " + path + " Name: " + name + " Target: [" + target + "]");

   if (name.length < 1) {
      dlgAlert("Name is too short");
      return false;
   }

   if (target.length <= 1) {
      dlgAlert("Link target is too short");
      return false;
   }

   let param = {};
   param.new_links = [path + "/" + name];
   param.target_paths = [target];

   mjsonrpc_call("db_link", param).then(function (rpc) {
      let status = rpc.result.status[0];
      if (status === 304) {
         dlgMessage("Error", "Invalid link, see MIDAS messages.", true, true, function () {
            //location.search = "?cmd=odb&odb_path="+path; // reloads the document
         });
      } else if (status === 311) {
         dlgMessage("Error", "ODB entry with this name already exists.", true, true, function () {
            //location.search = "?cmd=odb&odb_path="+path; // reloads the document
         });
      } else if (status === 312) {
         dlgMessage("Error", "Target path " + target + " does not exist in ODB.", true, true, function () {
            //location.search = "?cmd=odb&odb_path="+path; // reloads the document
         });
      } else if (status === 315) {
         dlgMessage("Error", "ODB data type mismatch, see MIDAS messages.", true, true, function () {
            //location.search = "?cmd=odb&odb_path="+path; // reloads the document
         });
      } else if (status !== 1) {
         dlgMessage("Error", "db_create_link() error " + status + ", see MIDAS messages.", true, true, function () {
            location.search = "?cmd=odb&odb_path=" + path; // reloads the document
         });
      } else {
         location.search = "?cmd=odb&odb_path=" + path; // reloads the document
      }
   }).catch(function (error) {
      mjsonrpc_error_alert(error);
      //location.search = "?cmd=odb&odb_path="+path; // reloads the document
   });

   return false;
}

function mhttpd_link_page_handle_cancel(mouseEvent) {
   dlgHide('dlgLink');
   return false;
}

function mhttpd_delete_page_handle_delete(mouseEvent, xpath) {
   let form = document.getElementsByTagName('form')[0];
   let path;
   let names = [];

   if (form) {
      path = form.elements['odb'].value;

      if (path === "/") path = "";

      for (let i = 0; ; i++) {
         let n = "name" + i;
         let v = form.elements[n];
         if (v === undefined)
            break;
         if (v.checked)
            names.push(path + "/" + v.value);
      }
   } else {
      let e = document.getElementById("odbpath");
      path = JSON.parse(e.innerHTML);
      if (path === "/") path = "";

      //alert("Path: " + path);

      for (i = 0; ; i++) {
         let v = document.getElementById("delete" + i);
         if (v === undefined || v === null)
            break;
         if (v.checked) {
            let name = JSON.parse(v.value);
            if (name.length > 0) {
               names.push(path + "/" + name);
            }
         }
      }

      //alert("Names: " + names);
      //return false;
   }

   if (names.length < 1) {
      dlgAlert("Please select at least one ODB entry to delete.");
      return false;
   }

   //alert(names);

   let params = {};
   params.paths = names;
   mjsonrpc_call("db_delete", params).then(function (rpc) {
      let message = "";
      let status = rpc.result.status;
      //alert(JSON.stringify(status));
      for (let i = 0; i < status.length; i++) {
         if (status[i] !== 1) {
            message += "Cannot delete \"" + rpc.request.params.paths[i] + "\", db_delete_key() status " + status[i] + "\n";
         }
      }
      if (message.length > 0) {
         dlgAlert(message);
      } else {
         location.search = "?cmd=odb&odb_path=" + path; // reloads the document
      }
   }).catch(function (error) {
      mjsonrpc_error_alert(error);
      //location.search = "?cmd=odb&odb_path="+path; // reloads the document
   });

   return false;
}

function mhttpd_delete_page_handle_cancel(mouseEvent) {
   dlgHide('dlgDelete');
   return false;
}

function mhttpd_start_run(ret) {
   mhttpd_goto_page("Start", ret); // DOES NOT RETURN
}

function mhttpd_stop_run(ret) {
   dlgConfirm('Are you sure to stop the run?', function (flag) {
      if (flag === true) {
         mjsonrpc_call("cm_transition", {"transition": "TR_STOP"}).then(function (rpc) {
            //mjsonrpc_debug_alert(rpc);
            if (rpc.result.status !== 1) {
               throw new Error("Cannot stop run, cm_transition() status " + rpc.result.status + ", see MIDAS messages");
            }
            mhttpd_goto_page("Transition", ret); // DOES NOT RETURN
         }).catch(function (error) {
            mjsonrpc_error_alert(error);
         });
      }

   });
}

function mhttpd_pause_run(ret) {
   dlgConfirm('Are you sure to pause the run?', function (flag) {
      if (flag === true) {
         mjsonrpc_call("cm_transition", {"transition": "TR_PAUSE"}).then(function (rpc) {
            //mjsonrpc_debug_alert(rpc);
            if (rpc.result.status !== 1) {
               throw new Error("Cannot pause run, cm_transition() status " + rpc.result.status + ", see MIDAS messages");
            }
            mhttpd_goto_page("Transition", ret); // DOES NOT RETURN
         }).catch(function (error) {
            mjsonrpc_error_alert(error);
         });
      }
   });
}


function mhttpd_resume_run(ret) {
   dlgConfirm('Are you sure to resume the run?', function (flag) {
      if (flag === true) {
         mjsonrpc_call("cm_transition", {"transition": "TR_RESUME"}).then(function (rpc) {
            //mjsonrpc_debug_alert(rpc);
            if (rpc.result.status !== 1) {
               throw new Error("Cannot resume run, cm_transition() status " + rpc.result.status + ", see MIDAS messages");
            }
            mhttpd_goto_page("Transition", ret); // DOES NOT RETURN
         }).catch(function (error) {
            mjsonrpc_error_alert(error);
         });
      }
   });
}

function mhttpd_cancel_transition() {
   dlgConfirm('Are you sure to cancel the currently active run transition?', function (flag) {
      if (flag === true) {
         let paths = [];
         let values = [];

         paths.push("/Runinfo/Requested Transition");
         values.push(0);
         paths.push("/Runinfo/Transition in progress");
         values.push(0);

         let params = {};
         params.paths = paths;
         params.values = values;

         mjsonrpc_call("db_paste", params).then(function (rpc) {
            //mjsonrpc_debug_alert(rpc);
            if ((rpc.result.status[0] !== 1) || (rpc.result.status[1] !== 1)) {
               throw new Error("Cannot cancel transition, db_paste() status " + rpc.result.status + ", see MIDAS messages");
            }
            mhttpd_goto_page("Transition"); // DOES NOT RETURN
         }).catch(function (error) {
            mjsonrpc_error_alert(error);
         });
      }
   });
}

function mhttpd_reset_alarm(alarm_name) {
   mjsonrpc_call("al_reset_alarm", {"alarms": [alarm_name]}).then(function (rpc) {
      //mjsonrpc_debug_alert(rpc);
      if (rpc.result.status[0] !== 1 && rpc.result.status[0] !== 1004) {
         throw new Error("Cannot reset alarm, status " + rpc.result.status + ", see MIDAS messages");
      }
   }).catch(function (error) {
      mjsonrpc_error_alert(error);
   });
}

/*---- site and session storage ----------------------------*/

/*
 Usage:

 flag = mhttpdConfig().speakChat;     // read

 mhttpdConfigSet('speakChat', false); // write individual config

 let c = mhttpdConfig();              // write whole config
 c.speakChat = false;
 c.... = ...;
 mhttpdConfigSetAll(c);


 Saves settings are kept in local storage, which gets
 cleared when the browser session ends. Then the default
 values are returned.
 */

let mhttpd_config_defaults = {
   'chatName': "",

   'pageTalk': true,
   'pageError': true,
   'pageInfo': true,
   'pageLog': false,

   'displayChat': true,
   'displayTalk': true,
   'displayError': true,
   'displayInfo': false,
   'displayLog': false,

   'speakChat': true,
   'speakTalk': true,
   'speakError': false,
   'speakInfo': false,
   'speakLog': false,

   'displayChat': true,
   'displayTalk': true,
   'displayError': true,
   'displayInfo': true,
   'displayLog': false,

   'speakVoice': 'Alex',
   'speakVolume': 1,

   'alarmSound': true,
   'alarmSoundFile': 'beep.mp3',
   'alarmRepeat': 60,
   'alarmVolume': 1,

   'timezone': 'local',

   'var': {
      'lastSpeak': 0,
      'lastAlarm': 0
   },

   'suppressMessageBefore': 0,
   'showMenu': true,

   'facility': 'midas',

   'fontSize': '10'
};

function mhttpdConfigODB(callback) {
   let c = mhttpd_config_defaults;
   try {
      if (localStorage.mhttpd) {
         c = JSON.parse(localStorage.mhttpd);
         callback();
      } else {
         // obtain some defaults from ODB
         mjsonrpc_db_get_value("/Experiment/Enable sound").then(function (rpc) {
            let flag = rpc.result.data[0];
            if (flag === null) {
               mjsonrpc_db_create([{"path" : "/Experiment/Enable sound", "type" : TID_BOOL}]).then(function (rpc) {
                  mjsonrpc_db_paste(["/Experiment/Enable sound"],[true]).then(function(rpc) {}).catch(function(error) {
                     mjsonrpc_error_alert(error); });
               }).catch(function (error) {
                  mjsonrpc_error_alert(error);
               });
            } else if (flag === false) {
               c.speakChat = false;
               c.speakTalk = false;
               c.speakError = false;
               c.speakInfo = false;
               c.speakLog = false;
               c.alarmSound = false;
               localStorage.setItem('mhttpd', JSON.stringify(c));
            }
            callback();
         }).catch(function (error) {
            mjsonrpc_error_alert(error);
         });
      }
   } catch (e) {
   }
}

function mhttpdConfig() {
   let c = mhttpd_config_defaults;
   try {
      if (localStorage.mhttpd)
         c = JSON.parse(localStorage.mhttpd);

      // if element has been added to mhttpd_config_defaults, merge it
      if (Object.keys(c).length !== Object.keys(mhttpd_config_defaults).length) {
         for (let o in mhttpd_config_defaults)
            if (!(o in c))
               c[o] = mhttpd_config_defaults[o];
      }
   } catch (e) {
   }

   return c;
}

function mhttpdConfigSet(item, value) {
   try {
      let c = mhttpdConfig();
      if (item.indexOf('.') > 0) {
         let c1 = item.substring(0, item.indexOf('.'));
         let c2 = item.substring(item.indexOf('.') + 1);
         c[c1][c2] = value;
      } else
         c[item] = value;
      localStorage.setItem('mhttpd', JSON.stringify(c));
   } catch (e) {
   }
}

function mhttpdConfigSetAll(new_config) {
   try {
      localStorage.setItem('mhttpd', JSON.stringify(new_config));
   } catch (e) {
   }
}

/*---- sound and speak functions --------------------------*/

let last_audio = null;
let inside_new_audio = false;
let count_audio = 0;

//
// Do not delete the following code. It is not used at this moment,
// but may become necessary in the future. Included
// is an alternative method of allocating and releasing Audio objects
// and code for tracing Audio object life time and activity. It is needed
// to debug interactions between Audio objects and throttled javascript code
// in inactive tabs; and with the "user did not interact with this tab" business.
// K.O. Jan 2021.
//
//function mhttpd_alarm_done() {
//   let ended;
//   if (last_audio) {
//      ended = last_audio.ended;
//      last_audio = null;
//   }
//   count_audio_done++;
//   console.log(Date() + ": mhttpd_alarm_done: created: " + count_audio_created + ", done: " + count_audio_done + ", last_ended: " + ended);
//}
//
//function mhttpd_audio_loadeddata(e) {
//   console.log(Date() + ": mhttpd_audio_loadeddata: counter " + e.target.counter);
//}
//
//function mhttpd_audio_canplay(e) {
//   console.log(Date() + ": mhttpd_audio_canplay: counter " + e.target.counter);
//}
//
//function mhttpd_audio_canplaythrough(e) {
//   console.log(Date() + ": mhttpd_audio_canplaythrough: counter " + e.target.counter);
//}
//
//function mhttpd_audio_ended(e) {
//   console.log(Date() + ": mhttpd_audio_ended: counter " + e.target.counter);
//}
//
//function mhttpd_audio_paused(e) {
//   console.log(Date() + ": mhttpd_audio_paused: counter " + e.target.counter);
//}

function mhttpd_alarm_play_now() {
   if (last_audio) {
      //
      // NOTE:
      // check for playing the alarm sound is done every few minutes.
      // it takes 3 seconds to play the alarm sound
      // so in theory, this check is not needed: previous alarm sound should
      // always have finished by the time we play a new sound.
      //
      // However, observed with google-chrome, in inactive and/or iconized tabs
      // javascript code is "throttled" and the above time sequence is not necessarily
      // observed.
      //
      // Specifically, I see the following sequence: after 1-2 days of inactivity,
      // (i.e.) the "programs" page starts consuming 10-15% CPU, if I open it,
      // CPU use goes to 100% for a short while, memory use goes from ~50 Mbytes
      // to ~150 Mbytes. console.log() debug print out shows that there are ~400
      // accumulated Audio() objects that have been created but did not finish playing
      // (because google-chrome is throttling javascript in inactive tabs?), and now
      // they all try to load the mp3 file and play all at the same time.
      //
      // This fix for this observed behaviour is to only create new Audio() objects
      // if previous one finished playing.
      //
      // K.O.
      //
      if (!last_audio.ended) {
         console.log(Date() + ": mhttpd_alarm_play: Cannot play alarm sound: previous alarm sound did not finish playing yet");
         return;
      }
   }

   if (inside_new_audio) {
      console.log(Date() + ": mhttpd_alarm_play: Cannot play alarm sound: already inside \"new Audio()\"");
      return;
   }
   inside_new_audio = true;

   // reference counting of Audio objects. Not in use today. Do not remove. K.O. Jan 2021
   //console.log(Date() + ": mhttpd_alarm_play: created: " + count_audio_created + ", done: " + count_audio_done + ", last_ended: " + ended + ", audio.play!");
   //count_audio_created++;

   let audio = new Audio(mhttpdConfig().alarmSoundFile);
   audio.volume = mhttpdConfig().alarmVolume;
   audio.counter = ++count_audio;

   last_audio = audio;
   inside_new_audio = false;

   // code for tracing activity of Audio objects. do not remove. K.O. Jan 2021
   //audio.addEventListener("loadeddata", mhttpd_audio_loadeddata);
   //audio.addEventListener("canplay", mhttpd_audio_canplay);
   //audio.addEventListener("canplaythrough", mhttpd_audio_canplaythrough);
   //audio.addEventListener("ended", mhttpd_audio_ended);
   //audio.addEventListener("paused", mhttpd_audio_paused);

   let promise = audio.play();
   if (promise) {
      promise.then(function(e) {
         //console.log(Date() + ": mhttpd_alarm_play: promise fulfilled, counter " + audio.counter);
      }).catch(function(e) {
         //count_audio_done++;
         //console.log(Date() + ": mhttpd_alarm_play: audio.play() exception: " + e + ", created: " + count_audio_created + ", done: " + count_audio_done);
         console.log(Date() + ": mhttpd_alarm_play: Cannot play alarm sound: audio.play() exception: " + e + ", counter " + audio.counter);
         // NB: must clear the URL of the sound file, otherwise, the sound file is still loaded (but not played)
         // the loading of sound files is observed to be delayed in inactive tabs
         // resulting in many (100-1000) pending loads getting queued, observed
         // to all of them attempt to run (load the sound file) in parallel,
         // consuming memory (100-300 Mbytes) and CPU (10-15% CPU per inactive tab).
         audio.src = "";
         // NB: setting audio to pause() does not seem to do anything.
         audio.pause();
         last_audio = null;
      });
   }
   //audio.pause();
}

function mhttpd_alarm_play() {
   if (mhttpdConfig().alarmSound && mhttpdConfig().alarmSoundFile) {
      let now = new Date() / 1000;
      let last = mhttpdConfig().var.lastAlarm;
      let next = last + parseFloat(mhttpdConfig().alarmRepeat);
      let wait = next - now;
      let do_play = (now > next);
      //console.log("mhttpd_alarm_play: now: " + now + ", next: " + next + ", last: " + last + ", wait: " + wait + ", play: " + do_play);
      if (do_play) {
         mhttpdConfigSet("var.lastAlarm", now);
         mhttpd_alarm_play_now();
      }
   }
}

function mhttpd_speak_now(text) {
   let u = new SpeechSynthesisUtterance(text);
   u.voice = speechSynthesis.getVoices().filter(function (voice) {
      return voice.name === mhttpdConfig().speakVoice;
   })[0];
   u.volume = mhttpdConfig().speakVolume;
   speechSynthesis.speak(u);
}

function mhttpd_speak(time, text) {

   if (!('speechSynthesis' in window))
      return;

   if (mhttpdConfig().speakChat) {
      if (time > mhttpdConfig().var.lastSpeak) {
         mhttpdConfigSet("var.lastSpeak", time);
         mhttpd_speak_now(text);
      }
   }
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * js-indent-level: 3
 * indent-tabs-mode: nil
 * End:
 */
