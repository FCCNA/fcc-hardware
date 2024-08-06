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
*/

// Using Solarized color scheme - https://ethanschoonover.com/solarized/

// Default msl syntax
const defKeywordGroups = {
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
};

// Iindentation keywords
const defIndent = {
   indentplus: ["IF", "LOOP", "ELSE", "SUBROUTINE"],
   indentminos: ["ENDIF", "ENDLOOP", "ELSE", "ENDSUBROUTINE"],
};

var seq_css = `
/* For buttons */
.seqbtn {
   display: none;
   width: 100px;
}

.msl_linenum {
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
   -moz-user-select: text
}

.msl_linenum span {
   display: block;
}

.msl_linenum_curr {
   color: #000000; 
   font-weight: bold;
}

.msl_area {
   width: calc(100% - 3.5em);
   overflow:auto; 
   resize: vertical;
   background-color:white;
   color: black;
/* for black bg */
/*   background-color:black;
   color: white;*/
   padding: 5px;
   box-sizing: border-box;
   vertical-align: top;
   display: inline-block;
   margin: 0 0 0 0;
   font-family: monospace;
   white-space: pre;
   -moz-user-select: text;
}

/*span[id^="sline"] {*/
.sline {

   display: inline-block;
}
.esline {
   display: block;
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

/* Dropdown button styles*/
.dropbtn {
   background-color: Transparent;
  font-size: 120%;
   font-family: verdana,tahoma,sans-serif;
   border: none;
   cursor: pointer;
  text-align: center;
  width: 2.9em;
  height: 25px;
}

.dropbtn:hover {
   background-color: #C0D0D0;
}

/* Style the dropdown content (hidden by default) */
.dropdown-content {
   display: none;
   position: absolute;
   background-color: #f9f9f9;
  min-width: 100px;
   box-shadow: 0 8px 16px rgba(0,0,0,0.2);
   z-index: 10;
}

/* Style the dropdown links */
.dropdown-content a{
  color: black;
  padding: 12px 16px;
  text-decoration: none;
   display: block;
  white-space: nowrap;
}

.dropdown-content div{
  color: black;
  padding: 12px 16px;
  text-decoration: none;
  display: block;
  white-space: nowrap;
}

/* Change color on hover */
.dropdown-content a:hover {
  background-color: #ddd;
}
.dropdown-content div:hover {
  background-color: #ddd;
}

/* Show the dropdown menu when the button is hovered over */
.dropdown:hover .dropdown-content {
   display: block;
}
`;

// Implement colors and styles from KeywordsGroups in CSS
for (const group in defKeywordGroups) {
   const { mslclass, color, fontWeight } = defKeywordGroups[group];
   
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
const previousRevisions = [];
const nRevisions = 20;
var revisionIndex = -1;
var saveRevision = true;
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

// Enable editing of sequence
function editCurrentSeq(divID) {
   /* Arguments:
      divID - ID of <pre> of the editor area
   */
   if (!divID) return;
   let editor = document.getElementById(divID);
   editor.contentEditable = true;
   // Attached syntax highlight event editor
   editor.addEventListener("keyup",checkSyntaxEventUp);
   editor.addEventListener("keydown",checkSyntaxEventDown);
   editor.addEventListener("paste", checkSyntaxEventPaste);
   // Change in text
   editor.addEventListener("input", function() {
      seqIsChanged(true);
   });
   document.addEventListener("selectionchange", function(event) {
      if (event.target.activeElement === editor) markCurrLineNum(editor);
   });
   
   // Short cuts have to be attached to window
   window.addEventListener("keydown",shortCutEvent);
}

// apply changes of filename in the ODB (triggers reload)
function seqChange(filename) {
   /* Arguments:
      filename - full file name with path to change
   */
   if (!filename) return;
   const lastIndex = filename.lastIndexOf('/');
   const path = filename.substring(0, lastIndex).replace(/^sequencer/,"").replace(/\/+/g, '/').replace(/^\/+|\/+$/g, '');
   const name = filename.substring(lastIndex + 1);
   mjsonrpc_db_paste(["/Sequencer/Command/Load filename","/Sequencer/State/Path","/Sequencer/Command/Load new file"], [name,path,true]).then(function (rpc) {
      sessionStorage.removeItem("depthDir");
      return;
   }).catch(function (error) {
      console.error(error);
   });
}

// Load the sequence text from the file name in the ODB
function seqLoad() {
   mjsonrpc_db_get_values(["/Sequencer/State/Path","/Sequencer/State/Filename"]).then(function(rpc) {
      let path = rpc.result.data[0].replace(/\/+/g, '/');
      let filenameText = rpc.result.data[1];
      sessionStorage.setItem("fileName", filenameText);
      file_picker('sequencer/' + path ,'*.msl',seqChange,false,{},true);
   }).catch(function (error) {
      mjsonrpc_error_alert(error);
   });
}

// Save sequence text in filename.
function seqSave(filename) {
   /* Arguments:
      filename (opt) - undeined save to file in ODB
      - empty trigger file_picker
      - save to provided filename with path 
   */
   let editor = document.getElementById("mslCurrent");
   let text = editor.innerText.replaceAll("\u200b","");
   // if a full filename is provided, save text and return
   if (filename && filename !== "") {
      file_save_ascii(filename,text,seqChange);
      updateBtns('Stopped');
      return;
   }
   // empty or undefined file name
   mjsonrpc_db_get_values(["/Sequencer/State/Path","/Sequencer/State/Filename"]).then(function(rpc) {
      let path = rpc.result.data[0].replace(/\/+/g, '/').replace(/^\/+|\/+$/g, '');
      let filenameText = rpc.result.data[1];
      sessionStorage.setItem("fileName", filenameText);
      if (filenameText === "(empty)" || filenameText.trim() === "" || filename === "") {
         file_picker('sequencer/' + path ,'*.msl',seqSave,true,{},true);
      } else {
         filename = (path === "") ?  "sequencer/" + filenameText : "sequencer/" + path + "/" + filenameText;
      }
      if (filename) {
         file_save_ascii_overwrite(filename,text,seqChange);
         updateBtns('Stopped');
      }
   }).catch(function (error) {
      mjsonrpc_error_alert(error);
   });
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
   const seqStateSpan = document.getElementById("seqState");
   seqStateSpan.innerHTML = state;
   seqStateSpan.style.backgroundColor = color;
   // hide all buttons
   const hideBtns = document.querySelectorAll('.seqbtn');
   hideBtns.forEach(button => {
      button.style.display = "none";
   });
   // then show only those belonging to the current state
   const showBtns = document.querySelectorAll('.seqbtn.' + state);
   showBtns.forEach(button => {
      button.style.display = "inline-block";
   });
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
   const m = document.getElementById("mmain");
   const mslCurrent = document.getElementById("mslCurrent");
   const lineNumbers = document.getElementById('lineNumbers');
   const seqTable = document.getElementById("seqTable");
   mslCurrent.style.height = document.documentElement.clientHeight - mslCurrent.getBoundingClientRect().top - 15 + "px";
   // Sync line number height
   lineNumbers.style.height = mslCurrent.style.height;
   seqTable.style.width = m.getBoundingClientRect().width - 10 + "px";
}

// Load the current sequence from ODB
function load_msl(divID) {
   /* Arguments:
      divID - (optional) div id of editor
   */
   if (divID === undefined || divID == "")
      divID = "mslCurrent";
   const editor = document.getElementById(divID);
   mjsonrpc_db_get_values(["/Sequencer/Script/Lines","/Sequencer/State/Running","/Sequencer/State/SCurrent line number"]).then(function(rpc) {
      let seqLines = rpc.result.data[0];
      let seqState = rpc.result.data[1];
      let currLine = rpc.result.data[2];
      // syntax highlight
      if (seqLines) {
         seqLines = syntax_msl(seqLines);
         let seqHTML = seqLines.join(lc);
         editor.innerHTML = seqHTML;
         if (seqState) hlLine(currLine);
      }
      // Make not editable until edit button is pressed
      editor.contentEditable = false;
      seqIsChanged(false);
      window.removeEventListener("keydown",shortCutEvent);
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

// shortcut event handling to overtake default behaviour
function shortCutEvent(event) {
   if (event.altKey && event.key === 's') {
      event.preventDefault();
      seqSave('');
      event.preventDefault();
   } else if ((event.ctrlKey || event.metaKey) && event.key === 's') {
      event.preventDefault();
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

// Trigger syntax highlighting on keyup events
function checkSyntaxEventUp(event) {
   if (event.ctrlKey || event.altKey || event.metaKey || MetaCombo) return;
   if (event.keyCode >= 0x30 || event.key === ' '
       || event.key === 'Backspace' || event.key === 'Delete'
       || event.key === 'Enter'
      ) {
      const e = event.target;
      let caretPos = getCurrentCursorPosition(e);
      // Indent according to previous line
      if (event.key === 'Enter') {
         // get previous and current line elements (before and after enter)
         let pline = whichLine(e,-1);
         let cline = whichLine(e);
         let plineText = (pline) ? pline.innerText : null;
         let clineText = (cline) ? cline.innerText : null;
         let indentLevel = 0;
         let preSpace = 0;
         
         if (plineText) {
            // indent line according to the previous line text
            // if, loop, else, subroutine - increase indentation
            const indentPlus = defIndent.indentplus.some(keyword => plineText.trim().toLowerCase().startsWith(keyword.toLowerCase()));
            // else, endif, endloop, endsubroutine - decrease indentation
            const indentMinos = defIndent.indentminos.some(keyword => plineText.trim().toLowerCase().startsWith(keyword.toLowerCase()));
            if (indentMinos) {
               indentLevel = -1;
            } else if (indentPlus) {
               indentLevel = 1;
            }
            // Count number of white spaces at begenning of pline and add indentation
            preSpace = plineText.replace("\n","").search(/\S|$/) + (indentLevel * 3);
            if (preSpace < 0) preSpace = 0;
            // Adjust and insert indentation 
            const indentString = " ".repeat(preSpace);
            let range = window.getSelection().getRangeAt(0);
            range.deleteContents();
            range.insertNode(document.createTextNode(indentString));
            caretPos += preSpace;
            if (indentMinos) {
               // remove extra space before line starting with indentMinos keyword
               let orgLine = plineText;
               let newLine = indentString + orgLine.trimStart();
               pline.innerText = newLine;
               // Adjust caret position accordingly
               caretPos = caretPos + newLine.length - orgLine.length;
            }
            // still needs to handle else which gives indentPlus=indentMinos=true
         }
      }
      e.innerHTML = syntax_msl(e.innerText).join(lc);
      setCurrentCursorPosition(e, caretPos);
      e.focus();
   }
   return;
}


// Trigger syntax highlighting on keydown events
function checkSyntaxEventDown(event) {
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
   e.innerHTML = syntax_msl(e.innerText).join(lc);
   let newText = e.innerText;
   setCurrentCursorPosition(e, caretPos + newText.length - currText.length);
   if (lines[0] !== lines[1]) selectLines(lines);
   event.preventDefault();
   return;
}

// Trigger syntax highlighting when you paste text
function checkSyntaxEventPaste(event) {
   // set time out to allow default to go first
   setTimeout(() => {
      let e = event.target;
      let caretPos = getCurrentCursorPosition(e);
      e.innerHTML = syntax_msl(e.innerText).join(lc);
      setCurrentCursorPosition(e, caretPos);
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
   let sline = document.getElementById("sline" + lineNum);
   return sline;
}

// Return an array with the first and last line numbers of the selected region
/* This assumes that the lines are in a <pre> element and that
   each line has an id="sline#" where # is the line number.
   When the caret in in an empty line, the anchorNode is the <pre> element. 
*/
function getLinesInSelection(e) {
   const selection = window.getSelection();
   //console.trace();
   if (selection.rangeCount === 0) return [0,0];
   // is it a single line?
   const singleLine = selection.isCollapsed;
   if (singleLine) {
      const line = whichLine(e);
      if (line) {
         const startLine = parseInt(line.id.replace("sline",""));
         return [startLine,startLine];
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
function updateLineNumbers(lineCount) {
   const lineNumbers = document.getElementById("lineNumbers");
   // Clear existing line numbers
   lineNumbers.innerHTML = "";
   // Add line numbers to lineNumbers
   for (let i = 1; i <= lineCount; i++) {
      const lineNumber = document.createElement('span');
      lineNumber.id = "lNum" + i.toString();
      lineNumber.textContent = i;
      lineNumbers.appendChild(lineNumber);
   }
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
      keywordGroups = defKeywordGroups;
   }

   // Keep original sequence lines (as array)
   let seqLines_org = Array.isArray(seqLines) ? seqLines : seqLines.split(/\r\n|\r|\n/);
   // Make full text if you get an array of lines
   let mslText = Array.isArray(seqLines) ? seqLines.join("\n") : seqLines;
   // Make some cleanup of illegal characters
   mslText = mslText.replace(/\t/g, "   ");
   
   let nLines = (mslText.match(/\n/g) || []).length;
   // save current revision for undo
   saveState(mslText);
   // These can be done on the text in one go
   // Strings
   let reg = /(["'])(.*?)\1/g;
   mslText = mslText.replace(reg,'<span class="msl_string">$1$2$1</span>');

   // Comments
   //reg = /^(COMMENT|#.*?)(.*)$/gim;
   //mslText = mslText.replace(reg,'<span class="msl_comment">$&</span>');
   
   // Variables
   reg = /(\$[\w]+|^\s*[\w]+(?=\s*=))/gm; // starting with $
   //reg = /^(?!COMMENT|#)(\$[\w]+|^\s*[\w]+(?=\s*=))/gm; // starting with $
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
   if (browserType() === 1) emptyClass = "sline";
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
      if (line === "") {
         //if (j === seqLines_org.length - 1) {
         seqLines[j] = "<span class='" + emptyClass + "' id='sline" + (j+1).toString() + "'></span>";
      } else {
         seqLines[j] = "<span class='sline' id='sline" + (j+1).toString() + "'>" + seqLines[j] + "</span>";
      }
   }
   //seqLines = seqLines.slice(0, nLines + 1);
   updateLineNumbers(seqLines.length);
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
   // Avoid issues of begenning of single line
   if (lines[0] > lines[1] || lines[0] == lines[1]) {
      lines[1] = lines[0];
      singleLine = true;
   }
   for (let j = lines[0]; j <= lines[1] ; j++) {
      let lineId = "sline" + j.toString();
      let prevLineId = "sline" + (j-1).toString();
      let lineEl = document.getElementById(lineId);
      let line = "";
      if (lineEl) line = lineEl.innerText;
      if (addTab === 1) {
         let indentString = " ".repeat(3);
         lineEl.innerText = indentString + line;
      } else if (addTab === -1) {
         lineEl.innerText = line.replace(/^\s{1,3}/, '');
      } else if (singleLine && document.getElementById(prevLineId)) {
         let prevLineEl = document.getElementById(prevLineId);
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

// Prepare the parameters/variables (if present) from the ODB as a table 
function seqParVarTable(odbTreeV,odbTreeC,odbTreeD,odbTreeO,odbTreeVar,varFlag) {
   /* Arguments:
      odbTree... - Objects of ODB values
      varFlag    - (opt) true/false return variable/param table
   */

   let html = `<span class="modb" data-odb-path="/Sequencer/Variables" onchange="seqParVar('parContainer');"></span>`;
   
   // If /Sequencer/Param/Value and /Sequencer/Variables are empty return empty
   if (!odbTreeV && !odbTreeC && !odbTreeVar) {
      // Clear container row
      html = `<td colspan="4">${html}</td>`;
      return html;
   }

   html += "<table id='paramTable' class='mtable partable' style='width:100%; border-spacing:0px; text-align:left; padding:5px;'>";
   html += varFlag ? "<tr><th style='width: 120px'>Variable&nbsp;&nbsp;</th><th style='width: 200px'>Current value&nbsp;&nbsp;</th><th>Comment</th></tr>" : "<tr><th>Parameter&nbsp;&nbsp;</th><th>Initial value&nbsp;&nbsp;</th><th>Comment</th></tr>";

   const processParam = (name, value, isBool, defValue, optValue, comment) => {
      let addString = "";
      let parLine = `<tr><td>${name}</td>`;
      let inParLine = "";
      if (defValue) {
         // set default value in ODB
         addString = `value="${defValue}"`;
         modbset(`/Sequencer/Param/Value/${name}`, defValue);
      }
      if (optValue) {
         if (!defValue) defValue = optValue[0];
         const optionsHtml = optValue.map(option => `<option value="${option}" ${option === defValue ? 'selected' : ''}>${option}</option>`).join('');
         inParLine += `<select onchange="modbset('/Sequencer/Param/Value/${name}', this.value)">${optionsHtml}</select>`;
      } else if (isBool) {
         let initState = defValue ? "checked" : "";
         inParLine += `<input type="checkbox" ${initState} class="modbcheckbox" data-odb-path="/Sequencer/Param/Value/${name}"></input>`;
      } else {
         inParLine += `<input ${addString} onchange="modbset('/Sequencer/Param/Value/${name}', this.value);"></input>`;
      }
      
      parLine += `<td>${inParLine}<span class="modb" data-odb-path="/Sequencer/Param/Value/${name}"></span></td>`;
      parLine += `<td>${comment}</td></tr>`;
      html += parLine;
   };
   
   if (varFlag) {
      // Go over all variables in ODB
      for (let key in odbTreeVar) {
         const match = key.match(/([^/]+)\/key$/);
         if (match) {
            const name = match[1];
            const value = odbTreeVar[name];
            let comment = (odbTreeC && odbTreeC[name]) ? odbTreeC[name] : '';
            html += `<tr><td>${name}</td><td><span class="modbvalue" data-odb-path="/Sequencer/Variables/${name}">${value}</span></td><td>${comment}</td></tr>\n`;
         }
      }
   } else {
      // Go over all parameters in ODB
      for (let key in odbTreeV) {
         const match = key.match(/([^/]+)\/key$/);
         if (match) {
            const name = match[1];
            // if variable is found use its value
            let value = (odbTreeVar && odbTreeVar[name]) ? odbTreeVar[name] : odbTreeV[name];
            let isBool = (odbTreeV[key].type == 8);
            let defValue = (value !== null && value !== undefined && value !== '') ? value : (odbTreeD && odbTreeD[name]) || value;
            let optValue = odbTreeO ? odbTreeO[name] : undefined; 
            let comment = odbTreeC[name] || '';
            let addString = "";
            if (typeof value !== "object") {
               processParam(name, value, isBool, defValue, optValue, comment);
            }
         }
      }
   }

   html += "</table>";
   html = `<td colspan="4">${html}</td>`;
   return html;
}

// Prepare the parameters/variables (if present) from the ODB as a table 
function seqParVar(parContainer,debugFlag) {
   /* Arguments:
      parContainer - (opt) id of element to be filled with html param table
      debugFlag    - (opt) true/false run in debug/normal mode
   */

   let html = "";
   mjsonrpc_db_ls(["/Sequencer/Param/Value","/Sequencer/Param/Comment","/Sequencer/Param/Defaults","/Sequencer/Param/Options","/Sequencer/Variables"]).then(function(rpc) {
      const odbTreeV = rpc.result.data[0]; // value
      const odbTreeC = rpc.result.data[1]; // comment
      const odbTreeD = rpc.result.data[2]; // defaults
      const odbTreeO = rpc.result.data[3]; // options
      const odbTreeVar = rpc.result.data[4]; // Variables
      // dialog is created if parContainer is undefined and variables are filled otherwise
      const dlgTable = !parContainer;
      // If /Sequencer/Param/Value and /Sequencer/Variables are empty start and return immediately
      if (!odbTreeV || !odbTreeC) {
         if (dlgTable) {
            parContainer = "parContainer";
            if (debugFlag) {
               modbset('/Sequencer/Command/Debug script',true);
            } else {
               modbset('/Sequencer/Command/Start script',true);
            }
         }
         html = seqParVarTable(odbTreeV,odbTreeC,odbTreeD,odbTreeO,odbTreeVar,!dlgTable);
         if (document.getElementById(parContainer))
            document.getElementById(parContainer).innerHTML = html; 
         return;
      }

      // For dialog use parameters
      if (dlgTable) {
         // Go over all parameters in ODB
         html = seqParVarTable(odbTreeV,odbTreeC,odbTreeD,odbTreeO,odbTreeVar,!dlgTable);
         // if parContainer not given produce a dialog
         let htmlDlg = `${html}<br><button class="dlgButtonDefault" id="seqParamStart" type="button">Start</button><button class="dlgButton" id="seqParamCancel" type="button">Cancel</button><br>`;
         let d = general_dialog(htmlDlg,"Variables");
         let e = document.getElementById("parContainer");
         // Append the table to a container
         let startBtn = document.getElementById("seqParamStart");
         let cancelBtn = document.getElementById("seqParamCancel");
         
         cancelBtn.addEventListener("click", function () {d.remove();});
         startBtn.addEventListener("click", function () {
            e.innerHTML = html;
            d.remove();
            if (debugFlag) {
               modbset('/Sequencer/Command/Debug script',true);
            } else {
               modbset('/Sequencer/Command/Start script',true);
            }
         });
      } else {
         // Go over all variables in ODB
         html = seqParVarTable(odbTreeV,odbTreeC,odbTreeD,odbTreeO,odbTreeVar,!dlgTable);
         if (document.getElementById(parContainer)) 
            document.getElementById(parContainer).innerHTML = html;
         windowResize();
      }
   }).catch (function (error) {
      console.error(error);
   });
}

function seqParamsDlg(html) {
   let htmlDlg = html + "<br />" +
       "<button class=\"dlgButtonDefault\" id=\"seqParamStart\" type=\"button\">Start</button>" +
       "<button class=\"dlgButton\" id=\"seqParamCancel\" type=\"button\">Cancel</button><br>";

   let d = general_dialog(htmlDlg,"Variables");
   let e = document.getElementById("parContainer");
   // Append the table to a container
   let startBtn = document.getElementById("seqParamStart");
   let cancelBtn = document.getElementById("seqParamCancel");

   cancelBtn.addEventListener("click", function () {
      d.remove();
   });
   startBtn.addEventListener("click", function () {
      e.innerHTML = "<td colspan='4'>" + html + "</td>";
      d.remove();
      if (debugFlag) {
         modbset('/Sequencer/Command/Debug script',true);
      } else {
         modbset('/Sequencer/Command/Start script',true);
      }
   });
}

// helper debug function
function debugSeq(parContainer) {
   seqParVar(parContainer,true);
}

// helper start function
function startSeq(parContainer) {
   seqParVar(parContainer,false);
}

// helper stop function
function stopSeq(flag) {
   modbset('/Sequencer/Command/Stop immediately',flag);
}

// Show or hide parameters table
function showParTable(parContainer) {
   let e = document.getElementById(parContainer);
   // update embedded table to make sure values are synced with ODB
   seqParVar(parContainer);
   let vis = document.getElementById("showParTable").checked;
   if (e.style.display == "none" && vis) {
      e.style.display = "table-row";
   } else {
      e.style.display = "none";
   }
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
// ToDo: the size of the editor should be adjusted to fill the screen
function extraRows(e) {
   /* Arguments:
      e - triggering element to identify wait or loop
   */
   // get current row
   let rIndex = e.parentElement.parentElement.rowIndex;
   let table = e.parentElement.parentElement.parentElement;
   // check if there is a wait or loop commands (if non-zero)
   if (e.value) {
      if (e.id === "waitTrig") {
         // Make sure there is only one wait row
         document.querySelectorAll('.waitTR').forEach(element => element.remove());
         // Insert a new row
         let tr = table.insertRow(rIndex+1);
         tr.className = "waitTR";
         tr.innerHTML = `<td></td><td>Wait:</td>
                  <td style="position: relative;" colspan="2">
                    <span class="modbhbar" style="z-index: 1; position: absolute; top: 0; left: 0; width: calc(100% - 2px); height: 100%; color: lightgreen;" data-odb-path="/Sequencer/State/Wait value" id="mwaitProgress"></span>
                    <span style="z-index: 2; position: absolute; top: 1px; left: 5px; display: inline-block;">
                      [<span class="modbvalue" data-odb-path="/Sequencer/State/Wait value"></span>/<span class="modbvalue" data-odb-path="/Sequencer/State/Wait limit" onchange="if (document.getElementById('mwaitProgress')) document.getElementById('mwaitProgress').dataset.maxValue = this.value;"></span>] <span class="modbvalue" data-odb-path="/Sequencer/State/Wait type"></span>
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
               tr.innerHTML = `<td></td><td>Loop ${i}:</td>
                  <td style="position: relative;" colspan="2">
                    <span class="modbhbar" style="z-index: 1; position: absolute; top: 0; left: 0; width: calc(100% - 2px); height: 100%; color: #CBC3E3;" data-odb-path="/Sequencer/State/Loop counter[${i}]" id="mloopProgress${i}"></span>
                    <span style="z-index: 2; position: absolute; top: 1px; left: 5px; display: inline-block;">
                      [<span class="modbvalue" data-odb-path="/Sequencer/State/Loop counter[${i}]"></span>/<span class="modbvalue" data-odb-path="/Sequencer/State/Loop n[${i}]" onchange="if (document.getElementById('mloopProgress${i}')) document.getElementById('mloopProgress${i}').dataset.maxValue = this.value;"></span>]
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
      windowResize();
   }
}

function extraCell(e) {
   // get current row
   // check if there is a wait or loop commands (if non-zero)
   if (e.value) {
      if (e.id === "waitTrig") {
         // Make sure there is only one wait row
         document.querySelectorAll('.waitTR').forEach(element => element.remove());
         // Insert a new row
         let waitDiv = document.createElement('div');
         waitDiv.style.position = "relative";
         waitDiv.style.width = "calc(100% - 2px)";
         waitDiv.className = "waitTR";
         waitDiv.innerHTML = `<span class="modbhbar" style="display: inline-block;z-index: 1; position: relative; top: 0; left: 0; width: 100%; height: 1.5em; color: lightgreen;" data-odb-path="/Sequencer/State/Wait value" id="mwaitProgress">&nbsp;</span>
                    <span style="z-index: 2; position: absolute; top: 1px; left: 5px; display: inline-block;">
                      Wait:[<span style="display: inline-block;" class="modbvalue" data-odb-path="/Sequencer/State/Wait value"></span>/<span style="display: inline-block;" class="modbvalue" data-odb-path="/Sequencer/State/Wait limit" onchange="if (document.getElementById('mwaitProgress')) document.getElementById('mwaitProgress').dataset.maxValue = this.value;"></span>] <span style="display: inline-block;" class="modbvalue" data-odb-path="/Sequencer/State/Wait type"></span>
                    </span>`;
         e.insertAdjacentElement('beforebegin', waitDiv);
      } else if (e.id === "loopTrig") {
         mjsonrpc_db_get_values(["/Sequencer/State/Loop n"]).then(function(rpc) {
            let loopArray = rpc.result.data[0];
            for (let i = 0; i < loopArray.length; i++) {
               if (loopArray[i] === 0) break;
               let tr = table.insertRow(rIndex+1);
               tr.className = "loopTR";
               tr.innerHTML = `<td></td>
                  <td style="position: relative;" colspan="3">&nbsp;
                    <span class="modbhbar" style="z-index: 1; position: absolute; top: 0; left: 0; width: 100%; height: 100%; color: #CBC3E3;" data-odb-path="/Sequencer/State/Loop counter[${i}]" id="mloopProgress${i}"></span>
                    <span style="z-index: 2; position: absolute; top: 1px; left: 5px; display: inline-block;">
                      Loop ${i}:[<span class="modbvalue" data-odb-path="/Sequencer/State/Loop counter[${i}]"></span>/<span class="modbvalue" data-odb-path="/Sequencer/State/Loop n[${i}]" onchange="if (document.getElementById('mloopProgress${i}')) document.getElementById('mloopProgress${i}').dataset.maxValue = this.value;"></span>]
                    </span>
                  </td>`;
            }
         }).catch (function (error) {
            console.error(error);
         });
      }
   } else {
      // remove rows
      if (e.id === "waitTrig") document.querySelectorAll('.waitTR').forEach(element => element.remove());
      if (e.id === "loopTrig") document.querySelectorAll('.loopTR').forEach(element => element.remove());
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
   let filechanged = document.getElementById("filechanged");
   filechanged.innerHTML = "";
   if (flag) {
      filechanged.innerHTML = "&nbsp;&#9998;"; // 2022 too small
   }
}

// save history of edits, called from syntax_msl()
function saveState(mslText) {
   if (!saveRevision) return;
   previousRevisions.push(mslText)
   // keep only nRevisions revisions
   if (previousRevisions.length > nRevisions) {
      previousRevisions.pop();
   }
   revisionIndex = previousRevisions.length - 1;
}

// undo last edit
function undoEdit(editor) {
   if (revisionIndex < 1) {
      return;
   } else {
      revisionIndex--;
   }
   let caretPos = getCurrentCursorPosition(editor);
   let currText = editor.innerText;
   saveRevision = false;   
   editor.innerHTML = syntax_msl(previousRevisions[revisionIndex]).join(lc);
   saveRevision = true;   
   // calculate change in caret position based on length
   caretPos = caretPos + previousRevisions[revisionIndex].length - currText.length;
   setCurrentCursorPosition(editor, caretPos);
}

// redo the undo
function redoEdit(editor) {
   if (revisionIndex >= previousRevisions.length - 1) {
      return;
   } else {
      revisionIndex++;
   }
   let caretPos = getCurrentCursorPosition(editor);
   let currText = editor.innerText;
   saveRevision = false;   
   editor.innerHTML = syntax_msl(previousRevisions[revisionIndex]).join(lc);
   saveRevision = true;   
   // calculate change in caret position based on length
   caretPos = caretPos + previousRevisions[revisionIndex].length - currText.length;
   setCurrentCursorPosition(editor, caretPos);
}

// Select slines from startLine to endLine
function selectLines([startLine, endLine]) {
   const selection = window.getSelection();
   // Remove existing selections
   selection.removeAllRanges();
   let startElementId = 'sline' + startLine;
   let endElementId = 'sline' + endLine;
   let startElement = null, endElement = null;
   if (document.getElementById(startElementId)) startElement = document.getElementById(startElementId).firstChild;
   if (document.getElementById(endElementId)) endElement = document.getElementById(endElementId).lastChild;
   // we need startElement and endElement with first/lastChild
   // the following prevents loosing selection but not ideal
   while (startElement === null && startLine <= endLine) {
      startLine++;
      startElementId = 'sline' + startLine;
      startElement = document.getElementById(startElementId).firstChild;
   }
   while (endElement === null && endLine > 0) {
      endLine--;
      endElementId = 'sline' + endLine;
      endElement = document.getElementById(endElementId).lastChild;
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
   const msl_area = document.querySelector('.msl_area');
   if (lToDcheck.checked) {
      localStorage.setItem("darkMode", true);
      msl_area.style.backgroundColor = "black";
      msl_area.style.color = "white";
   } else {
      localStorage.removeItem("darkMode");
      msl_area.style.backgroundColor = "white";
      msl_area.style.color = "black";
   }      
}

// Mark the current line number
function markCurrLineNum(editor) {
   const currLines = document.querySelectorAll(".msl_linenum_curr");
   currLines.forEach((line) => line.classList.remove("msl_linenum_curr"));
   const [startLine,endLine] = getLinesInSelection(editor);
   if (startLine === 0 && endLine === 0) return;
   for (let i = startLine; i <= endLine; i++) {
      let lineNumId = "#lNum" + i.toString();
      let lineNum = lineNumbers.querySelector(lineNumId);
      if (lineNum) 
         lineNum.className = "msl_linenum_curr";
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

// Populate a modal with a general html
function general_dialog(html = "", iddiv = "dlgGeneral", width, height, x, y) {
   /* general dialog containing html code, optional parameters
      iddiv        - the name of the dialog div (optional)
      width/height - minimal width/height of dialog (optional)
      x/y          - initial position of dialog (optional)
   */

   // First make sure you removed exisitng iddiv
   if (document.getElementById(iddiv)) document.getElementById(iddiv).remove();
   let d = document.createElement("div");
   d.className = "dlgFrame";
   d.id = iddiv;
   d.style.zIndex = "30";
   d.style.overflow = "hidden";
   d.style.resize = "both";
   d.style.minWidth = width ? width + "px" : "400px";
   d.style.minHeight = height ? height + "px" : "200px";
   //d.style.maxWidth = "50vw";
   //d.style.maxHeight = "50vh";
   d.style.width = width + "px";
   d.style.height = height ? height + "px" : "200px";
   d.shouldDestroy = true;

   const dlgTitle = document.createElement("div");
   dlgTitle.className = "dlgTitlebar";
   dlgTitle.id = "dlgMessageTitle";
   dlgTitle.innerText = iddiv ? iddiv + " dialog" : "General dialog";
   d.appendChild(dlgTitle);

   const dlgPanel = document.createElement("div");
   dlgPanel.className = "dlgPanel";
   dlgPanel.id = "dlgPanel";
   d.appendChild(dlgPanel);

   const content = document.createElement("div");
   content.id = "dlgHTML";
   content.style.overflow = "auto";
   content.innerHTML = html;
   dlgPanel.appendChild(content);
   
   document.body.appendChild(d);
   console.log( content.style.width, d.style.minWidth);
   dlgShow(d);

   if (x !== undefined && y !== undefined)
      dlgMove(d, x, y);

   // Initial size based on content
   d.style.height = (content.offsetHeight + dlgTitle.offsetHeight + 5 ) + "px";
   // adjust size when resizing modal
   const resizeObs = new ResizeObserver(() => {
      content.style.height = (d.offsetHeight - dlgTitle.offsetHeight - 5 ) + "px";
      //d.style.height = (content.offsetHeight + dlgTitle.offsetHeight + 5 ) + "px";
   });
   resizeObs.observe(d);

   return d;
}
