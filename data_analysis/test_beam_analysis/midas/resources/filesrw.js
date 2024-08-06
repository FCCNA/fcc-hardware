/*

   File read/write functions using mjsonrpc calls 

   Created by Zaher Salman on April 25th, 2023

   Usage
   =====

   Open file selection dialogs to load and save files. The accessible
   files are restricted to folders within the

   experiment_directory/userfiles/

   To use the file picker, you have to include this file in your custom page via

   <scropt src="filesrw.js"></script>

   Then you can use the file picker like:
   
   To load a file: file_picker(subfolder, extension, callback, false, param, mkdir);
   To save a file: file_picker(subfolder, extension, callback, true, param, mkdir);

   - subfolder - is the subfolder of experiment_directory/userfiles/ to browse
   - extension - show only files with this extension
   - callback  - (optional) function to call when save/load is pressed (or file doubleclicked)
                 the callback function will receive the selected file name as an argument
   - saveflag  - (optional) false for load dialog and true for save dialog
   - param     - (optional) extra parameter to pass to the callback function
   - mkdir     - (optional) true to allow creating subfolders

   Featrures of the file_picker:

   - Navigate using the keyboard to look for file names starting with the pressed key sequence
   - Click on the colum headers (Name, Modified and Size) to sort the files accordingly.
   - Click on the column header edge to change the width of the columns.
   - Click on the folder with plus sign to create a new folder (if enabled)

   Additional helper functions:
   
   Save ascii file: file_save_ascii(filename, text, alert) 

   - filename - name of the file to save (relative to experiment_directory/userfiles/)
   - text     - ascii content of the file
   - alert    - (opt) string to be shown in dlgAlert when file is saved

   Save ascii file: file_save_ascii_overwrite(filename, text, alert) 

   - filename - name of the file to save (relative to experiment_directory/userfiles/)
   - text     - ascii content of the file
   - alert    - (opt) string to be shown in dlgAlert when file is saved. Unlike
                file_save_ascii, alert can be a function that will be called alert(filename).

   Load ascii file: file_load_ascii(filename, callback)

   - filename - name of the file to load (relative to experiment_directory/userfiles/)
   - callback - call back function recieving the ascii content of the file

   Load binary file: file_load_bin(filename, callback)

   - filename - name of the file to load (relative to experiment_directory/userfiles/)
   - callback - call back function recieving the binary content of the file

*/



var filesrw_css = `
option:nth-of-type(even) { 
  background-color: #DDDDDD; 
}

option:nth-of-type(odd) { 
  background-color: #EEEEEE; 
}

.colTitle {
  display: inline-block;
  font-weight: bold;
  height: 1.2em;
  text-align: left;
  padding-top: 5px;
  padding-bottom: 0px;
  padding-left: 0px;
  padding-right: 0px;
  overflow: hidden;
}

a {
  color: #0000FF;
  cursor: pointer;
}

.filesTable {
  white-space: nowrap;
  overflow: auto;
  /*height: 100px;*/
  vertical-align: top;
  padding-top: 0px;
  padding: 0px;
  border-spacing: 0px;
  border: 0;
  table-layout:fixed;
  margin-top: 0;
}

.filesTable td {
  overflow: hidden;
  text-align: left;
  height: 1.2em;
  text-overflow: ellipsis;
}

.filesTable th {
  position: sticky;
  top: 0;
  z-index: 31;
  font-weight: bold;
  background-color: #C0C0C0;
  text-align: left;
  vertical-align: middle;
  padding-top: 6px;
  padding-bottom: 0px;
  padding-right: 2px;
  padding-left: 2px;
  user-select: none;
  overflow: hidden;
  height: 1.2em;
  border-top: 0;
  border-bottom: 0;
  border-left: 0;
  /* border-right: 1px dotted black; */
  border-right: 1px solid #A9A9A9;
  border-collapse: collapse;
}

.filesTable th:last-child {
  border-right: 0;
}

.filesTable tr:hover {
  /*background-color: #004CBD;*/
  cursor: pointer;
}

.filesTable tr.selFile td {
  background-color: #004CBD;
  color: white;
  user-select: none;
}

.hlkeys{
    background-color: yellow; 
    color: black;
    font-weight: bold; 
}
`;

const fstyle = document.createElement('style');
fstyle.textContent = filesrw_css;
document.head.appendChild(fstyle);

// Load or save file from server, possible only restricted path
function file_browser(pathName, ext, funcCall, saveFlag, param = {}) {
   /* Function to browse files to load/save
      pathName - relative subdirectory of files to load
      ext      - extension of files to load
      funcCall - (opt) call back function when/if file is selected
      flag     - (opt) true makes it a save dialog
      param    - (opt) extra parameter to pass to funcCall
   */

   let depthDir = sessionStorage.getItem("depthDir") || 0;
   
   const d = select_file_dialog("dlgPrompt");
   const fContainer = document.getElementById("fContainer");
   const dlgButton = document.getElementById("dlgButton");
   dlgButton.textContent = saveFlag ? "Save" : "Load";
   if (saveFlag) {
      const lblFilename = document.createElement("label");
      lblFilename.innerHTML = "Save as: ";
      lblFilename.style.textAlign = "left";
      lblFilename.style.width = "100px";
      lblFilename.style.paddingBottom = "5px";
      lblFilename.style.verticalAlign = "middle";
      const dlgFilename = document.createElement("input");
      dlgFilename.id = "dlgFilename";
      dlgFilename.type = "text";
      dlgFilename.style.width = "calc(100% - 100px)";
      fContainer.appendChild(lblFilename);
      fContainer.appendChild(dlgFilename);
   }

   // Focus on fileSelect so user can scroll by typing
   const sel = document.getElementById("fileSelect");
   sel.focus();
   // Empty selection - different
   sel.innerHTML = "";

   const folders = pathName.split("/");
   const folderTree = document.getElementById("folderTree");
   folderTree.innerHTML = "";
   let tmpPath = ""
   for (let i = 0; i < folders.length; i++) {
      tmpPath += folders[i];
      let addFolder = document.createElement("a");
      // create closure to capture the current value of `i` and `tmpPath`
      (function (i, tmpPath) {
         addFolder.addEventListener("click", function () {
            // Start all over with clicked folder
            depthDir = i;
            file_browser(tmpPath, ext, funcCall, saveFlag, param);
            sessionStorage.setItem("depthDir", depthDir)
         });
      })(i, tmpPath);
      
      tmpPath += "/";
      if (i == 0) {
         addFolder.innerHTML += '<img style="height:16px;vertical-align: middle;margin-bottom: 2px;" src="icons/slash-square.svg" title="Root folder"> ';
      } else {
         addFolder.innerText = folders[i] + "/";
      }
      folderTree.appendChild(addFolder);
   }

   const req = mjsonrpc_make_request("ext_list_files", {"subdir": pathName, "fileext": ext});
   mjsonrpc_send_request(req).then(function (rpc) {
      const fList = get_list_files(rpc.result.files, rpc.result.subdirs);
      let count = 0;
      let options = fList.map(function (item) {
         let o = document.createElement("option");
         let filename = item.filename;
         let modtime = item.modtime;
         let fsize = item.size;
         // convert modtime from integer to date, here you can change format
         let moddate = new Date(modtime * 1000).toString().split(" GMT")[0];
         
         // fill lines with file names etc.
         if (filename) {
            o.innerHTML = `<span style="width:40%">${filename}</span>
                           <span style="width:60%;float:right;">
                           <span style="width:80%">${moddate}</span>
                           <span style="float:right;width:20%">${fsize}</span>
                           </span>`;
            o.value = filename;
            o.addEventListener("click", function () {
               if (saveFlag) dlgFilename.value = this.value;
            });
            // add calback function if provided
            o.addEventListener("dblclick", function () {
               if (saveFlag) dlgFilename.value = this.value;
               if (funcCall) {
                  funcCall(tmpPath + this.value, param);
               }
	       sessionStorage.removeItem("depthDir");
               d.remove();
            });
         } else {
            let folderName = item.replace(/[\[\]]/g, "");
            o.innerHTML = `<a>${folderName}</a>`;
            const newPath = pathName + "/" + folderName;
            // No dblclick to be mobile friendly
            o.addEventListener("click", function () {
               // Start all over with newPath
               file_browser(newPath, ext, funcCall, saveFlag, param);
               sessionStorage.setItem("depthDir", depthDir + 1)
            });
         }
         count++;
         return o;
      });

      if (options.length == 0) {
         // empty list of files
         let o = document.createElement("option");
         o.innerHTML = "No files found!";
         options = [o];
      }
      
      sel.append(...options);
      sel.size = count + 2;
   
      //  sort the files by modified time
      check_sorting();
   }).catch(function (error) {
      console.error(error);
   });

   dlgButton.addEventListener("click", function () {
      const selFile = sel.value;
      if (saveFlag) dlgFilename.value = selFile;
      if (funcCall) {
         funcCall(pathName + "/" + selFile, param);
      }
      sessionStorage.removeItem("depthDir");
      d.remove();
   });

   return d;
}

// Load or save file from server, possible only restricted path
function file_picker(pathName, ext, funcCall, saveFlag = false, param = {}, crtFldr = false) {
   /* Function to browse files to load/save
      pathName - relative subdirectory of files to load
      ext      - extension of files to load
      funcCall - (opt) call back function when/if file is selected
      flag     - (opt) true makes it a save dialog
      param    - (opt) extra parameter to pass to funcCall
      crtFldr  - (opt) enable create new folder tool
   */

   let depthDir = sessionStorage.getItem("depthDir") || 0;
   
   const d = table_file_dialog("dlgPrompt");
   const fContainer = document.getElementById("fContainer");
   const dlgButton = document.getElementById("dlgButton");
   dlgButton.textContent = saveFlag ? "Save" : "Load";
   if (saveFlag) {
      const lblFilename = document.createElement("label");
      lblFilename.innerHTML = "Save as: ";
      lblFilename.style.textAlign = "left";
      lblFilename.style.width = "100px";
      lblFilename.style.paddingBottom = "5px";
      lblFilename.style.verticalAlign = "middle";
      const dlgFilename = document.createElement("input");
      dlgFilename.id = "dlgFilename";
      dlgFilename.type = "text";
      dlgFilename.style.width = "calc(100% - 100px)";
      fContainer.appendChild(lblFilename);
      fContainer.appendChild(dlgFilename);
      // Focus on filename and fill with previously used value or default
      dlgFilename.focus();
      let tmp_ext = ext != "*.*" ? ext.substring(ext.indexOf(".")+1) : "";
      dlgFilename.value = sessionStorage.getItem("fileName") || "fname." + tmp_ext;
      //dlgFilename.select();
      // replace extension with the correct one if different
      let index = dlgFilename.value.indexOf(".");
      if (index) {
         dlgFilename.value = dlgFilename.value.substring(0,index+1) + tmp_ext;
      } else {
         if (tmp_ext != "") {
            dlgFilename.value = dlgFilename.value + "." + tmp_ext;
         }
      }
      dlgFilename.setSelectionRange(0,index);
      dlgFilename.addEventListener("keypress", function(event) {
         if (event.key === "Enter") {
            event.preventDefault();
            dlgButton.click();
         }
      }); 
   }

   // Show tool bar to create folders if requested
   if (crtFldr)
      document.getElementById("pickerTools").style.display = "block";

   const sel = document.getElementById("fileSelect");
   const folders = pathName.split("/");
   const folderTree = document.getElementById("folderTree");
   // Handle create subdir clicks
   const createSubdir = document.getElementById("createSubdir");
   let subdirValue = "New Folder";
   createSubdir.addEventListener("click", function () {
      dlgQuery("New folder: ",subdirValue,create_subdir,pathName);
   });

   folderTree.innerHTML = "";
   let tmpPath = ""
   for (let i = 0; i < folders.length; i++) {
      tmpPath += folders[i];
      let addFolder = document.createElement("a");
      // create closure to capture the current value of `i` and `tmpPath`
      (function (i, tmpPath) {
         addFolder.addEventListener("click", function () {
            // Start all over with clicked folder
            depthDir = i;
            file_picker(tmpPath, ext, funcCall, saveFlag, param, crtFldr);
            sessionStorage.setItem("depthDir", depthDir);
         });
      })(i, tmpPath);

      tmpPath += "/";
      if (i == 0) {
         addFolder.innerHTML += '<img style="height:16px;vertical-align: middle;margin-bottom: 2px;" src="icons/slash-square.svg" title="Root folder"> ';
      } else {
         addFolder.innerText = folders[i] + "/";
      }
      if (i == folders.length -1) 
         addFolder.id = "lastFolder"; // mark last folder
      folderTree.appendChild(addFolder);
   }

   const req = mjsonrpc_make_request("ext_list_files", {"subdir": pathName, "fileext": ext});
   mjsonrpc_send_request(req).then(function (rpc) {
      if (rpc.result.status !== 1) {
         //throw new Error(pathName + " does not exist, create it first.");
         // Or just create directories and call again
         create_subdir(pathName,"");
      }
      const fList = get_list_files(rpc.result.files, rpc.result.subdirs);
      let countFolder = 0;
      let options = fList.map(function (item) {
         let tr = document.createElement("tr");
         let filename = item.filename;
         let modtime = item.modtime;
         let fsize = item.size;
         // convert modtime from integer to date, here you can change format
         let moddate = new Date(modtime * 1000).toString().split(" GMT")[0];
         
         // fill lines with file names etc.
         if (filename) {
            tr.innerHTML = `<td>${filename}</td>
                           <td>${moddate}</td>
                           <td>${fsize}</td>`;
            tr.addEventListener("click", function () {
               if (saveFlag) dlgFilename.value = this.firstChild.innerText;
               // deselect all
               const selFiles = Array.from(document.getElementsByClassName('selFile'));
               selFiles.forEach(selFile => {
                  selFile.classList.remove('selFile');
               });
               this.classList.add('selFile');
            });
            // add calback function if provided
            tr.addEventListener("dblclick", function () {
               if (saveFlag) {
                  dlgFilename.value = this.firstChild.innerText;
                  // Save the name to use later
                  sessionStorage.setItem("fileName", dlgFilename.value);
               }
               if (funcCall) {
                  tmpPath = tmpPath.replace(/\/\//g, "/");
                  funcCall(tmpPath + this.firstChild.innerText, param);
               }
	       sessionStorage.removeItem("depthDir");
               document.removeEventListener("keydown", tableClickHandler);
               d.remove();
            });
         } else {
            countFolder++;
            let folderName = item.replace(/[\[\]]/g, "");
            tr.innerHTML = `<td colspan="3"><a>${folderName}</a></td>`;
            const newPath = pathName + "/" + folderName;
            // No dblclick to be mobile friendly
            tr.addEventListener("click", function () {
               // Start all over with newPath
               file_picker(newPath, ext, funcCall, saveFlag, param, crtFldr);
               sessionStorage.setItem("depthDir", depthDir + 1)
            });
         }
         return tr;
      });
      
      if (options.length == 0) {
         // empty list of files
         let tr = document.createElement("tr");
         tr.innerHTML = "<td colspan='3'>No files found!</td>";
         options = [tr];
      } 
      
      sel.append(...options);
      // add tabindex and focus on Modified column
      const firstRow = sel.rows[0];
      if (firstRow) {
         const secondCell = firstRow.cells[1];
         if (secondCell) {
            secondCell.setAttribute("tabindex", "0");
            if (typeof dlgFilename === 'undefined') secondCell.focus();
            // sort the files by modified time
            secondCell.click();
         }
      }
   }).catch(function (error) {
      console.error(error);
   });
   
   dlgButton.addEventListener("click", function () {
      const selFiles = Array.from(document.getElementsByClassName('selFile'));
      let selFile;
      if ((!saveFlag && selFiles.length == 0) || (saveFlag && dlgFilename.value == "")) {
         dlgAlert(saveFlag ? "Please select a file or provide a name." : "Please select a file.");
      } else {
         selFile = (typeof dlgFilename === 'undefined') ? selFiles[0].firstChild.innerText : dlgFilename.value;
         // Save the name to use later
         sessionStorage.setItem("fileName", selFile);
         if (funcCall) {
            funcCall(pathName + "/" + selFile, param);
         }
         sessionStorage.removeItem("depthDir");
         document.removeEventListener("keydown", tableClickHandler);
         d.remove();
      }
   });
   

   // allow resizing table columns
   resize_th();

   return d;
}

// Populates a modal with a file load/save options in select
function select_file_dialog(iddiv = "dlgFile", width = 600, height = 350, x, y) {
   /* Load/save file dialog, optional parameters
      iddiv        - give the dialog a name
      width/height - minimal width/height of dialog
      x/y          - initial position of dialog
   */

   // First make sure you removed exisitng iddiv
   if (document.getElementById(iddiv)) document.getElementById(iddiv).remove();
   const d = document.createElement("div");
   d.className = "dlgFrame";
   d.id = iddiv;
   d.style.zIndex = "30";
   // allow resizing modal
   d.style.overflow = "hidden";
   d.style.resize = "both";
   d.style.minWidth = width + "px";
   d.style.minHeight = height + "px";
   d.style.width = width + "px";
   d.style.height = height + "px";
   d.shouldDestroy = true;

   const dlgTitle = document.createElement("div");
   dlgTitle.className = "dlgTitlebar";
   dlgTitle.id = "dlgMessageTitle";
   dlgTitle.innerText = "Select file dialog";
   d.appendChild(dlgTitle);

   const dlgPanel = document.createElement("div");
   dlgPanel.className = "dlgPanel";
   dlgPanel.id = "dlgPanel";
   d.appendChild(dlgPanel);
   
   const dlgFolder = document.createElement("div");
   dlgFolder.innerHTML = '<span id="folderTree"><img style="height:16px;vertical-align: middle;margin-bottom: 2px;" src="icons/slash-square.svg" title="Current folder"> </span>';
   dlgFolder.style.textAlign = "left";
   dlgPanel.appendChild(dlgFolder);

   const filesTable = document.createElement("div");
   const colTitles = document.createElement("div");
   colTitles.style.backgroundColor = "#C0C0C0";
   colTitles.innerHTML = `
     <span id='nameSort' class='colTitle' onclick='check_sorting(this);'>
        &nbsp;Name
        <img id='nameArrow' style='float:right;visibility:hidden;' src='icons/chevron-up.svg'>
     </span>
     <span id='timeSort' class='colTitle' onclick='check_sorting(this);'>
        &nbsp;Modified
        <img id='timeArrow' style='float:right;' src='icons/chevron-down.svg'>
    </span>
    <span id='sizeSort' class='colTitle' onclick='check_sorting(this);'>
        &nbsp;Size
        <img id='sizeArrow' style='float:right;' src='icons/chevron-down.svg'>
    </span>`;
   filesTable.appendChild(colTitles);

   const fileSelect = document.createElement("select");
   fileSelect.id = "fileSelect";
   fileSelect.style.overflow = "auto";
   fileSelect.style.height = "100%";
   fileSelect.style.width = "100%";
   filesTable.appendChild(fileSelect);
   dlgPanel.appendChild(filesTable);

   const fContainer = document.createElement("div");
   fContainer.id = "fContainer";
   fContainer.style.width = "100%";
   fContainer.style.paddingTop = "5px";
   fContainer.style.paddingBottom = "3px";
   dlgPanel.appendChild(fContainer);
   const btnContainer = document.createElement("div");
   btnContainer.id = "btnContainer";
   btnContainer.style.paddingBottom = "3px";
   dlgPanel.appendChild(btnContainer);

   const dlgButton = document.createElement("button");
   dlgButton.className = "dlgButtonDefault";
   dlgButton.id = "dlgButton";
   dlgButton.type = "button";
   btnContainer.appendChild(dlgButton);
   const dlgCancelBtn = document.createElement("button");
   dlgCancelBtn.className = "dlgButton";
   dlgCancelBtn.id = "dlgCancelBtn";
   dlgCancelBtn.type = "button";
   dlgCancelBtn.textContent = "Cancel";
   dlgCancelBtn.addEventListener("click", function () {
      d.remove();
   });
   btnContainer.appendChild(dlgCancelBtn);

   document.body.appendChild(d);
   dlgShow(d);

   if (x !== undefined && y !== undefined)
      dlgMove(d, x, y);

   // adjust select size when resizing modal
   const resizeObs = new ResizeObserver(() => {
      fileSelect.style.height = (d.offsetHeight - dlgTitle.offsetHeight - dlgFolder.offsetHeight - fContainer.offsetHeight - btnContainer.offsetHeight - colTitles.offsetHeight -5) + "px";
   });
   resizeObs.observe(d);

   return d;
}

// Populates a modal with a file load/save options in select
function table_file_dialog(iddiv = "dlgFile", width = 600, height = 350, x, y) {
   /* Load/save file dialog, optional parameters
      iddiv        - give the dialog a name
      width/height - minimal width/height of dialog
      x/y          - initial position of dialog
   */

   // First make sure you removed exisitng iddiv
   if (document.getElementById(iddiv)) document.getElementById(iddiv).remove();
   const d = document.createElement("div");
   d.className = "dlgFrame";
   d.id = iddiv;
   d.style.zIndex = "30";
   // allow resizing modal
   d.style.overflow = "hidden";
   d.style.resize = "both";
   d.style.minWidth = width + "px";
   d.style.minHeight = height + "px";
   d.style.width = width + "px";
   d.style.height = height + "px";
   d.shouldDestroy = true;

   const dlgTitle = document.createElement("div");
   dlgTitle.className = "dlgTitlebar";
   dlgTitle.id = "dlgMessageTitle";
   dlgTitle.innerText = "Select file dialog";
   d.appendChild(dlgTitle);

   const dlgPanel = document.createElement("div");
   dlgPanel.className = "dlgPanel";
   dlgPanel.id = "dlgPanel";
   dlgPanel.onclick = unselect_files;
   d.appendChild(dlgPanel);

   const dlgFolder = document.createElement("div");
   dlgFolder.innerHTML = `
      <div id="folderTree"><img style="height:16px;vertical-align: middle;margin-bottom: 2px;" src="icons/slash-square.svg" title="Root folder"> </div>
      <div style="background-color: #DDDDDD;display: none;" id="pickerTools"><img style="padding: 5px;height:22px;vertical-align: middle;margin-bottom: 2px;" src="icons/folder-plus.svg" title="Create subdirectory" id="createSubdir"> </div>
   `;
   dlgFolder.style.textAlign = "left";
   dlgPanel.appendChild(dlgFolder);

   const filesTable = document.createElement("div");
   filesTable.style.overflow = "auto";
   const fileSelect = document.createElement("table");
   fileSelect.className = "mtable dialogTable filesTable";
   fileSelect.id = "fileSelect";
   fileSelect.style.border = "none";
   fileSelect.style.width = "100%";
   //fileSelect.setAttribute("tabindex","0");
   //fileSelect.focus();
   
   const colTitles = fileSelect.createTHead();
   colTitles.innerHTML = `
     <tr>
       <th id='nameSort' style='width: 45%;' onclick='check_sorting(this);'>Name
         <img id='nameArrow' style='float:right;visibility:hidden;' src='icons/chevron-up.svg'>
       </th>
       <th id='timeSort' style='width: 40%;' onclick='check_sorting(this);'>Modified
         <img id='timeArrow' style='float:right;' src='icons/chevron-down.svg'>
       </th>
       <th id='sizeSort' style='width: auto;' onclick='check_sorting(this);'>Size
         <img id='sizeArrow' style='float:right;visibility:hidden;' src='icons/chevron-down.svg'>
       </th>
     </tr>`;
   fileSelect.appendChild(document.createElement("tbody"));
   filesTable.appendChild(fileSelect);
   dlgPanel.appendChild(filesTable);

   const fContainer = document.createElement("div");
   fContainer.id = "fContainer";
   fContainer.style.width = "100%";
   fContainer.style.paddingTop = "5px";
   fContainer.style.paddingBottom = "3px";
   dlgPanel.appendChild(fContainer);
   const btnContainer = document.createElement("div");
   btnContainer.id = "btnContainer";
   btnContainer.style.paddingBottom = "3px";
   dlgPanel.appendChild(btnContainer);

   const dlgButton = document.createElement("button");
   dlgButton.className = "dlgButtonDefault";
   dlgButton.id = "dlgButton";
   dlgButton.type = "button";
   btnContainer.appendChild(dlgButton);
   const dlgCancelBtn = document.createElement("button");
   dlgCancelBtn.className = "dlgButton";
   dlgCancelBtn.id = "dlgCancelBtn";
   dlgCancelBtn.type = "button";
   dlgCancelBtn.textContent = "Cancel";
   dlgCancelBtn.addEventListener("click", function () {
      document.removeEventListener("keydown", tableClickHandler);
      d.remove();
   });
   btnContainer.appendChild(dlgCancelBtn);

   document.body.appendChild(d);
   dlgShow(d);

   if (x !== undefined && y !== undefined)
      dlgMove(d, x, y);

   // adjust select size when resizing modal
   const resizeObs = new ResizeObserver(() => {
      filesTable.style.height = (d.offsetHeight - dlgTitle.offsetHeight - dlgFolder.offsetHeight - fContainer.offsetHeight - btnContainer.offsetHeight - 5 ) + "px";
   });
   resizeObs.observe(d);

   // Add key navigation
   document.addEventListener("keydown", tableClickHandler);

   return d;
}

// Handle create new folder
function create_subdir(subDir,pathName = "") {
   if (subDir == false) return;
   if (!pathName.endsWith("/")) pathName += "/";
   const fullPath = pathName + subDir;
   // clean up the fullPath and split into folders
   const folders = fullPath.replaceAll("..","").replaceAll("//","/").split("/");
   // create folders one by one
   let tmpPath = "";
   for (let i = 0; i < folders.length; i++) {
      if (folders[i] != "") {
         tmpPath += "/" + folders[i]; 
         const req = mjsonrpc_make_request("make_subdir", {"subdir": tmpPath});
         mjsonrpc_send_request(req).then(function (rpc) {
            if (document.getElementById("lastFolder")) 
               document.getElementById("lastFolder").click();
         }).catch(function (error) {
            console.error(error);
         });
      }
   }
}

// Handle mouse click to unselect files
function unselect_files() {
   if (event.target.tagName === "DIV") {
      const selFiles = document.querySelectorAll('.selFile');
      selFiles.forEach(selFile => {
         selFile.classList.remove('selFile');
      });
   }
}

// Handle key events in the files table created file_picker
let sequenceOfLetters = '';
let sequenceTimer = null;
function tableClickHandler() {
   // Do not take key events from editable elements
   const target = event.target;
   const tagName = target.tagName;
   if (event.target.isContentEditable || tagName === "INPUT") 
      return;

   const fileSelect = document.getElementById("fileSelect");
   if (fileSelect) {
      let selectedRow = document.querySelector('.selFile');
      let selectedRowIndex = (selectedRow) ? selectedRow.rowIndex : 0;
      const rows = fileSelect.rows;
      const nRows = rows.length;
      const key = event.key.toLowerCase();
      // Ignore special keys (except arrow up/down and enter) and non-characters
      if (key.length !== 1 && key !== "arrowup" && key !== "arrowdown" && key !== "enter") {
         return;
      }

      if (key === "arrowup") {
         // scroll up and loop
         selectedRowIndex = (selectedRowIndex >= 2) ? selectedRowIndex - 1 : nRows - 1;
         event.preventDefault();
      } else if (key === "arrowdown") {
         // scroll down and loop
         selectedRowIndex = (selectedRowIndex < nRows - 1) ? selectedRowIndex + 1 : 1;
         event.preventDefault();
      } else if (key === "enter") {
         const selFiles = document.querySelectorAll('.selFile');
         var doubleClickEvent = new Event('dblclick');
         selFiles[0].dispatchEvent(doubleClickEvent);
      } else {
         clearTimeout(sequenceTimer);
         sequenceOfLetters = !sequenceOfLetters ? key : sequenceOfLetters + key;
         sequenceTimer = setTimeout(() => {
            sequenceOfLetters = '';
            clearHighlights();
         },1000);
         
         for (let i = selectedRowIndex + 1; i < selectedRowIndex + nRows; i++) {
            const rowIndex = i % nRows; // Calculate the current row index
            const firstCellText = rows[rowIndex].childNodes[0].textContent;
            let currentIndex = 0;
            let matched = true; // Assume a match by default
            
            for (let j = 0; j < sequenceOfLetters.length; j++) {
               const letter = sequenceOfLetters[j];
               if (currentIndex >= firstCellText.length || firstCellText[currentIndex].toLowerCase() !== letter) {
                  matched = false; // No match found
                  break;
               }
               currentIndex++;
            }
            
            if (matched) {
               selectedRowIndex = rowIndex;
               break;
            }
         }
         clearHighlights();
         // Apply new highlights based on sequenceOfLetters
         if (sequenceOfLetters) {
            applyHighlights(rows[selectedRowIndex].childNodes[0], sequenceOfLetters);
         }
      }
         
      // de-select all files
      const selFiles = document.querySelectorAll('.selFile');
      selFiles.forEach(selFile => {
         selFile.classList.remove('selFile');
      });
      rows[selectedRowIndex].scrollIntoView({behavior: 'smooth',block: 'center'});
      rows[selectedRowIndex].classList.add('selFile');
   }
}

// highlight characters in file selection table
function applyHighlights(node, letters) {
   const text = node.textContent;
   let highlighted = '';
   let currentIndex = 0;
   
   for (let i = 0; i < text.length; i++) {
      const originalChar = text[i];
      const lowerCaseChar = originalChar.toLowerCase();
      
      if (lowerCaseChar === letters[currentIndex]) {
         highlighted += `<span class="hlkeys">${originalChar}</span>`;
         currentIndex++;
      } else {
         highlighted += originalChar;
      }
      
      if (currentIndex === letters.length) {
         highlighted += text.substring(i + 1);
         break;
      }
   }
   
   node.innerHTML = highlighted;
}

// clear highlighetd characters in file selction table
function clearHighlights() {
   const highlightedElements = document.querySelectorAll('.hlkeys');
   highlightedElements.forEach(element => {
      const parent = element.parentNode;
      parent.replaceChild(document.createTextNode(element.textContent), element);
   });
}

// get list of files and folders (in [])
function get_list_files(files, subdirs) {
   /* Sort array of file objects
      files - array of file objects
      subdirs - (optional) subdirst to be included in array
   */

   const fList = [];

   // Consider subdirs
   if (subdirs) {
      fList.push(...subdirs.map((item) => `[${item}]`));
   }

   if (files) {
      fList.push(...files);
   }

   return fList;
}

// load an ascii file from server using http request
function file_load_ascii_http(file, callback){
   // Both file and callback must be provided
   if (!file || !callback) return;
   const xhr = new XMLHttpRequest();
   xhr.onload = () => {
      if (xhr.status === 200) callback(xhr.responseText);
   };
   xhr.open('GET', `${file}?${Date.now()}`, true);
   xhr.send();
}

// load an ascii file from server using mjsonrpc call
function file_load_ascii(filename,callback){
   /*
      filename - the name of the file to save
      callback - the call back function recieving the contect
   */
   // Both file and callback must be provided
   if (!filename || !callback) return;
   // Possible checks here for illegal names
   if (filename == "") {
      dlgAlert("Illegal empty file name! Please provide a file name.");
      return;
   }

   const ext = "*" + filename.substr( filename.lastIndexOf("."));
   const pathName = filename.substr(0,filename.lastIndexOf("/") + 1);
   // File name without path
   let fname = filename.substr(filename.lastIndexOf("/") + 1)

   // Check if file exists before even trying
   mjsonrpc_send_request(mjsonrpc_make_request("ext_list_files", {
      "subdir": pathName,
      "fileext": ext
   })).then(function (rpc) {
      // Get list of files, ignore subdirectories
      const fList = get_list_files(rpc.result.files);
      if (fList.map(({ filename }) => filename).indexOf(fname) > -1) {
         // Prepare read request
         const reqread = mjsonrpc_make_request("ext_read_file", { "filename": filename });
         mjsonrpc_send_request(reqread).then(function (rpc2) {
            if (rpc2.result.status != 1) {
               throw new Error("Cannot read file, error: " + rpc2.result.error);
            } else {
               callback(rpc2.result.content);
               return rpc2.result.content;
            }
         }).catch(function (error) {
            console.error(error);
         });
      }
   }).catch(function (error) {
      console.error(error);
   });
}

// Save file to server
function file_save_ascii(filename, text, alert) {
   /* 
      filename - the name of the file to save
      text     - the content of the file
      alert    - string to be shown in dlgAlert when file is saved
   */

   // Possible checks here for illegal names 
   if (filename === "") {
      dlgAlert("Illegal empty file name! Please provide a file name.");
      return;
   }
   const ext = "*" + filename.substring(filename.lastIndexOf("."));
   const pathName = filename.substring(0,filename.lastIndexOf("/") + 1);
   // File name without path
   let fname = filename.substring(filename.lastIndexOf("/") + 1)

   // ToDo: check if the mimetype is in ODB before accepting an extension

   // Check if file already exists
   mjsonrpc_send_request(mjsonrpc_make_request("ext_list_files", {
      "subdir": pathName,
      "fileext": ext
   })).then(function (rpc) {
      if (rpc.result.status !== 1) {
         // throw new Error(rpc.result.error);
         // Or just create directories and call again
         create_subdir(pathName,"");
      }
      // Get list of files, ignore subdirectories
      const fList = get_list_files(rpc.result.files);
      if (fList.map(({filename}) => filename).indexOf(fname) > -1) {
         // File exists, overwrite?
         dlgConfirm("File " + filename + " exists. Overwrite it?", (flag) => {
            if (flag === true)
               file_save_ascii_overwrite(filename, text, alert);
         });
         return;
      }
      file_save_ascii_overwrite(filename, text, alert);
   }).catch(function (error) {
      console.error(error);
   });
}

function file_save_ascii_overwrite(filename, text, alert) {
   // Prepare save request
   const reqsave = mjsonrpc_make_request("ext_save_file", {"filename": filename, "script": text});
   mjsonrpc_send_request(reqsave).then(function (rpc) {
      if (rpc.result.status !== 1) {
         throw new Error("Cannot save file, error: " + rpc.result.error);
      } else {

         // show dialog box if alert is a string, otherwise call if it's a funciton
         if (typeof alert === "string" && alert !== "")
            dlgAlert(alert);
         else if (typeof alert === "function")
            alert(filename);
         return filename;
      }
   }).catch(function (error) {
      console.error(error);
   });
}

function sort_files_select(sort) {
   const select = document.getElementById("fileSelect");
   let fsType = select.nodeName;
   let options = "";
   if (fsType == "SELECT") {
      options = Array.from(select.options);
   } else {
      options = Array.from(select.rows);
      // Keep first row (labels).
      select.innerHTML = "";
      select.appendChild(options[0]);
      options.shift();
   }
   
   options.sort((a, b) => {
      const aFields = a.innerText.split(/\n|\t/).map(field => field.trim());
      const bFields = b.innerText.split(/\n|\t/).map(field => field.trim());
      if (aFields.length === 1 || bFields.length === 1) {
         return 0; // Skip sorting if either option has only one field
      }

      const aFilename = aFields[0];
      const bFilename = bFields[0];
      const aModtime = new Date(aFields[1]).getTime();
      const bModtime = new Date(bFields[1]).getTime();
      const aSize = aFields[2];
      const bSize = bFields[2];
      if (sort == "ni") {
         // Sort increasing alphabetically on filenames
         return aFilename.localeCompare(bFilename);
      } else if (sort == "nd") {
         // Sort increasing alphabetically on filenames
         return bFilename.localeCompare(aFilename);
      } else if (sort == "si") {
         // Sort increasing file size
         return (aSize - bSize);
      } else if (sort == "sd") {
         // Sort increasing file size
         return (bSize - aSize);
      } else if (sort == "ti") {
         // Sort increasing modification time
         return (aModtime - bModtime);
      } else {
         // Sort increasing modification time (default)
         return (bModtime - aModtime);
      }
   });

   options.forEach((option) => {
      if (fsType == "SELECT") {
         select.add(option);
      } else {
         select.appendChild(option);
      }
   });
}

function check_sorting(e) {
   if (e) {
      // Get the bounding rectangle of the <th> element and exclude clicks near the resize
      // boundaries.
      const thRect = e.getBoundingClientRect();
      const xFromLEdge = Math.abs(event.clientX - thRect.left); 
      const xFromREdge = Math.abs(event.clientX - thRect.right); 
      // exclude 10px from edges (5px if we want a tight fit).
      const excludedRange = 10;
      if (xFromLEdge < excludedRange || xFromREdge < excludedRange) {
         // Igone click
         return;
      }
   }

   const nameArrow = document.getElementById("nameArrow");
   const timeArrow = document.getElementById("timeArrow");
   const sizeArrow = document.getElementById("sizeArrow");

   let sort = "";
   if (!e) {
      e = timeArrow.parentElement;
   }

   if (e.id === "nameSort") {
      sort = "n";
      nameArrow.style.visibility = "visible";
      timeArrow.style.visibility = "hidden";
      sizeArrow.style.visibility = "hidden";
   } else if (e.id === "sizeSort") {
      sort = "s";
      nameArrow.style.visibility = "hidden";
      timeArrow.style.visibility = "hidden";
      sizeArrow.style.visibility = "visible";
   } else {
      sort = "t";
      nameArrow.style.visibility = "hidden";
      timeArrow.style.visibility = "visible";
      sizeArrow.style.visibility = "hidden";
   }

   sort += (e.childNodes[1].src === "" || e.childNodes[1].src.includes("chevron-down")) ? "d" : "i";
   e.childNodes[1].src = `icons/chevron-${sort.endsWith("d") ? "up" : "down"}.svg`;
   e.childNodes[1].alt = `${sort.startsWith("t") ? "Modified time" : "Name"} ${sort.endsWith("d") ? "decreasing" : "increasing"}`;
   // run the sorting
   sort_files_select(sort);
}

function resize_th() {
   var pressed = false;
   var start = undefined;
   var startX, startWidth;
   
   var tables = document.querySelectorAll(".filesTable");
   tables.forEach(function(table) {
      var thElements = table.querySelectorAll("th");
      thElements.forEach(function(th) {
         th.addEventListener("mousedown", function(e) {
            if (!pressed) {
               var rect = this.getBoundingClientRect();
               var edgeWidth = 5; // Adjust this value to change the clickable edge width

               if (e.pageX >= rect.right - edgeWidth) {
                  start = this;
                  pressed = true;
                  startX = e.pageX;
                  startWidth = this.offsetWidth;
                  start.classList.add("resizing");
                  start.classList.add("noSelect");
               }
            }
         });

         th.addEventListener("click", function(e) {
            if (pressed) {
               e.stopPropagation();
               e.preventDefault();
            }
         });
      });

      table.addEventListener("mousemove", function(e) {
         var thElements = table.querySelectorAll("th");
         thElements.forEach(function(th) {
            var rect = th.getBoundingClientRect();
            var edgeWidth = 5; // Adjust this value to match the clickable edge width

            if (e.pageX >= rect.right - edgeWidth) {
               th.style.cursor = "ew-resize";
            } else {
               th.style.cursor = "default";
            }
         });
      });
   });

   document.addEventListener("mousemove", function(e) {
      if (pressed) {
         start.style.width = startWidth + (e.pageX - startX) + "px";
      }
   });

   document.addEventListener("mouseup", function() {
      if (pressed) {
         start.classList.remove("resizing");
         start.classList.remove("noSelect");
         pressed = false;
      }
   });

}


// load a binary file from server using mjsonrpc call
function file_load_bin(filename,callback){
   /*
      filename - the name of the file to save
      callback - the call back function receiving the content
   */
   // Both file and callback must be provided
   if (!filename || !callback) return;
   // Possible checks here for illegal names
   if (filename === "") {
      dlgAlert("Illegal empty file name! Please provide a file name.");
      return;
   }

   const ext = "*" + filename.substring( filename.lastIndexOf("."));
   const pathName = filename.substring(0,filename.lastIndexOf("/") + 1);
   // File name without path
   let fname = filename.substring(filename.lastIndexOf("/") + 1)

   // Check if file exists before even trying
   mjsonrpc_send_request(mjsonrpc_make_request("ext_list_files", {
      "subdir": pathName,
      "fileext": ext
   })).then(function (rpc) {
      // Get list of files, ignore subdirectories
      const fList = get_list_files(rpc.result.files);
      if (fList.map(({ filename }) => filename).indexOf(fname) > -1) {
         // Prepare read request
         mjsonrpc_call("read_binary_file", { "filename": filename },"arraybuffer").then(function (result) {
            let array = new Uint8Array(result)
            if (array) {
               callback(array);
               return array;
            } else {
               throw new Error("Cannot read file or file is empty.");
            }
         }).catch(function (error) {
            console.error(error);
         });
      }
   }).catch(function (error) {
      console.error(error);
   });
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * js-indent-level: 3
 * indent-tabs-mode: nil
 * End:
 */
