/*

  Sequencer syntax highlighting and operation functions 

  Created by Zaher Salman on September 16th, 2023

  Usage
  =====

  Some of these functions, although specific to the midas sequencer,
  can be used for general syntax highlighting and editing.

  All files created and read from subdirectories within
  experiment_directory/userfiles/sequencer. The path defined in ODB
  /Sequencer/State/Path is added to this folder.

  Generate highlighted script text: 
  syntax_msl(seqLines,keywordGroups)
  - seqLines      - is the text of the script (or array of lines).
  - keywordGroups - (optional) object defining the command groups for
  highlighting. If not provided, a msl default will 
  be used.

  Highlight the current line: 
  hlLine(lineNum,color)
  - lineNum - the current line number.
  - color   - (optional) the bg color to use

  todo:
  - start -> load, switch to main tab and confirm
  - progress to embed in infocolumn
*/

// Using Solarized color scheme - https://ethanschoonover.com/solarized/

// Default msl definitions, keywords, indentation and file name
const mslDefs = {
   groups : {
   strings: {
      keywords: [/(["'])(.*?)\1/g],
      color: "#008000",
      fontWeight: "",
      mslclass: "msl_string",
   },
   variables: {
      keywords: [/(\$[\w]+|^\s*[\w]+(?=\s*=))/gm],
      color: "#8B0000",
      fontWeight: "",
      mslclass: "msl_variable",
   },
   controlFlow: {
      keywords: ["GOTO", "CALL", "SCRIPT", "SUBROUTINE", "ENDSUBROUTINE", "TRANSITION", "INCLUDE", "EXIT"],
      color: "#268bd2",  // blue
      fontWeight: "bold",
      mslclass: "msl_control_flow",
   },
   dataManagement: {
      keywords: ["ODBSET", "ODBGET", "ODBCREATE", "ODBDELETE", "ODBINC", "ODBLOAD", "ODBSAVE", "ODBSUBDIR", "PARAM", "SET", "CAT"],
      color: "#2aa198",  // cyan
      fontWeight: "bold",
      mslclass: "msl_data_management",
   },
   info: {
      keywords: ["RUNDESCRIPTION", "LIBRARY", "MESSAGE", "MSG"],
      color: "#6c71c4",  // violet
      fontWeight: "bold",
      mslclass: "msl_info",
   },
   cond: {
      keywords: ["IF", "ELSE", "ENDIF", "WAIT"],
      color: "#c577f6", // pink (not solarized)
      fontWeight: "bold",
      mslclass: "msl_cond",
   },
   loops: {
      keywords: ["BREAK", "LOOP", "ENDLOOP"],
      color: "#d33682",  // magenta
      fontWeight: "bold",
      mslclass: "msl_loops",
   },
   dataTypes: {
      keywords: ["UNIT8", "INT8", "UNIT16", "INT16", "UNIT32", "INT32", "BOOL", "FLOAT", "DOUBLE", "STRING"],
      color: "#859900",  // green
      fontWeight: "bold",
      mslclass: "msl_data_types",
   },
   units: {
      keywords: ["SECONDS", "EVENTS", "ODBVALUE"],
      color: "#cb4b16",  // orange
      fontWeight: "bold",
      mslclass: "msl_units",
   },
   actions: {
      keywords: ["start", "stop", "pause", "resume"],
      color: "#dc322f", // red
      fontWeight: "bold",
      mslclass: "msl_actions",
   },
   bool: {
      keywords: ["true","false"],
      color: "#61b7b7",
      fontWeight: "",
      mslclass: "msl_bool",
   },
   numbers: {
      keywords: [/\b(?<![0-9a-fA-F#])\d+(\.\d+)?([eE][-+]?\d+)?\b/g],
      color: "#b58900", // yellow
      fontWeight: "",
      mslclass: "msl_number",
   },
   comments: {
      keywords: ["#","COMMENT"],
      color: "#839496",  // base0
      fontWeight: "italic",
      mslclass: "msl_comment",
   },
   },
   defIndent: {
      indentplus: ["IF", "LOOP", "ELSE", "SUBROUTINE"],
      indentminos: ["ENDIF", "ENDLOOP", "ELSE", "ENDSUBROUTINE"],
   },
   filename: {
      ext : "*.msl",
      path : "sequencer",
   },
};

// Iindentation keywords
const defIndent = mslDefs.defIndent;
// Default values for MSL
const fnExt = mslDefs.filename.ext;
const fnPath = mslDefs.filename.path;

var seq_css = `
/* For buttons */
.seqbtn {
   display: none;
}

#iconsRow img {
   cursor: pointer;
   padding: 0.4em;
   margin: 0;
}
#iconsRow img:hover {
   background: #C0D0D0;
}

.edt_linenum {
   background-color: #f0f0f0;
   user-select: none;
   width: 3em;
   overflow: hidden;
   color: gray;
   text-align: right;
   padding: 5px;
   box-sizing: border-box;
   vertical-align: top;
   display: inline-block;
   margin: 0 0 0 0;
   font-family: monospace;
   white-space: pre;
   -moz-user-select: text;
   pointer-events: none;
}
.edt_linenum span {
   display: block;
}
.edt_linenum_curr {
   color: #000000; 
   font-weight: bold;
}
.edt_area {
   width: calc(100% - 3em);
   max-width: calc(100% - 3em);
   overflow:auto; 
   resize: horizontal;
   background-color:white;
   color: black;
   padding: 5px;
   box-sizing: border-box;
   vertical-align: top;
   display: inline-block;
   margin: -1px 0 0 0;
   border-top: 1px solid gray;
   font-family: monospace;
   white-space: pre;
   -moz-user-select: text;
}

.sline {
   display: inline-block;
}
.esline {
   display: inline-block;
}

.msl_current_line {
   background-color: #FFFF00; 
   font-weight: bold;
}
.msl_fatal {
   background-color: #FF0000;
   font-weight: bold;
}
.msl_err {
   background-color: #FF8800;
   font-weight: bold;
}
.msl_msg {
   background-color: #11FF11;
   font-weight: bold;
}
.msl_current_cmd {
   background-color: #FFFF00;
   font-weight: bold;
}
.info_column{
   display: flex;
   flex-grow: 1;
   align-items: start;
   padding-left: 5px;
   flex-direction: column;
}
.infotable {
   width: 100%;
   border-spacing: 0px;
   border: 1px solid black;
   border-radius: 5px;
   text-align: left;
   padding: 0px;
   float: right;
   line-height: 30px; /* since modbvalue forces resize */
}
.infotable th{
   white-space: nowrap;
}
.infotable tr:first-child th:first-child,
.infotable tr:first-child td:first-child {
    border-top-left-radius: 5px;
}
.infotable tr:first-child th:last-child,
.infotable tr:first-child td:last-child {
    border-top-right-radius: 5px;
}
.infotable tr:last-child td:first-child {
    border-bottom-left-radius: 5px;
}
.infotable tr:last-child td:last-child {
    border-bottom-right-radius: 5px;
}
.waitlooptxt {
   z-index: 33;
   position: absolute;
   top: 1px;
   left: 5px;
   display: inline-block;
}
.waitloopbar {
   z-index: 32;
   width: calc(100% - 2px);
   height: 100%;
   text-align: left;
}

/* Dropdown button styles*/
.dropmenu {
   background-color: Transparent;
   font-size: 100%;
   font-family: verdana,tahoma,sans-serif;
   border: none;
   cursor: pointer;
   text-align: left;
   padding: 5px;
   line-height: 1;
   color:#404040;
}
.dropmenu:hover {
   background-color: #C0D0D0;
}

/* Style the dropdown content (hidden by default) */
.dropdown {
   position: relative;
   display: inline-block;
}
.dropdown-content {
   display: none;
   position: absolute;
   background-color: #f9f9f9;
   min-width: 150px;
   box-shadow: 0 8px 16px rgba(0,0,0,0.2);
   z-index: 10;
   font-size: 90%;
   color: black;
   white-space: nowrap;
   cursor: pointer;
}
.dropdown-content a{
   display: block;
   padding: 4px 8px;
}
.dropdown-content div{
   display: flex;
   justify-content: space-between;
   align-items: center;
   padding: 4px 8px;
}
.dropdown-content a:hover {
   background-color: #C0D0D0;
}
.dropdown-content div:hover {
   background-color: #C0D0D0;
}
.dropdown:hover .dropdown-content {
   display: block;
}

 /* Style the tab */
.etab {
   position: relative;
   top: 2px;
   z-index: 1;
   overflow-x: auto;
   white-space: nowrap;
   width: 100%;
   display: block;
   -webkit-overflow-scrolling: touch;
   border: none;
   /* background-color: #475057;*/
}
.etab button {
   background-color: #D0D0D0;
   float: left;
   margin: 4px 2px 0px 2px;
   border-top-left-radius: 10px;
   border-top-right-radius: 10px;
   border: none;
   border-top: 1px solid Transparent;
   border-right: 1px solid Transparent;
   border-left: 1px solid Transparent;
   cursor: pointer;
   padding: 3px 5px 3px 5px;
}
.etab button:hover {
   background-color: #FFFFFF;
   border-bottom: 5px solid #FFFFFF;
   /*opacity: 0.7;*/
}
.etab button.edt_active:hover {
   /*opacity: 0.7;*/
   background-color: #FFFFFF;
   border-bottom: 5px solid #FFFFFF;
}
.etab button.edt_active {
   background-color: white;/*Transparent;*/
   border-top: 1px solid gray;
   border-right: 1px solid gray;
   border-left: 1px solid gray;
   border-bottom: 5px solid white;
   color: black;
}
.etabcontent {
   display: none;
   /*padding: 6px 12px;*/
   padding: 0;
   border: 1px solid #ccc;
   border-top: none;
   height: 100%;
   /*width: 100%;
   display: flex;*/
}
.editbtn{
   background-color: #f0f0f0;
/*
   font-family: verdana,tahoma,sans-serif;
   font-size: 1.0em;
*/
   border: 1px solid black;
   border-radius: 3px;
   cursor: pointer;
   padding: 0px 2px 0px 2px;
   margin-left: 3px;
}
.editbtn:hover{
   background-color: #C0D0D0;
}
.closebtn {
   font-family: verdana,tahoma,sans-serif;
   border: none;
   border-radius: 3px;
   cursor: pointer;
   padding: 2px;
   margin-left: 5px;
} 
.closebtn:hover {
   background-color: #FD5E59;
}

.progress {
   width: 200px;
   font-size: 0.7em;
}
.dlgProgress {
   border: 1px solid black;
   box-shadow: 6px 6px 10px 4px rgba(0,0,0,0.2);
   border-radius: 6px;
   display: none;
}

.empty-dragrow td{
   border: 2px dashed #6bb28c;
   background-color: white;
}
tr.dragstart td{
   background-color: gray;
   color: white;
}
tr.dragrow td{
   overflow: hidden;
   text-overflow: ellipsis;
   max-width: calc(10em - 30px);
}
#nextFNContainer img {
   cursor: pointer;
   height: 1em;
   vertical-align: center;
}
`;

// Implement colors and styles from KeywordsGroups in CSS
for (const group in mslDefs.groups) {
   const { mslclass, color, fontWeight } = mslDefs.groups[group];
   
   if (mslclass) {
      seq_css += `.${mslclass} { color: ${color}; font-weight: ${fontWeight}; }\n`;
   }
}

const seqStyle = document.createElement('style');
seqStyle.textContent = seq_css;
document.head.appendChild(seqStyle);

// line connector string
const lc = "\n";
// revisions array, maximum nRevisions
const previousRevisions = {};
const nRevisions = 20;
var revisionIndex = {};
var saveRevision = {};
// Meta combo keydown flag
var MetaCombo = false;

// -- Sequencer specific functions --
// Setup the correct sequencer state visually
function seqState(funcCall) {
   /* Arguments:
      funcCall - (optional) a function to be called when the state is set (with the state text)
   */
   let stateText  = "Stopped";
   // Check sequence state
   mjsonrpc_db_get_values(["/Sequencer/State/Running","/Sequencer/State/Paused","/Sequencer/State/Finished","/Sequencer/State/Debug"]).then(function(rpc) {
      if (rpc.result.data[0] && !rpc.result.data[1] && !rpc.result.data[2]) {
	 stateText  = "Running";
      } else if (rpc.result.data[1]  && rpc.result.data[0] && !rpc.result.data[2]) {
	 stateText  = "Paused";
      } else {
         stateText  = "Stopped";
      }
      if (funcCall) {
         funcCall(stateText);
      }
      return stateText;
   }).catch (function (error) {
      console.error(error);
      return "Unknown";
   });
}

// Ask user to edit current sequence
function askToEdit(flag,event) {
   if (flag) {
      openETab(document.getElementById("etab1-btn"));
      const [lineNumbers,editor,btnLabel,label] = editorElements();
      // make editable and add event listeners
      //editor.contentEditable = true;
      addETab(document.getElementById("addETab"));
      seqOpen(label.title.split("\n")[0]);
      event.stopPropagation();
      return;
   }
   const message = "To edit the sequence it must be opened in an editor tab.<br>Would you like to proceed?";
   dlgConfirm(message,function(resp) {
      if (resp) {
         const label = editorElements()[3];
         addETab(document.getElementById("addETab"));
         seqOpen(label.title.split("\n")[0]);
      }
   });
}

// Enable editing of sequence
function editorEventListeners() {
   let [lineNumbers,editor] = editorElements();
   editor.contentEditable = true;
   // Attached syntax highlight event editor
   editor.addEventListener("keydown",checkSyntaxEventDown);
   editor.addEventListener("keyup",checkSyntaxEventUp);
   editor.addEventListener("paste", checkSyntaxEventPaste);
   // Change in text
   editor.addEventListener("input", function() {
      seqIsChanged(true);
   });
   document.addEventListener("selectionchange", function(event) {
      if (event.target.activeElement === editor) markCurrLineNum();
   });
   // Synchronize the scroll position of lineNumbers with editor
   editor.addEventListener("scroll", function() {
      lineNumbers.scrollTop = editor.scrollTop;
   });
}

// apply changes of filename in the ODB (triggers reload)
function seqChange(filename) {
   /* Arguments:
      filename - full file name with path to change
   */
   if (!filename) return;
   const lastIndex = filename.lastIndexOf('/');
   const path = filename.substring(0, lastIndex).replace(new RegExp('^' + fnPath),"").replace(/\/+/g, '/').replace(/^\/+|\/+$/g, '');
   const name = filename.substring(lastIndex + 1);
   // set path, and filename, wait 3s, then load and wait 1s
   mjsonrpc_db_paste(["/Sequencer/State/Path","/Sequencer/Command/Load filename"],[path,name]).then(function (rpc1) {
      sessionStorage.removeItem("depthDir");
      if (rpc1.result.status[0] === 1 && rpc1.result.status[1] === 1) {
         mjsonrpc_db_paste(["/Sequencer/Command/Load new file"],[true]).then(function (rpc2) {
            if (rpc2.result.status[0] === 1) {
               return;
            }
         }).catch(function (error) {console.error(error);});
      } else {
         dlgAlert("Something went wrong, I could not set the filename!");
      }
   }).catch(function (error) {console.error(error);});
}

// Save sequence text in filename.
function seqSave(filename) {
   /* Arguments:
      filename (opt) - save to provided filename with path. If undefined save to original
                       filename and if empty trigger file_picker.
   */
   let [lineNumbers,editor,label] = editorElements();
   let text = editor.innerText;
   if (editor.id !== "editorTab1") {
      if (filename === undefined) {
         // take name from button title
         filename = label.title;
         if (filename.endsWith(".msl")) {
            file_save_ascii_overwrite(filename,text);
            seqIsChanged(false);
         } else {
            seqSave("");
         }
      } else if (filename === "") {
         filename = label.title;
         let file = filename.split("/").pop();
         let path = filename.substring(0, filename.lastIndexOf("/") + 1).replace(/\/+/g, '/').replace(/^\/+|\/+$/g, '');
         // If file/path are empty start with default value
         if (path === "")
            path = sessionStorage.getItem("pathName") ? sessionStorage.getItem("pathName") : fnPath + "/";
         if (file === "")
            file = sessionStorage.getItem("fileName") ? sessionStorage.getItem("fileName") : "filename.msl";
         file_picker(path,fnExt,seqSave,true,{},true);
      } else {
         file_save_ascii_overwrite(filename,text);
         let file = filename.split("/").pop();
         let path = filename.substring(0, filename.lastIndexOf("/") + 1).replace(/\/+/g, '/').replace(/^\/+|\/+$/g, '');
         label.title = filename;
         label.innerText = file;
         sessionStorage.setItem("fileName",file);
         sessionStorage.setItem("pathName",path);
         seqIsChanged(false);
      }
      // Check if filename is currently in editorTab1 and reload
      let currFilename = document.getElementById("etab1-btn").title;
      if (filename == currFilename) {
         modbset("/Sequencer/Command/Load new file",true);
      }
      return;
   }
}

// Open filename
function seqOpen(filename) {
   /* Arguments:
      filename - file name to open (empty trigger file_picker)
   */
   // if a full filename is provided, open and return
   if (filename && filename !== "") {
      // Identify active tab
      let [lineNumbers,editor,label] = editorElements();
      // Check the option to open in new tab
      if (document.getElementById("inNewTab").checked && editor.id !== "editorTab1") {
         if (label.title !== "") { // if the tab is empty open new file there
            addETab(document.getElementById("addETab"));
            [lineNumbers,editor,label] = editorElements();
         }
      }
      let file = filename.split("/").pop();
      let path = filename.substring(0, filename.lastIndexOf("/") + 1).replace(/\/+/g, '/').replace(/^\/+|\/+$/g, '');
      label.title = filename.replaceAll(/\/+/g, '/');
      label.innerText = file;
      sessionStorage.setItem("fileName",file);
      sessionStorage.setItem("pathName",path);      
      if (editor.id === "editorTab1") {
         seqChange(filename);
      } else {
         file_load_ascii(filename, function(text) {
            editor.innerHTML = syntax_msl(text).join(lc).slice(0,-1);
            updateLineNumbers(lineNumbers,editor);
         });
      }
      return;
   } else {
      // empty or undefined file name
      mjsonrpc_db_get_values(["/Sequencer/State/Path"]).then(function(rpc) {
         let path = fnPath + "/" + rpc.result.data[0].replace(/\/+/g, '/').replace(/^\/+|\/+$/g, '');
         sessionStorage.setItem("pathName",path);
         file_picker(path,fnExt,seqOpen,false);
   }).catch(function (error) {
      mjsonrpc_error_alert(error);
   });
}
}

// Show/hide buttons according to sequencer state
function updateBtns(state) {
   /* Arguments:
      state - the state of the sequencer
   */
   const seqState = {
      Running: {
         color: "var(--mgreen)",
      },
      Stopped: {
         color: "var(--mred)",
      },
      Editing: {
         color: "var(--mred)",
      },
      Paused: {
         color: "var(--myellow)",
      },
   }
   const color = seqState[state].color;
   const seqStateSpans = document.querySelectorAll('.seqstate');
   seqStateSpans.forEach(e => {e.style.backgroundColor = color;});
   // hide all buttons
   const hideBtns = document.querySelectorAll('.seqbtn');
   hideBtns.forEach(button => {
      button.style.display = "none";
   });
   // then show only those belonging to the current state
   const showBtns = document.querySelectorAll('.seqbtn.' + state);
   showBtns.forEach(button => {
      if (button.tagName === "IMG") {
         button.style.display = "inline-block";
      } else {
         button.style.display = "flex";
      }
   });
   // Hide progress modal when stopped
   const hideProgress = document.getElementById("Progress");
   if (state === "Stopped") hideProgress.style.display = "none";
}

// Show sequencer messages if present
function mslMessage(message) {
   // Empty message, return
   if (!message) return;
   // Check message and message wait
   mjsonrpc_db_get_values(["/Sequencer/State/Message","/Sequencer/State/Message Wait"]).then(function(rpc) {
      const message = rpc.result.data[0];
      const hold = rpc.result.data[1];
      if (hold) {
         dlgMessage("Message", message, true, false,clrMessage);
      } else {
         dlgAlert(message);
      }
   }).catch (function (error) {
      console.error(error);
   });
}

// Clear sequencer messages
function clrMessage() {
   mjsonrpc_db_paste(["Sequencer/State/Message"], [""]).then(function (rpc) {
      return;
   }).catch(function (error) {
      console.error(error);
   });
}

// Adjust size of sequencer editor according to browser window size 
function windowResize() {
   const [lineNumbers,editor] = editorElements();
   const seqTable = document.getElementById("seqTable");
   //const parContainer = document.getElementById("parContainer");
   editor.style.height = document.documentElement.clientHeight - editor.getBoundingClientRect().top - 20 + "px";
   // Sync line number height
   lineNumbers.style.height = editor.style.height;
   seqTable.style.width = (window.innerWidth - seqTable.getBoundingClientRect().left - 10) + "px";
   seqTable.style.maxWidth = (window.innerWidth - seqTable.getBoundingClientRect().left - 10) + "px";
}

// Load the current sequence from ODB (only on main tab)
function load_msl() {
   const editor = document.getElementById("editorTab1");
   const btn = document.getElementById("etab1-btn");
   mjsonrpc_db_get_values(["/Sequencer/Script/Lines","/Sequencer/State/Running","/Sequencer/State/SCurrent line number","/Sequencer/Command/Load filename","/Sequencer/State/SFilename","/Sequencer/State/Path"]).then(function(rpc) {
      let seqLines = rpc.result.data[0];
      let seqState = rpc.result.data[1];
      let currLine = rpc.result.data[2];
      let filename = rpc.result.data[3];
      //if (rpc.result.data[4] === null) dlgAlert("You may be using an old sequencer.<br>Please recompile and/or restart it.");
      let sfilename = rpc.result.data[4] ? rpc.result.data[4].split('userfiles/sequencer/')[1] : "";
      filename = (fnPath + "/" + rpc.result.data[5] + "/" + filename).replace("//","/");
      // syntax highlight
      if (seqLines) {
         editor.innerHTML = syntax_msl(seqLines).join(lc);
         updateLineNumbers(editor.previousElementSibling,editor);
         if (seqState) hlLine(currLine);
      }
      let file = filename.split("/").pop();
      let path = filename.substring(0, filename.lastIndexOf("/") + 1).replace(/\/+/g, '/').replace(/^\/+|\/+$/g, '');
      sessionStorage.setItem("fileName",file);
      sessionStorage.setItem("pathName",path);      
      // change button title to add sfilename if present
      btn.title = (sfilename && sfilename !== filename) ? filename + "\n" + sfilename : filename;
   }).catch (function (error) {
      console.error(error);
   });
}

// Highlight (background color) and scroll to current line
function hlLine(lineNum,color) {
   /* Arguments:
      lineNum - the line number to be highlighted
      color   - (optional) background color
   */
   const lineId = "sline" + lineNum;
   const lineHTML = document.getElementById(lineId);
   
   // Remove highlight from all lines with the class "msl_current_line"
   const highlightedLines = document.querySelectorAll(".msl_current_line");
   highlightedLines.forEach((line) => line.classList.remove("msl_current_line"));

   if (lineHTML) {
      lineHTML.classList.add("msl_current_line");
      if (color) lineHTML.style.backgroundColor = color;
      // Scroll to the highlighted line if the checkbox is checked
      const scrollToCurrCheckbox = document.getElementById("scrollToCurr");
      if (scrollToCurrCheckbox && scrollToCurrCheckbox.checked) {
         lineHTML.scrollIntoView({ block: "center" });
      }
   }
}

// Scroll to make line appear in the center of editor
function scrollToCurr(scrToCur) {
   if (scrToCur.checked) {
      localStorage.setItem("scrollToCurr",true);
      const currLine = document.querySelector(".msl_current_line");
      if (currLine) {
         currLine.scrollIntoView({ block: "center" });
      }
   } else {
      localStorage.removeItem("scrollToCurr",true);
   }
}

// Open files in new tabs
function toggleCheck(e) {
   if (e.checked) {
      localStorage.setItem(e.id,true);
   } else {
      localStorage.removeItem(e.id);
   }
}

// shortcut event handling to overtake default behaviour
function shortCutEvent(event) {
   const parEditor = editorElements();
   const notFirstTab = (parEditor[1].id !== "editorTab1");
   if (notFirstTab) {
      // Check these only for editors
      if (event.altKey && event.key === 's') {
         event.preventDefault();
         //save as with file_picker
         seqSave('');
         event.preventDefault();
      } else if ((event.ctrlKey || event.metaKey) && event.key === 's') {
         event.preventDefault();
         //save to the same filename
         seqSave();
         event.preventDefault();
      } else if ((event.ctrlKey || event.metaKey) && event.key === 'z') {
         event.preventDefault();
         undoEdit(event.target);
         event.preventDefault();
      } else if ((event.ctrlKey || event.metaKey) && event.key === 'r') {
         event.preventDefault();
         redoEdit(event.target);
         event.preventDefault();
      }
   }
   if (!notFirstTab) {
      // Check these only for first tab
      if ((event.ctrlKey || event.metaKey) && event.key === 'e') {
         // open new tab and load current sequence
         event.preventDefault();
         addETab(document.getElementById("addETab"));
         seqOpen(parEditor[3].title.split("\n")[0]);
         event.preventDefault();
      }
   }
   // Check these for all tabs
   if (event.altKey && event.key === 'n') {
      // open new tab
      event.preventDefault();
      addETab(document.getElementById("addETab"));
      event.preventDefault();
   } else if (event.altKey && event.key === 'o') {
      event.preventDefault();
      seqOpen();
      event.preventDefault();
   }
   
}

// Trigger syntax highlighting on keyup events
function checkSyntaxEventUp(event) {
   if (event.ctrlKey || event.altKey || event.metaKey || MetaCombo) return;
   if (event.keyCode >= 0x30 || event.key === ' '
       || event.key === 'Backspace' || event.key === 'Delete'
       || event.key === 'Enter'
      ) {
      const e = event.target;
      let caretPos = getCurrentCursorPosition(e);
      let currText = e.innerText;
      // Unclear why sometimes two \n are added
      if (currText === sessionStorage.getItem("keydown") + "\n\n") {
         e.innerText = currText.slice(0,-1);
         currText = currText.slice(0,-1);
      }
      // save current revision for undo
      if (event.key === ' ' || event.key === 'Backspace' || event.key === 'Delete' || event.key === 'Enter') {
         saveState(currText,e);
      }

      // Indent according to previous line
      if (event.key === 'Enter') {
         event.preventDefault();
         e.innerHTML = syntax_msl(e.innerText).join(lc).slice(0,-1);
         setCurrentCursorPosition(e, caretPos);
         // get previous and current line elements (before and after enter)
         let pline = whichLine(e,-1);
         let cline = whichLine(e);
         let plineText = (pline) ? pline.innerText : "";
         let clineText = (cline) ? cline.innerText : "";
         let indentLevel = 0;
         let preSpace = 0;
         let indentString = "";
         if (plineText) {
            // indent line according to the previous line text
            // if, loop, else, subroutine - increase indentation
            const indentPlus = defIndent.indentplus.some(keyword => plineText.trim().toLowerCase().startsWith(keyword.toLowerCase()));
            // else, endif, endloop, endsubroutine - decrease indentation
            const indentMinos = defIndent.indentminos.some(keyword => plineText.trim().toLowerCase().startsWith(keyword.toLowerCase()));
            /* (indentMinos/indentPlus)
               true/false - pline indent -1, cline indent 0
               fale/true  - pline indent 0, cline indent +1
               true/true  - pline indent -1, cline indent +1
               false/false- pline indent 0, cline indent 0 
            */
            // Count number of white spaces at begenning of pline 
            preSpace = plineText.replace("\n","").search(/\S|$/);
            pPreSpace = preSpace - indentMinos * 3;
            if (pPreSpace < 0) pPreSpace = 0;
            cPreSpace = pPreSpace + indentPlus * 3;
            // Calculate and insert indentation 
            pIndentString = " ".repeat(pPreSpace);
            cIndentString = " ".repeat(cPreSpace);
            cline.innerText = cIndentString + clineText.trimStart();
            caretPos += cline.innerText.length - clineText.length;
            pline.innerText = pIndentString + plineText.trimStart();
            caretPos += pline.innerText.length - plineText.length;
         }
         event.preventDefault();
      }
      e.innerHTML = syntax_msl(e.innerText).join(lc).slice(0,-1);
      setCurrentCursorPosition(e, caretPos);
      updateLineNumbers(e.previousElementSibling,e);
      e.focus();
   }
   return;
}


// Trigger syntax highlighting on keydown events
function checkSyntaxEventDown(event) {
   sessionStorage.setItem("keydown",event.target.innerText);
   // take care of Mac odd keyup behaviour
   if (event.metaKey && (/^[a-z]$/.test(event.key) || event.shiftKey || event.altKey)) {
      MetaCombo = true;
   } else {
      MetaCombo = false;
   }

   if (event.ctrlKey || event.altKey || event.metaKey) return;
   // Only pass indentation related keys
   if (event.key !== 'Tab' && event.key !== 'Escape') return;
   event.preventDefault();
   let e = event.target;
   let caretPos = getCurrentCursorPosition(e);
   let currText = e.innerText;
   let lines = getLinesInSelection(e);
   if (event.shiftKey && event.key === 'Tab') {
      indent_msl(lines,-1);
   } else if (event.key === 'Tab') {
      indent_msl(lines,+1);
   } else if (event.key === 'Escape') {
      indent_msl(lines);
   }
   e.innerHTML = syntax_msl(e.innerText).join(lc).slice(0,-1);
   let newText = e.innerText;
   setCurrentCursorPosition(e, caretPos + newText.length - currText.length);
   if (lines[0] !== lines[1]) selectLines(lines,e);
   event.preventDefault();
   return;
}

// Trigger syntax highlighting when you paste text
function checkSyntaxEventPaste(event) {
   // set time out to allow default to go first
   setTimeout(() => {
      let e = event.target;
      // make sure you paste in the editor area
      if (e.tagName !== 'PRE') e = e.parentElement;
      let caretPos = getCurrentCursorPosition(e);
      let currText = e.innerText;
      // save current revision for undo
      saveState(currText,e);
      e.innerHTML = syntax_msl(currText).join(lc).slice(0,-1);
      setCurrentCursorPosition(e, caretPos);
      updateLineNumbers(e.previousElementSibling,e);
      e.focus();
   }, 0);
   return;
}

// Find on which line is the current carret position in e
// This assumes each line has an id="sline#" where # is the line number.
function whichLine(e,offset = 0) {
   // offset allows to pick previous line (after enter)
   let pos = getCurrentCursorPosition(e);
   if (!pos) return;
   let lineNum = e.innerText.substring(0,pos).split("\n").length + offset;
   let sline = e.querySelector("#sline" + lineNum.toString());
   return sline;
}

// Return an array with the first and last line numbers of the selected region
/* This assumes that the lines are in a <pre> element and that
   each line has an id="sline#" where # is the line number.
   When the caret in in an empty line, the anchorNode is the <pre> element. 
*/
function getLinesInSelection(e) {
   const selection = window.getSelection();
   if (selection.rangeCount === 0) return [0,0];
   // is it a single line?
   const singleLine = selection.isCollapsed;
   if (singleLine) {
      const line = whichLine(e);
      if (line) {
         const startLine = parseInt(line.id.replace("sline",""));
         return [startLine,startLine];
      } else {
         return [0,0];
      }
   }
   const anchorNode = selection.anchorNode;
   const range = selection.getRangeAt(0);
   let startNode,endNode;
   if (anchorNode.tagName === 'PRE') {
      let startOffset = range.startOffset;
      let endOffset = range.endOffset;
      startNode = range.startContainer.childNodes[startOffset];
      endNode = range.startContainer.childNodes[endOffset-1];
   } else {
      startNode = (range.startContainer && range.startContainer.parentElement.tagName !== 'PRE') ? range.startContainer :  range.startContainer.nextSibling;
      if (startNode && startNode.tagName === 'PRE') startNode = startNode.firstChild;
      endNode = (range.endContainer && range.endContainer.parentElement.tagName !== 'PRE') ? range.endContainer : range.endContainer.previousSibling;
      if (endNode && endNode.tagName === 'PRE') endNode = endNode.lastChild;
   }
   let startID = (startNode && startNode.id) ? startNode.id : "";
   let endID = (endNode && endNode.id) ? endNode.id : "";
   // get first line
   while (startNode && !startID.startsWith("sline") && startNode.tagName !== 'PRE') {
      startNode = (startNode.parentNode.tagName !== 'PRE') ? startNode.parentNode : startNode.nextSibling;
      startID = (startNode && startNode.id) ? startNode.id : "";
   }
   // get last line
   while (endNode && !endID.startsWith("sline") && endNode.tagName !== 'PRE') {
      endNode = (endNode.parentNode.tagName !== 'PRE') ? endNode.parentNode : endNode.previousSibling;
      endID = (endNode && endNode.id) ? endNode.id : "";
   }
   let startLine = (startNode && startNode.id) ? parseInt(startNode.id.replace("sline","")) : 0;
   let endLine = (endNode && endNode.id) ? parseInt(endNode.id.replace("sline","")) : 0;
   if (singleLine) {
      startLine = endLine = Math.min(startLine, endLine);
   }
   return [startLine,endLine];
}

// get current caret position in chars within element parent
function getCurrentCursorPosition(parent) {
   let sel = window.getSelection();
   if (!sel.focusNode || !parent) return;
   const range = sel.getRangeAt(0);
   const prefix = range.cloneRange();
   prefix.selectNodeContents(parent);
   prefix.setEnd(range.endContainer, range.endOffset);
   return prefix.toString().length;
}

// set current caret position at pos within element parent
function setCurrentCursorPosition(parent,pos) {
   for (const node of parent.childNodes) {
      if (node.nodeType == Node.TEXT_NODE) {
         if (node.length >= pos) {
            const range = document.createRange();
            const sel = window.getSelection();
            range.setStart(node, pos);
            range.collapse(true);
            sel.removeAllRanges();
            sel.addRange(range);
            return -1;
         } else {
            pos = pos - node.length;
         }
      } else {
         pos = setCurrentCursorPosition(node, pos);
         if (pos < 0) {
            return pos;
         }
      }
   }
   return pos;
}

// Update line numbers in lineNumbers div
function updateLineNumbers(lineNumbers,editor) {
   if (lineNumbers === undefined || editor === undefined)
      [lineNumbers,editor] = editorElements();
   // Clear existing line numbers
   lineNumbers.innerHTML = "";
   // Get the number of lines accurately
   let lineCount = editor.querySelectorAll('[id^="sline"]').length;
   let lineTextCount = editor.innerText.split("\n").length;
   lineCount = (lineTextCount - lineCount) < 2 ? lineTextCount : lineTextCount - 1;
   // Add line numbers to lineNumbers
   for (let i = 1; i <= lineCount; i++) {
      const lineNumber = document.createElement('span');
      lineNumber.id = "lNum" + i.toString();
      lineNumber.textContent = i;
      lineNumbers.appendChild(lineNumber);
   }
   lineNumbers.scrollTop = editor.scrollTop;
   windowResize();
}

// Utility function to escape special characters in a string for use in a regular expression
function escapeRegExp(s) {
   if (!s) return "";
   return s.replace(/[-/\\^$*+?.()|[\]{}]/g, '\\$&');
}

// Syntax highlight any text according to provided rules
function syntax_msl(seqLines,keywordGroups) {
   // If not provided use the default msl keywords and groups
   if (!keywordGroups) {
      // take msl default
      keywordGroups = mslDefs.groups;
   }

   let mslText = Array.isArray(seqLines) ? seqLines.join("\n") : seqLines;
   // Make some cleanup of illegal characters
   mslText = escapeSpecialCharacters(mslText);
   // Keep original sequence lines (as array)
   let seqLines_org = mslText.split(/\n/);
   // Make full text if you get an array of lines
   let nLines = (mslText.match(/\n/g) || []).length;
   // These can be done on the text in one go
   // Strings
   let reg = /(["'])(.*?)\1/g;
   mslText = mslText.replace(reg,'<span class="msl_string">$1$2$1</span>');

   // Comments
   //reg = /^(COMMENT|#.*?)(.*)$/gim;
   //mslText = mslText.replace(reg,'<span class="msl_comment">$&</span>');
   
   // Variables
   //reg = /(\$[\w]+|^\s*[\w]+(?=\s*=))/gm;
   reg = /(?:\$[\w]+|^\b\w+(?=\s*=))/gm; // starting with $ or something = 
   mslText = mslText.replace(reg,'<span class="msl_variable">$&</span>');
   reg = new RegExp("(^(?:\\s*)\\b(PARAM|CAT|SET)\\s+)(\\w+)\\b", "gim");  // after PARAM, CAT and SET
   mslText = mslText.replace(reg,'$1<span class="msl_variable">$3</span>');
   
   // Data Management group excluding variables (must be after variables)
   reg = new RegExp("^(\\s*)\\b(" + keywordGroups.dataManagement.keywords.join("|") + ")\\b", "gim");
   mslText = mslText.replace(reg, "$1<span class='msl_data_management'>$2</span>");
   
   // Data Type group (must have comma before the keyword)
   reg = new RegExp("(?<=,\\s*)\\b(" + keywordGroups.dataTypes.keywords.join("|") + ")\\b", "gim");
   mslText = mslText.replace(reg, "<span class='msl_data_types'>$1</span>");

   // Loops group (must be at the begenning of the line)
   reg = new RegExp("^(\\s*)\\b(" + keywordGroups.loops.keywords.join("|") + ")\\b", "gim");
   mslText = mslText.replace(reg, "$1<span class='msl_loops'>$2</span>");
   
   // Control Flow group
   reg = new RegExp("^(\\s*)\\b(" + keywordGroups.controlFlow.keywords.join("|") + ")\\b", "gim");
   mslText = mslText.replace(reg, "$1<span class='msl_control_flow'>$2</span>");
   
   // Data Management group
   reg = new RegExp("^(\\s*)\\b(" + keywordGroups.dataManagement.keywords.join("|") + ")\\b", "gim");
   mslText = mslText.replace(reg, "$1<span class='msl_data_managemen'>$2</span>");

   // Information group
   reg = new RegExp("^(\\s*)\\b(" + keywordGroups.info.keywords.join("|") + ")\\b", "gim");
   mslText = mslText.replace(reg, "$1<span class='msl_info'>$2</span>");

   // Conditional group
   reg = new RegExp("^(\\s*)\\b(" + keywordGroups.cond.keywords.join("|") + ")\\b", "gim");
   mslText = mslText.replace(reg,"$1<span class='msl_cond'>$2</span>");

   // Units group
   reg = new RegExp("\\b(" + keywordGroups.units.keywords.join("|") + ")\\b", "gi");
   mslText = mslText.replace(reg, "<span class='msl_units'>$1</span>");
   
   // Action group
   reg = new RegExp("\\b(" + keywordGroups.actions.keywords.join("|") + ")\\b(\\s*)$", "gim");
   mslText = mslText.replace(reg, "<span class='msl_actions'>$1</span>$2");

   // Numbers/boolean group
   reg = /\b(\d+(\.\d+)?([eE][-+]?\d+)?)\b/g;
   mslText = mslText.replace(reg, '<span class="msl_number">$1</span>');
   reg = /\b(true|false)\b/gi;
   mslText = mslText.replace(reg, '<span class="msl_bool">$1</span>');
   
   // Break lines and handle one by one
   seqLines = mslText.split("\n");
   
   // This is important for Firefox
   let emptyClass = "";
   let isChrome = browserType();
   if (isChrome === 1) emptyClass = "esline";
   // Loop and restore comment lines and empty lines 
   for (let j = 0; j < seqLines_org.length ; j++) {
      let line = seqLines_org[j];
      commentIndex =  line.indexOf("#");
      if (line.trim().startsWith("#") || line.trim().toLowerCase().startsWith("comment")) {
	 // Restore comment lines without highlighting
	 seqLines[j] = `<span class='msl_comment'>${line}</span>`;
      } else if (commentIndex > 0) {
         // Restore comment section at end of line
         const comment = line.slice(commentIndex);
         seqLines[j] = seqLines[j].slice(0, seqLines[j].indexOf("#")) + `</span><span class='msl_comment'>${comment}</span>`;
      }

      // empty class is needed for cursor movement in Firefox
      // for Chrome empty lines are skipped with arrow up??
      if (line === "") {
         if ((j === seqLines_org.length - 1) && (isChrome === 1)) {
            seqLines[j] = "<span class=' ' id='sline" + (j+1).toString() + "'></span>";
         } else if (isChrome === 1) {
            seqLines[j] = "<span class='" + emptyClass + "' id='sline" + (j+1).toString() + "'></span>";
         } else {
            seqLines[j] = "<span class='" + emptyClass + "' id='sline" + (j+1).toString() + "'></span>";
         }
      } else {
         seqLines[j] = "<span class='sline' id='sline" + (j+1).toString() + "'>" + seqLines[j] + "</span>";
      }
   }
   return seqLines;
}

// Adjust indentation of a selection of lines
function indent_msl(lines,addTab) {
   /* Arguments:
      lines  - an array of two elements, first and last line numbers
      addTab - (opt) +/-1 to add/subtract three spaces to selected lines
   */
   let indentLevel = 0;
   let singleLine = false;
   let editor = editorElements()[1];
   // Avoid issues of begenning of single line
   if (lines[0] > lines[1] || lines[0] == lines[1]) {
      lines[0] = lines[0] > 0 ? lines[0] : 1;
      lines[1] = lines[0];
      singleLine = true;
   }
   for (let j = lines[0]; j <= lines[1] ; j++) {
      let lineId = "#sline" + j.toString();
      let prevLineId = "#sline" + (j-1).toString();
      let lineEl = editor.querySelector(lineId);
      let line = "";
      if (lineEl) line = lineEl.innerText;
      if (addTab === 1) {
         let indentString = " ".repeat(3);
         lineEl.innerText = indentString + line;
      } else if (addTab === -1) {
         lineEl.innerText = line.replace(/^\s{1,3}/, '');
      } else if (singleLine && editor.querySelector(prevLineId)) {
         let prevLineEl = editor.querySelector(prevLineId);
         let prevLine = prevLineEl.innerText;
         const indentMinos = defIndent.indentminos.some(keyword => prevLine.trim().toLowerCase().startsWith(keyword.toLowerCase()));
         const indentPlus = defIndent.indentplus.some(keyword => prevLine.trim().toLowerCase().startsWith(keyword.toLowerCase()));
         if (indentMinos) {
            indentLevel = -1;
         } else if (indentPlus) {
            indentLevel = 1;
         }
         let preSpace = prevLine.search(/\S|$/) + (indentLevel * 3);
         if (preSpace < 0) preSpace = 0;
         let indentString = " ".repeat(preSpace);
         lineEl.innerText = indentString + line.trimStart();
      } else {
         const indentMinos = defIndent.indentminos.some(keyword => line.trim().toLowerCase().startsWith(keyword.toLowerCase()));
         if (indentMinos && indentLevel > 0) indentLevel--;
         let indentString = " ".repeat(indentLevel * 3);
         if (line !== "" || indentString !== "") { 
            lineEl.innerText = indentString + line.trimStart();
         }
         const indentPlus = defIndent.indentplus.some(keyword => line.trim().toLowerCase().startsWith(keyword.toLowerCase()));
         if (indentPlus) indentLevel++;
      }
   }
}

// Prepare the parameters/variables (if present) from the ODB as an html table
// Also return default ODB Paths and their respective values 
function varTable(id,odbTreeVar) {
   /* Arguments:
      id         - ID of div to fill with the table of variables
      odbTreeVar - values of /Sequencer/Variables
   */

   let e = document.getElementById(id);
   if (e === null) {
      dlgAlert("Container ID was not give.");
      return;
   }
   let nVars = 1,nVarsOld = 0;
   let oldTable = document.getElementById("varTable");
   if (oldTable) nVarsOld = oldTable.rows.length;
   // If /Sequencer/Variables are empty return empty
   if (!odbTreeVar) {
      // Clear container row
      e.innerHTML = "";
      return;
   }
   let html = "<table id='varTable' class='mtable infotable'>\n";
   html += "<tr><th style='width: 120px'>Variable&nbsp;&nbsp;</th><th style='width: 200px'>Current value&nbsp;&nbsp;</th></tr>\n";
     
   // Go over all variables in ODB and count them
   let varCount = Object.keys(odbTreeVar).length;
   for (let key in odbTreeVar) {
      const match = key.match(/([^/]+)\/name$/);
      if (match) {
         nVars++;
         const name = match[1];
         const value = odbTreeVar[name];
         let isBool = (typeof(value) === "boolean");
         if (isBool) {
            html += `<tr><td>${name}</td><td><input type="checkbox" class="modbcheckbox" data-odb-path="/Sequencer/Variables/${name}"></span></td></tr>\n`;
         } else {
            html += `<tr><td>${name}</td><td><span class="modbvalue" data-odb-path="/Sequencer/Variables/${name}"></span></td></tr>\n`;
         }
      }
   }
   html += "</table>";
   if (nVars !== nVarsOld) {
      e.innerHTML = html;
   }
   return;
}

// Prepare the parameters/variables (if present) from the ODB as an html table
// Also return default ODB Paths and their respective values 
function parTable(odbTreeV,odbTreeC,odbTreeD,odbTreeO,odbTreeVar,editOnSeq) {
   /* Arguments:
      odbTree... - Objects of ODB values
   */
   let html = "";
   let odbDefPaths = [];
   let odbDefValues = [];
   let NValues = 0;
   
   html += "<table id='paramTable' class='mtable infotable'>";
   html += "<tr><th>Parameter&nbsp;&nbsp;</th><th>Initial value&nbsp;&nbsp;</th><th>Comment</th></tr>";
   
   const processParam = (name, value, isBool, defValue, optValue, comment) => {
      let parLine = `<tr><td>${name}</td>`;
      let inParLine = "";
      if (optValue) {
         // if not given the default is the first option
         if (defValue === undefined || defValue === "") defValue = optValue[0];
         const optionsHtml = optValue.map(option => `<option value="${option}" ${option === defValue ? 'selected' : ''}>${option}</option>`).join('');
         inParLine += `<select class="modbselect" data-odb-path="/Sequencer/Param/Value/${name}" data-odb-editable="1">${optionsHtml}</select>`;
      } else if (isBool) {
         inParLine += `<input type="checkbox" class="modbcheckbox" data-odb-path="/Sequencer/Param/Value/${name}" data-odb-editable="1"></input>`;
      } else {
         inParLine += `<span class="modbvalue" data-odb-path="/Sequencer/Param/Value/${name}" data-odb-editable="1" data-input="1"></span>`;
      }
      if (defValue !== undefined) {
         NValues++;
         odbDefPaths.push(`/Sequencer/Param/Value/${name}`);
         odbDefValues.push(defValue);
      }
      
      parLine += `<td>${inParLine}</td>`;
      parLine += `<td>${comment}</td></tr>`;
      html += parLine;
   };

   // Go over all parameters in ODB
   for (let key in odbTreeV) {
      const match = key.match(/([^/]+)\/name$/);
      if (match) {
         const name = match[1];
         // if variable is found use its value
         let value = (odbTreeVar && odbTreeVar[name]) ? odbTreeVar[name] : odbTreeV[name];
         let isBool = (typeof(value) === "boolean");
         let defValue = (value !== null && value !== undefined && value !== '') ? value : (odbTreeD && odbTreeD[name]) || value;
         let optValue = odbTreeO ? odbTreeO[name] : undefined; 
         let comment = odbTreeC[name] || '';
         if (typeof value !== "object") {
            processParam(name, value, isBool, defValue, optValue, comment);
         }
      }
   }
   
   // Go over Edit on sequence links
   for (let key in editOnSeq) {
     const match = key.match(/([^/]+)\/name$/);
      if (match) {
         const name = match[1];
         const value = editOnSeq[name];
         let isBool = (typeof(value) === "boolean");
         if (isBool) {
            html += `<tr><td>${name}</td><td><input type="checkbox" class="modbcheckbox" data-odb-path="/Experiment/Edit on sequence/${name}" data-odb-editable="1"></input></td><td></td></tr>\n`;
         } else {
            html += `<tr><td>${name}</td><td><span class="modbvalue" data-odb-path="/Experiment/Edit on sequence/${name}" data-odb-editable="1" data-input="1"></span></td><td></td></tr>\n`;
         }
      }
   }
   
   html += "</table>";
   return {
      html: html,
      paths: odbDefPaths,
      values: odbDefValues,
      NValues: NValues
   };
}


// Prepare the parameters/variables (if present) from the ODB as a table 
function dlgParam(debugFlag) {
   /* Arguments:
      debugFlag    - (opt) true/false run in debug/normal mode
   */

   let odbTree = JSON.parse(sessionStorage.getItem('parameters'));
   const editOnSeq = JSON.parse(sessionStorage.getItem('editonseq'));
   // If /Sequencer/Param are empty, start and return
   if (odbTree === null && editOnSeq === null ) {
   //if ((odbTree === null || Object.keys(odbTree).length) && (editOnSeq === null || Object.keys(editOnSeq).length)) {
      if (debugFlag) {
         modbset('/Sequencer/Command/Debug script',true);
      } else {
         modbset('/Sequencer/Command/Start script',true);
      }
      return;
   }

   let odbTreeV = null;
   let odbTreeC = null;
   let odbTreeD = null;
   let odbTreeO = null;
   let odbTreeVar = null;

   if (odbTree) {
      odbTreeV = odbTree.value;
      odbTreeC = odbTree.comment;
      odbTreeD = odbTree.defaults;
      odbTreeO = odbTree.options;
      odbTreeVar = JSON.parse(sessionStorage.getItem('variables'));
   }

   // Go over all parameters in ODB
   let seqParTable = parTable(odbTreeV,odbTreeC,odbTreeD,odbTreeO,odbTreeVar,editOnSeq);
   let html = seqParTable.html;
   // set all default values and once finished produce dialog
   // Collect paths where values start with "/"
   let valuesLinkODB = seqParTable.values.map((valueLinkODB, indexLinkODB) => {
      if (valueLinkODB && valueLinkODB.startsWith("/")) {
         return seqParTable.values[indexLinkODB];
      } else {
         return null;
      }
   }).filter(path => path !== null);
   mjsonrpc_db_get_values(valuesLinkODB).then(function (rpc) {
      if (rpc.result.status.every(status => status === 1)) {
         // substitute values
         rpc.result.data.forEach((newData, index) => {
            let pathIndex = seqParTable.values.indexOf(valuesLinkODB[index]);
            seqParTable.values[pathIndex] = `${newData}`; // Update corresponding value
         });
         console.log('Substituted seqParTable.values:', seqParTable.values);
         mjsonrpc_db_paste(seqParTable.paths,seqParTable.values).then(function (rpc) {
            if ((rpc.result.status.every(status => status === 1)) || seqParTable.values.length === 0) {
               console.log("db_paste finished.\n",seqParTable.paths,seqParTable.values);
               // if parContainer not given produce a dialog
               let htmlDlg = `${html}<br><button class="dlgButtonDefault" id="dlgParamStart" type="button">Start</button><button class="dlgButton" id="dlgParamCancel" type="button">Cancel</button><br>`;
               let d = dlgGeneral({html: htmlDlg,iddiv: "Parameters",minWidth:500});
               let e = document.getElementById("parContainer");
               // Append the table to a container
               let startBtn = document.getElementById("dlgParamStart");
               let cancelBtn = document.getElementById("dlgParamCancel");
               cancelBtn.addEventListener("click", function () {d.remove();});
               startBtn.addEventListener("click", function () {
                  d.remove();
                  if (debugFlag) {
                     modbset('/Sequencer/Command/Debug script',true);
                  } else {
                     modbset('/Sequencer/Command/Start script',true);
                  }
               });
            } else {
               console.log("db_paste failed:",rpc.result.status," Trying again...");
               dlgParam(debugFlag);
            }
         }).catch(function (error) {
            console.error(error);
         });
      } else {
         let message = `ODB "${valuesLinkODB}" was not found.<br>Cannot start sequence!`;
         dlgAlert(message);
      }
   }).catch(function (error) {
      console.error(error);
   });
}


// helper debug function
function debugSeq(parContainer) {
   startSeq(parContainer,true);
}

// helper start function
function startSeq(parContainer,debugFlag) {
   const [lineNumbers,editor,label] = editorElements();
   if (!debugFlag) debugFlag = false;
   if (editor.id !== "editorTab1" && parContainer === undefined) {
      let filename = label.title;
      if (!filename) {
         dlgAlert("Please give the file a name first (Save as).");
         return;
      }
      const message = debugFlag ? `Save and debug ${filename}?` : `Save and start ${filename}?`;
      dlgConfirm(message,function(resp) {
         if (resp) {
            seqSave(filename);
            openETab(document.getElementById("etab1-btn"));
            seqOpen(filename);
            // Make sure to load file an reset parameters
            mjsonrpc_db_paste(["/Sequencer/Command/Load new file"],[true]).then(function (rpc) {
               if (rpc.result.status[0] === 1) {
                  dlgParam(debugFlag);
               }
            });
         }
      });
   } else {
      // make sure to load file first
      mjsonrpc_db_paste(["/Sequencer/Command/Load new file"],[true]).then(function (rpc) {
         if (rpc.result.status[0] === 1) {
            dlgParam(debugFlag);
         }
      });
   }
}

// Helper function to add the current file to next files queue
function setAsNext() {
   const [lineNumbers,editor,label] = editorElements();
   // Not allowed from console
   if (editor.id === "editorTab1") return;
   // This is the addAsNext button cell
   const e = document.getElementById("addAsNext");
   let filename = label.title.split("\n")[0].replace(/^sequencer\//,'');
   let message = `Save and put ${filename} in the next file queue?`;
   dlgConfirm(message,function(resp) {
      if (resp) {
         seqSave(filename);
         let order = chngNextFilename(e,filename);
         //dlgAlert(`File save and placed in position ${order} in the queue.`);
      }
   });
}

// helper stop function
function stopSeq() {
   const message = `Are you sure you want to stop the sequence?`;
   dlgConfirm(message,function(resp) {
      if (resp) {
         modbset('/Sequencer/Command/Stop immediately',true);
      }
   });
}

// Show or hide parameters table
function showParTable(varContainer) {
   let e = document.getElementById(varContainer);
   let vis = document.getElementById("showParTable").checked;
   let visNF = (document.getElementById("nextFNContainer").style.display === "none");
   let calcWidth = (window.innerWidth - e.parentElement.previousElementSibling.getBoundingClientRect().left - 28);
   if (vis) {
      e.parentElement.previousElementSibling.style.width = `calc(${calcWidth}px - 19.5em)`;
      e.parentElement.previousElementSibling.style.maxWidth = `calc(${calcWidth}px - 19.5em)`;
      e.style.display = "flex";
      e.parentElement.style.width = "19.5em";
   } else {
      e.style.display = "none";
      if (visNF) {
         e.parentElement.style.width = "0";
         e.parentElement.previousElementSibling.style.width = `calc(${calcWidth}px )`;
         e.parentElement.previousElementSibling.style.maxWidth = `calc(${calcWidth}px )`;
      }
   }
}

// Show or hide next file list
function showFNTable(nextFNContainer) {
   let e = document.getElementById(nextFNContainer);
   let vis = document.getElementById("showNextFile").checked;
   let visVar = (document.getElementById("varContainer").style.display === "none");
   let html = "";
   let addFileRow = "";
   let nFiles = 0;
   mjsonrpc_db_get_values(["/Sequencer/State/Next Filename"]).then(function(rpc) {
      if (rpc.result.status[0] !== 1) return;
      let fList = rpc.result.data[0];
      for (let i = 0; i < fList.length; i++) {
         if (fList[i] && fList[i].trim() !== "") {
            html += `<tr class="dragrow" draggable="true"><td style="cursor: all-scroll;"><img draggable="false" style="cursor: all-scroll;" src="icons/menu.svg" title="Drag and drop to reorder"></td><td><img draggable="false" src="icons/folder-open.svg" title="Change file" onclick="chngNextFilename(this.parentElement);"></td><td style="cursor: all-scroll;" title="Drag and drop to reorder" ondblclick="chngNextFilename(this);">${fList[i]}</td><td><img draggable="false" title="Remove file from list" onclick="remNextFilename(this.parentElement);" src="icons/trash-2.svg"></td></tr>`;
            nFiles++;
         }
      }
      let calcWidth = (window.innerWidth - e.parentElement.previousElementSibling.getBoundingClientRect().left - 28);
      if (vis && html !== "") {
         e.parentElement.previousElementSibling.style.width = `calc(${calcWidth}px - 19.5em)`;
         e.parentElement.previousElementSibling.style.maxWidth = `calc(${calcWidth}px - 19.5em)`;
         e.style.display = "flex";
         e.parentElement.style.width = "19.5em";
         if (nFiles < 10) {
            addFileRow = `<tr><td id="addAsNext"><img src="icons/file-plus.svg" title="Add file" onclick="chngNextFilename(this.parentElement);"></td><td></td><td></td><td></td></tr>`;
         }
         e.innerHTML = `<table class="mtable infotable" style="width:100%"><tr><th style="width:10px;"></th><th style="width:10px;"></th><th>Next files</th><th  style="width:10px;"></th></tr>${html}${addFileRow}</table>`;
      } else {
         e.style.display = "none";
         if (visVar) {
            e.parentElement.style.width = "0";
            e.parentElement.previousElementSibling.style.width = `calc(${calcWidth}px )`;
            e.parentElement.previousElementSibling.style.maxWidth = `calc(${calcWidth}px )`;
         }
      }
      activateDragDrop(e);
   }).catch (function (error) {
      console.error(error);
   });
}

// Show error state of sequencer
function checkError(element) {
   let e = element.parentElement.parentElement;
   if (element.value === "") {
      e.style.display = "none";
   } else {
      e.style.display = "table-row";
   }
}

// Show extra rows for wait and loop
function extraRows(e) {
   /* Arguments:
      e - triggering element to identify wait or loop
   */
   // get current row, table and dialog
   let rIndex = e.parentElement.parentElement.rowIndex;
   let table = e.parentElement.parentElement.parentElement;
   let progressDlg = table.parentElement.parentElement.parentElement.parentElement.parentElement;
   // check if there is a wait or loop commands (if non-zero)
   if (e.value) {
      showProgress();
      if (e.id === "waitTrig") {
         // Make sure there is only one wait row
         const waitTRs = document.querySelectorAll('.waitTR');
         const waitFormula = (e.value === "Seconds") ? 'data-formula="x/1000"' : '';
         if (waitTRs.length) waitTRs.forEach(element => element.remove());
         // Insert a new row
         let tr = table.insertRow(rIndex+1);
         tr.className = "waitTR";
         tr.innerHTML = `<td style="position: relative;width:80%;" colspan="2">&nbsp;
                    <span class="modbhbar waitloopbar" style="position: absolute; top: 0; left: 0; color: lightgreen;" data-odb-path="/Sequencer/State/Wait value" ${waitFormula} id="mwaitProgress"></span>
                    <span class="waitlooptxt">
                      Wait: [<span class="modbvalue" data-odb-path="/Sequencer/State/Wait value" ${waitFormula}></span>/<span class="modbvalue" data-odb-path="/Sequencer/State/Wait limit" ${waitFormula} onchange="if (document.getElementById('mwaitProgress')) document.getElementById('mwaitProgress').dataset.maxValue = this.value;"></span>] <span class="modbvalue" data-odb-path="/Sequencer/State/Wait type"></span>
                    </span>
                  </td>`;
         windowResize();
      } else if (e.id === "loopTrig") {
         mjsonrpc_db_get_values(["/Sequencer/State/Loop n"]).then(function(rpc) {
            let loopArray = rpc.result.data[0];
            for (let i = 0; i < loopArray.length; i++) {
               if (loopArray[i] === 0) break;
               let tr = table.insertRow(rIndex+1);
               tr.className = "loopTR";
               tr.innerHTML = `<td style="position: relative;width:80%;" colspan="2">&nbsp;
                    <span class="modbhbar waitloopbar" style="position: absolute; top: 0; left: 0; color: #CBC3E3;" data-odb-path="/Sequencer/State/Loop counter[${i}]" id="mloopProgress${i}"></span>
                    <span class="waitlooptxt">
                      Loop ${i}: [<span class="modbvalue" data-odb-path="/Sequencer/State/Loop counter[${i}]"></span>/<span class="modbvalue" data-odb-path="/Sequencer/State/Loop n[${i}]" onchange="if (document.getElementById('mloopProgress${i}')) document.getElementById('mloopProgress${i}').dataset.maxValue = this.value;"></span>]
                    </span>
                  </td>`;
               windowResize();
            }
         }).catch (function (error) {
            console.error(error);
         });
      }
   } else {
      // remove rows
      if (e.id === "waitTrig") document.querySelectorAll('.waitTR').forEach(element => element.remove());
      if (e.id === "loopTrig") document.querySelectorAll('.loopTR').forEach(element => element.remove());
      // hide progress div
      //dlgHide(progressDlg);
      windowResize();
   }
}

// Helper function to identify browser, 1 FF, 2 Chrome, 3, other
function browserType() {
   if (navigator.userAgent.indexOf("Chrome") !== -1) {
      return 1;
   } else if (navigator.userAgent.indexOf("Firefox") !== -1) {
      return 2;
   }

   return 1;
}

// make visual hint that file is changes
function seqIsChanged(flag) {
   // flag - true is change, false is saved
   let fileChangedId = "filechanged" + editorElements()[0].id.replace("lineNumbers","");
   let filechanged = document.getElementById(fileChangedId);
   if (filechanged) {
      if (flag) {
         filechanged.innerHTML = "&nbsp;&#9998;";
      } else if (flag === undefined) {
         // true if text has changed false if not
         return (filechanged.innerHTML !== "");
      } else {
         filechanged.innerHTML = "";
      }
   }
}

// save history of edits in element editor
function saveState(mslText,editor) {
   editor = (editor) ? editor : editorElements()[1];
   const editorId = editor.id;
   if (saveRevision[editorId] === false) {
      return;
   } else if (saveRevision[editorId] === undefined){
      saveRevision[editorId] = true;
   }

   if (!previousRevisions[editorId]) {
      previousRevisions[editorId] = [''];
      revisionIndex[editorId] = 0;
   }
   
   // Add one more revision, and trim array if we had some undos
   revisionIndex[editorId]++;
   if (revisionIndex[editorId] < previousRevisions[editorId].length - 1) {
      previousRevisions[editorId].splice(revisionIndex[editorId] + 1);
   }
   // Push new revision and keep only nRevisions revisions
   previousRevisions[editorId].push(mslText)
   if (previousRevisions[editorId].length > nRevisions) {
      previousRevisions[editorId].shift();
   }
   revisionIndex[editorId] = previousRevisions[editorId].length - 1;
}

// undo last edit
function undoEdit(editor) {
   editor = (editor) ? editor : editorElements()[1];
   const editorId = editor.id;
   if (revisionIndex[editorId] < 1) {
      // disable menu item
      disableMenuItems("undoMenu",true);
      return;
   } else {
      // enable menu item
      disableMenuItems("undoMenu",false);
      revisionIndex[editorId]--;
   }
   // reset redo
   disableMenuItems("redoMenu",false);

   let caretPos = getCurrentCursorPosition(editor);
   let currText = editor.innerText;
   saveRevision[editorId] = false;   
   editor.innerHTML = syntax_msl(previousRevisions[editorId][revisionIndex[editorId]]).join(lc).slice(0,-1);
   updateLineNumbers(editor.previousElementSibling,editor);
   saveRevision[editorId] = true;   
   // calculate change in caret position based on length
   caretPos = caretPos + previousRevisions[editorId][revisionIndex[editorId]].length - currText.length;
   setCurrentCursorPosition(editor, caretPos);
}

// redo the undo
function redoEdit(editor) {
   editor = (editor) ? editor : editorElements()[1];
   const editorId = editor.id;
   if (revisionIndex[editorId] >= previousRevisions[editorId].length - 1) {
      // disable menu item
      disableMenuItems("redoMenu",true);
      return;
   } else {
      // enable menu item
      disableMenuItems("redoMenu",false);
      revisionIndex[editorId]++;
   }
   // reset undo
   disableMenuItems("undoMenu",false);

   let caretPos = getCurrentCursorPosition(editor);
   let currText = editor.innerText;
   saveRevision[editorId] = false;   
   editor.innerHTML = syntax_msl(previousRevisions[editorId][revisionIndex[editorId]]).join(lc).slice(0,-1);
   updateLineNumbers(editor.previousElementSibling,editor);
   saveRevision[editorId] = true;   
   // calculate change in caret position based on length
   caretPos = caretPos + previousRevisions[editorId][revisionIndex[editorId]].length - currText.length;
   setCurrentCursorPosition(editor, caretPos);
}

// Select slines from startLine to endLine
function selectLines([startLine, endLine],e) {
   const selection = window.getSelection();
   // Remove existing selections
   selection.removeAllRanges();
   let startElementId = '#sline' + startLine;
   let endElementId = '#sline' + endLine;
   let startElement = null, endElement = null;
   if (e.querySelector(startElementId)) startElement = e.querySelector(startElementId).firstChild;
   if (e.querySelector(endElementId)) endElement = e.querySelector(endElementId).lastChild;
   // we need startElement and endElement with first/lastChild
   // the following prevents loosing selection but not ideal
   while (startElement === null && startLine <= endLine) {
      startLine++;
      startElementId = '#sline' + startLine;
      startElement = e.querySelector(startElementId).firstChild;
   }
   while (endElement === null && endLine > 0) {
      endLine--;
      endElementId = '#sline' + endLine;
      endElement = e.querySelector(endElementId).lastChild;
   }
   if (startElement && endElement) {
      const range = document.createRange();
      // Set the start of the range to the startElement at offset 0
      range.setStart(startElement, 0);
      // Set the end of the range to the endElement at its length
      range.setEnd(endElement, endElement.childNodes.length);
      // Add the range to the selection
      selection.addRange(range);
   }
}

// switch between dark and light modes on request
function lightToDark(lToDcheck) {
   const edt_areas = document.querySelectorAll('.edt_area');
   if (lToDcheck.checked) {
      localStorage.setItem("darkMode", true);
      edt_areas.forEach(area => {
         area.style.backgroundColor = "black";
         area.style.color = "white";
      });
      updateCSSRule(".etab button:hover","background-color","black");
      updateCSSRule(".etab button:hover","color","white");
      updateCSSRule(".etab button:hover","border-bottom","5px solid black");
      updateCSSRule(".etab button.edt_active","background-color","black");
      updateCSSRule(".etab button.edt_active","color","white");
      updateCSSRule(".etab button.edt_active","border-bottom","5px solid black");
      updateCSSRule(".etab button.edt_active:hover","background-color","black");
      updateCSSRule(".etab button.edt_active:hover","border-bottom","5px solid black");
   } else {
      localStorage.removeItem("darkMode");
      edt_areas.forEach(area => {
         area.style.backgroundColor = "white";
         area.style.color = "black";
      });
      updateCSSRule(".etab button:hover","background-color","white");
      updateCSSRule(".etab button:hover","color","black");
      updateCSSRule(".etab button:hover","border-bottom","5px solid white");
      updateCSSRule(".etab button.edt_active","background-color","white");
      updateCSSRule(".etab button.edt_active","color","black");
      updateCSSRule(".etab button.edt_active","border-bottom","5px solid white");
      updateCSSRule(".etab button.edt_active:hover","background-color","white");
      updateCSSRule(".etab button.edt_active:hover","border-bottom","5px solid white");
   }
}

function updateCSSRule(selector, property, value) {
   for (let i = 0; i < document.styleSheets.length; i++) {
      let styleSheet = document.styleSheets[i];
      let rules = styleSheet.cssRules || styleSheet.rules;
      if (!rules) continue;
      for (let j = 0; j < rules.length; j++) {
         let rule = rules[j];
         if (rule.selectorText === selector) {
            rule.style[property] = value;
            return;
         }
      }
   }
}

// show/hide wait and loop progress
function showProgress(e) {
   //const progressDiv = document.getElementById("progressDiv");
   const progressDiv = document.getElementById("Progress");
   if (e === undefined) e = document.getElementById("showProgressBars");
   if (e.checked) {
      localStorage.setItem("showProgress",true);
      progressDiv.style.display = "block";
      //dlgShow(progressDiv);
   } else {
      localStorage.removeItem("showProgress");
      progressDiv.style.display = "none";
      //dlgHide(progressDiv);
   }      
}

// Mark the current line number
function markCurrLineNum() {
   const [lineNumbers,editor] = editorElements();
   const currLines = lineNumbers.querySelectorAll(".edt_linenum_curr");
   const [startLine,endLine] = getLinesInSelection(editor);
   if (startLine === 0 && endLine === 0) return;
   currLines.forEach((line) => line.classList.remove("edt_linenum_curr"));
   for (let i = startLine; i <= endLine; i++) {
      let lineNumId = "#lNum" + i.toString();
      let lineNum = lineNumbers.querySelector(lineNumId);
      if (lineNum) 
         lineNum.className = "edt_linenum_curr";
   }
}

// Check if sequencer program is in ODB and running.
// If not, try to get it going
function checkSequencer() {
   mjsonrpc_call('cm_exist', {"name":"Sequencer","unique":true}).then(function (rpc1) {
      mjsonrpc_db_get_values(["/Programs/Sequencer/Start command"]).then(function(rpc2) {
         let isRunning = (rpc1.result.status === 1);
         let isDefined = ((rpc2.result.data[0] !== null) && (rpc2.result.data[0] !== ""));
         if (isRunning && isDefined) return;
         // sequencer not running or not defined, stop it just in case and check the reason
         mjsonrpc_stop_program("Sequencer");
         let message = "";
         if (isDefined) {
            message = "Sequencer program is not running.<br>Should I start it?"
         } else {
            message = "Sequencer program is not configured and not running.<br>Should I try to start it anyway?"
         }
         dlgConfirm(message,function(resp) {
            if (resp) {
               if (!isDefined) {
                  // assume that sequencer in path and create a start command, sleep 2s,
                  // set value to "msequencer -D", sleep 2s, start program
                  mjsonrpc_db_create([{"path" : "/Programs/Sequencer/Start command", "type" : TID_STRING}]).then(function (rpc3) {
                     setTimeout(function(){
                        mjsonrpc_db_paste(["/Programs/Sequencer/Start command"],["msequencer -D"]).then(function (rpc4) {
                           if (rpc4.result.status[0] === 1) {
                              mjsonrpc_start_program("Sequencer");
                           }
                        }).catch(function (error) {
                           console.error(error);
                        });
                     }, 2000);
                  }).catch(function (error) {
                     console.error(error);
                  });
               } else {
                  mjsonrpc_start_program("Sequencer");
               }
               // take 3 seconds and check that it actually started
               setTimeout(function(){
                  mjsonrpc_call('cm_exist', {"name":"Sequencer","unique":true}).then(function (rpc5) {
                     if (rpc5.result.status === 1) {
                        dlgAlert("Sequencer started successfully.");
                     } else {
                        dlgAlert("Failed to start Sequencer!<br>Try to start it manually (msequencer -D)");
                     }
                  });
               }, 3000);
            }
         });
      }).catch (function (error) {
         console.error(error);
      });
   }).catch(function (error) {
      console.error(error);
   });
}

function captureSelected() {
   if (window.getSelection) {
      let selection = window.getSelection();
      let text = selection.toString();
      let range = selection.getRangeAt(0);
      if (text && range) {
         const rangeData = {
            startPath: range.startContainer,
            startOffset: range.startOffset,
            endPath: range.endContainer,
            endOffset: range.endOffset
         };
         sessionStorage.setItem("tempSelText", text);
         sessionStorage.setItem("tempSelRange", JSON.stringify(rangeData));
      }
   }
}

function editMenu(action) {
   let text = sessionStorage.getItem("tempSelText") ?? "";
   let storedRange = sessionStorage.getItem("tempSelRange") ?? "";
   /*
   if (storedRange) {
      let rangeData = JSON.parse(storedRange);
      let startContainer = nodeFromPath(rangeData.startPath);
      let endContainer = nodeFromPath(rangeData.endPath);
      
      // Create a new range
      let newRange = new Range();
      newRange.setStart(startContainer, rangeData.startOffset);
      newRange.setEnd(endContainer, rangeData.endOffset);

      // Select the new range
      let selection = window.getSelection();
      selection.removeAllRanges();
      selection.addRange(newRange);
   }
   */
   if (action === "Copy") {
      if (text) {
         sessionStorage.setItem("copiedText",text);
      }
   } else if (action === "Paste") {
      const copiedText = sessionStorage.getItem("copiedText");
      if (copiedText) {
         newRange.deleteContents();
         newRange.insertNode(document.createTextNode(copiedText));
      }
   } else if (action === "Cut") {
      if (text) {
         sessionStorage.setItem("copyText",text);
         //document.execCommand("cut");
         newRange.deleteContents();
      }
   } else if (action === "Undo") {
      undoEdit();
   } else if (action === "Redo") {
      redoEdit();
   }
}

// Switch to the clicked tab
function openETab(btn) {
   const tabcontent = document.querySelectorAll(".etabcontent");
   const tablinks = document.querySelectorAll(".etablinks");
   const tab = btn ? btn : document.querySelectorAll(".edt_active")[0];
   const tabID = tab.id.replace("-btn","")
   tabcontent.forEach(content => {
      content.style.display = "none";
   });
   tablinks.forEach(link => {
      link.classList.remove("edt_active");
   });
   tab.className += " edt_active";
   // For the main sequence tab disable Save and Save as...
   if (tabID === "etab1") {
      document.getElementById(tabID).style.display = "flex";
      disableMenuItems("noteditor",true);
   } else {
      document.getElementById(tabID).style.display = "block";
      disableMenuItems("noteditor",false);
   }
   // Adjust height of active editor
   windowResize();
}

// Close the clicked tab
function closeTab(tab,event) {
   const tablinks = document.querySelectorAll(".etablinks");
   if (tablinks.length < 3) return;
   const tabCount = parseInt(tab.parentElement.id.replace("-btn","").replace("etab",""));
   const tabBtn = document.getElementById(`etab${tabCount}-btn`);
   const tabContent = document.getElementById(`etab${tabCount}`);
   if (seqIsChanged()) {
      dlgConfirm("File was changed, close anyway?", function(resp) {
         if (resp) {
            tabBtn.remove();
            tabContent.remove();
            // switch to previous tab
            openETab(document.getElementById(`etab${(tabCount-1)}-btn`));
         }
      });
   } else {
      tabBtn.remove();
      tabContent.remove();
      // switch to previous tab
      openETab(document.getElementById(`etab${(tabCount-1)}-btn`));
   }
   // need this since the close button is inside the tab button
   event.stopPropagation();
}

// Create and add a new editor tab
function addETab(btn) {
   // Create tab button
   const tabBtn = document.createElement("button");
   tabBtn.className = "etablinks";
   const tabCount = (btn.previousElementSibling) ? parseInt(btn.previousElementSibling.id.replace("-btn","").replace("etab","")) + 1 : 1; 
   tabBtn.id = "etab" + tabCount + "-btn";
   tabBtn.innerHTML = `<span id="etab${tabCount}-lbl">new file${tabCount}</span><span style="color: var(--mred);" id="filechanged${tabCount}"></span><span onclick="closeTab(this,event);" class="closebtn">&times;</span>`;
   btn.parentNode.insertBefore(tabBtn,btn);
   tabBtn.onclick = function () { openETab(this);};

   // Create editor area
   const tabContent = document.createElement("div");
   tabContent.id = "etab" + tabCount;
   tabContent.className = "etabcontent";
   let makeDark = "";
   if (localStorage.getItem("darkMode")) makeDark = "style='background-color: black; color: white;'";
   const html =
         `<pre id="lineNumbers${tabCount}" class="edt_linenum"></pre><pre id="editorTab${tabCount}" ${makeDark} class="edt_area" spellcheck="false" contenteditable="false"></pre>`;
   tabContent.innerHTML = html;
   const lastETab = document.getElementById("lastETab");
   lastETab.parentNode.insertBefore(tabContent,lastETab);
   tabBtn.click();
   // Add event listeners
   editorEventListeners();
} 

// Return the pre of lineNumbers, editor, tab label and tab button element of the active tab
function editorElements() {
   const btn = (document.querySelectorAll(".edt_active")[0]) ? document.querySelectorAll(".edt_active")[0] : document.getElementById("etab1-btn");
   const tab = document.getElementById(btn.id.replace("-btn",""));
   const [lineNumbers,editor] = tab.children;
   const btnLabel = (btn.id !== "etab1-btn") ? btn.children[0] : btn.children[1];
   return [lineNumbers,editor,btnLabel,btn];
}
   
// disable and enable clicking on menu item
function disableMenuItems(className,flag) {
   /* Arguments:
      className - the class name of the item
      flag      - true/false to enable/disable item
   */
   const els = document.querySelectorAll(`.${className}`);
   els.forEach(e => {
      if (flag) {
         e.style.opacity = 0.5;
         e.style.pointerEvents = "none";
      } else {
         e.style.opacity = "";
         e.style.pointerEvents = "";
      }
   });
}

// Function to replace some special characters in the text
function escapeSpecialCharacters(text) {
   return text.replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/\r\n|\n\r/g, '\n')
      //.replace(/"/g, "&quot;")
      //.replace(/'/g, "&#39;")
      .replace(/\t/g, "   ");
}

// Produce a help window
function dlgHelp() {
   let help = `
<span style='text-align: left;'>
<b>Hidden features of the sequencer editor</b>
<ul style='white-space: pre;font-family: monospace;'>
<li>Double click on the edit area of the first (main) tab to edit the currently loaded sequence.</li>
<li>Tab       - Indent selected lines.</li>
<li>Shift+Tab - Unindent selected lines.</li>
<li>Escape    - Autoindent selected lines according to syntax rules.</li>
<li>Ctrl+C    - Copy selected text.</li>
<li>Ctrl+V    - Paste selected text.</li>
<li>Ctrl+A    - Select all text.</li>
<li>Ctrl+Z    - Undo last change.</li>
<li>Ctrl+R    - Redo last undo.</li>
</ul>
</span>
`;
   const d = dlgMessage("Editor help",help, false, false);
   const btn = d.querySelectorAll('.dlgButton')[0];
   btn.className = "dlgButtonDefault";
   btn.focus();
   
}

// Activate drag and drop events on next files table
function activateDragDrop(table) {
   /* Arguments:
      table - The table element containing the list of next files.
   */
   // collect all rows with class dragrow
   const rows = table.querySelectorAll('.dragrow');
   let dragStartIndex,dragEndIndex;
   let emptyRow;
   // add event listeners
   rows.forEach(row => {
      row.addEventListener('dragstart', dragStart);
      row.addEventListener('dragover', dragOver);
      row.addEventListener('dragend', dragEnd);
   });
   
   function dragStart(e) {
      dragStartIndex = Array.from(rows).indexOf(this);
      rows.forEach(row => row.classList.remove('dragstart'));
      this.classList.add('dragstart');
   }
   function dragOver(e) {
      e.preventDefault();
      dragEndIndex = Array.from(rows).indexOf(this);
      // Create or update the empty row element
      if (!emptyRow) {
         emptyRow = document.createElement('tr');
         emptyRow.innerHTML = "<td colspan=4>&nbsp;</td>";
         emptyRow.classList.add('empty-dragrow');
      }
      // Insert the empty row element at the appropriate position
      if (dragEndIndex > dragStartIndex) {
         this.parentNode.insertBefore(emptyRow, this.nextSibling);
      } else if (dragEndIndex < dragStartIndex) {
         this.parentNode.insertBefore(emptyRow, this);
      }
   }
   function dragEnd(e) {
      if (emptyRow) {
         emptyRow.remove();
         emptyRow = null;
      }
      reorderNextFilenames(dragStartIndex,dragEndIndex);
      rows.forEach(row => {
         row.classList.remove('dragstart');
      });
   }
}

// Move next filename from dragStarIndex to dragEndIndex position
function reorderNextFilenames(dragStarIndex,dragEndIndex) {
   /* Arguments:
      dragStarIndex - move file from this indext.
      dragEndIndex  - destination index of the file.
   */
   if (dragStarIndex === dragEndIndex) return;
   let istep = 1;
   let istart = dragStarIndex;
   let iend = dragEndIndex;
   let odbpath = "/Sequencer/State/Next Filename";
   if (istart > iend) {
      step = -1;
      istart = dragEndIndex;
      iend = dragStarIndex;
   }
   mjsonrpc_db_get_values([odbpath]).then(function(rpc) {
      if (rpc.result.status[0] !== 1) return;
      let fList = rpc.result.data[0];
      let draggedFile = fList.splice(dragStarIndex,1)[0];
      fList.splice(dragEndIndex,0,draggedFile);
      let paths = [];
      let values = [];
      let j = 0;
      for (let i = istart; i <= iend; i=i+istep) {
         paths[j] = odbpath + "[" + i.toString() + "]";
         values[j] = fList[i];
         j++;
      }
      mjsonrpc_db_paste(paths,values).then(function (rpc2) {
         if (rpc2.result.status[0] !== 1) {
            dlgAlert("Failed to move the file!<br>Please check.");
         } else if (rpc2.result.status[0] === 1) {
            showFNTable('nextFNContainer');
         }
      }).catch(function (error) {
         console.error(error);
      });
   }).catch (function (error) {
      console.error(error);
   });
}

// Change the next file name in the clicked row
function chngNextFilename(e,filename) {
   /* Arguments:
      e -        (optional) cell element on the same row of "next file" to be changed.
                 If last row (or undefined) add a file to the end of the queue.
      filename - (optional) The file name to add to the end of the queue.
  */
   if (e === undefined) e = document.getElementById("addAsNext");
   // file index from table row index
   let index = e ? e.parentElement.rowIndex - 1 : 0;
   // Only 10 files are allowed
   if (index > 9) {
      dlgAlert("Maximum number (10) of next files reached!");
      return;
   }
   let odbpath = "/Sequencer/State/Next Filename";
   mjsonrpc_db_get_values(["/Sequencer/State/Path"]).then(function(rpc) {
      let path = fnPath + "/" + rpc.result.data[0].replace(/\/+/g, '/').replace(/^\/+|\/+$/g, '');
      sessionStorage.setItem("pathName",path);
      if (filename) {
         odbpath = odbpath + "[" + index.toString() + "]";
         modbset(odbpath,filename);
      } else {
         file_picker(path,fnExt,function(filename) {
            filename = filename.replace(/^sequencer\//,'').replace(/^\//,'');
            odbpath = odbpath + "[" + index.toString() + "]";
            if (filename) {
               modbset(odbpath,filename);
            }
         });
      }
      return index+1;
   }).catch(function (error) {
      mjsonrpc_error_alert(error);
   });
}

// Reomve the next file name from the queue
function remNextFilename(e) {
   /* Arguments:
      e - cell element on the same row of "next file" to be removed
   */
   // file index from table row index
   let index = e.parentElement.rowIndex - 1;
   let odbpath = "/Sequencer/State/Next Filename";
   mjsonrpc_db_get_values([odbpath]).then(function(rpc) {
      if (rpc.result.status[0] !== 1) return;
      let fList = rpc.result.data[0];
      // empty last element
      fList.push("");
      let paths = [];
      let values = [];
      let j = 0;
      for (let i = index+1; i < fList.length; i++) {
         paths[j] = odbpath + "[" + (i-1).toString() + "]";
         values[j] = fList[i];
         j++;
      }
      mjsonrpc_db_paste(paths,values).then(function (rpc2) {
         if (rpc2.result.status[0] !== 1) {
            dlgAlert("Failed to remove file from list!<br>Please check.");
         } else if (rpc2.result.status[0] === 1) {
            showFNTable('nextFNContainer');
         }
      }).catch(function (error) {
         console.error(error);
      });
   }).catch (function (error) {
      console.error(error);
   });   
}

