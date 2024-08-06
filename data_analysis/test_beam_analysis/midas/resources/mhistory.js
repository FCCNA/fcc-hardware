//
// Name:         mhistory.js
// Created by:   Stefan Ritt
//
// Contents:     JavaScript history plotting routines
//
// Note: please load midas.js, mhttpd.js and control.js before mhistory.js
//

LN10 = 2.302585094;
LOG2 = 0.301029996;
LOG5 = 0.698970005;

function log_hs_read(str, a, b)
{
   let da = new Date(a*1000);
   let db = new Date(b*1000);
   let s = str + ": " +
      da.toLocaleDateString() +
      " " +
      da.toLocaleTimeString() +
      " ---- " +
      db.toLocaleDateString() +
      " " +
      db.toLocaleTimeString();

   // out-comment following line to log all history requests
   //console.log(s);
}

function profile(flag) {
   if (flag === true || flag === undefined) {
      console.log("");
      profile.startTime = new Date().getTime();
      return;
   }

   let now = new Date().getTime();
   console.log("Profile: " + flag + ": " + (now-profile.startTime) + "ms");
   profile.startTime = new Date().getTime();
}

function mhistory_init(mhist, noBorder, param) {
   // go through all data-name="mhistory" tags if not passed
   let floating;
   if (mhist === undefined) {
      var mhist = document.getElementsByClassName("mjshistory");
   } else if (!Array.isArray(mhist)) {
      mhist = [mhist];
      floating = true;
   }

   let baseURL = window.location.href;
   if (baseURL.indexOf("?cmd") > 0)
      baseURL = baseURL.substring(0, baseURL.indexOf("?cmd"));
   baseURL += "?cmd=history";

   for (let i = 0; i < mhist.length; i++) {
      mhist[i].innerHTML = "";  // Needed to make sure of a fresh start
      mhist[i].dataset.baseURL = baseURL;
      mhist[i].mhg = new MhistoryGraph(mhist[i], noBorder, floating);
      mhist[i].mhg.initializePanel(i, param);
      mhist[i].mhg.resize();
      mhist[i].resize = function () {
         this.mhg.resize();
      };
   }
}

function mhistory_dialog_var(historyVar, param) {
   if (param === undefined)
      mhistory_dialog(undefined, historyVar);
   else
      mhistory_dialog(undefined, historyVar, param.width, param.height, param.x, param.y, param);
}


function mhistory_dialog(group, panel, width, height, x, y, param) {

   // default minimal/initial width and height if not defined
   if (width === undefined)
      width = 500;
   if (height === undefined)
      height = 300;

   let d = document.createElement("div");
   d.className = "dlgFrame";
   d.style.zIndex = "30";
   d.style.backgroundColor = "white";
   // allow resizing modal  
   d.style.overflow = "hidden";
   d.style.resize = "both";
   d.style.minWidth = width + "px";
   d.style.width = width + "px";
   d.style.height = height + "px";
   d.shouldDestroy = true;

   let dlgTitle = document.createElement("div");
   dlgTitle.className = "dlgTitlebar";
   dlgTitle.id = "dlgMessageTitle";
   dlgTitle.innerText = "History " + panel;
   d.appendChild(dlgTitle);
   document.body.appendChild(d);
   dlgShow(d);

   // Now we can adjust for the title bar height and dlgPanel padding
   d.style.height = (height + dlgTitle.offsetHeight + 6) + "px";
   d.style.minHeight = (height + dlgTitle.offsetHeight + 6) + "px";

   let dlgPanel = document.createElement("div");
   dlgPanel.className = "dlgPanel";
   dlgPanel.style.padding = "3px";
   dlgPanel.style.minWidth = width + "px";
   dlgPanel.style.minHeight = height + "px";
   dlgPanel.style.width = "100%";
   dlgPanel.style.padding = "0";
   d.appendChild(dlgPanel);

   let dlgHistory = document.createElement("div");
   dlgHistory.className = "mjshistory";
   if (group !== undefined) {
      dlgHistory.setAttribute("data-group", group);
      dlgHistory.setAttribute("data-panel", panel);
   } else {
      dlgHistory.setAttribute("data-history-var", panel);
   }

   dlgHistory.style.height = "100%";
   dlgPanel.appendChild(dlgHistory);

   if (x !== undefined && y !== undefined)
      dlgMove(d, x, y);

   // initialize history when resizing modal
   const resizeObs = new ResizeObserver(() => {
      dlgPanel.style.height = (100 * (d.offsetHeight - dlgTitle.offsetHeight) / d.offsetHeight) + "%";
      mhistory_init(dlgHistory, true, param);
   });
   resizeObs.observe(d);
   // catch event when history dialog is closed
   const observer = new MutationObserver(([{removedNodes}]) => {
      if (removedNodes.length && removedNodes[0] === d) {
         resizeObs.unobserve(d);
         document.getElementById(dlgHistory.id + "intSel").remove();
         document.getElementById(dlgHistory.id + "downloadSel").remove();
      }
   });
   observer.observe(d.parentNode, { childList: true });

   return d;
}

function mhistory_create(parentElement, baseURL, group, panel, tMin, tMax, index) {
   let d = document.createElement("div");
   parentElement.appendChild(d);
   d.dataset.baseURL = baseURL;
   d.dataset.group = group;
   d.dataset.panel = panel;
   d.mhg = new MhistoryGraph(d, undefined, false);
   if (!Number.isNaN(tMin) && !Number.isNaN(tMax)) {
      d.mhg.initTMin = tMin;
      d.mhg.initTMax = tMax;
   }
   d.mhg.initializePanel(index);
   return d;
}

function mhistory_create_var(parentElement, baseURL, historyVar, tMin, tMax, index) {
   let d = document.createElement("div");
   parentElement.appendChild(d);
   d.dataset.baseURL = baseURL;
   d.dataset.historyVar = historyVar;
   d.mhg = new MhistoryGraph(d, undefined, true);
   if (!Number.isNaN(tMin) && !Number.isNaN(tMax)) {
      d.mhg.initTMin = tMin;
      d.mhg.initTMax = tMax;
   }
   d.mhg.initializePanel(index);
   return d;
}

function getUrlVars() {
   let vars = {};
   window.location.href.replace(/[?&]+([^=&]+)=([^&]*)/gi, function (m, key, value) {
      vars[key] = value;
   });
   return vars;
}

function MhistoryGraph(divElement, noBorder, floating) { // Constructor

   // create canvas inside the div
   this.parentDiv = divElement;
   // if absent, generate random string (5 char) to give an id to mhistory
   if (!this.parentDiv.id)
      this.parentDiv.id = (Math.random() + 1).toString(36).substring(7);
   this.baseURL = divElement.dataset.baseURL;
   this.group = divElement.dataset.group;
   this.panel = divElement.dataset.panel;
   this.historyVar = divElement.dataset.historyVar;
   this.floating = floating;
   this.canvas = document.createElement("canvas");
   if (noBorder !== true)
      this.canvas.style.border = "1px solid black";
   this.canvas.style.height = "100%";
   divElement.appendChild(this.canvas);

   // colors
   this.color = {
      background: "#FFFFFF",
      axis: "#808080",
      grid: "#D0D0D0",
      label: "#404040",
      data: [
         "#00AAFF", "#FF9000", "#FF00A0", "#00C030",
         "#A0C0D0", "#D0A060", "#C04010", "#807060",
         "#F0C000", "#2090A0", "#D040D0", "#90B000",
         "#B0B040", "#B0B0FF", "#FFA0A0", "#A0FFA0"],
   };

   // scales
   this.tScale = 3600;
   this.yMin0 = undefined;
   this.yMax0 = undefined;
   this.tMax = Math.floor(new Date() / 1000);
   this.tMin = this.tMax - this.tScale;
   this.yMin = undefined;
   this.yMax = undefined;
   this.scroll = true;
   this.yZoom = false;
   this.showZoomButtons = true;
   this.showMenuButtons = true;
   this.tMinRequested = 0;
   this.tMinReceived = 0;
   this.tMaxRequested = 0;
   this.tMaxReceived = 0;

   // overwrite scale from URL if present
   let tMin = decodeURI(getUrlVars()["A"]);
   if (tMin !== "undefined") {
      this.initTMin = parseInt(tMin);
      this.tMin = parseInt(tMin);
   }
   let tMax = decodeURI(getUrlVars()["B"]);
   if (tMax !== "undefined") {
      this.initTMax = parseInt(tMax);
      this.tMax = parseInt(tMax);
   }

   // data arrays
   this.data = [];
   this.lastWritten = [];
   this.binned = false;
   this.binSize = 0;

   // graph arrays (in screen pixels)
   this.x = [];
   this.y = [];
   // t/v arrays corresponding to x/y
   this.t = [];
   this.v = [];
   this.vRaw = [];

   // points array with min/max/avg
   this.p = [];

   // dragging
   this.drag = {
      active: false,
      lastT: 0,
      lastOffsetX: 0,
      lastDt: 0,
      lastVt: 0,
      lastMoveT : 0
   };

   // axis zoom
   this.zoom = {
      x: {active: false},
      y: {active: false}
   };

   // callbacks when certain actions are performed.
   // All callback functions should accept a single parameter, which is the 
   // MhistoryGraph object that triggered the callback.
   this.callbacks = {
      resetAxes: undefined,
      timeZoom: undefined,
      jumpToCurrent: undefined
   };

   // marker
   this.marker = {active: false};
   this.variablesWidth = 0;
   this.variablesHeight = 0;

   // labels
   this.showLabels = false;

   // solo
   this.solo = {active: false, index: undefined};

   // time when panel was drawn last
   this.lastDrawTime = 0;
   this.forceRedraw = false;

   // buttons
   this.button = [
      {
         src: "menu.svg",
         title: "Show / hide legend",
         click: function (t) {
            t.showLabels = !t.showLabels;
            t.redraw(true);
         }
      },
      {
         src: "maximize-2.svg",
         title: "Show only this plot",
         click: function (t) {
            window.location.href = t.baseURL + "&group=" + t.group + "&panel=" + t.panel;
         }
      },
      {
         src: "rotate-ccw.svg",
         title: "Reset histogram axes",
         click: function (t) {
            t.resetAxes();

            if (t.callbacks.resetAxes !== undefined) {
               t.callbacks.resetAxes(t);
            }
         }
      },
      {
         src: "play.svg",
         title: "Jump to current time",
         click: function (t) {

            let dt = Math.floor(t.tMax - t.tMin);

            // limit to one week maximum (otherwise we have to read binned data)
            if (dt > 24*3600*7)
               dt = 24*3600*7;

            t.tMax = new Date() / 1000;
            t.tMin = t.tMax - dt;
            t.scroll = true;

            t.loadFullData(t.tMin, t.tMax, true);

            if (t.callbacks.jumpToCurrent !== undefined) {
               t.callbacks.jumpToCurrent(t);
            }
         }
      },
      {
         src: "clock.svg",
         title: "Select timespan...",
         click: function (t) {
            if (t.intSelector.style.display === "none") {
               t.intSelector.style.display = "block";
               t.intSelector.style.left = ((t.canvas.getBoundingClientRect().x + window.pageXOffset +
                  t.x2) - t.intSelector.offsetWidth) + "px";
               t.intSelector.style.top = (t.canvas.getBoundingClientRect().y + window.pageYOffset +
                  this.y1 - 1) + "px";
               t.intSelector.style.zIndex = "32";
            } else {
               t.intSelector.style.display = "none";
            }
         }
      },
      {
         src: "download.svg",
         title: "Download image/data...",
         click: function (t) {
            if (t.downloadSelector.style.display === "none") {
               t.downloadSelector.style.display = "block";
               t.downloadSelector.style.left = ((t.canvas.getBoundingClientRect().x + window.pageXOffset +
                  t.x2) - t.downloadSelector.offsetWidth) + "px";
               t.downloadSelector.style.top = (t.canvas.getBoundingClientRect().y + window.pageYOffset +
                  this.y1 - 1) + "px";
               t.downloadSelector.style.zIndex = "32";
            } else {
               t.downloadSelector.style.display = "none";
            }
         }
      },
      {
         src: "settings.svg",
         title: "Configure this plot",
         click: function (t) {
            window.location.href = "?cmd=hs_edit&group=" + encodeURIComponent(t.group) + "&panel=" + encodeURIComponent(t.panel) + "&redir=" + encodeURIComponent(window.location.href);
         }
      },
      {
         src: "help-circle.svg",
         title: "Show help",
         click: function () {
            dlgShow("dlgHelp", false);
         }
      },
      {
         src: "corner-down-left.svg",
         title: "Return to all variables",
         click: function (t) {
            t.solo.active = false;
            t.findMinMax();
            t.redraw();
         }
      }
   ];

   // remove settings for single variable plot
   if (this.group === undefined) {
      this.button.splice(1, 1);
      this.button.splice(5, 1);
   }

   // load dialogs
   dlgLoad('dlgHistory.html');

   this.button.forEach(b => {
      b.img = new Image();
      b.img.src = "icons/" + b.src;
   });

   // marker
   this.marker = {active: false};

   // mouse event handlers
   divElement.addEventListener("mousedown", this.mouseEvent.bind(this), true);
   divElement.addEventListener("dblclick", this.mouseEvent.bind(this), true);
   divElement.addEventListener("mousemove", this.mouseEvent.bind(this), true);
   divElement.addEventListener("mouseup", this.mouseEvent.bind(this), true);
   divElement.addEventListener("wheel", this.mouseWheelEvent.bind(this), true);

   // Keyboard event handler (has to be on the window!)
   window.addEventListener("keydown", this.keyDown.bind(this));
}

function timeToSec(str) {
   let s = parseFloat(str);
   switch (str[str.length - 1]) {
      case 'm':
      case 'M':
         s *= 60;
         break;
      case 'h':
      case 'H':
         s *= 3600;
         break;
      case 'd':
      case 'D':
         s *= 3600 * 24;
         break;
   }

   return s;
}

function doQueryAB(t) {

   dlgHide('dlgQueryAB');

   let d1 = new Date(
      document.getElementById('y1').value,
      document.getElementById('m1').selectedIndex,
      document.getElementById('d1').selectedIndex + 1,
      document.getElementById('h1').selectedIndex);

   let d2 = new Date(
      document.getElementById('y2').value,
      document.getElementById('m2').selectedIndex,
      document.getElementById('d2').selectedIndex + 1,
      document.getElementById('h2').selectedIndex);

   if (d1 > d2)
      [d1, d2] = [d2, d1];

   t.tMin = Math.floor(d1.getTime() / 1000);
   t.tMax = Math.floor(d2.getTime() / 1000);
   t.scroll = false;

   t.loadFullData(t.tMin, t.tMax);

   if (t.callbacks.timeZoom !== undefined)
      t.callbacks.timeZoom(t);
}

MhistoryGraph.prototype.keyDown = function (e) {
   if (e.target && e.target.contentEditable === true) return;
   if (e.key === "u") {  // 'u' key
      // force next update t.Min-dt/2 to current time
      let dt = Math.floor(this.tMax - this.tMin);

      // limit to one week maximum (otherwise we have to read binned data)
      if (dt > 24*3600*7)
         dt = 24*3600*7;

      this.tMax = new Date() / 1000;
      this.tMin = this.tMax - dt;
      this.scroll = true;

      this.loadFullData(this.tMin, this.tMax, true);

      if (this.callbacks.jumpToCurrent !== undefined)
          this.callbacks.jumpToCurrent(this);

      e.preventDefault();
   }
   if (e.key === "r") {  // 'r' key
      this.resetAxes();
      e.preventDefault();
   }
   if (e.key === "Escape") {
      this.solo.active = false;
      this.findMinMax();
      this.redraw(true);
      e.preventDefault();
   }
   if (e.key === "y") {
      this.yZoom = false;
      this.findMinMax();
      this.redraw(true);
      e.preventDefault();
   }
}

MhistoryGraph.prototype.initializePanel = function (index, param) {

   // initialize variables
   this.plotIndex = index;
   this.marker = {active: false};
   this.drag = {active: false};
   this.data = undefined;
   this.x = [];
   this.y = [];
   this.events = [];
   this.tags = [];
   this.index = [];
   this.pendingUpdates = 0;

   // single variable mode
   if (this.parentDiv.dataset.historyVar !== undefined) {
      this.param = {
         "Timescale": "1h",
         "Minimum": 0,
         "Maximum": 0,
         "Zero ylow": false,
         "Log axis": false,
         "Show run markers": false,
         "Show values": true,
         "Show fill": true,
         "Variables": this.parentDiv.dataset.historyVar.split(','),
         "Formula": "",
         "Show raw value": false
      };

      this.param.Label = new Array(this.param.Variables.length).fill('');
      this.param.Colour = this.color.data.slice(0, this.param.Variables.length);

      // optionally overwrite default parameters from argument
      if (param !== undefined)
         Object.keys(param).forEach(p => {
            this.param[p] = param[p];
         });

      this.loadInitialData();

   } else {

      // ODB history panel mode
      this.group = this.parentDiv.dataset.group;
      this.panel = this.parentDiv.dataset.panel;

      if (this.group === undefined) {
         dlgMessage("Error", "Definition of \'dataset-group\' missing for history panel \'" + this.parentDiv.id + "\'. " +
            "Please use syntax:<br /><br /><b>&lt;div class=\"mjshistory\" " +
            "data-group=\"&lt;Group&gt;\" data-panel=\"&lt;Panel&gt;\"&gt;&lt;/div&gt;</b>", true);
         return;
      }
      if (this.panel === undefined) {
         dlgMessage("Error", "Definition of \'dataset-panel\' missing for history panel \'" + this.parentDiv.id + "\'. " +
            "Please use syntax:<br /><br /><b>&lt;div class=\"mjshistory\" " +
            "data-group=\"&lt;Group&gt;\" data-panel=\"&lt;Panel&gt;\"&gt;&lt;/div&gt;</b>", true);
         return;
      }

      if (this.group === "" || this.panel === "")
         return;

      // retrieve panel definition from ODB
      mjsonrpc_db_copy(["/History/Display/" + this.group + "/" + this.panel]).then(function (rpc) {
         if (rpc.result.status[0] !== 1) {
            dlgMessage("Error", "Panel \'" + this.group + "/" + this.panel + "\' not found in ODB", true)
         } else {
            let odb = rpc.result.data[0];
            this.param = {};
            this.param["Timescale"] = odb["Timescale"];
            this.param["Minimum"] = odb["Minimum"];
            this.param["Maximum"] = odb["Maximum"];
            this.param["Zero ylow"] = odb["Zero ylow"];
            this.param["Log axis"] = odb["Log axis"];
            this.param["Show run markers"] = odb["Show run markers"];
            this.param["Show values"] = odb["Show values"];
            this.param["Show fill"] = odb["Show fill"];
            this.param["Variables"] = odb["Variables"];
            this.param["Formula"] = odb["Formula"];
            this.param["Colour"] = odb["Colour"];
            this.param["Label"] = odb["Label"];
            this.param["Show raw value"] = odb["Show raw value"];

            this.loadInitialData();
         }
      }.bind(this)).catch(function (error) {
         if (error.xhr !== undefined)
            mjsonrpc_error_alert(error);
         else
            throw (error);
      });
   }
};

MhistoryGraph.prototype.updateLastWritten = function () {
   //console.log("update last_written!!!\n");

   // load date of latest data points
   mjsonrpc_call("hs_get_last_written",
      {
         "time": this.tMin,
         "events": this.events,
         "tags": this.tags,
         "index": this.index
      }).then(function (rpc) {
      this.lastWritten = rpc.result.last_written;
      // protect against an infinite loop from draw() if rpc returns invalid times.
      // by definition, last_written returned by RPC is supposed to be less then tMin.
      for (let i = 0; i < this.lastWritten.length; i++) {
         let l = this.lastWritten[i];
         //console.log("updated last_written: event: " + this.events[i] + ", l: " + l + ", tmin: " + this.tMin + ", diff: " + (l - this.tMin));
         if (l > this.tMin) {
            this.lastWritten[i] = this.tMin;
         }
      }
      this.redraw(true);
   }.bind(this))
      .catch(function (error) {
         mjsonrpc_error_alert(error);
      });
}

MhistoryGraph.prototype.loadInitialData = function () {

   if (this.initTMin !== undefined && this.initTMin !== "undefined") {
      this.tMin = this.initTMin;
      this.tMax = this.initTMax;
      this.tScale = this.tMax - this.tMin;
      this.scroll = false;
   } else {
      this.tScale = timeToSec(this.param["Timescale"]);

      // overwrite via <div ... data-scale=<value> >
      if (this.parentDiv.dataset.scale !== undefined)
         this.tScale = timeToSec(this.parentDiv.dataset.scale);

      this.tMax = Math.floor(new Date() / 1000);
      this.tMin = this.tMax - this.tScale;
   }

   this.showLabels = this.param["Show values"];
   this.showFill = this.param["Show fill"];

   this.autoscaleMin = (this.param["Minimum"] === this.param["Maximum"] ||
      this.param["Minimum"] === "-Infinity" || this.param["Minimum"] === "Infinity");
   this.autoscaleMax = (this.param["Minimum"] === this.param["Maximum"] ||
      this.param["Maximum"] === "-Infinity" || this.param["Maximum"] === "Infinity");

   if (this.param["Zero ylow"]) {
      this.autoscaleMin = false;
      this.param["Minimum"] = 0;
   }

   this.logAxis = this.param["Log axis"];

   // protect against empty history plot
   if (!this.param.Variables) {
      this.param.Variables = "(empty):(empty)";
      this.param.Label = "(empty)";
      this.param.Colour = "";
   }

   // if only one variable present, convert it to array[0]
   if (!Array.isArray(this.param.Variables))
      this.param.Variables = new Array(this.param.Variables);
   if (!Array.isArray(this.param.Label))
      this.param.Label = new Array(this.param.Label);
   if (!Array.isArray(this.param.Colour))
      this.param.Colour = new Array(this.param.Colour);

   this.param["Variables"].forEach(v => {
      let event_and_tag = splitEventAndTagName(v);
      this.events.push(event_and_tag[0]);
      let t = event_and_tag[1];
      if (t.indexOf('[') !== -1) {
         this.tags.push(t.substr(0, t.indexOf('[')));
         this.index.push(parseInt(t.substr(t.indexOf('[') + 1)));
      } else {
         this.tags.push(t);
         this.index.push(0);
      }
   });

   if (this.param["Show run markers"]) {
      this.events.push("Run transitions");
      this.events.push("Run transitions");

      this.tags.push("State");
      this.tags.push("Run number");
      this.index.push(0);
      this.index.push(0);
   }

   // interval selector
   let intSelId = this.parentDiv.id + "intSel";
   if (document.getElementById(intSelId)) document.getElementById(intSelId).remove();
   this.intSelector = document.createElement("div");
   this.intSelector.id = intSelId;
   this.intSelector.style.display = "none";
   this.intSelector.style.position = "absolute";
   this.intSelector.className = "mtable";
   this.intSelector.style.borderRadius = "0";
   this.intSelector.style.border = "2px solid #808080";
   this.intSelector.style.margin = "0";
   this.intSelector.style.padding = "0";
   this.intSelector.style.left = "100px";
   this.intSelector.style.top = "100px";

   let table = document.createElement("table");
   let row = null;
   let cell;
   let link;
   let buttons = this.param["Buttons"];
   if (buttons === undefined) {
      buttons = [];
      buttons.push("10m", "1h", "3h", "12h", "24h", "3d", "7d");
   }
   buttons.push("A&rarr;B");
   buttons.push("&lt;&lt;&lt;");
   buttons.push("&lt;&lt;");
   buttons.forEach(function (b, i) {
      if (i % 2 === 0)
         row = document.createElement("tr");

      cell = document.createElement("td");
      cell.style.padding = "0";

      link = document.createElement("a");
      link.href = "#";
      link.innerHTML = b;
      if (b === "A&rarr;B")
         link.title = "Display data between two dates";
      else if (b === "&lt;&lt;")
         link.title = "Go back in time to last available data";
      else if (b === "&lt;&lt;&lt;")
         link.title = "Go back in time to last available data for all variables on plot";
      else
         link.title = "Show last " + b;

      let mhg = this;
      link.onclick = function () {
         if (b === "A&rarr;B") {
            let currentYear = new Date().getFullYear();
            let dMin = new Date(this.tMin * 1000);
            let dMax = new Date(this.tMax * 1000);

            if (document.getElementById('y1').length === 0) {
               for (let i = currentYear; i > currentYear - 5; i--) {
                  let o = document.createElement('option');
                  o.value = i.toString();
                  o.appendChild(document.createTextNode(i.toString()));
                  document.getElementById('y1').appendChild(o);
                  o = document.createElement('option');
                  o.value = i.toString();
                  o.appendChild(document.createTextNode(i.toString()));
                  document.getElementById('y2').appendChild(o);
               }
            }

            document.getElementById('m1').selectedIndex = dMin.getMonth();
            document.getElementById('d1').selectedIndex = dMin.getDate() - 1;
            document.getElementById('h1').selectedIndex = dMin.getHours();
            document.getElementById('y1').selectedIndex = currentYear - dMin.getFullYear();

            document.getElementById('m2').selectedIndex = dMax.getMonth();
            document.getElementById('d2').selectedIndex = dMax.getDate() - 1;
            document.getElementById('h2').selectedIndex = dMax.getHours();
            document.getElementById('y2').selectedIndex = currentYear - dMax.getFullYear();

            document.getElementById('dlgQueryQuery').onclick = function () {
               doQueryAB(this);
            }.bind(this);

            dlgShow("dlgQueryAB");

         } else if (b === "&lt;&lt;") {

            mjsonrpc_call("hs_get_last_written",
               {
                  "time": this.tMin,
                  "events": this.events,
                  "tags": this.tags,
                  "index": this.index
               })
               .then(function (rpc) {

                  let last = rpc.result.last_written[0];
                  for (let i = 0; i < rpc.result.last_written.length; i++) {
                     if (this.events[i] === "Run transitions") {
                        continue;
                     }
                     let l = rpc.result.last_written[i];
                     last = Math.max(last, l);
                  }

                  if (last !== 0) { // no data, at all!
                     let scale = mhg.tMax - mhg.tMin;
                     mhg.tMax = last + scale / 2;
                     mhg.tMin = last - scale / 2;

                     mhg.scroll = false;
                     mhg.marker.active = false;

                     mhg.loadFullData(mhg.tMin, mhg.tMax);

                     if (mhg.callbacks.timeZoom !== undefined)
                        mhg.callbacks.timeZoom(mhg);
                  }

               }.bind(this))
               .catch(function (error) {
                  mjsonrpc_error_alert(error);
               });

         } else if (b === "&lt;&lt;&lt;") {

            mjsonrpc_call("hs_get_last_written",
               {
                  "time": this.tMin,
                  "events": this.events,
                  "tags": this.tags,
                  "index": this.index
               })
               .then(function (rpc) {

                  let last = 0;
                  for (let i = 0; i < rpc.result.last_written.length; i++) {
                     let l = rpc.result.last_written[i];
                     if (this.events[i] === "Run transitions") {
                        continue;
                     }
                     if (last === 0) {
                        // no data for first variable
                        last = l;
                     } else if (l === 0) {
                        // no data for this variable
                     } else {
                        last = Math.min(last, l);
                     }
                  }
                  //console.log("last: " + last);

                  if (last !== 0) { // no data, at all!
                     let scale = mhg.tMax - mhg.tMin;
                     mhg.tMax = last + scale / 2;
                     mhg.tMin = last - scale / 2;

                     mhg.scroll = false;
                     mhg.marker.active = false;

                     mhg.loadFullData(mhg.tMin, mhg.tMax);

                     if (mhg.callbacks.timeZoom !== undefined)
                        mhg.callbacks.timeZoom(mhg);
                  }

               }.bind(this))
               .catch(function (error) {
                  mjsonrpc_error_alert(error);
               });

         } else {

            mhg.tMax = new Date() / 1000;
            mhg.tMin = mhg.tMax - timeToSec(b);
            mhg.scroll = true;
            mhg.loadFullData(mhg.tMin, mhg.tMax, true);

            if (mhg.callbacks.timeZoom !== undefined)
               mhg.callbacks.timeZoom(mhg);
         }
         mhg.intSelector.style.display = "none";
         return false;
      }.bind(this);

      cell.appendChild(link);
      row.appendChild(cell);
      if (i % 2 === 1)
         table.appendChild(row);
   }, this);

   if (buttons.length % 2 === 1)
      table.appendChild(row);

   this.intSelector.appendChild(table);
   document.body.appendChild(this.intSelector);

   // download selector
   let downloadSelId = this.parentDiv.id + "downloadSel";
   if (document.getElementById(downloadSelId)) document.getElementById(downloadSelId).remove();
   this.downloadSelector = document.createElement("div");
   this.downloadSelector.id = downloadSelId;
   this.downloadSelector.style.display = "none";
   this.downloadSelector.style.position = "absolute";
   this.downloadSelector.className = "mtable";
   this.downloadSelector.style.borderRadius = "0";
   this.downloadSelector.style.border = "2px solid #808080";
   this.downloadSelector.style.margin = "0";
   this.downloadSelector.style.padding = "0";

   this.downloadSelector.style.left = "100px";
   this.downloadSelector.style.top = "100px";

   table = document.createElement("table");
   let mhg = this;

   row = document.createElement("tr");
   cell = document.createElement("td");
   cell.style.padding = "0";
   link = document.createElement("a");
   link.href = "#";
   link.innerHTML = "CSV";
   link.title = "Download data in Comma Separated Value format";
   link.onclick = function () {
      mhg.downloadSelector.style.display = "none";
      mhg.download("CSV");
      return false;
   }.bind(this);
   cell.appendChild(link);
   row.appendChild(cell);
   table.appendChild(row);

   row = document.createElement("tr");
   cell = document.createElement("td");
   cell.style.padding = "0";
   link = document.createElement("a");
   link.href = "#";
   link.innerHTML = "PNG";
   link.title = "Download image in PNG format";
   link.onclick = function () {
      mhg.downloadSelector.style.display = "none";
      mhg.download("PNG");
      return false;
   }.bind(this);
   cell.appendChild(link);
   row.appendChild(cell);
   table.appendChild(row);

   this.downloadSelector.appendChild(table);
   document.body.appendChild(this.downloadSelector);

   // load one window ahead in past and future
   this.loadFullData(this.tMin - this.tScale/2, this.tMax + this.tScale/2, this.scroll);
}

MhistoryGraph.prototype.loadFullData = function (t1, t2, scrollFlag) {

   // retrieve binned data if we request more than one week
   this.binned = this.tMax - this.tMin > 3600*24*7;

   // don't update in binned mode
   if (this.binned)
      this.scroll = false;

   // drop current data
   this.discardCurrentData();

   // prevent future date
   let now = Math.floor(new Date() / 1000);
   if (t2 > now)
      t2 = now;
   if (t1 >= t2)
      t1 = t2 - 600;

   this.tMaxRequested = t2;
   this.tMinRequested = t1;

   if (this.binned) {

      log_hs_read("loadFullData binned", t1, t2);
      this.parentDiv.style.cursor = "progress";
      this.pendingUpdates++;
      mjsonrpc_call("hs_read_binned_arraybuffer",
         {
            "start_time": t1,
            "end_time": t2,
            "num_bins": 5000,
            "events": this.events,
            "tags": this.tags,
            "index": this.index
         }, "arraybuffer")
         .then(function (rpc) {

            this.tMinReceived = this.tMinRequested;
            this.tMaxReceived = this.tMaxRequested;
            this.pendingUpdates--;

            this.receiveDataBinned(rpc);
            this.findMinMax();
            this.redraw(true);

            this.parentDiv.style.cursor = "default";

         }.bind(this))
         .catch(function (error) {
            mjsonrpc_error_alert(error);
         });

   } else {
      
      // limit one request to maximum one month
      if (t2 - t1 > 3600 * 24 * 30) {
         t1 = (t1 + t2)/2 - 3600 * 24 * 15;
         t2 = (t1 + t2)/2 + 3600 * 24 * 15;
         let now = Math.floor(new Date() / 1000);
         if (t2 > now)
            t2 = now;
      }

      log_hs_read("loadFullData un-binned", t1, t2);
      this.parentDiv.style.cursor = "progress";
      this.pendingUpdates++;
      mjsonrpc_call("hs_read_arraybuffer",
         {
            "start_time": Math.floor(this.tMinRequested),
            "end_time": Math.floor(this.tMaxRequested),
            "events": this.events,
            "tags": this.tags,
            "index": this.index
         }, "arraybuffer")
         .then(function (rpc) {

            this.tMinReceived = this.tMinRequested;
            this.tMaxReceived = this.tMaxRequested;
            this.pendingUpdates--;

            this.receiveData(rpc);
            this.findMinMax();
            this.redraw(true);

            if (scrollFlag)
               this.loadNewData(); // triggers scrolling

            this.parentDiv.style.cursor = "default";

         }.bind(this))
         .catch(function (error) {
            mjsonrpc_error_alert(error);
         });
   }
}

MhistoryGraph.prototype.loadSideData = function () {

   let dt = this.tMaxReceived - this.tMinReceived;
   let t1, t2;

   // check for left side data
   if (this.tMin < this.tMinRequested) {

      t1 = this.tMin - dt; // request one window
      t2 = this.tMinReceived;
      this.tMinRequested = t1;

      if (this.binned) {
         log_hs_read("loadSideData left binned", t1, t2);
         this.parentDiv.style.cursor = "progress";
         this.pendingUpdates++;
         mjsonrpc_call("hs_read_binned_arraybuffer",
            {
               "start_time": t1,
               "end_time": t2,
               "num_bins": 5000,
               "events": this.events,
               "tags": this.tags,
               "index": this.index
            }, "arraybuffer")
            .then(function (rpc) {

               this.tMinReceived = this.tMinRequested;
               this.pendingUpdates--;

               this.receiveDataBinned(rpc);
               this.findMinMax();
               this.redraw(true);

               this.parentDiv.style.cursor = "default";

            }.bind(this))
            .catch(function (error) {
               mjsonrpc_error_alert(error);
            });

      } else { // un-binned

         log_hs_read("loadSideData left un-binned", t1, t2);
         this.pendingUpdates++;
         mjsonrpc_call("hs_read_arraybuffer",
            {
               "start_time": t1,
               "end_time": t2,
               "events": this.events,
               "tags": this.tags,
               "index": this.index
            }, "arraybuffer")
            .then(function (rpc) {

               this.tMinReceived = this.tMinRequested;
               this.pendingUpdates--;

               this.receiveData(rpc);
               this.findMinMax();
               this.redraw(true);

               this.parentDiv.style.cursor = "default";

            }.bind(this))
            .catch(function (error) {
               mjsonrpc_error_alert(error);
            });
      }
   }

   // check for right side data
   if (this.tMax > this.tMaxRequested) {

      t1 = this.tMaxReceived;
      t2 = this.tMax + dt; // request one window
      this.tMaxRequested = t2;

      if (this.binned) {
         log_hs_read("loadSideData right binned", t1, t2);
         this.parentDiv.style.cursor = "progress";
         this.pendingUpdates++;
         mjsonrpc_call("hs_read_binned_arraybuffer",
            {
               "start_time": t1,
               "end_time": t2,
               "num_bins": 5000,
               "events": this.events,
               "tags": this.tags,
               "index": this.index
            }, "arraybuffer")
            .then(function (rpc) {

               this.tMaxReceived = this.tMaxRequested;
               this.pendingUpdates--;

               this.receiveDataBinned(rpc);
               this.findMinMax();
               this.redraw(true);

               this.parentDiv.style.cursor = "default";

            }.bind(this))
            .catch(function (error) {
               mjsonrpc_error_alert(error);
            });

      } else { // un-binned

         this.parentDiv.style.cursor = "progress";
         log_hs_read("loadSideData right un-binned", t1, t2);
         this.pendingUpdates++;
         mjsonrpc_call("hs_read_arraybuffer",
            {
               "start_time": t1,
               "end_time": t2,
               "events": this.events,
               "tags": this.tags,
               "index": this.index
            }, "arraybuffer")
            .then(function (rpc) {

               this.tMaxReceived = this.tMaxRequested;
               this.pendingUpdates--;

               this.receiveData(rpc);
               this.findMinMax();
               this.redraw(true);

               this.parentDiv.style.cursor = "default";

            }.bind(this))
            .catch(function (error) {
               mjsonrpc_error_alert(error);
            });
      }
   }
};

MhistoryGraph.prototype.receiveData = function (rpc) {

   // decode binary array
   let array = new Float64Array(rpc);
   let nVars = array[1];
   let nData = array.slice(2 + nVars, 2 + 2 * nVars);
   let i = 2 + 2 * nVars;

   if (i >= array.length) {
      // RPC did not return any data

      if (this.data === undefined) {
         // must initialize the arrays otherwise nothing works
         this.data = [];
         for (let index = 0; index < nVars; index++) {
            this.data.push({time:[], value:[], rawValue:[], bin:[], rawBin:[]});
         }
      }

      return false;
   }

   // bin size 1 for un-binned data
   this.binSize = 1;

   // push empty arrays on the first time
   if (this.data === undefined) {
      this.data = [];
      for (let index = 0; index < nVars; index++) {
         this.data.push({time:[], value:[], rawValue:[], bin:[], rawBin:[]});
      }
   }

   // append new values to end of arrays
   for (let index = 0; index < nVars; index++) {
      if (nData[index] === 0)
         continue;

      let formula = this.param["Formula"];
      if (Array.isArray(formula))
         formula = formula[index];

      let t1 = [];
      let v1 = [];
      let v1Raw = [];
      let x, v, t;
      if (formula !== undefined && formula !== "") {
         for (let j = 0; j < nData[index]; j++) {
            t = array[i++];
            x = array[i++];
            v = eval(formula);
            t1.push(t);
            v1.push(v);
            v1Raw.push(x);
         }
      } else {
         for (let j = 0; j < nData[index]; j++) {
            t = array[i++];
            v = array[i++];
            t1.push(t);
            v1.push(v);
         }
      }

      if (t1.length > 0) {

         let da = new Date(t1[0]*1000);
         let db = new Date(t1[t1.length-1]*1000);

         if (index === 0)
            log_hs_read("receiveData  un-binned", t1[0], t1[t1.length-1]);

         let told = this.data[index].time;

         if (this.data[index].time.length === 0 ||
            t1[0] >= this.data[index].time[this.data[index].time.length-1]) {

            // remove double event
            while (t1[0] === this.data[index].time[this.data[index].time.length-1]) {
               t1 = t1.slice(1);
               v1 = v1.slice(1);
               if (v1Raw.length > 0)
                  v1Raw = v1Raw.slice(1);
            }

            // add data to the right
            this.data[index].time = this.data[index].time.concat(t1);
            this.data[index].value = this.data[index].value.concat(v1);
            if (v1Raw.length > 0)
               this.data[index].rawValue = this.data[index].rawValue.concat(v1Raw);

         } else if (t1[t1.length-1] < this.data[index].time[0]) {

            // add data to the left
            this.data[index].time = t1.concat(this.data[index].time);
            this.data[index].value = v1.concat(this.data[index].value);
            if (v1Raw.length > 0)
               this.data[index].rawValue = v1Raw.concat(this.data[index].rawValue);
         }

          if (index === 0) {
              for (let i = 1; i < this.data[index].time.length; i++)
                  if (this.data[index].time[i] < this.data[index].time[i - 1]) {
                      console.log("Error non-continuous data");
                      log_hs_read("told", told[0], told[told.length-1]);
                      log_hs_read("t1", t1[0], t1[t1.length-1]);
                  }
          }
      }
   }

   return true;
}

MhistoryGraph.prototype.receiveDataBinned = function (rpc) {

   // decode binary array
   let array = new Float64Array(rpc);

   // let status = array[0];
   // let startTime = array[1];
   // let endTime = array[2];
   let numBins = array[3];
   let nVars = array[4];

   let i = 5;
   // let hsStatus = array.slice(i, i + nVars);
   i += nVars;
   let numEntries = array.slice(i, i + nVars);
   i += nVars;
   // let lastTime = array.slice(i, i + nVars);
   i += nVars;
   // let lastValue = array.slice(i, i + nVars);
   i += nVars;

   if (i >= array.length) {
      // RPC did not return any data

      if (this.data === undefined) {
         // must initialize the arrays otherwise nothing works
         this.data = [];
         for (let index = 0; index < nVars; index++) {
            this.data.push({time:[], value:[], rawValue:[], bin:[], rawBin:[]});
         }
      }

      return false;
   }

   // push empty arrays on the first time
   if (this.data === undefined) {
      this.data = [];
      for (let index = 0; index < nVars; index++) {
         this.data.push({time:[], value:[], rawValue:[], bin:[], rawBin:[]});
      }
   }

   let binSize = 0;
   let binSizeN = 0;

   // create arrays of new values
   for (let index = 0; index < nVars; index++) {
      if (numEntries[index] === 0)
         continue;

      let t1 = [];
      let bin1 = [];
      let binRaw1 = [];

      // add data to the right
      let formula = this.param["Formula"];
      if (Array.isArray(formula))
         formula = formula[index];

      if (formula === undefined || formula === "") {
         for (let j = 0; j < numBins; j++) {

            let count = array[i++];
            // let mean = array[i++];
            // let rms = array[i++];
            i += 2;
            let minValue = array[i++];
            let maxValue = array[i++];
            let firstTime = array[i++];
            let firstValue = array[i++];
            let lastTime = array[i++];
            let lastValue = array[i++];
            let t = Math.floor((firstTime + lastTime) / 2);

            if (count > 0) {
               // append to the right
               t1.push(t);

               let bin = {};
               bin.count = count;
               bin.firstValue = firstValue;
               bin.lastValue = lastValue;
               bin.minValue = minValue;
               bin.maxValue = maxValue;

               bin1.push(bin);

               // calculate average bin count
               binSize += count;
               binSizeN++;
            }
         }

      } else { // use formula

         for (let j = 0; j < numBins; j++) {

            let count = array[i++];
            // let mean = array[i++];
            // let rms = array[i++];
            i += 2;
            let minValue = array[i++];
            let maxValue = array[i++];
            let firstTime = array[i++];
            let firstValue = array[i++];
            let lastTime = array[i++];
            let lastValue = array[i++];

            if (count > 0) {
               // append to the right
               t1.push(Math.floor((firstTime + lastTime) / 2));

               let bin = {};
               let binRaw = {};
               let x = firstValue;
               binRaw.firstValue = firstValue;
               bin.firstValue = eval(formula);
               x = lastValue;
               binRaw.lastValue = lastValue;
               bin.lastValue = eval(formula);
               x = minValue;
               binRaw.minValue = minValue;
               bin.minValue = eval(formula);
               x = maxValue;
               binRaw.maxValue = maxValue;
               bin.maxValue = eval(formula);

               bin1.push(bin);
               binRaw1.push(binRaw);

               // calculate average bin count
               binSize += count;
               binSizeN++;
            }
         }
      }

      if (t1.length > 0) {

         let da = new Date(t1[0]*1000);
         let db = new Date(t1[t1.length-1]*1000);

         if (index === 0)
            log_hs_read("receiveData binned", t1[0], t1[t1.length-1]);

         if (this.data[index].time.length === 0 ||
            t1[0] > this.data[index].time[0]) {

            // append to right if new data
            this.data[index].time = this.data[index].time.concat(t1);
            this.data[index].bin = this.data[index].bin.concat(bin1);
            if (binRaw1.length > 0)
               this.data[index].binRaw = this.data[index].rawValue.concat(binRaw1);

         } else {

            // append to left if old data
            this.data[index].time = t1.concat(this.data[index].time);
            this.data[index].bin = bin1.concat(this.data[index].bin);
            if (binRaw1.length > 0)
               this.data[index].binRaw = binRaw1.concat(this.data[index].binRaw);
         }
      }
   }

   // calculate average bin size
   if (binSizeN > 0)
      this.binSize = binSize / binSizeN;

   return true;
};

MhistoryGraph.prototype.loadNewData = function () {

   if (this.updateTimer)
      window.clearTimeout(this.updateTimer);

   // don't update window if content is hidden (other tab, minimized, etc.)
   if (document.hidden) {
      this.updateTimer = window.setTimeout(this.loadNewData.bind(this), 500);
      return;
   }

   // don't update if not in scrolling mode
   if (!this.scroll) {
      this.updateTimer = window.setTimeout(this.loadNewData.bind(this), 500);
      return;
   }

   // update data from last point to current time
   let t1 = this.tMaxReceived;
   if (t1 === undefined)
      t1 = this.tMin;
   let t2 = Math.floor(new Date() / 1000);

   // for strip-chart mode always use non-binned data
   this.binned = false;

   log_hs_read("loadNewData  un-binned", t1, t2);
   mjsonrpc_call("hs_read_arraybuffer",
      {
         "start_time": Math.floor(t1),
         "end_time": Math.floor(t2),
         "events": this.events,
         "tags": this.tags,
         "index": this.index
      }, "arraybuffer")
      .then(function (rpc) {

         if (this.tMinRequested === undefined || t1 < this.tMinRequested) {
            this.tMinRequested = t1;
            this.tMinReceived = t1;
         }

         if (this.tMaxRequested === undefined || t2 > this.tMaxRequested) {
            this.tMaxReceived = t2;
            this.tMaxRequested = t2;
         }

         if (this.receiveData(rpc)) {
            this.findMinMax();
            this.scrollRedraw();
         }

         this.updateTimer = window.setTimeout(this.loadNewData.bind(this), 1000);

      }.bind(this)).catch(function (error) {
      mjsonrpc_error_alert(error);
   });
}

MhistoryGraph.prototype.scrollRedraw = function () {
   if (this.scrollTimer)
      window.clearTimeout(this.scrollTimer);

   if (this.scroll) {
      let dt = this.tMax - this.tMin;
      this.tMax = new Date() / 1000;
      this.tMin = this.tMax - dt;
      this.findMinMax();
      this.redraw(true);

      // calculate time for one pixel
      dt = (this.tMax - this.tMin) / (this.x2 - this.x1);
      dt = Math.min(Math.max(0.1, dt), 60);
      this.scrollTimer = window.setTimeout(this.scrollRedraw.bind(this), dt / 2 * 1000);
   } else {
      this.scrollTimer = window.setTimeout(this.scrollRedraw.bind(this), 1000);
      this.redraw(true);
   }
}

MhistoryGraph.prototype.discardCurrentData = function () {

   if (this.data === undefined)
      return;

   // drop all data
   for (let i = 0 ; i<this.data.length ; i++) {
      this.data[i].time.length = 0;
      if (this.data[i].value)
         this.data[i].value.length = 0;
      if (this.data[i].bin)
         this.data[i].bin.length = 0;
      if (this.data[i].rawBin)
         this.data[i].rawBin.length = 0;
      if (this.data[i].rawValue)
         this.data[i].rawValue.length = 0;

      this.x[i].length = 0;
      this.y[i].length = 0;
      this.t[i].length = 0;
      this.v[i].length = 0;
      if (this.vRaw[i])
         this.vRaw[i].length = 0;
   }

   this.tMaxReceived = undefined ;
   this.tMaxRequested = undefined;
   this.tMinRequested = undefined;
   this.tMinReceived = undefined;
}

function binarySearch(array, target) {
   let startIndex = 0;
   let endIndex = array.length - 1;
   let middleIndex;
   while (startIndex <= endIndex) {
      middleIndex = Math.floor((startIndex + endIndex) / 2);
      if (target === array[middleIndex])
         return middleIndex;

      if (target > array[middleIndex])
         startIndex = middleIndex + 1;
      if (target < array[middleIndex])
         endIndex = middleIndex - 1;
   }

   return middleIndex;
}

function splitEventAndTagName(var_name) {
   let colons = [];

   for (let i = 0; i < var_name.length; i++) {
      if (var_name[i] == ':') {
         colons.push(i);
      }
   }

   let slash_pos = var_name.indexOf("/");
   let uses_per_variable_naming = (slash_pos != -1);

   if (uses_per_variable_naming && colons.length % 2 == 1) {
      let middle_colon_pos = colons[Math.floor(colons.length / 2)];
      let slash_to_mid = var_name.substr(slash_pos + 1, middle_colon_pos - slash_pos - 1);
      let mid_to_end = var_name.substr(middle_colon_pos + 1);

      if (slash_to_mid == mid_to_end) {
         // Special case - we have a string of the form Beamlime/GS2:FC1:GS2:FC1.
         // Logger has already warned people that having colons in the equipment/event 
         // names is a bad idea, so we only need to worry about them in the tag name.
         split_pos = middle_colon_pos;
      } else {
         // We have a string of the form Beamlime/Demand:GS2:FC1. Split at the first colon.
         split_pos = colons[0];
      }
   } else {
      // Normal case - split at the fist colon.
      split_pos = colons[0];
   }

   let event_name = var_name.substr(0, split_pos);
   let tag_name = var_name.substr(split_pos + 1);

   return [event_name, tag_name];
}

MhistoryGraph.prototype.mouseEvent = function (e) {

   // fix buttons for IE
   if (!e.which && e.button) {
      if ((e.button & 1) > 0) e.which = 1;      // Left
      else if ((e.button & 4) > 0) e.which = 2; // Middle
      else if ((e.button & 2) > 0) e.which = 3; // Right
   }

   let cursor = this.pendingUpdates > 0 ? "progress" : "default";
   let title = "";
   let cancel = false;

   // cancel dragging in case we did not catch the mouseup event
   if (e.type === "mousemove" && e.buttons === 0 &&
      (this.drag.active || this.zoom.x.active || this.zoom.y.active))
      cancel = true;

   if (e.type === "mousedown") {

      this.intSelector.style.display = "none";
      this.downloadSelector.style.display = "none";

      // check for buttons
      this.button.forEach(b => {
         if (e.offsetX > b.x1 && e.offsetX < b.x1 + b.width &&
            e.offsetY > b.y1 && e.offsetY < b.y1 + b.width &&
            b.enabled) {
            b.click(this);
         }
      });

      // check for zoom buttons
      if (e.offsetX > this.width - 26 - 40 && e.offsetX < this.width - 26 - 20 &&
         e.offsetY > this.y1 - 20 && e.offsetY < this.y1) {
         // zoom in
         let delta = this.tMax - this.tMin;
         if (this.scroll) {
            this.tMin += delta / 2; // only zoom on left side in scroll mode
         } else {
            this.tMin += delta / 4;
            this.tMax -= delta / 4; // zoom to center
         }

         this.loadFullData(this.tMin, this.tMax);

         if (this.callbacks.timeZoom !== undefined)
            this.callbacks.timeZoom(this);

         e.preventDefault();
         return;
      }
      if (e.offsetX > this.width - 26 - 20 && e.offsetX < this.width - 26 &&
         e.offsetY > this.y1 - 20 && e.offsetY < this.y1) {
         // zoom out
         if (this.pendingUpdates > 0) {
            dlgMessage("Warning", "Don't press the '-' too fast!", true, false);
         } else {
            let delta = this.tMax - this.tMin;
            this.tMin -= delta / 2;
            this.tMax += delta / 2;
            // don't go into the future
            let now = Math.floor(new Date() / 1000);
            if (this.tMax > now) {
               this.tMax = now;
               this.tMin = now - 2*delta;
            }

            this.loadFullData(this.tMin, this.tMax);

            if (this.callbacks.timeZoom !== undefined)
               this.callbacks.timeZoom(this);
         }

         e.preventDefault();
         return;
      }

      // check for dragging
      if (e.offsetX > this.x1 && e.offsetX < this.x2 &&
         e.offsetY > this.y2 && e.offsetY < this.y1) {
         this.drag.active = true;
         this.marker.active = false;
         this.scroll = false;
         this.drag.xStart = e.offsetX;
         this.drag.yStart = e.offsetY;
         this.drag.tStart = this.xToTime(e.offsetX);
         this.drag.tMinStart = this.tMin;
         this.drag.tMaxStart = this.tMax;
         this.drag.yMinStart = this.yMin;
         this.drag.yMaxStart = this.yMax;
         this.drag.vStart = this.yToValue(e.offsetY);
      }

      // check for axis dragging
      if (e.offsetX > this.x1 && e.offsetX < this.x2 && e.offsetY > this.y1) {
         this.zoom.x.active = true;
         this.scroll = false;
         this.zoom.x.x1 = e.offsetX;
         this.zoom.x.x2 = undefined;
         this.zoom.x.t1 = this.xToTime(e.offsetX);
      }
      if (e.offsetY < this.y1 && e.offsetY > this.y2 && e.offsetX < this.x1) {
         this.zoom.y.active = true;
         this.scroll = false;
         this.zoom.y.y1 = e.offsetY;
         this.zoom.y.y2 = undefined;
         this.zoom.y.v1 = this.yToValue(e.offsetY);
      }

   } else if (cancel || e.type === "mouseup") {

      if (this.drag.active) {
         this.drag.active = false;
      }

      if (this.zoom.x.active) {
         if (this.zoom.x.x2 !== undefined &&
            Math.abs(this.zoom.x.x1 - this.zoom.x.x2) > 5) {
            let t1 = this.zoom.x.t1;
            let t2 = this.xToTime(this.zoom.x.x2);
            if (t1 > t2)
               [t1, t2] = [t2, t1];
            if (t2 - t1 < 1)
               t1 -= 1;
            this.tMin = t1;
            this.tMax = t2;
         }
         this.zoom.x.active = false;

         this.loadFullData(this.tMin, this.tMax);

         if (this.callbacks.timeZoom !== undefined)
            this.callbacks.timeZoom(this);
      }

      if (this.zoom.y.active) {
         if (this.zoom.y.y2 !== undefined &&
            Math.abs(this.zoom.y.y1 - this.zoom.y.y2) > 5) {
            let v1 = this.zoom.y.v1;
            let v2 = this.yToValue(this.zoom.y.y2);
            if (v1 > v2)
               [v1, v2] = [v2, v1];
            this.yMin = v1;
            this.yMax = v2;
         }
         this.zoom.y.active = false;
         this.yZoom = true;
         this.findMinMax();
         this.redraw(true);
      }

   } else if (e.type === "mousemove") {

      if (this.drag.active) {

         // execute dragging
         cursor = "move";
         let dt = Math.round((e.offsetX - this.drag.xStart) / (this.x2 - this.x1) * (this.tMax - this.tMin));
         this.tMin = this.drag.tMinStart - dt;
         this.tMax = this.drag.tMaxStart - dt;
         this.drag.lastDt = (e.offsetX - this.drag.lastOffsetX) / (this.x2 - this.x1) * (this.tMax - this.tMin);
         this.drag.lastT = new Date().getTime();
         this.drag.lastOffsetX = e.offsetX;
         if (this.yZoom) {
            let dy = (this.drag.yStart - e.offsetY) / (this.y1 - this.y2) * (this.yMax - this.yMin);
            this.yMin = this.drag.yMinStart - dy;
            this.yMax = this.drag.yMaxStart - dy;
            if (this.logAxis && this.yMin <= 0)
               this.yMin = 1E-20;
            if (this.logAxis && this.yMax <= 0)
               this.yMax = 1E-18;
         }

         this.loadSideData();
         this.findMinMax();
         this.redraw();

         if (this.callbacks.timeZoom !== undefined)
            this.callbacks.timeZoom(this);

      } else {

         let redraw = false;

         // change cursor to pointer over buttons
         this.button.forEach(b => {
            if (e.offsetX > b.x1 && e.offsetY > b.y1 &&
               e.offsetX < b.x1 + b.width && e.offsetY < b.y1 + b.height) {
               cursor = "pointer";
               title = b.title;
            }
         });

         if (this.showZoomButtons) {
            // check for zoom buttons
            if (e.offsetX > this.width - 26 - 40 && e.offsetX < this.width - 26 - 20 &&
               e.offsetY > this.y1 - 20 && e.offsetY < this.y1) {
               cursor = "pointer";
               title = "Zoom in";
            }
            if (e.offsetX > this.width - 26 - 20 && e.offsetX < this.width - 26 &&
               e.offsetY > this.y1 - 20 && e.offsetY < this.y1) {
               cursor = "pointer";
               title = "Zoom out";
            }
         }

         // display zoom cursor
         if (e.offsetX > this.x1 && e.offsetX < this.x2 && e.offsetY > this.y1)
            cursor = "ew-resize";
         if (e.offsetY < this.y1 && e.offsetY > this.y2 && e.offsetX < this.x1)
            cursor = "ns-resize";

         // execute axis zoom
         if (this.zoom.x.active) {
            this.zoom.x.x2 = Math.max(this.x1, Math.min(this.x2, e.offsetX));
            this.zoom.x.t2 = this.xToTime(e.offsetX);
            redraw = true;
         }
         if (this.zoom.y.active) {
            this.zoom.y.y2 = Math.max(this.y2, Math.min(this.y1, e.offsetY));
            this.zoom.y.v2 = this.yToValue(e.offsetY);
            redraw = true;
         }

         // check if cursor close to graph point
         if (this.data !== undefined && this.x.length && this.y.length) {

            let minDist = 10000;
            let markerX, markerY, markerT, markerV;
            for (let di = 0; di < this.data.length; di++) {

               if (this.solo.active && di !== this.solo.index)
                  continue;

               let i1 = binarySearch(this.x[di], e.offsetX - 10);
               let i2 = binarySearch(this.x[di], e.offsetX + 10);

               if (!this.binned) {
                  for (let i = i1; i <= i2; i++) {
                     let d = (e.offsetX - this.x[di][i]) * (e.offsetX - this.x[di][i]) +
                        (e.offsetY - this.y[di][i]) * (e.offsetY - this.y[di][i]);
                     if (d < minDist) {
                        minDist = d;
                        markerX = this.x[di][i];
                        markerY = this.y[di][i];
                        markerT = this.t[di][i];
                        markerV = this.v[di][i];

                        if (this.param["Show raw value"] !== undefined &&
                           this.param["Show raw value"][di])
                           markerV = this.vRaw[di][i];
                        else
                           markerV = this.v[di][i];

                        this.marker.graphIndex = di;
                        this.marker.index = i;
                     }
                  }
               } else {

                  // check max values
                  for (let i = i1; i <= i2; i++) {
                     let d = (e.offsetX - this.p[di][i].x) * (e.offsetX - this.p[di][i].x) +
                        (e.offsetY - this.p[di][i].max) * (e.offsetY - this.p[di][i].max);
                     if (d < minDist) {
                        minDist = d;
                        markerX = this.p[di][i].x;
                        markerY = this.p[di][i].max;
                        markerT = this.p[di][i].t;

                        if (this.param["Show raw value"] !== undefined &&
                           this.param["Show raw value"][di])
                           markerV = this.p[di][i].rawMaxValue;
                        else
                           markerV = this.p[di][i].maxValue;

                        this.marker.graphIndex = di;
                        this.marker.index = i;
                     }
                  }

                  // check min values
                  for (let i = i1; i <= i2; i++) {
                     let d = (e.offsetX - this.p[di][i].x) * (e.offsetX - this.p[di][i].x) +
                        (e.offsetY - this.p[di][i].min) * (e.offsetY - this.p[di][i].min);
                     if (d < minDist) {
                        minDist = d;
                        markerX = this.p[di][i].x;
                        markerY = this.p[di][i].min;
                        markerT = this.p[di][i].t;

                        if (this.param["Show raw value"] !== undefined &&
                           this.param["Show raw value"][di])
                           markerV = this.p[di][i].rawMinValue;
                        else
                           markerV = this.p[di][i].minValue;

                        this.marker.graphIndex = di;
                        this.marker.index = i;
                     }
                  }

               }
            }

            // exclude zoom buttons if visible
            let exclude = false;
            if (this.showZoomButtons &&
               e.offsetX > this.width - 26 - 40 && this.offsetX < this.width - 26 &&
               e.offsetY > this.y1 - 20 && e.offsetY < this.y1) {
               exclude = true;
            }
            // exclude label area
            if (this.showLabels &&
               e.offsetX > this.x1 && e.offsetX < this.x1 + 25 + this.variablesWidth + 7 &&
               e.offsetY > this.y2 && e.offsetY < this.y2 + this.variablesHeight + 2) {
               exclude = true;
            }

            if (exclude) {
               this.marker.active = false;
            } else {
               this.marker.active = Math.sqrt(minDist) < 10 && e.offsetX > this.x1 && e.offsetX < this.x2;
               if (this.marker.active) {
                  this.marker.x = markerX;
                  this.marker.y = markerY;
                  this.marker.t = markerT;
                  this.marker.v = markerV;

                  this.marker.mx = e.offsetX;
                  this.marker.my = e.offsetY;
               }
            }
            if (this.marker.active)
               redraw = true;
            if (!this.marker.active && this.marker.activeOld)
               redraw = true;
            this.marker.activeOld = this.marker.active;

            if (redraw)
               this.redraw(true);
         }
      }

   } else if (e.type === "dblclick") {

      // check if inside zoom buttons
      if (e.offsetX > this.width - 26 - 40 && e.offsetX < this.width - 26 &&
         e.offsetY > this.y1 - 20 && e.offsetY < this.y1) {
         // just ignore it

      } else {

         // measure distance to graphs
         if (this.data !== undefined && this.x.length && this.y.length) {

            // check if inside label area
            let flag = false;
            if (this.showLabels) {
               if (e.offsetX > this.x1 && e.offsetX < this.x1 + 25 + this.variablesWidth + 7) {
                  let i = Math.floor((e.offsetY - (this.y2 + 4)) / 17);
                  if (i < this.data.length) {
                     this.solo.active = true;
                     this.solo.index = i;
                     this.findMinMax();
                     flag = true;
                  }
               }
            }

            if (!flag) {
               let minDist = 100;
               for (let di = 0; di < this.data.length; di++) {
                  for (let i = 0; i < this.x[di].length; i++) {
                     if (this.x[di][i] > this.x1 && this.x[di][i] < this.x2) {
                        let d = Math.sqrt(Math.pow(e.offsetX - this.x[di][i], 2) +
                           Math.pow(e.offsetY - this.y[di][i], 2));
                        if (d < minDist) {
                           minDist = d;
                           this.solo.index = di;
                        }
                     }
                  }
               }
               // check if close to graph point
               if (minDist < 10 && e.offsetX > this.x1 && e.offsetX < this.x2)
                  this.solo.active = !this.solo.active;
               this.findMinMax();
            }

            this.redraw(true);
         }
      }
   }

   this.parentDiv.title = title;
   this.parentDiv.style.cursor = cursor;

   e.preventDefault();
};

MhistoryGraph.prototype.mouseWheelEvent = function (e) {

   if (e.offsetX > this.x1 && e.offsetX < this.x2 &&
      e.offsetY > this.y2 && e.offsetY < this.y1) {

      if (e.altKey || e.shiftKey) {

         // zoom Y axis
         this.yZoom = true;
         let f = (e.offsetY - this.y1) / (this.y2 - this.y1);

         let step = e.deltaY / 100;
         if (step > 0.5)
            step = 0.5;
         if (step < -0.5)
            step = -0.5;

         let dtMin = f * (this.yMax - this.yMin) * step;
         let dtMax = (1 - f) * (this.yMax - this.yMin) * step;

         if (((this.yMax + dtMax) - (this.yMin - dtMin)) / (this.yMax0 - this.yMin0) < 1000 &&
            (this.yMax0 - this.yMin0) / ((this.yMax + dtMax) - (this.yMin - dtMin)) < 1000) {
            this.yMin -= dtMin;
            this.yMax += dtMax;

            if (this.logAxis && this.yMin <= 0)
               this.yMin = 1E-20;
            if (this.logAxis && this.yMax <= 0)
               this.yMax = 1E-18;
         }

         this.redraw();

      } else if (e.ctrlKey || e.metaKey) {

         this.showZoomButtons = false;

         // zoom time axis
         let f = (e.offsetX - this.x1) / (this.x2 - this.x1);
         let m = e.deltaY / 100;
         if (m > 0.3)
            m = 0.3;
         if (m < -0.3)
            m = -0.3;
         let dtMin = Math.abs(f * (this.tMax - this.tMin) * m);
         let dtMax = Math.abs((1 - f) * (this.tMax - this.tMin) * m);

         if (e.deltaY < 0) {
            // zoom in
            if (this.scroll) {
               this.tMin += dtMin;
            } else {
               this.tMin += dtMin;
               this.tMax -= dtMax;
            }

            this.loadFullData(this.tMin, this.tMax);
         }
         if (e.deltaY > 0) {
            // zoom out
            if (this.scroll) {
               this.tMin -= dtMin;
            } else {
               this.tMin -= dtMin;
               this.tMax += dtMax;
            }

            this.loadFullData(this.tMin, this.tMax);
         }

         if (this.callbacks.timeZoom !== undefined)
            this.callbacks.timeZoom(this);

      } else if (e.deltaX !== 0) {

         let dt = (this.tMax - this.tMin) / 1000 * e.deltaX;
         this.tMin += dt;
         this.tMax += dt;

         if (dt < 0)
            this.loadSideData();
         this.findMinMax();
         this.redraw();
      } else
         return;

      this.marker.active = false;

      e.preventDefault();
   }
};

MhistoryGraph.prototype.resetAxes = function () {
   this.tMax = Math.floor(new Date() / 1000);
   this.tMin = this.tMax - this.tScale;

   this.scroll = true;
   this.yZoom = false;
   this.showZoomButtons = true;
   this.loadFullData(this.tMin, this.tMax);
};

MhistoryGraph.prototype.jumpToCurrent = function () {
   let dt = Math.floor(this.tMax - this.tMin);

   // limit to one week maximum (otherwise we have to read binned data)
   if (dt > 24*3600*7)
      dt = 24*3600*7;

   this.tMax = Math.floor(new Date() / 1000);
   this.tMin = this.tMax - dt;
   this.scroll = true;

   this.loadFullData(this.tMin, this.tMax);
};

MhistoryGraph.prototype.setTimespan = function (tMin, tMax, scroll) {
   this.tMin = tMin;
   this.tMax = tMax;
   this.scroll = scroll;
   
   this.loadFullData(tMin, tMax, scroll);
};

MhistoryGraph.prototype.resize = function () {
   this.canvas.width = this.parentDiv.clientWidth;
   this.canvas.height = this.parentDiv.clientHeight;
   this.width = this.parentDiv.clientWidth;
   this.height = this.parentDiv.clientHeight;

   if (this.intSelector !== undefined)
      this.intSelector.style.display = "none";

   this.forceConvert = true;
   this.redraw(true);
};

MhistoryGraph.prototype.redraw = function (force) {
   this.forceRedraw = force;
   let f = this.draw.bind(this);
   window.requestAnimationFrame(f);
};

MhistoryGraph.prototype.timeToXInit = function () {
   this.timeToXScale = 1 / (this.tMax - this.tMin) * (this.x2 - this.x1);
}

MhistoryGraph.prototype.timeToX = function (t) {
   return (t - this.tMin) * this.timeToXScale + this.x1;
};

MhistoryGraph.prototype.truncateInfinity = function(v) {
   if (v === Infinity) {
      return Number.MAX_VALUE;
   } else if (v === -Infinity) {
      return -Number.MAX_VALUE;
   } else {
      return v;
   }
};

MhistoryGraph.prototype.valueToYInit = function () {
   // Avoid overflow of max - min > inf
   let max_scaled = this.yMax / 1e4;
   let min_scaled = this.yMin / 1e4;
   this.valueToYScale = (this.y1 - this.y2) * 1e-4 / (max_scaled - min_scaled);
}

MhistoryGraph.prototype.valueToY = function (v) {
   if (v === Infinity) {
      return this.yMax >= Number.MAX_VALUE ? this.y2 : 0;
   } else if (v === -Infinity) {
      return this.yMin <= -Number.MAX_VALUE ? this.y1 : this.y1 * 2;
   } else if (this.logAxis) {
      if (v <= 0)
         return this.y1;
      else
         return this.y1 - (Math.log(v) - Math.log(this.yMin)) /
            (Math.log(this.yMax) - Math.log(this.yMin)) * (this.y1 - this.y2);
   } else {
      return this.y1 - (v - this.yMin) * this.valueToYScale;
   }
};

MhistoryGraph.prototype.xToTime = function (x) {
   return (x - this.x1) / (this.x2 - this.x1) * (this.tMax - this.tMin) + this.tMin;
};

MhistoryGraph.prototype.yToValue = function (y) {
   if (!isFinite(this.yMax - this.yMin)) {
      // Contortions to avoid Infinity.
      let scaled = (this.yMax / 1e4) - (this.yMin / 1e4);
      let retval = ((((this.y1 - y) / (this.y1 - this.y2)) * scaled) + (this.yMin / 1e4)) * 1e4;
      return retval;
   }
   return (this.y1 - y) / (this.y1 - this.y2) * (this.yMax - this.yMin) + this.yMin;
};

MhistoryGraph.prototype.findMinMax = function () {

   if (this.yZoom)
      return;

   if (!this.autoscaleMin)
      this.yMin0 = this.param["Minimum"];

   if (!this.autoscaleMax)
      this.yMax0 = this.param["Maximum"];

   if (!this.autoscaleMin && !this.autoscaleMax) {
      this.yMin = this.yMin0;
      this.yMax = this.yMax0;
      return;
   }

   let minValue = undefined;
   let maxValue = undefined;
   for (let index = 0; index < this.data.length; index++) {
      if (this.events[index] === "Run transitions")
         continue;
      if (this.data[index].time.length === 0)
         continue;
      if (this.solo.active && this.solo.index !== index)
         continue;
      let i1 = binarySearch(this.data[index].time, this.tMin) + 1;
      let i2 = binarySearch(this.data[index].time, this.tMax);
      while ((minValue === undefined ||
              maxValue === undefined ||
              Number.isNaN(minValue) ||
              Number.isNaN(maxValue)) &&
              i1 < i2) {
         // find first valid value
         if (this.binned) {
            if (this.data[index].bin[i1].count !== 0) {
               minValue = this.data[index].bin[i1].minValue;
               maxValue = this.data[index].bin[i1].maxValue;
            }
         } else {
            minValue = this.data[index].value[i1];
            maxValue = this.data[index].value[i1];
         }
         i1++;
      }
      for (let i = i1; i <= i2; i++) {
         if (this.binned) {
            if (this.data[index].bin[i].count === 0)
               continue;
            let v = this.data[index].bin[i].minValue;
            if (v < minValue)
               minValue = v;
            v = this.data[index].bin[i].maxValue;
            if (v > maxValue)
               maxValue = v;
         } else {
            let v = this.data[index].value[i];
            if (Number.isNaN(v))
               continue;
            if (v < minValue)
               minValue = v;
            if (v > maxValue)
               maxValue = v;
         }
      }
   }

   if (this.autoscaleMin)
      this.yMin0 = this.yMin = minValue;
   if (this.autoscaleMax)
      this.yMax0 = this.yMax = maxValue;

   if (minValue === undefined || maxValue === undefined) {
      this.yMin0 = -0.5;
      this.yMax0 = 0.5;
   }

   if (this.yMin0 === this.yMax0) {
      this.yMin0 -= 0.5;
      this.yMax0 += 0.5;
   }

   if (this.yMax0 < this.yMin0)
      this.yMax0 = this.yMin0 + 1;

   if (!this.yZoom) {
      if (this.autoscaleMin) {
         if (this.logAxis)
            this.yMin = 0.8 * this.yMin0;
         else
            // leave 10% space below graph
            this.yMin = this.yMin0 - this.truncateInfinity(this.yMax0 - this.yMin0) / 10;
      } else
         this.yMin = this.yMin0;
      if (this.logAxis && this.yMin <= 0)
         this.yMin = 1E-20;

      if (this.autoscaleMax) {
         if (this.logAxis)
            this.yMax = 1.2 * this.yMax0;
         else
            // leave 10% space above graph
            this.yMax = this.yMax0 + this.truncateInfinity(this.yMax0 - this.yMin0) / 10;
      } else
         this.yMax = this.yMax0;
      if (this.logAxis && this.yMax <= 0)
         this.yMax = 1E-18;
   }

   this.yMax = this.truncateInfinity(this.yMax)
   this.yMin = this.truncateInfinity(this.yMin)
};

function convertLastWritten(last) {
   if (last === 0)
      return "no data available";

   let d = new Date(last * 1000).toLocaleDateString(
      'en-GB', {
         day: '2-digit', month: 'short', year: '2-digit',
         hour12: false, hour: '2-digit', minute: '2-digit'
      }
   );

   return "last data: " + d;
}

MhistoryGraph.prototype.updateURL = function() {
   let url = window.location.href;
   if (url.search("&A=") !== -1)
      url = url.slice(0, url.search("&A="));
   url += "&A=" + Math.round(this.tMin) + "&B=" + Math.round(this.tMax);

   if (url !== window.location.href)
      window.history.replaceState({}, "Midas History", url);
}

function createPinstripeCanvas() {
   const patternCanvas = document.createElement("canvas");
   const pctx = patternCanvas.getContext('2d', { antialias: true });
   const colour = "#FFC0C0";

   const CANVAS_SIDE_LENGTH = 90;
   const WIDTH = CANVAS_SIDE_LENGTH;
   const HEIGHT = CANVAS_SIDE_LENGTH;
   const DIVISIONS = 4;

   patternCanvas.width = WIDTH;
   patternCanvas.height = HEIGHT;
   pctx.fillStyle = colour;

   // Top line
   pctx.beginPath();
   pctx.moveTo(0, HEIGHT * (1 / DIVISIONS));
   pctx.lineTo(WIDTH * (1 / DIVISIONS), 0);
   pctx.lineTo(0, 0);
   pctx.lineTo(0, HEIGHT * (1 / DIVISIONS));
   pctx.fill();

   // Middle line
   pctx.beginPath();
   pctx.moveTo(WIDTH, HEIGHT * (1 / DIVISIONS));
   pctx.lineTo(WIDTH * (1 / DIVISIONS), HEIGHT);
   pctx.lineTo(0, HEIGHT);
   pctx.lineTo(0, HEIGHT * ((DIVISIONS - 1) / DIVISIONS));
   pctx.lineTo(WIDTH * ((DIVISIONS - 1) / DIVISIONS), 0);
   pctx.lineTo(WIDTH, 0);
   pctx.lineTo(WIDTH, HEIGHT * (1 / DIVISIONS));
   pctx.fill();

   // Bottom line
   pctx.beginPath();
   pctx.moveTo(WIDTH, HEIGHT * ((DIVISIONS - 1) / DIVISIONS));
   pctx.lineTo(WIDTH * ((DIVISIONS - 1) / DIVISIONS), HEIGHT);
   pctx.lineTo(WIDTH, HEIGHT);
   pctx.lineTo(WIDTH, HEIGHT * ((DIVISIONS - 1) / DIVISIONS));
   pctx.fill();

   return patternCanvas;
}

MhistoryGraph.prototype.draw = function () {
   //profile(true);

   // draw maximal 30 times per second
   if (!this.forceRedraw) {
      if (new Date().getTime() < this.lastDrawTime + 30)
         return;
      this.lastDrawTime = new Date().getTime();
   }
   this.forceRedraw = false;

   let update_last_written = false;

   let ctx = this.canvas.getContext("2d");

   ctx.fillStyle = this.color.background;
   ctx.fillRect(0, 0, this.width, this.height);

   if (this.data === undefined) {
      ctx.lineWidth = 1;
      ctx.font = "14px sans-serif";
      ctx.strokeStyle = "#808080";
      ctx.fillStyle = "#808080";
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText("Data being loaded ...", this.width / 2, this.height / 2);
      return;
   }

   ctx.lineWidth = 1;
   ctx.font = "14px sans-serif";

   if (this.height === undefined || this.width === undefined)
      return;
   if (this.yMin === undefined || Number.isNaN(this.yMin))
      return;
   if (this.yMax === undefined || Number.isNaN(this.yMax))
      return;

   let axisLabelWidth = this.drawVAxis(ctx, 50, this.height - 25, this.height - 35,
      -4, -7, -10, -12, 0, this.yMin, this.yMax, this.logAxis, false);

   if (axisLabelWidth === undefined)
      return;

   this.x1 = axisLabelWidth + 15;
   this.y1 = this.height - 25;
   this.x2 = this.showMenuButtons ? this.width - 26 : this.width - 2;
   this.y2 = this.floating ? 10 : 26;

   this.timeToXInit();  // initialize scale factor t -> x
   this.valueToYInit(); // initialize scale factor v -> y

   if (this.showMenuButtons === false)
      this.x2 = this.width - 2;

   // title
   if (!this.floating) { // suppress title since this is already in the dialog box
      ctx.strokeStyle = this.color.axis;
      ctx.fillStyle = "#F0F0F0";
      ctx.strokeRect(this.x1, 6, this.x2 - this.x1, 20);
      ctx.fillRect(this.x1, 6, this.x2 - this.x1, 20);
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillStyle = "#808080";
      if (this.group !== undefined)
         ctx.fillText(this.group + " - " + this.panel, (this.x2 + this.x1) / 2, 16);
      else if (this.historyVar !== undefined)
         ctx.fillText(this.historyVar, (this.x2 + this.x1) / 2, 16);

      // display binning
      let s = Math.round(this.binSize);
      ctx.textAlign = "right";
      ctx.fillText(s, this.x2 - 10, 16);
   }

   // draw axis
   ctx.strokeStyle = this.color.axis;
   ctx.drawLine(this.x1, this.y2, this.x2, this.y2);
   ctx.drawLine(this.x2, this.y2, this.x2, this.y1);

   if (this.logAxis && this.yMin < 1E-20)
      this.yMin = 1E-20;
   if (this.logAxis && this.yMax < 1E-18)
      this.yMax = 1E-18;
   this.drawVAxis(ctx, this.x1, this.y1, this.y1 - this.y2,
      -4, -7, -10, -12, this.x2 - this.x1, this.yMin, this.yMax, this.logAxis, true);
   this.drawTAxis(ctx, this.x1, this.y1, this.x2 - this.x1, this.width,
      4, 7, 10, 10, this.y2 - this.y1, this.tMin, this.tMax);

   // draw hatched area for "future"
   let t = new Date() / 1000;
   if (this.tMax > t) {
      let x = this.timeToX(t);
      if (x < this.x1)
         x = this.x1;
      ctx.fillStyle = ctx.createPattern(createPinstripeCanvas(), 'repeat');
      ctx.fillRect(x, 26, this.x2 - x, this.y1 - this.y2);

      ctx.strokeStyle = this.color.axis;
      ctx.strokeRect(x, 26, this.x2 - x, this.y1 - this.y2);
   }

   // determine precision
   let n_sig1, n_sig2;
   if (this.yMin === 0)
      n_sig1 = 1;
   else
      n_sig1 = Math.floor(Math.log(Math.abs(this.yMin)) / Math.log(10)) -
         Math.floor(Math.log(Math.abs((this.yMax - this.yMin) / 50)) / Math.log(10)) + 1;

   if (this.yMax === 0)
      n_sig2 = 1;
   else
      n_sig2 = Math.floor(Math.log(Math.abs(this.yMax)) / Math.log(10)) -
         Math.floor(Math.log(Math.abs((this.yMax - this.yMin) / 50)) / Math.log(10)) + 1;

   n_sig1 = Math.max(n_sig1, n_sig2);
   n_sig1 = Math.max(1, n_sig1);

   // toPrecision displays 1050 with 3 digits as 1.05e+3, so increase precision to number of digits
   if (Math.abs(this.yMin) < 100000)
      n_sig1 = Math.max(n_sig1, Math.floor(Math.log(Math.abs(this.yMin)) /
         Math.log(10) + 0.001) + 1);
   if (Math.abs(this.yMax) < 100000)
      n_sig1 = Math.max(n_sig1, Math.floor(Math.log(Math.abs(this.yMax)) /
         Math.log(10) + 0.001) + 1);

   if (isNaN(n_sig1))
      n_sig1 = 6;

   this.yPrecision = Math.max(6, n_sig1); // use at least 5 digits

   ctx.save();
   ctx.beginPath();
   ctx.rect(this.x1, this.y2, this.x2 - this.x1, this.y1 - this.y2);
   ctx.clip();

   //profile("drawinit");

   let nPoints = 0;
   for (let di = 0; di < this.data.length; di++)
      nPoints += this.data[di].time.length;

   // convert values to points if window has changed or number of points have changed
   if (this.tMin !== this.tMinOld || this.tMax !== this.tMaxOld ||
      this.yMin !== this.yMinOld || this.yMax !== this.yMaxOld ||
      nPoints !== this.nPointsOld || this.forceConvert) {

      this.tMinOld = this.tMin;
      this.tMaxOld = this.tMax;
      this.yMinOld = this.yMin;
      this.yMaxOld = this.yMax;
      this.nPointsOld = nPoints;
      this.forceConvert = false;

      //profile();
      for (let di = 0; di < this.data.length; di++) {

         if (this.x[di] === undefined) {
            this.x[di] = []; // x/y contain visible part of graph
            this.y[di] = [];
            this.t[di] = []; // t/v contain time/value pairs corresponding to x/y
            this.v[di] = [];
            this.p[di] = [];
            this.vRaw[di] = []; // vRaw contains the value before the formula
         }

         let n = 0;

         if (this.data[di].time.length === 0)
            continue;

         let i1 = binarySearch(this.data[di].time, this.tMin);
         if (i1 > 0)
            i1--; // add point to the left
         let i2 = binarySearch(this.data[di].time, this.tMax);
         if (i2 < this.data[di].time.length - 1)
            i2++; // add points to the right

         // un-binned data
         if (!this.binned || this.events[di] === "Run transitions") {
            for (let i = i1; i <= i2; i++) {
               let x = this.timeToX(this.data[di].time[i]);
               let y = this.valueToY(this.data[di].value[i]);
               if (y < -100000)
                  y = -100000;
               if (y > 100000)
                  y = 100000;
               if (!Number.isNaN(y)) {
                  this.x[di][n] = x;
                  this.y[di][n] = y;
                  this.t[di][n] = this.data[di].time[i];
                  this.v[di][n] = this.data[di].value[i];
                  if (this.data[di].rawValue)
                     this.vRaw[di][n] = this.data[di].rawValue[i];
                  n++;
               }
            }

            // truncate arrays if now shorter
            this.x[di].length = n;
            this.y[di].length = n;
            this.t[di].length = n;
            this.v[di].length = n;
            if (this.data[di].rawValue)
               this.vRaw[di].length = n;

         } else {

            // binned data
            for (let i = i1; i <= i2; i++) {

               if (this.data[di].bin[i].count === 0)
                  continue;

               let p = {};
               p.n = this.data[di].bin[i].count;
               p.x = Math.round(this.timeToX(this.data[di].time[i]));
               p.t = this.data[di].time[i];

               p.first = this.valueToY(this.data[di].bin[i].firstValue);
               p.min = this.valueToY(this.data[di].bin[i].minValue);
               p.minValue = this.data[di].bin[i].minValue;
               p.max = this.valueToY(this.data[di].bin[i].maxValue);
               p.maxValue = this.data[di].bin[i].maxValue;
               p.last = this.valueToY(this.data[di].bin[i].lastValue);

               if (this.data[di].binRaw) {
                  p.rawFirstValue = this.data[di].binRaw[i].firstValue;
                  p.rawMinValue = this.data[di].binRaw[i].minValue;
                  p.rawMaxValue = this.data[di].binRaw[i].maxValue;
                  p.rawLastValue = this.data[di].binRaw[i].lastValue;
               }

               this.p[di][n] = p;

               this.x[di][n] = p.x;

               n++;
            }

            // truncate arrays if now shorter
            this.p[di].length = n;
            this.x[di].length = n;
            if (this.data[di].rawValue)
               this.vRaw[di].length = n;
         }
      }
   }

   // draw shaded areas
   if (this.showFill) {
      for (let di = 0; di < this.data.length; di++) {
         if (this.solo.active && this.solo.index !== di)
            continue;

         if (this.events[di] === "Run transitions")
            continue;

         ctx.fillStyle = this.param["Colour"][di];

         // don't draw lines over "gaps"
         let gap = this.timeToXScale * 600; // 10 min
         if (gap < 5)
            gap = 5; // minimum of 5 pixels

         if (this.binned) {
            if (this.p[di].length > 0) {
               let p = this.p[di][0];
               let x0 = p.x;
               let xold = p.x;
               let y0 = p.first;
               ctx.beginPath();
               ctx.moveTo(p.x, p.first);
               ctx.lineTo(p.x, p.last);
               for (let i = 1; i < this.p[di].length; i++) {
                  p = this.p[di][i];
                  if (p.x - xold < gap) {
                     ctx.lineTo(p.x, p.first);
                     ctx.lineTo(p.x, p.last);
                  } else {
                     ctx.lineTo(xold, this.valueToY(0));
                     ctx.lineTo(p.x, this.valueToY(0));
                     ctx.lineTo(p.x, p.first);
                     ctx.lineTo(p.x, p.last);
                  }
                  xold = p.x;
               }
               ctx.lineTo(xold, this.valueToY(0));
               ctx.lineTo(x0, this.valueToY(0));
               ctx.lineTo(x0, y0);
               ctx.globalAlpha = 0.1;
               ctx.fill();
               ctx.globalAlpha = 1;
            }
         } else { // binned
            if (this.x[di].length > 0 && this.y[di].length > 0) {
               let x = this.x[di][0];
               let y = this.y[di][0];
               let x0 = x;
               let y0 = y;
               let xold = x;
               ctx.beginPath();
               ctx.moveTo(x, y);
               for (let i = 1; i < this.x[di].length; i++) {
                  x = this.x[di][i];
                  y = this.y[di][i];
                  if (x - xold < gap)
                     ctx.lineTo(x, y);
                  else {
                     ctx.lineTo(xold, this.valueToY(0));
                     ctx.lineTo(x, this.valueToY(0));
                     ctx.lineTo(x, y);
                  }
                  xold = x;
               }
               ctx.lineTo(xold, this.valueToY(0));
               ctx.lineTo(x0, this.valueToY(0));
               ctx.lineTo(x0, y0);
               ctx.globalAlpha = 0.1;
               ctx.fill();
               ctx.globalAlpha = 1;
            }
         }
      }
   }

   // profile("Draw shaded areas");

   // draw graphs
   for (let di = 0; di < this.data.length; di++) {
      if (this.solo.active && this.solo.index !== di)
         continue;

      if (this.events[di] === "Run transitions") {

         if (this.tags[di] === "State") {
            if (this.x[di].length < 200) {
               for (let i = 0; i < this.x[di].length; i++) {
                  if (this.v[di][i] === 1) {
                     ctx.strokeStyle = "#FF0000";
                     ctx.fillStyle = "#808080";
                     ctx.textAlign = "right";
                     ctx.textBaseline = "top";
                     ctx.fillText(this.v[di + 1][i], this.x[di][i] - 5, this.y2 + 3);
                  } else if (this.v[di][i] === 3) {
                     ctx.strokeStyle = "#00A000";
                     ctx.fillStyle = "#808080";
                     ctx.textAlign = "left";
                     ctx.textBaseline = "top";
                     ctx.fillText(this.v[di + 1][i], this.x[di][i] + 3, this.y2 + 3);
                  } else {
                     ctx.strokeStyle = "#F9A600";
                  }

                  ctx.setLineDash([8, 2]);
                  ctx.drawLine(Math.floor(this.x[di][i]), this.y1, Math.floor(this.x[di][i]), this.y2);
                  ctx.setLineDash([]);
               }
            }
         }

      } else {

         ctx.strokeStyle = this.param["Colour"][di];

         // don't draw lines over "gaps"
         let gap = this.timeToXScale * 600; // 10 min
         if (gap < 5)
            gap = 5; // minimum of 5 pixels

         if (this.binned) {
            if (this.p[di].length > 0) {
               let p = this.p[di][0];
               //console.log("di:" + di + " i:" + 0 + " x:" + p.x, " y:" + p.first);
               let xold = p.x;
               ctx.beginPath();
               ctx.moveTo(p.x, p.first);
               ctx.lineTo(p.x, p.max + 1); // in case min==max
               ctx.lineTo(p.x, p.min);
               ctx.lineTo(p.x, p.last);
               for (let i = 1; i < this.p[di].length; i++) {
                  p = this.p[di][i];
                  //console.log("di:" + di + " i:" + i + " x:" + p.x, " y:" + p.first);
                  if (p.x - xold < gap) {
                     // draw lines first - max - min - last
                     ctx.lineTo(p.x, p.first);
                     ctx.lineTo(p.x, p.max + 1); // in case min==max
                     ctx.lineTo(p.x, p.min);
                     ctx.lineTo(p.x, p.last);
                  } else { // don't draw gap
                     // draw lines first - max - min - last
                     ctx.moveTo(p.x, p.first);
                     ctx.lineTo(p.x, p.max + 1); // in case min==max
                     ctx.lineTo(p.x, p.min);
                     ctx.lineTo(p.x, p.last);
                  }
                  xold = p.x;
               }
               ctx.stroke();
            }
         } else { // binned
            if (this.x[di].length === 1) {
               let x = this.x[di][0];
               let y = this.y[di][0];
               ctx.fillStyle = this.param["Colour"][di];
               ctx.fillRect(x - 1, y - 1, 3, 3);
            } else {
               if (this.x[di].length > 0) {
                  ctx.beginPath();
                  let x = this.x[di][0];
                  let y = this.y[di][0];
                  let xold = x;
                  ctx.moveTo(x, y);
                  for (let i = 1; i < this.x[di].length; i++) {
                     let x = this.x[di][i];
                     let y = this.y[di][i];
                     if (x - xold > gap)
                        ctx.moveTo(x, y);
                     else
                        ctx.lineTo(x, y);
                     xold = x;
                  }
                  ctx.stroke();
               }
            }
         }
      }
   }

   ctx.restore(); // remove clipping

   // profile("Draw graphs");

   // labels with variable names and values
   if (this.showLabels) {
      if (this.solo.active)
         this.variablesHeight = 17 + 7;
      else
         this.variablesHeight = this.param["Variables"].length * 17 + 7;
      this.variablesWidth = 0;

      // determine width of widest label
      this.param["Variables"].forEach((v, i) => {
         let width;
         if (this.param.Label[i] !== "") {
            width = ctx.measureText(this.param.Label[i]).width;
         } else {
            width = ctx.measureText(splitEventAndTagName(v)[1]).width;
         }

         if (this.param["Show raw value"] !== undefined &&
            this.param["Show raw value"][i])
            width += ctx.measureText(" (Raw)").width;

         width += 20; // space between name and value

         if (this.v[i] !== undefined && this.v[i].length > 0) {
            // use last point in array
            let index = this.v[i].length - 1;

            // use point at current marker
            if (this.marker.active)
               index = this.marker.index;

            if (index < this.v[i].length) {
               let value;
               if (this.param["Show raw value"] !== undefined &&
                  this.param["Show raw value"][i])
                  value = this.vRaw[i][index];
               else
                  value = this.v[i][index];

               // convert value to string with 6 digits
               let str = "  " + value.toPrecision(this.yPrecision).stripZeros();
               width += ctx.measureText(str).width;
            }
         } else if (this.p[i] !== undefined && this.p[i].length > 0) {
             // use last point in array
             let index = this.p[i].length - 1;

             // use point at current marker
             if (this.marker.active)
                 index = this.marker.index;

             if (index < this.p[i].length) {
                 let value;
                 if (this.param["Show raw value"] !== undefined &&
                     this.param["Show raw value"][i])
                     value = (this.p[i][index].rawMinValue + this.p[i][index].rawMaxValue)/2;
                 else
                     value = (this.p[i][index].minValue + this.p[i][index].maxValue)/2;

                 // convert value to string with 6 digits
                 let str = "  " + value.toPrecision(this.yPrecision).stripZeros();
                 width += ctx.measureText(str).width;
             }
         } else {
            width += ctx.measureText(convertLastWritten(this.lastWritten[i])).width;
         }

         this.variablesWidth = Math.max(this.variablesWidth, width);
      });

      let xLabel = this.x1;
      if (this.solo.active)
         xLabel = this.x1 + 28;

      ctx.save();
      ctx.beginPath();
      ctx.rect(xLabel, this.y2, 25 + this.variablesWidth + 7, this.variablesHeight + 2);
      ctx.clip();

      ctx.strokeStyle = this.color.axis;
      ctx.fillStyle = "#F0F0F0";
      ctx.globalAlpha = 0.5;
      ctx.strokeRect(xLabel, this.y2, 25 + this.variablesWidth + 5, this.variablesHeight);
      ctx.fillRect(xLabel, this.y2, 25 + this.variablesWidth + 5, this.variablesHeight);
      ctx.globalAlpha = 1;

      this.param["Variables"].forEach((v, i) => {

         if (this.solo.active && i !== this.solo.index)
            return;

         let yLabel = 0;
         if (!this.solo.active)
            yLabel = i * 17;

         ctx.lineWidth = 4;
         ctx.strokeStyle = this.param["Colour"][i];
         ctx.drawLine(xLabel + 5, this.y2 + 14 + yLabel, xLabel + 20, this.y2 + 14 + yLabel);
         ctx.lineWidth = 1;

         ctx.textAlign = "left";
         ctx.textBaseline = "middle";
         ctx.fillStyle = "#404040";

         let str;
         if (this.param.Label[i] !== "")
            str = this.param.Label[i];
         else
            str = splitEventAndTagName(v)[1];

         if (this.param["Show raw value"] !== undefined &&
            this.param["Show raw value"][i])
            str += " (Raw)";

         ctx.fillText(str, xLabel + 25, this.y2 + 14 + yLabel);

         ctx.textAlign = "right";

         // un-binned data
         if (this.v[i] !== undefined && this.v[i].length > 0) {
            // use last point in array
            let index = this.v[i].length - 1;

            // use point at current marker
            if (this.marker.active)
               index = this.marker.index;

            if (index < this.v[i].length) {
               // convert value to string with 6 digits
               let value;
               if (this.param["Show raw value"] !== undefined &&
                   this.param["Show raw value"][i])
                  value = this.vRaw[i][index];
               else
                  value = this.v[i][index];
               let str = value.toPrecision(this.yPrecision).stripZeros();
               ctx.fillText(str, xLabel + 25 + this.variablesWidth, this.y2 + 14 + yLabel);
            }
         } else if (this.p[i] !== undefined && this.p[i].length > 0) {

            // binned data

            // use last point in array
            let index = this.p[i].length - 1;

            // use point at current marker
            if (this.marker.active)
               index = this.marker.index;

            if (index < this.p[i].length) {
               // convert value to string with 6 digits
               let value;
               if (this.param["Show raw value"] !== undefined &&
                  this.param["Show raw value"][i])
                  value = (this.p[i][index].rawMinValue + this.p[i][index].rawMaxValue)/2;
               else
                  value = (this.p[i][index].minValue + this.p[i][index].maxValue) / 2;
               let str = value.toPrecision(this.yPrecision).stripZeros();
               ctx.fillText(str, xLabel + 25 + this.variablesWidth, this.y2 + 14 + yLabel);
            }

         } else {

            if (this.lastWritten.length > 0) {
               if (this.lastWritten[i] > this.tMax) {
                  //console.log("last written is in the future: " + this.events[i] + ", lw: " + this.lastWritten[i], ", this.tMax: " + this.tMax, ", diff: " + (this.lastWritten[i] - this.tMax));
                  update_last_written = true;
               }
               ctx.fillText(convertLastWritten(this.lastWritten[i]),
                  xLabel + 25 + this.variablesWidth, this.y2 + 14 + yLabel);
            } else {
               //console.log("last_written was not loaded yet");
               update_last_written = true;
            }
         }

      });

      ctx.restore(); // remove clipping
   }

   // "updating" notice
   if (this.pendingUpdates > 0 && this.tMinReceived > this.tMin) {
      let str = "Updating data ...";
      ctx.strokeStyle = "#404040";
      ctx.fillStyle = "#FFC0C0";
      ctx.fillRect(this.x1 + 5, this.y1 - 22, 10 + ctx.measureText(str).width, 17);
      ctx.strokeRect(this.x1 + 5, this.y1 - 22, 10 + ctx.measureText(str).width, 17);
      ctx.fillStyle = "#404040";
      ctx.textAlign = "left";
      ctx.textBaseline = "middle";
      ctx.fillText(str, this.x1 + 10, this.y1 - 13);
   }

   let no_data = true;

   for (let i = 0; i < this.data.length; i++) {
      if (this.data[i].time === undefined || this.data[i].time.length === 0) {
      } else {
         no_data = false;
      }
   }

   // "empty window" notice
   if (no_data) {
      ctx.font = "16px sans-serif";
      let str = "No data available";
      ctx.strokeStyle = "#404040";
      ctx.fillStyle = "#F0F0F0";
      let w = ctx.measureText(str).width + 10;
      let h = 16 + 10;
      ctx.fillRect((this.x1 + this.x2) / 2 - w / 2, (this.y1 + this.y2) / 2 - h / 2, w, h);
      ctx.strokeRect((this.x1 + this.x2) / 2 - w / 2, (this.y1 + this.y2) / 2 - h / 2, w, h);
      ctx.fillStyle = "#404040";
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(str, (this.x1 + this.x2) / 2, (this.y1 + this.y2) / 2);
      ctx.font = "14px sans-serif";
   }

   // buttons
   if (this.showMenuButtons) {
      let y = 0;
      let buttonSize = 20;
      this.button.forEach(b => {
         b.x1 = this.width - buttonSize - 6;
         b.y1 = 6 + y * (buttonSize + 4);
         b.width = buttonSize + 4;
         b.height = buttonSize + 4;
         b.enabled = true;

         if (b.src === "maximize-2.svg") {
            let s = window.location.href;
            if (s.indexOf("&A") > -1)
               s = s.substr(0, s.indexOf("&A"));
            if (s === encodeURI(this.baseURL + "&group=" + this.group + "&panel=" + this.panel)) {
               b.enabled = false;
               return;
            }
         }

         if (b.src === "corner-down-left.svg") {
            b.x1 = this.x1;
            b.y1 = this.y2;
            if (this.solo.active)
               b.enabled = true;
            else {
               b.enabled = false;
               return;
            }
         }

         if (b.src === "play.svg" && !this.scroll)
            ctx.fillStyle = "#FFC0C0";
         else
            ctx.fillStyle = "#F0F0F0";
         ctx.strokeStyle = "#808080";
         ctx.fillRect(b.x1, b.y1, b.width, b.height);
         ctx.strokeRect(b.x1, b.y1, b.width, b.height);
         ctx.drawImage(b.img, b.x1 + 2, b.y1 + 2);

         y++;
      });
   }

   // zoom buttons
   if (this.showZoomButtons) {
      let xb = this.width - 26 - 40;
      let yb = this.y1 - 20;
      ctx.fillStyle = "#F0F0F0";
      ctx.globalAlpha = 0.5;
      ctx.fillRect(xb, yb, 20, 20);
      ctx.globalAlpha = 1;
      ctx.strokeStyle = "#808080";
      ctx.strokeRect(xb, yb, 20, 20);
      ctx.strokeStyle = "#202020";
      ctx.drawLine(xb + 4, yb + 10, xb + 17, yb + 10);
      ctx.drawLine(xb + 10, yb + 4, xb + 10, yb + 17);

      xb += 20;
      ctx.globalAlpha = 0.5;
      ctx.fillRect(xb, yb, 20, 20);
      ctx.globalAlpha = 1;
      ctx.strokeStyle = "#808080";
      ctx.strokeRect(xb, yb, 20, 20);
      ctx.strokeStyle = "#202020";
      ctx.drawLine(xb + 4, yb + 10, xb + 17, yb + 10);
   }

   // axis zoom
   if (this.zoom.x.active) {
      ctx.fillStyle = "#808080";
      ctx.globalAlpha = 0.2;
      ctx.fillRect(this.zoom.x.x1, this.y2, this.zoom.x.x2 - this.zoom.x.x1, this.y1 - this.y2);
      ctx.globalAlpha = 1;
      ctx.strokeStyle = "#808080";
      ctx.drawLine(this.zoom.x.x1, this.y1, this.zoom.x.x1, this.y2);
      ctx.drawLine(this.zoom.x.x2, this.y1, this.zoom.x.x2, this.y2);
   }
   if (this.zoom.y.active) {
      ctx.fillStyle = "#808080";
      ctx.globalAlpha = 0.2;
      ctx.fillRect(this.x1, this.zoom.y.y1, this.x2 - this.x1, this.zoom.y.y2 - this.zoom.y.y1);
      ctx.globalAlpha = 1;
      ctx.strokeStyle = "#808080";
      ctx.drawLine(this.x1, this.zoom.y.y1, this.x2, this.zoom.y.y1);
      ctx.drawLine(this.x1, this.zoom.y.y2, this.x2, this.zoom.y.y2);
   }

   // marker
   if (this.marker.active) {

      // round marker
      ctx.beginPath();
      ctx.globalAlpha = 0.1;
      ctx.arc(this.marker.x, this.marker.y, 10, 0, 2 * Math.PI);
      ctx.fillStyle = "#000000";
      ctx.fill();
      ctx.globalAlpha = 1;

      ctx.beginPath();
      ctx.arc(this.marker.x, this.marker.y, 4, 0, 2 * Math.PI);
      ctx.fillStyle = "#000000";
      ctx.fill();

      ctx.strokeStyle = "#A0A0A0";
      ctx.drawLine(this.marker.x, this.y1, this.marker.x, this.y2);

      // text label
      let v = this.marker.v;

      let s;
      if (this.param.Label[this.marker.graphIndex] !== "")
         s = this.param.Label[this.marker.graphIndex];
      else
         s = this.param["Variables"][this.marker.graphIndex];

      if (this.param["Show raw value"] !== undefined &&
         this.param["Show raw value"][this.marker.graphIndex])
         s += " (Raw)";

      s += ": " + v.toPrecision(this.yPrecision).stripZeros();

      let w = ctx.measureText(s).width + 6;
      let h = ctx.measureText("M").width * 1.2 + 6;
      let x = this.marker.mx + 20;
      let y = this.marker.my + h / 3 * 2;
      let xl = x;
      let yl = y;

      if (x + w >= this.x2) {
         x = this.marker.x - 20 - w;
         xl = x + w;
      }

      if (y > (this.y1 - this.y2) / 2) {
         y = this.marker.y - h / 3 * 5;
         yl = y + h;
      }

      ctx.strokeStyle = "#808080";
      ctx.fillStyle = "#F0F0F0";
      ctx.textBaseline = "middle";
      ctx.fillRect(x, y, w, h);
      ctx.strokeRect(x, y, w, h);
      ctx.fillStyle = "#404040";
      ctx.fillText(s, x + 3, y + h / 2);

      // vertical line
      ctx.strokeStyle = "#808080";
      ctx.drawLine(this.marker.x, this.marker.y, xl, yl);

      // time label
      s = timeToLabel(this.marker.t, 1, true);
      w = ctx.measureText(s).width + 10;
      h = ctx.measureText("M").width * 1.2 + 11;
      x = this.marker.x - w / 2;
      y = this.y1;
      if (x <= this.x1)
         x = this.x1;
      if (x + w >= this.x2)
         x = this.x2 - w;

      ctx.strokeStyle = "#808080";
      ctx.fillStyle = "#F0F0F0";
      ctx.fillRect(x, y, w, h);
      ctx.strokeRect(x, y, w, h);
      ctx.fillStyle = "#404040";
      ctx.fillText(s, x + 5, y + h / 2);
   }

   this.lastDrawTime = new Date().getTime();

   // profile("Finished draw");

   if (update_last_written) {
      this.updateLastWritten();
   }

   // update URL
   if (this.updateURLTimer !== undefined)
      window.clearTimeout(this.updateURLTimer);

   if (this.plotIndex === 0 && this.floating !== true)
      this.updateURLTimer = window.setTimeout(this.updateURL.bind(this), 10);
};

MhistoryGraph.prototype.drawVAxis = function (ctx, x1, y1, height, minor, major,
                                              text, label, grid, ymin, ymax, logaxis, draw) {
   let dy, int_dy, frac_dy, y_act, label_dy, major_dy, y_screen;
   let tick_base, major_base, label_base, n_sig1, n_sig2, ys;
   let base = [1, 2, 5, 10, 20, 50, 100, 200, 500, 1000];

   if (x1 > 0)
      ctx.textAlign = "right";
   else
      ctx.textAlign = "left";
   ctx.textBaseline = "middle";
   let textHeight = parseInt(ctx.font.match(/\d+/)[0]);

   if (ymax <= ymin || height <= 0)
      return undefined;

   if (!isFinite(ymax - ymin) || ymax == Number.MAX_VALUE) {
      dy = Number.MAX_VALUE / 10;
      label_dy = dy;
      major_dy = dy;
      n_sig1 = 1;
   } else if (logaxis) {
      dy = Math.pow(10, Math.floor(Math.log(ymin) / Math.log(10)));
      if (dy === 0) {
         ymin = 1E-20;
         dy = 1E-20;
      }
      label_dy = dy;
      major_dy = dy * 10;
      n_sig1 = 4;
   } else {
      // use 6 as min tick distance
      dy = (ymax - ymin) / (height / 6);

      int_dy = Math.floor(Math.log(dy) / Math.log(10));
      frac_dy = Math.log(dy) / Math.log(10) - int_dy;

      if (frac_dy < 0) {
         frac_dy += 1;
         int_dy -= 1;
      }

      tick_base = frac_dy < (Math.log(2) / Math.log(10)) ? 1 : frac_dy < (Math.log(5) / Math.log(10)) ? 2 : 3;
      major_base = label_base = tick_base + 1;

      // rounding up of dy, label_dy
      dy = Math.pow(10, int_dy) * base[tick_base];
      major_dy = Math.pow(10, int_dy) * base[major_base];
      label_dy = major_dy;

      // number of significant digits
      if (ymin === 0)
         n_sig1 = 1;
      else
         n_sig1 = Math.floor(Math.log(Math.abs(ymin)) / Math.log(10)) -
            Math.floor(Math.log(Math.abs(label_dy)) / Math.log(10)) + 1;

      if (ymax === 0)
         n_sig2 = 1;
      else
         n_sig2 = Math.floor(Math.log(Math.abs(ymax)) / Math.log(10)) -
            Math.floor(Math.log(Math.abs(label_dy)) / Math.log(10)) + 1;

      n_sig1 = Math.max(n_sig1, n_sig2);
      n_sig1 = Math.max(1, n_sig1);

      // toPrecision displays 1050 with 3 digits as 1.05e+3, so increase precision to number of digits
      if (Math.abs(ymin) < 100000)
         n_sig1 = Math.max(n_sig1, Math.floor(Math.log(Math.abs(ymin)) /
            Math.log(10) + 0.001) + 1);
      if (Math.abs(ymax) < 100000)
         n_sig1 = Math.max(n_sig1, Math.floor(Math.log(Math.abs(ymax)) /
            Math.log(10) + 0.001) + 1);

      // increase label_dy if labels would overlap
      while (label_dy / (ymax - ymin) * height < 1.5 * textHeight) {
         label_base++;
         label_dy = Math.pow(10, int_dy) * base[label_base];
         if (label_base % 3 === 2 && major_base % 3 === 1) {
            major_base++;
            major_dy = Math.pow(10, int_dy) * base[major_base];
         }
      }
   }

   y_act = Math.floor(ymin / dy) * dy;

   let last_label_y = y1;
   let maxwidth = 0;

   if (draw) {
      ctx.strokeStyle = this.color.axis;
      ctx.drawLine(x1, y1, x1, y1 - height);
   }

   do {
      if (logaxis)
         y_screen = y1 - (Math.log(y_act) - Math.log(ymin)) /
            (Math.log(ymax) - Math.log(ymin)) * height;
      else if (!(isFinite(ymax - ymin)))
         y_screen = y1 - ((y_act/ymin) - 1) / ((ymax/ymin) - 1) * height;
      else
         y_screen = y1 - (y_act - ymin) / (ymax - ymin) * height;
      ys = Math.round(y_screen);

      if (y_screen < y1 - height - 0.001 || isNaN(ys))
         break;

      if (y_screen <= y1 + 0.001) {
         if (Math.abs(Math.round(y_act / major_dy) - y_act / major_dy) <
            dy / major_dy / 10.0) {

            if (Math.abs(Math.round(y_act / label_dy) - y_act / label_dy) <
               dy / label_dy / 10.0) {
               // label tick mark
               if (draw) {
                  ctx.strokeStyle = this.color.axis;
                  ctx.drawLine(x1, ys, x1 + text, ys);
               }

               // grid line
               if (grid !== 0 && ys < y1 && ys > y1 - height)
                  if (draw) {
                     ctx.strokeStyle = this.color.grid;
                     ctx.drawLine(x1, ys, x1 + grid, ys);
                  }

               // label
               if (label !== 0) {
                  let str;
                  if (Math.abs(y_act) < 0.001 && Math.abs(y_act) > 1E-20)
                     str = y_act.toExponential(n_sig1).stripZeros();
                  else
                     str = y_act.toPrecision(n_sig1).stripZeros();
                  maxwidth = Math.max(maxwidth, ctx.measureText(str).width);
                  if (draw) {
                     ctx.strokeStyle = this.color.label;
                     ctx.fillStyle = this.color.label;
                     ctx.fillText(str, x1 + label, ys);
                  }
                  last_label_y = ys - textHeight / 2;
               }
            } else {
               // major tick mark
               if (draw) {
                  ctx.strokeStyle = this.color.axis;
                  ctx.drawLine(x1, ys, x1 + major, ys);
               }

               // grid line
               if (grid !== 0 && ys < y1 && ys > y1 - height)
                  if (draw) {
                     ctx.strokeStyle = this.color.grid;
                     ctx.drawLine(x1, ys, x1 + grid, ys);
                  }
            }

            if (logaxis) {
               dy *= 10;
               major_dy *= 10;
               label_dy *= 10;
            }

         } else
            // minor tick mark
         if (draw) {
            ctx.strokeStyle = this.color.axis;
            ctx.drawLine(x1, ys, x1 + minor, ys);
         }

         // for logaxis, also put labels on minor tick marks
         if (logaxis) {
            if (label !== 0) {
               let str;
               if (Math.abs(y_act) < 0.001 && Math.abs(y_act) > 1E-20)
                  str = y_act.toExponential(n_sig1).stripZeros();
               else
                  str = y_act.toPrecision(n_sig1).stripZeros();
               if (ys - textHeight / 2 > y1 - height &&
                  ys + textHeight / 2 < y1 &&
                  ys + textHeight < last_label_y + 2) {
                  maxwidth = Math.max(maxwidth, ctx.measureText(str).width);
                  if (draw) {
                     ctx.strokeStyle = this.color.label;
                     ctx.fillStyle = this.color.label;
                     ctx.fillText(str, x1 + label, ys);
                  }
               }

               last_label_y = ys;
            }
         }
      }

      y_act += dy;

      // suppress 1.23E-17 ...
      if (Math.abs(y_act) < dy / 100)
         y_act = 0;

   } while (1);

   return maxwidth;
};

let options1 = {
   timeZone: 'UTC',
   day: '2-digit', month: 'short', year: '2-digit',
   hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit'
};

let options2 = {
   timeZone: 'UTC',
   day: '2-digit', month: 'short', year: '2-digit',
   hour12: false, hour: '2-digit', minute: '2-digit'
};

let options3 = {
   timeZone: 'UTC',
   day: '2-digit', month: 'short', year: '2-digit',
   hour12: false, hour: '2-digit', minute: '2-digit'
};

let options4 = {
   timeZone: 'UTC',
   day: '2-digit', month: 'short', year: '2-digit'
};

let options5 = {
   timeZone: 'UTC',
   hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit'
};

let options6 = {
   timeZone: 'UTC',
   hour12: false, hour: '2-digit', minute: '2-digit'
};

let options7 = {
   timeZone: 'UTC',
   hour12: false, hour: '2-digit', minute: '2-digit'
};

let options8 = {
   timeZone: 'UTC',
   day: '2-digit', month: 'short', year: '2-digit',
   hour12: false, hour: '2-digit', minute: '2-digit'
};

let options9 = {
   timeZone: 'UTC',
   day: '2-digit', month: 'short', year: '2-digit'
};

function timeToLabel(sec, base, forceDate) {
   let d = mhttpd_get_display_time(sec).date;

   if (forceDate) {
      if (base < 60) {
         return d.toLocaleTimeString('en-GB', options1);
      } else if (base < 600) {
         return d.toLocaleTimeString('en-GB', options2);
      } else if (base < 3600 * 24) {
         return d.toLocaleTimeString('en-GB', options3);
      } else {
         return d.toLocaleDateString('en-GB', options4);
      }
   }

   if (base < 60) {
      return d.toLocaleTimeString('en-GB', options5);
   } else if (base < 600) {
      return d.toLocaleTimeString('en-GB', options6);
   } else if (base < 3600 * 3) {
      return d.toLocaleTimeString('en-GB', options7);
   } else if (base < 3600 * 24) {
      return d.toLocaleTimeString('en-GB', options8);
   } else {
      return d.toLocaleDateString('en-GB', options9);
   }
}

MhistoryGraph.prototype.drawTAxis = function (ctx, x1, y1, width, xr, minor, major,
                                              text, label, grid, xmin, xmax) {
   const base = [1, 5, 10, 60, 2 * 60, 5 * 60, 10 * 60, 15 * 60, 30 * 60, 3600,
      3 * 3600, 6 * 3600, 12 * 3600, 24 * 3600];

   ctx.textAlign = "left";
   ctx.textBaseline = "top";

   if (xmax <= xmin || width <= 0)
      return;

   /* force date display if xmax not today */
   let d1 = new Date(xmax * 1000);
   let d2 = new Date();
   let forceDate = d1.getDate() !== d2.getDate() || (d2 - d1 > 1000 * 3600 * 24);

   /* use 5 pixel as min tick distance */
   let dx = Math.round((xmax - xmin) / (width / 5));

   let tick_base;
   for (tick_base = 0; base[tick_base]; tick_base++) {
      if (base[tick_base] > dx)
         break;
   }
   if (!base[tick_base])
      tick_base--;
   dx = base[tick_base];

   let major_base = tick_base;
   let major_dx = dx;

   let label_base = major_base;
   let label_dx = dx;

   do {
      let str = timeToLabel(xmin, label_dx, forceDate);
      let maxwidth = ctx.measureText(str).width;

      /* increasing label_dx, if labels would overlap */
      if (maxwidth > 0.75 * label_dx / (xmax - xmin) * width) {
         if (base[label_base + 1])
            label_dx = base[++label_base];
         else
            label_dx += 3600 * 24;

         if (label_base > major_base + 1 || !base[label_base + 1]) {
            if (base[major_base + 1])
               major_dx = base[++major_base];
            else
               major_dx += 3600 * 24;
         }

         if (major_base > tick_base + 1 || !base[label_base + 1]) {
            if (base[tick_base + 1])
               dx = base[++tick_base];
            else
               dx += 3600 * 24;
         }

      } else
         break;
   } while (1);

   let d = new Date(xmin * 1000);
   let tz = d.getTimezoneOffset() * 60;

   let x_act = Math.floor((xmin - tz) / dx) * dx + tz;

   ctx.strokeStyle = this.color.axis;
   ctx.drawLine(x1, y1, x1 + width, y1);

   do {
      let xs = ((x_act - xmin) / (xmax - xmin) * width + x1);

      if (xs > x1 + width + 0.001)
         break;

      if (xs >= x1) {
         if ((x_act - tz) % major_dx === 0) {
            if ((x_act - tz) % label_dx === 0) {
               // label tick mark
               ctx.strokeStyle = this.color.axis;
               ctx.drawLine(xs, y1, xs, y1 + text);

               // grid line
               if (grid !== 0 && xs > x1 && xs < x1 + width) {
                  ctx.strokeStyle = this.color.grid;
                  ctx.drawLine(xs, y1, xs, y1 + grid);
               }

               // label
               if (label !== 0) {
                  let str = timeToLabel(x_act, label_dx, forceDate);

                  // if labels at edge, shift them in
                  let xl = xs - ctx.measureText(str).width / 2;
                  if (xl < 0)
                     xl = 0;
                  if (xl + ctx.measureText(str).width >= xr)
                     xl = xr - ctx.measureText(str).width - 1;
                  ctx.strokeStyle = this.color.label;
                  ctx.fillStyle = this.color.label;
                  ctx.fillText(str, xl, y1 + label);
               }
            } else {
               // major tick mark
               ctx.strokeStyle = this.color.axis;
               ctx.drawLine(xs, y1, xs, y1 + major);
            }

            // grid line
            if (grid !== 0 && xs > x1 && xs < x1 + width) {
               ctx.strokeStyle = this.color.grid;
               ctx.drawLine(xs, y1 - 1, xs, y1 + grid);
            }
         } else {
            // minor tick mark
            ctx.strokeStyle = this.color.axis;
            ctx.drawLine(xs, y1, xs, y1 + minor);
         }
      }

      x_act += dx;

   } while (1);
};

MhistoryGraph.prototype.download = function (mode) {

   let leftDate = mhttpd_get_display_time(this.tMin).date;
   let rightDate = mhttpd_get_display_time(this.tMax).date;
   let filename = this.group + "-" + this.panel + "-" +
      leftDate.getFullYear() +
      ("0" + leftDate.getUTCMonth() + 1).slice(-2) +
      ("0" + leftDate.getUTCDate()).slice(-2) + "-" +
      ("0" + leftDate.getUTCHours()).slice(-2) +
      ("0" + leftDate.getUTCMinutes()).slice(-2) +
      ("0" + leftDate.getUTCSeconds()).slice(-2) + "-" +
      rightDate.getFullYear() +
      ("0" + rightDate.getUTCMonth() + 1).slice(-2) +
      ("0" + rightDate.getUTCDate()).slice(-2) + "-" +
      ("0" + rightDate.getUTCHours()).slice(-2) +
      ("0" + rightDate.getUTCMinutes()).slice(-2) +
      ("0" + rightDate.getUTCSeconds()).slice(-2);

   // use trick from FileSaver.js
   let a = document.getElementById('downloadHook');
   if (a === null) {
      a = document.createElement("a");
      a.style.display = "none";
      a.id = "downloadHook";
      document.body.appendChild(a);
   }

   if (mode === "CSV") {
      filename += ".csv";

      let data = "";
      this.param["Variables"].forEach(v => {
         data += "Time,";
         if (this.binned)
            data += v + " MIN," + v + " MAX,";
         else
            data += v + ",";
      });
      data = data.slice(0, -1);
      data += '\n';

      let maxlen = 0;
      let nvar = this.param["Variables"].length;
      for (let index=0 ; index < nvar ; index++)
         if (this.data[index].time.length > maxlen)
            maxlen = this.data[index].time.length;
      let index = [];
      for (let di=0 ; di < nvar ; di++)
         for (let i = 0; i < maxlen; i++) {
            if (i < this.data[di].time.length &&
               this.data[di].time[i] > this.tMin) {
               index[di] = i;
               break;
            }
         }

      for (let i = 0; i < maxlen; i++) {
         let l = "";
         for (let di = 0 ; di < nvar ; di++) {
            if (index[di] < this.data[di].time.length &&
               this.data[di].time[index[di]] > this.tMin && this.data[di].time[index[di]] < this.tMax) {
               if (this.binned) {
                  l += this.data[di].time[index[di]] + ",";

                  if (this.param["Show raw value"] !== undefined &&
                     this.param["Show raw value"][di]) {
                     l += this.data[di].binRaw[index[di]].minValue + ",";
                     l += this.data[di].binRaw[index[di]].maxValue + ",";
                  } else {
                     l += this.data[di].bin[index[di]].minValue + ",";
                     l += this.data[di].bin[index[di]].maxValue + ",";
                  }

               } else {

                  l += this.data[di].time[index[di]] + ",";

                  if (this.param["Show raw value"] !== undefined &&
                     this.param["Show raw value"][di])
                     l += this.data[di].rawValue[index[di]] + ",";
                  else
                     l += this.data[di].value[index[di]] + ",";
               }
            } else {
               l += ",,";
            }
            index[di]++;
         }
         if (l.split(',').some(s => s)) { // don't add if only commas
            l = l.slice(0, -1); // remove last comma
            data += l + '\n';
         }
      }

      let blob = new Blob([data], {type: "text/csv"});
      let url = window.URL.createObjectURL(blob);

      a.href = url;
      a.download = filename;
      a.click();
      window.URL.revokeObjectURL(url);
      dlgAlert("Data downloaded to '" + filename + "'");

   } else if (mode === "PNG") {
      filename += ".png";

      this.showZoomButtons = false;
      this.showMenuButtons = false;
      this.forceRedraw = true;
      this.forceConvert = true;
      this.draw();

      let h = this;
      this.canvas.toBlob(function (blob) {
         let url = window.URL.createObjectURL(blob);

         a.href = url;
         a.download = filename;
         a.click();
         window.URL.revokeObjectURL(url);
         dlgAlert("Image downloaded to '" + filename + "'");

         h.showZoomButtons = true;
         h.showMenuButtons = true;
         h.forceRedraw = true;
         h.forceConvert = true;
         h.draw();

      }, 'image/png');
   } else if (mode === "URL") {
      // Create new element
      let el = document.createElement('textarea');

      // Set value (string to be copied)
      let url = this.baseURL + "&group=" + this.group + "&panel=" + this.panel +
         "&A=" + this.tMin + "&B=" + this.tMax;
      url = encodeURI(url);
      el.value = url;

      // Set non-editable to avoid focus and move outside of view
      el.setAttribute('readonly', '');
      el.style = {position: 'absolute', left: '-9999px'};
      document.body.appendChild(el);
      // Select text inside element
      el.select();
      // Copy text to clipboard
      document.execCommand('copy');
      // Remove temporary element
      document.body.removeChild(el);

      dlgMessage("Info", "URL<br/><br/>" + url + "<br/><br/>copied to clipboard", true, false);
   }

};
