let eqtable_style = `
.mtable {
   border-spacing: 0;
}
.mvarheader {
   border-bottom-left-radius: 0;
   border-bottom-right-radius: 0;
   border-right: 1px solid white;
}
.mtable td {
   padding: 5px;
   border-right: 1px solid white;
}
.mtable th {
   padding: 5px;
   text-align: left;
}
.mthselect {
   color:white;
   font-weight:bold;
   text-align-last:center;
   border: none;
   outline: none;
   background-color: transparent;
   width:100%;
}
`;

let estyle = document.createElement('style');
estyle.textContent = eqtable_style;
document.head.appendChild(estyle);

<!-- Select file dialog -->
let dlgFileSelect = `
<div class="dlgTitlebar">Select file</div>
<div class="dlgPanel">
   <br />
   <div style="margin: auto;width: 70%">
      <input type="file" id="fileSelector" accept=".json">
   </div>
   <br /><br />
   <button class="dlgButton" onclick="importFileFromSelector(this.parentNode.parentNode);dlgHide('dlgFileSelect');">Load</button>
   <button class="dlgButton" onclick="dlgHide('dlgFileSelect')">Cancel</button>
</div>
`;

let equipment = {
   "name" : "",
   "tid": "",
   "table" : [],
   "vars": [],
   "group": []
};

function valueChanged(t) {

   let td = t.parentElement;

   // check if value really changed
   if (t.oldValue === undefined) {
      t.oldValue = t.value;
      return;
   }
   if (t.value === t.oldValue)
      return;
   t.oldValue = t.value;

   td.style.backgroundColor = 'var(--myellow)';
   td.style.setProperty("-webkit-transition", "", "");
   td.style.setProperty("transition", "", "");

   window.setTimeout(() => {
      td.style.setProperty("-webkit-transition", "background-color 1s", "");
      td.style.setProperty("transition", "background-color 1s", "");
      td.style.backgroundColor = "";
   }, 1000);
}

function redirectEq(div, eqName) {
   // update URL manually
   let url = window.location.href;
   if (url.search("&eq=") !== -1) {
      url = url.slice(0, url.search("&eq="));
      url += "&eq=" + eqName;
      if (url !== window.location.href)
         window.history.replaceState({}, "Equipment", url);
   }

   // delete lower part of old table
   let table = document.getElementById(equipment.tid);
   table.remove();

   equipment = {
      "name" : "",
      "table" : [],
      "vars": [],
      "group": []
   };

   // add new equipment
   eqtable_init(div, eqName, true, true);
}

function validateData(value, elem) {
   // get selected rows
   let nSel = 0;
   equipment.table.forEach( e => {
      if (e.selected)
         nSel++;
   });

   if (nSel > 1) {
      dlgConfirm("You have selected " + nSel + " values. Do you want to change all of them to " + value + " ?",
         setAll, { "value" : value, "elem": elem });
      return false;
   }
   return true;
}

function setAll(flag, param) {
   if (flag === true) {
      let tr = param.elem.parentNode.parentNode;
      let index;
      for (let i=0 ; i<tr.childNodes.length ; i++)
         if (tr.childNodes[i] === param.elem.parentNode) {
            index = i;
            break;
         }
      equipment.table.forEach(e => {
         if (index && e.selected) {
            let m = e.tr.childNodes[index].firstChild;
            m.setValue(param.value);
            m.sendValueToOdb();
         }
      });
   }
}
function goToOdb() {
   window.location.href = "?cmd=odb&odb_path=/Equipment/" + equipment.name;
}

function selectGroup(t) {
   for (let g of equipment.group) {
      let trGroup = document.getElementsByName(g);

      for (let tr of trGroup)
         tr.style.display = (g === t.value || t.value === "All") ? "table-row" : "none";
   }
}
function eqtable_init(div, eqName, bButtons, bEqSelect) {

   if (eqName === undefined)
      equipment.name = new URLSearchParams(window.location.search).get('eq');
   else
      equipment.name = eqName;

   if (equipment.name === undefined || equipment.name === null) {
      dlgAlert("Please specify equipment name in URL with \"eq=<name>\"");
      return;
   }

   // add equipment table to <div>
   if (div === undefined) {
      dlgAlert("Please specify <div> element when calling eqtable_init()");
      return;
   }

   equipment.tid = 'eqTable' + equipment.name.replace(/\s+/g, ''); // remove all spaces from eq

   let html = '<table id="' + equipment.tid +'" class="mtable" style="visibility: hidden">';

   if (bEqSelect)
      html +=
         '<tr>\n' +
         '   <td id="tableHeader" class="mtableheader">\n' +
         '      <select class="mthselect" id="eqSelect" onchange="redirectEq(document.getElementById(\'' + div.id + '\'), this.value)">\n' +
         '      </select>' +
         '   </td>' +
         '</tr>';

   if (bButtons)
      html += `
         <tr>
            <td id="menuButtons" style="border-radius: 12px;">
               <button class="dlgButton" id="btnSave" title="Save current setting" onclick="saveFilePicker();">Save</button>
               <button class="dlgButton" id="btnLoad" title="Load setting from file" style="display: none" onclick="loadFilePicker();">Load</button>
               <button class="dlgButton" id="btnExport" title="Export current setting" onclick="exportFileQuery();">Export</button>
               <button class="dlgButton" id="btnImport" title="Import current setting" style="display: none" onclick="importFile();">Import</button>
               <button class="dlgButton" id="btnODB" title="Go to ODB" style="float: right" onclick="goToOdb();">ODB</button>
            </td>
         </tr>`;

   html += '</table>';

   div.innerHTML = html;

   mjsonrpc_db_get_value('/Equipment/' + equipment.name).then(
      rpc => {
         let eq = rpc.result.data[0];
         if (eq === null) {
            dlgAlert('Equipment \"' + equipment.name + '\" does not exist in ODB',
               () => window.location.href = ".");
            return;
         }

         if (eq.variables === null) {
            dlgAlert('No valid \"/Equipment/'+ equipment.name + '/Variables\" in ODB',
               () => window.location.href = ".");
            return;
         }

         let table = document.getElementById(equipment.tid);
         let columns = 0;
         let nRows = 0;
         let gridDisplay = true;

         if (document.getElementById('eqSelect'))
            document.getElementById('eqSelect').innerHTML = equipment.name;
         if (document.getElementById('tableHeader'))
            document.getElementById('tableHeader').style.width = "300px";

         // obtain lengths of elements under /Variables
         let nVar = 0;
         for (const e in eq.variables) {
            if (e.includes('/'))
               continue;

            nVar++;
            let v = eq.variables[e];
            let len = v.length ? v.length : 1;
            if (nRows === 0)
               nRows = len;
            if (nRows !== len) {
               gridDisplay = false;
               break;
            }
         }

         if (nVar < 2)
            gridDisplay = false;

         // overwrite gridDisplay flag from ODB if present
         if (eq.settings["grid display"] === undefined) {
            eq.settings["grid display"] = gridDisplay;

            let p = "/Equipment/" + equipment.name + "/Settings/Grid display";
            mjsonrpc_db_create([{"path" : p, "type" : TID_BOOL}]).then(function (rpc) {
               mjsonrpc_db_paste([p],[gridDisplay]).then(function(rpc) {}).catch(function(error) {
                  mjsonrpc_error_alert(error); });
            }).catch(function (error) {
               mjsonrpc_error_alert(error);
            });

         } else
            gridDisplay = eq.settings["grid display"];

         if (eq.settings && eq.settings.names !== undefined && !Array.isArray(eq.settings.names))
            eq.settings.names = [eq.settings.names];

         let groupDisplay = false;
         let groupList = [];
         if (eq.settings && eq.settings.names !== undefined)
            groupDisplay = eq.settings.names.every(e => e.includes('%'));

         if (groupDisplay) {
            let tr = table.insertRow();
            let td = tr.insertCell();
            td.id = "groupSelector";
            let h = "Group:&nbsp;&nbsp;";

            eq.settings.names.forEach( n => {
               let g = n.substring(0, n.indexOf('%'));
               if (!groupList.includes(g))
                  groupList.push(g);
            });

            h += "<select onchange='selectGroup(this)'>";
            groupList.forEach(g => {
               h += "<option value=\"" + g + "\">" + g + "</option>";
            });
            h += "<option value=\"All\">- All -</option>";
            h += "</select>";
            td.innerHTML = h;

            equipment.group = groupList;
         }

         if (gridDisplay) {

            // assemble display list
            let display = [];

            if (eq.settings && eq.settings.display) {
               display = eq.settings.display.split(',').map(item => item.trim());
            } else {
               display.push("#");
               display.push("Names");
               let v = Object.entries(eq.variables)
                  .filter(([key, value]) => key.includes('/name'))
                  .map(([key, value]) => value);
               display = display.concat(v);
            }

            let tr = table.insertRow();

            for (const title of display) {
               let th = document.createElement('th');
               th.innerHTML = (title === "Names" ? "Name" : title);
               th.className = "mtableheader mvarheader";

               let subdir;
               if (eq.variables[title.toLowerCase()] !== undefined)
                  subdir = "/Variables/";
               else if (eq.settings && eq.settings[title.toLowerCase()] !== undefined)
                  subdir = "/Settings/";

               if (eq.settings && eq.settings['unit ' + title.toLowerCase()]) {
                  th.colSpan = 2;
                  columns++;
               }

               tr.appendChild(th);
               columns++;

               let ed = (eq.settings && eq.settings.editable && eq.settings.editable.includes(title));

               // enable load/import if we have editable variables
               if (ed && bButtons) {
                  document.getElementById('btnLoad').style.display = "inline";
                  document.getElementById('btnImport').style.display = "inline";
               }

               equipment.vars.push({
                  'name': title,
                  'editable': ed,
                  'subdir': subdir
               });
            }

            for (let i = 0; i < nRows ; i++) {
               if (equipment.table.length <= i)
                  equipment.table.push({});

               equipment.table[i].selected = false;
               equipment.table[i].lastSelected = false;

               // create new row, install listener and set style
               let tr = table.insertRow();

               tr.id = "spTr";
               tr.addEventListener('mousedown', mouseEvent);
               tr.addEventListener('mousemove', mouseEvent);
               tr.addEventListener('mouseout', mouseEvent);

               // catch all mouseup events to stop dragging
               document.addEventListener('mouseup', mouseEvent);

               tr.style.userSelect = 'none';

               if (groupDisplay) {
                  let group = eq.settings.names[i];
                  group = group.substring(0, group.indexOf('%'));
                  tr.setAttribute('name', group);
                  if (groupList.length > 0 && group !== groupList[0])
                     tr.style.display = "none";
               }

               equipment.table[i].tr = tr;

               // iterate over all variables in equipment
               for (let v of equipment.vars) {

                  let td = tr.insertCell();
                  td.style.textAlign = "right";

                  if (v.name === "#") {
                     td.innerHTML = i.toString();
                     td.style.width = "10px";
                  } else if (v.name === "Names") {
                     td.style.textAlign = "center";
                     if (eq.settings && eq.settings.names) {
                        if (groupDisplay)
                           td.innerHTML = eq.settings.names[i].substring(eq.settings.names[i].indexOf('%') + 1);
                        else
                           td.innerHTML = eq.settings.names[i];
                     }
                  } else {
                     let format = eq.settings && eq.settings['format ' + v.name.toLowerCase()];
                     if (format === undefined || format === null)
                        format = "%f2"; // default format
                     else if (Array.isArray(format))
                        format = format[i];

                     if (v.editable) {

                        if (typeof eq.settings[v.name.toLowerCase()] === 'boolean' ||
                           (typeof eq.settings[v.name.toLowerCase()] === 'object' &&
                              typeof eq.settings[v.name.toLowerCase()][0] === 'boolean')) {
                           td.innerHTML = "<select class=\"modbselect\" " +
                              "data-odb-path=\"/Equipment/" + equipment.name + v.subdir + v.name + "[" + i + "]\">" +
                              "<option value='false'>No</option>" +
                              "<option value='true'>Yes</option>" +
                              "</select>";
                        } else {
                           td.style.width = "90px";
                           td.style.height = "22px";
                           td.innerHTML = "<div class=\"modbvalue\" " +
                              "data-odb-path=\"/Equipment/" + equipment.name + v.subdir + v.name + "[" + i + "]\" " +
                              "data-odb-editable='1' " +
                              "data-size='10' " +
                              "data-format=\"" + format + "\" " +
                              "data-validate='validateData' " +
                              "onchange='valueChanged(this);' " +
                              "></div>";
                        }
                     } else
                        td.innerHTML = "<div class=\"modbvalue\" " +
                           "data-odb-path=\"/Equipment/" + equipment.name + v.subdir + v.name + "[" + i + "]\" " +
                           "data-format=\"" + format + "\" " +
                           "onchange='valueChanged(this);' " +
                           "></div>";

                     // Unit
                     if (eq.settings && eq.settings['unit ' + v.name.toLowerCase()]) {

                        let unit = eq.settings['unit ' + v.name.toLowerCase()];
                        if (Array.isArray(unit))
                           unit = unit[i];

                        td = tr.insertCell();
                        td.style.width = "10px";
                        td.innerHTML = unit;
                     } else
                        td.style.textAlign = 'center';

                  }
               }

               document.getElementById(equipment.tid).style.visibility = '';

            }

         } else { // ------------ non-grid display

            let n = 0;

            for (const e in eq.variables) {
               if (!e.includes('/name'))
                  continue;

               let vName = eq.variables[e];
               let vArr = eq.variables[vName.toLowerCase()];
               let editable = (eq.settings && eq.settings.editable && eq.settings.editable.includes(vName));

               // skip subdirectories
               if (typeof vArr === 'object' && !Array.isArray(vArr))
                  continue;

               equipment.vars.push({
                  'name': vName,
                  'editable': editable
               });

               // enable load/import if we have editable variables
               if (editable && bButtons) {
                  document.getElementById('btnLoad').style.display = "inline";
                  document.getElementById('btnImport').style.display = "inline";
               }

               let tr = table.insertRow();

               // Index
               let th = document.createElement('th');
               th.className = "mtableheader mvarheader";
               th.innerHTML = "#";
               tr.appendChild(th);
               columns = 1;

               // Name
               if (eq.settings && eq.settings['names ' + vName.toLowerCase()] !== undefined) {
                  let th = document.createElement('th');
                  th.className = "mtableheader mvarheader";
                  th.innerHTML = "Name";
                  tr.appendChild(th);
                  columns = 2;
               }

               th = document.createElement('th');
               th.innerHTML = vName;
               th.className = "mtableheader mvarheader";
               th.colSpan = 2;
               tr.appendChild(th);
               columns += 2;

               // variables
               if (!Array.isArray(vArr))
                  vArr = [vArr];
               for (let i=0 ; i<vArr.length ; i++) {

                  equipment.table.push({});

                  equipment.table[n].selected = false;
                  equipment.table[n].lastSelected = false;

                  // create new row, install listener and set style
                  let tr = table.insertRow();

                  tr.id = "spTr";
                  tr.addEventListener('mousedown', mouseEvent);
                  tr.addEventListener('mousemove', mouseEvent);
                  tr.addEventListener('mouseout', mouseEvent);

                  // catch all mouseup events to stop dragging
                  document.addEventListener('mouseup', mouseEvent);

                  tr.style.userSelect = 'none';

                  if (groupDisplay) {
                     let group = eq.settings.names[i];
                     group = group.substring(0, group.indexOf('%'));
                     tr.setAttribute('name', group);
                     if (groupList.length > 0 && group !== groupList[0])
                        tr.style.display = "none";
                  }

                  equipment.table[n].tr = tr;

                  // index and name
                  if (eq.settings && eq.settings['names ' + vName.toLowerCase()] !== undefined) {
                     td = tr.insertCell();
                     td.innerHTML = i;
                     td = tr.insertCell();
                     if (Array.isArray(eq.settings['names ' + vName.toLowerCase()])) {
                        td.innerHTML = eq.settings['names ' + vName.toLowerCase()][i]
                     } else {
                        td.innerHTML = eq.settings['names ' + vName.toLowerCase()];
                     }
                  } else {
                     td = tr.insertCell();
                     td.innerHTML = vName + '[' + i + ']';
                  }

                  // units
                  let unit;
                  if (eq.settings && Array.isArray(eq.settings['unit ' + vName.toLowerCase()]))
                     unit = eq.settings['unit ' + vName.toLowerCase()];
                  else if (eq.settings && eq.settings['unit ' + vName.toLowerCase()])
                     unit = new Array(vArr.length).fill(eq.settings['unit ' + vName.toLowerCase()]);

                  // value
                  td = tr.insertCell();

                  let format;
                  if (eq.settings)
                     format = eq.settings['format ' + vName.toLowerCase()];
                  if (format === undefined || format === null)
                     format = new Array(vArr.length).fill("%f2"); // default format

                  if (editable) {
                     td.style.width = "90px";
                     td.style.height = "22px";
                     td.style.textAlign = "right";
                     td.innerHTML = "<div class=\"modbvalue\" " +
                        "data-odb-path=\"/Equipment/" + equipment.name + "/Variables/" + vName + "[" + i + "]\" " +
                        "data-odb-editable='1' " +
                        "data-size='10' " +
                        "data-format=\"" + format[i] + "\" " +
                        "onchange='valueChanged(this);' " +
                        "></div>";
                  } else {
                     td.style.textAlign = "right";
                     td.innerHTML = "<div class=\"modbvalue\" " +
                        "data-odb-path=\"/Equipment/" + equipment.name + "/Variables/" + vName + "[" + i + "]\" " +
                        "data-format=\"" + format[i] + "\" " +
                        "onchange='valueChanged(this);' " +
                        "></div>";
                  }

                  td = tr.insertCell();
                  td.style.width = "10px";

                  if (unit)
                     td.innerHTML = unit[i];
                  else
                     td.innerHTML = " ";

                  n++;
               }
            }

            document.getElementById(equipment.tid).style.visibility = '';
         }

         if (document.getElementById('tableHeader'))
            document.getElementById('tableHeader').colSpan = columns;
         if (document.getElementById('menuButtons'))
            document.getElementById('menuButtons').colSpan = columns;
         if (groupDisplay)
            document.getElementById('groupSelector').colSpan = columns;

         // populate equipment table
         if (bEqSelect) {
            mjsonrpc_db_ls(["/Equipment"]).then(
               rpc => {
                  let eqList = rpc.result.data[0];
                  let sel = document.getElementById('eqSelect');
                  for (eq in eqList) {
                     let o = document.createElement('option');
                     o.text = eq;
                     sel.add(o);
                  }
                  sel.value = equipment.name;
               });
         }

         let d = document.getElementById(equipment.tid);
         if (d) {
            d = d.parentNode.parentNode.parentNode;
            if (d.tagName === 'DIV')
               dlgCenter(d);
         }

         document.addEventListener('mousedown', removeSelection);
         mhttpd_refresh();
      });
}

let allowIncDec = false;

function saveFilePicker() {
   file_picker("equipment", "*.json", saveFile, true, equipment.name, true);
}

function saveFile(filename) {
   mjsonrpc_db_copy(['/Equipment/' + equipment.name + '/Variables']).then(rpc => {

      if (filename.indexOf('.json') === -1)
         filename += '.json';

      let header = {
         "/MIDAS version": "2.1",
         "/filename": filename,
         "/ODB path": "/Equipment/" + equipment.name + "/Variables"
      }
      header = JSON.stringify(header, null, '   ');
      header = header.substring(0, header.length-2) + ','; // strip trailing '}'

      let odbJson = JSON.stringify(rpc.result.data[0], null, '   ');
      if (odbJson.indexOf('{') === 0)
         odbJson = odbJson.substring(1); // strip leading '{'

      odbJson = header + odbJson;

      file_save_ascii(filename, odbJson, "Equipment \"" + equipment.name +
         "\" saved to file \"" + filename + "\"");

   }).catch(error => mjsonrpc_error_alert(error));
}

function exportFileQuery() {
   dlgQuery("Enter filename: ", "default.json", exportFile);
}

function exportFile(filename) {
   if (filename === false)
      return;
   mjsonrpc_db_copy(['/Equipment/' + equipment.name + '/Variables']).then(rpc => {

      if (filename.indexOf('.json') === -1)
         filename += '.json';

      let header = {
         "/MIDAS version": "2.1",
         "/filename": filename,
         "/ODB path": "/Equipment/" + equipment.name + "/Variables"
      }
      header = JSON.stringify(header, null, '   ');
      header = header.substring(0, header.length-2) + ','; // strip trailing '}'

      let odbJson = JSON.stringify(rpc.result.data[0], null, '   ');
      if (odbJson.indexOf('{') === 0)
         odbJson = odbJson.substring(1); // strip leading '{'

      odbJson = header + odbJson;

      // use trick from FileSaver.js
      let a = document.getElementById('downloadHook');
      if (a === null) {
         a = document.createElement("a");
         a.style.display = "none";
         a.id = "downloadHook";
         document.body.appendChild(a);
      }

      let blob = new Blob([odbJson], {type: "text/json"});
      let url = window.URL.createObjectURL(blob);

      a.href = url;
      a.download = filename;
      a.click();
      window.URL.revokeObjectURL(url);
      dlgAlert("Equipment \"" + equipment.name +
         "\" downloaded to file \"" + filename + "\"");

   }).catch(error => mjsonrpc_error_alert(error));
}

function loadFilePicker() {
   file_picker("equipment", "*.json", loadFile);
}

function loadFile(filename) {
   file_load_ascii(filename, (text) => fileInterprete(filename, text) );
}

async function importFile() {
   // Chrome has file picker, others might have not
   try {
      let fileHandle;
      [fileHandle] = await window.showOpenFilePicker();
      const file = await fileHandle.getFile();
      const text = await file.text();
      fileInterprete(file.name, text);
   } catch (error) {
      if (error.name !== 'AbortError') {
         // fall-back to old method
         if (document.getElementById('dlgFileSelect') === null) {
            let fsDiv = document.createElement('div');
            fsDiv.id = "dlgFileSelect";
            fsDiv.className = "dlgFrame";
            fsDiv.innerHTML = dlgFileSelect;
            document.body.appendChild(fsDiv);
         }
         dlgShow('dlgFileSelect', true);
      }
   }
}

function importFileFromSelector() {

   let input = document.getElementById('fileSelector');
   let file = input.files[0];
   if (file !== undefined) {
      let reader = new FileReader();
      reader.readAsText(file);

      reader.onerror = function () {
         dlgAlert('File read error: ' + reader.error);
      }

      reader.onload = function () {
         fileInterprete(file.name, reader.result);
      }
   }
}
function fileInterprete(filename, text) {
   try {
      let j = JSON.parse(text);

      // check for correct equipment
      let path = j["/ODB path"].split('/');
      if (path[2] !== equipment.name) {
         dlgAlert("File contains settings for equipment \"" + path[2] + "\"<br />"+
            "and therefore cannot be loaded into equipment \"" + equipment.name +"\"");
         return;
      }

      mhttpd_refresh_pause(true);

      for (let v of equipment.vars)
         if (v.editable)
            modbset("/Equipment/" + equipment.name + "/Variables/" + v.name, j[v.name]);

      mhttpd_refresh_pause(false);

      window.setTimeout(mhttpd_refresh, 10);

      mjsonrpc_cm_msg("Values loaded from file \"" + filename + "\"");
   } catch (error) {
      dlgAlert("File \"" + filename + "\" is not a valid JSON file:<br /><br />" + error);
   }
}

function renderSelection() {
   // change color according to selection
   for (let i=0 ; i<equipment.table[i].length ; i++) {
      if (equipment.table[i].selected) {
         equipment.table[i].tr.style.backgroundColor = '#004CBD';
         equipment.table[i].tr.style.color = '#FFFFFF';

         // change <a> color
         if (equipment.table[i].tr.getElementsByTagName('a').length > 0)
            equipment.table[i].tr.getElementsByTagName('a')[0].style.color = '#FFFFFF';
      } else {
         equipment.table[i].tr.style.backgroundColor = '';
         equipment.table[i].tr.style.color = '';
         
         // change <a> color
         let a = equipment.table[i].tr.getElementsByTagName('a');
         if (a.length > 0) {
            a = a[0];
            a.style.color = '';
         }
      }
   }
}

let mouseDragged = false;

// function gets called from document
function removeSelection(e) {
   // don't de-select if we click on "->"
   if (e.target.tagName === 'BUTTON')
      return;

   for (let i = 0; i < equipment.table.length; i++) {
      equipment.table[i].selected = false;
      equipment.table[i].lastSelected = false;
   }
   renderSelection();
}

function getSelectedElements() {
   let sel = [];
   for (let i = 0; i < equipment.table.length; i++)
      if (equipment.table[i].selected)
         sel.push(i);

   return sel;
}

function mouseEvent(e) {

   // keep off secondary mouse buttons
   if (e.button !== 0)
      return;

   // keep off drop down boxes
   if (e.target.tagName === 'SELECT')
      return;

   e.preventDefault();
   e.stopPropagation();

   // find current row
   let index;
   for (index = 0; index < equipment.table[index].length; index++)
      if (equipment.table[index].tr === e.target ||
         equipment.table[index].tr.contains(e.target))
         break;
   if (index === equipment.table.length)
      index = undefined;

   // ignore clicks to <button>'a and <a>'s
   if (e.target.tagName === 'A' || e.target.tagName === 'BUTTON' || e.target.tagName === 'IMG')
      return;

   if (e.type === 'mousedown') {
      mouseDragged = true;

      if (e.shiftKey) {

         // search last selected element
         let i1;
         for (i1 = 0 ; i1 < equipment.table.length ; i1++)
            if (equipment.table[i1].lastSelected)
               break;
         if (i1 === equipment.table.length)
            i1 = 0; // none selected, so use first one

         if (i1 > index)
            [i1, index] = [index, i1];

         for (let i=i1 ; i<= index ; i++)
            equipment.table[i].selected = true;

      } else if (e.metaKey || e.ctrlKey) {

         // just toggle current selection
         equipment.table[index].selected = !equipment.table[index].selected;

         // remember which row was last selected
         for (let i=0 ; i<equipment.table.length ; i++)
            equipment.table[i].lastSelected = false;
         if (equipment.table[index].selected)
            equipment.table[index].lastSelected = true;

      } else {
         // no key pressed -> un-select all but current
         for (let i = 0; i < equipment.table.length; i++) {
            equipment.table[i].selected = false;
            equipment.table[i].lastSelected = false;
         }

         equipment.table[index].selected = true;
         equipment.table[index].lastSelected = true;
      }

      renderSelection();
   }

   if (e.type === 'mousemove' && mouseDragged) {

      // unselect all
      for (let i = 0; i < equipment.table.length; i++)
         equipment.table[i].selected = false;

      // current row
      let i2;
      for (i2 = 0; i2 < equipment.table.length; i2++)
         if (equipment.table[i2].tr === e.target ||
            equipment.table[i2].tr.contains(e.target))
            break;

      if (i2 < equipment.table.length) {
         // search last selected element
         let i1;
         for (i1 = 0 ; i1 < equipment.table.length ; i1++)
            if (equipment.table[i1].lastSelected)
               break;
         if (i1 === equipment.table.length)
            i1 = 0; // none selected, so use first one

         if (i1 > i2)
            [i1, i2] = [i2, i1];

         for (let i=i1 ; i<= i2 ; i++)
            equipment.table[i].selected = true;

         renderSelection();
      }
   }

   if (e.type === 'mouseup')
      mouseDragged = false;

}

function dlgEquipment(eqName, bButtons = false, bEqSelect = false) {

   // check if equipment exists in ODB
   mjsonrpc_db_ls(['/Equipment/' + eqName]).then(
      rpc => {
         let eq = rpc.result.data[0];
         if (eq === null) {
            dlgAlert('dlgEquipment: Equipment \"' + eqName + '\" does not exist in ODB');
            return;
         }

         if (eq.variables === null) {
            dlgAlert('dlgEquipment: No valid \"/Equipment/' + eqName + '/Variables\" in ODB');
            return;
         }

         let d = document.getElementById('dlgEqTitle');

         // If dialog is already there, reuse it.
         if (!d) {
            d = document.createElement("div");
            d.className = "dlgFrame";
            d.style.zIndex = "30";
            d.shouldDestroy = true;

            d.innerHTML = "<div class=\"dlgTitlebar\" id=\"dlgEqTitle\">" + eqName + "</div>" +
               "<div class=\"dlgPanel\" style=\"padding: 4px;\">" + "<div id=\"dlgEquipment\">" + "</div></div>";
            document.body.appendChild(d);
            dlgShow(d);
         }

         eqtable_init(document.getElementById('dlgEquipment'), eqName, bButtons, bEqSelect);

         if (bEqSelect) {
            let eqSelect = document.getElementById('eqSelect');
            eqSelect.onchange = (event) => {
               redirectEq(document.getElementById('dlgEquipment'), event.target.value);
            };
         }
      });

}


