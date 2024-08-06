//
// Name:         mplot.js
// Created by:   Stefan Ritt
//
// Contents:     JavaScript graph plotting routines
//
// Note: please load midas.js, mhttpd.js and control.js before mplot.js
//

let defaultParam = {

   showMenuButtons: true,

   color: {
      background: "#FFFFFF",
      axis: "#808080",
      grid: "#D0D0D0",
      label: "#404040",
      data: [
         "#00AAFF", "#FF9000", "#FF00A0", "#00C030",
         "#A0C0D0", "#D0A060", "#C04010", "#807060",
         "#F0C000", "#2090A0", "#D040D0", "#90B000",
         "#B0B040", "#B0B0FF", "#FFA0A0", "#A0FFA0"],
   },

   title: {
      color: "#000000",
      backgroundColor: "#808080",
      textSize: 24,
      text: ""
   },

   legend: {
      show: true,
      color: "#D0D0D0",
      backgroundColor: "#FFFFFF",
      textColor: "#404040",
      textSize: 16,
   },

   xAxis: {
      log: false,
      min: undefined,
      max: undefined,
      grid: true,
      textSize: 20,
      title: {
         text: "",
         textSize : 20
      }
   },

   yAxis: {
      log: false,
      min: undefined,
      max: undefined,
      grid: true,
      textSize: 20,
      title: {
         text: "",
         textSize : 20
      }
   },

   zAxis: {
      show: true,
      min: undefined,
      max: undefined,
      textSize: 14,
      title: {
         text: "",
         textSize : 20
      }
   },

   plot: [
      {
         type: "scatter",
         odbPath: "",
         x: "",
         y: "",
         label: "",
         alpha: 1,

         marker: {
            draw: true,
            lineColor: 0,
            fillColor: 0,
            style: "circle",
            size: 10,
            lineWidth: 2
         },

         line: {
            draw: true,
            fill: false,
            color: 0,
            style: "solid",
            width: 2
         }
      },
   ],
};

function mplot_init() {
   // go through all data-name="mplot" tags
   let mPlot = document.getElementsByClassName("mplot");

   for (let i = 0; i < mPlot.length; i++)
      mPlot[i].mpg = new MPlotGraph(mPlot[i]);

   loadMPlotData();

   window.addEventListener('resize', windowResize);
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

function windowResize() {
   let mPlot = document.getElementsByClassName("mplot");
   for (const m of mPlot)
      m.mpg.resize();
}

function isObject(item) {
   return (item && typeof item === 'object' && !Array.isArray(item));
}

function deepMerge(target, source) {
   for (let key in source) {
      if (source.hasOwnProperty(key)) {
         if (isObject(source[key])) {
            if (!target[key]) Object.assign(target, { [key]: {} });
            deepMerge(target[key], source[key]);
         } else {
            Object.assign(target, { [key]: source[key] });
         }
      }
   }
   return target;
}

function MPlotGraph(divElement, param) { // Constructor

   // save parameters from <div>
   this.parentDiv = divElement;
   this.divParam = divElement.innerHTML;
   divElement.innerHTML = "";

   // if absent, generate random string (5 char) to give an id to parent element
   if (!this.parentDiv.id)
      this.parentDiv.id = (Math.random() + 1).toString(36).substring(7);

   // default parameters
   this.param = JSON.parse(JSON.stringify(defaultParam)); // deep copy

   // overwrite default parameters from <div> text body
   try {
      if (this.divParam.includes('{')) {
         let p = JSON.parse(this.divParam);
         this.param = deepMerge(this.param, p);
      }
   } catch (error) {
      this.parentDiv.innerHTML = "<pre>" + this.divParam + "</pre>";
      dlgAlert(error);
      return;
   }

   // obtain parameters form <div> attributes ---

   // data-odb-path
   if (this.parentDiv.dataset.odbPath)
      for (let p of this.param.plot)
         p.odbPath = this.parentDiv.dataset.odbPath;

   // data-title
   if (this.parentDiv.dataset.title)
      this.param.title.text = this.parentDiv.dataset.title;

   // data-x/y/z-text
   if (this.parentDiv.dataset.xText)
      this.param.xAxis.title.text =this.parentDiv.dataset.xText;
   if (this.parentDiv.dataset.yText)
      this.param.yAxis.title.text =this.parentDiv.dataset.yText;
   if (this.parentDiv.dataset.zText)
      this.param.zAxis.title.text =this.parentDiv.dataset.zText;

   // data-x/y
   if (this.parentDiv.dataset.x)
      this.param.plot[0].x = this.parentDiv.dataset.x;
   if (this.parentDiv.dataset.y)
      this.param.plot[0].y = this.parentDiv.dataset.y;

   // data-x/y/z-min/max
   if (this.parentDiv.dataset.xMin)
      this.param.xAxis.min = parseFloat(this.parentDiv.dataset.xMin);
   if (this.parentDiv.dataset.xMax)
      this.param.xAxis.max = parseFloat(this.parentDiv.dataset.xMax);
   if (this.parentDiv.dataset.yMin)
      this.param.yAxis.min = parseFloat(this.parentDiv.dataset.yMin);
   if (this.parentDiv.dataset.yMax)
      this.param.yAxis.max = parseFloat(this.parentDiv.dataset.yMax);
   if (this.parentDiv.dataset.zMin)
      this.param.zAxis.min = parseFloat(this.parentDiv.dataset.zMin);
   if (this.parentDiv.dataset.zMax)
      this.param.zAxis.max = parseFloat(this.parentDiv.dataset.zMax);

   // data-x/y-log
   if (this.parentDiv.dataset.xLog)
      this.param.xAxis.log = this.parentDiv.dataset.xLog === "true" || this.parentDiv.dataset.xLog === "1";
   if (this.parentDiv.dataset.yLog)
      this.param.yAxis.log = this.parentDiv.dataset.yLog === "true" || this.parentDiv.dataset.yLog === "1";

   // data-h
   if (this.parentDiv.dataset.h) {
      this.param.plot[0].type = "histogram";
      this.param.plot[0].y = this.parentDiv.dataset.h;
      this.param.plot[0].line.color = "#404040";
      if (!this.parentDiv.dataset.x) {
         this.param.plot[0].xMin = this.param.xAxis.min;
         this.param.plot[0].xMax = this.param.xAxis.max;
      }
   }

   // data-z
   if (this.parentDiv.dataset.z) {
      this.param.plot[0].type = "colormap";
      this.param.plot[0].showZScale = true;
      this.param.plot[0].z = this.parentDiv.dataset.z;
      this.param.plot[0].xMin = this.param.xAxis.min;
      this.param.plot[0].xMax = this.param.xAxis.max;
      this.param.plot[0].yMin = this.param.yAxis.min;
      this.param.plot[0].yMax = this.param.yAxis.max;
      this.param.plot[0].zMin = this.param.zAxis.min;
      this.param.plot[0].zMax = this.param.zAxis.max;
      this.param.plot[0].nx = parseInt(this.parentDiv.dataset.nx);
      this.param.plot[0].ny = parseInt(this.parentDiv.dataset.ny);
      if (this.param.plot[0].nx === undefined) {
         dlgAlert("\"data-nx\" missing for colormap mplot <div>");
         return;
      }
      if (this.param.plot[0].ny === undefined) {
         dlgAlert("\"data-ny\" missing for colormap mplot <div>");
         return;
      }
   }

   // data-x<n>/y<n>/label<n>/alpha<n>
   for (let i=0 ; i<16 ; i++) {
      let index = 0;
      if (this.parentDiv.dataset["x"+i]) {
         if (this.param.plot[0].x !== "") {
            this.param.plot.push(JSON.parse(JSON.stringify(defaultParam.plot[0])));
            index = this.param.plot.length-1;
            this.param.plot[index].marker.lineColor = index;
            this.param.plot[index].marker.fillColor = index;
            this.param.plot[index].line.color = index;
         }
         this.param.plot[index].x = this.parentDiv.dataset["x" + i];
      }
      if (this.parentDiv.dataset["y"+i])
         this.param.plot[index].y = this.parentDiv.dataset["y"+i];
      if (this.parentDiv.dataset["label"+i])
         this.param.plot[index].label = this.parentDiv.dataset["label"+i];
      if (this.parentDiv.dataset["alpha"+i])
         this.param.plot[index].alpha = parseFloat(this.parentDiv.dataset["alpha"+i]);
      this.param.plot[index].odbPath = this.param.plot[0].odbPath;
   }

   // data-h<n>
   for (let i=0 ; i<16 ; i++) {
      let index = 0;
      if (this.parentDiv.dataset["h"+i]) {
         if (this.param.plot[0].y !== "") {
            this.param.plot.push(JSON.parse(JSON.stringify(defaultParam.plot[0])));
            index = this.param.plot.length-1;
            this.param.plot[index].marker.lineColor = index;
            this.param.plot[index].marker.fillColor = index;
            this.param.plot[index].line.color = index;
         }
         this.param.plot[index].type = "histogram";
         this.param.plot[index].y = this.parentDiv.dataset["h" + i];

         this.param.plot[index].xMin = this.param.xAxis.min;
         this.param.plot[index].xMax = this.param.xAxis.max;

         if (this.parentDiv.dataset["label"+i])
            this.param.plot[index].label = this.parentDiv.dataset["label"+i];
         this.param.plot[index].odbPath = this.param.plot[0].odbPath;
      }
   }

   // data-overlay
   if (this.parentDiv.dataset.overlay) {
      this.param.overlay = this.parentDiv.dataset.overlay;
      if (this.param.overlay.indexOf('(') !== -1) // strip any '('
         this.param.overlay = this.param.overlay.substring(0, this.param.overlay.indexOf('('));
   }

   // set parameters from constructor
   if (param) {
      this.param.plot[0] = deepMerge(this.param.plot[0], param);
      if (this.param.plot[0].type === "colormap") {
         this.calcMinMax();

         if (this.param.plot[0].nx === undefined) {
            dlgAlert("\"nx\" missing in param for colormap mplot <div>");
            return;
         }
         if (this.param.plot[0].ny === undefined) {
            dlgAlert("\"ny\" missing in param for colormap mplot <div>");
            return;
         }
      }
   }

   // dragging
   this.drag = {
      active: false,
      sxStart: 0,
      syStart: 0,
      xStart: 0,
      yStart: 0,
      xMinStart: 0,
      xMaxStart: 0,
      yMinStart: 0,
      yMaxStart: 0,
   };

   // axis zoom
   this.zoom = {
      x: {active: false},
      y: {active: false}
   };

   // marker
   this.marker = {active: false};
   this.blockAutoScale = false;

   this.error = null;

   // buttons
   this.button = [
      {
         src: "rotate-ccw.svg",
         title: "Reset histogram axes",
         click: function (t) {
            t.resetAxes();
         }
      },
      {
         src: "download.svg",
         title: "Download image/data...",
         click: function (t) {
            if (t.downloadSelector.style.display === "none") {
               t.downloadSelector.style.display = "block";
               let w = t.downloadSelector.getBoundingClientRect().width;
               t.downloadSelector.style.left = (t.canvas.getBoundingClientRect().x + window.scrollX +
                  t.width - 26 - w) + "px";
               t.downloadSelector.style.top = (t.canvas.getBoundingClientRect().y + window.scrollY +
                  this.y1) + "px";
               t.downloadSelector.style.zIndex = "32";
            } else {
               t.downloadSelector.style.display = "none";
            }
         }
      },
   ];

   this.button.forEach(b => {
      b.img = new Image();
      b.img.src = "icons/" + b.src;
   });

   this.createDownloadSelector();

   // mouse event handlers
   divElement.addEventListener("mousedown", this.mouseEvent.bind(this), true);
   divElement.addEventListener("dblclick", this.mouseEvent.bind(this), true);
   divElement.addEventListener("mousemove", this.mouseEvent.bind(this), true);
   divElement.addEventListener("mouseup", this.mouseEvent.bind(this), true);
   divElement.addEventListener("wheel", this.mouseEvent.bind(this), true);

   // Keyboard event handler (has to be on the window!)
   window.addEventListener("keydown", this.keyDown.bind(this));

   // create canvas
   this.canvas = document.createElement("canvas");
   this.canvas.style.border = "1px solid black";

   if (parseInt(this.parentDiv.style.width) > 0)
      this.canvas.width = parseInt(this.parentDiv.style.width);
   else
      this.canvas.width = 500;
   if (parseInt(this.parentDiv.style.height) > 0)
      this.canvas.height = parseInt(this.parentDiv.style.height);
   else
      this.canvas.height = 300;

   divElement.appendChild(this.canvas);
}

MPlotGraph.prototype.createDownloadSelector = function () {
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

   let table = document.createElement("table");
   let mhg = this;

   let row = document.createElement("tr");
   let cell = document.createElement("td");
   cell.style.padding = "0";
   let link = document.createElement("a");
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
}

MPlotGraph.prototype.keyDown = function (e) {

   if (e.key === "r" && !e.ctrlKey && !e.metaKey) {  // 'r' key
      this.resetAxes();
      e.preventDefault();
   }
}

function loadMPlotData() {

   // go through all data-name="mplot" tags
   let mPlot = document.getElementsByClassName("mplot");

   let v = [];
   for (const mp of mPlot) {
      for (const pl of mp.mpg.param.plot) {
         if (pl.odbPath === undefined || pl.odbPath === "")
            continue;

         let name = pl.label;
         if (name === "")
            name = mp.id;

         if ((pl.type === "scatter" || pl.type === "histogram") &&
            (pl.y === undefined || pl.y === null || pl.y === "")) {
            mp.mpg.error ="Invalid Y data \"" + pl.y + "\" for " + pl.type + " plot \"" + name+ "\"";
            mp.mpg.draw();
            pl.invalid = true;
            continue;
         }

         if ((pl.type === "colormap") &&
            (pl.z === undefined || pl.z === null || pl.z === "")) {
            mp.mpg.error = "Invalid Z data \"" + pl.y + "\" for colormap plot \"" + name + "\"";
            mp.mpg.draw();
            pl.invalid = true;
            continue;
         }

         if (pl.odbPath.slice(-1) !== '/')
            pl.odbPath += '/';

         if (pl.x !== undefined && pl.x !== null && pl.x !== "")
            v.push(pl.odbPath + pl.x);
         if (pl.y !== undefined && pl.y !== null && pl.y !== "")
            v.push(pl.odbPath + pl.y);
         if (pl.z !== undefined && pl.z !== null && pl.z !== "")
            v.push(pl.odbPath + pl.z);
      }
   }

   mjsonrpc_db_get_values(v).then( function(rpc) {

      let mPlot = document.getElementsByClassName("mplot");
      let i = 0;
      for (let mp of mPlot) {
         for (let p of mp.mpg.param.plot) {
            if (!p.odbPath === undefined || p.odbPath === "" || p.invalid)
               continue;

            let name = p.label;
            if (name === "")
               name = mp.id;

            if (p.x !== undefined && p.x !== null && p.x !== "") {
               p.xData = rpc.result.data[i++];
               if (p.xData === null)
                  mp.mpg.error = "Invalid X data \"" + p.x + "\" for scatter plot \"" + name + "\"";
            }
            if (p.y !== undefined && p.y !== null && p.y !== "") {
               p.yData = rpc.result.data[i++];
               if (p.yData === null)
                  mp.mpg.error = "Invalid Y data \"" + p.y + "\" for scatter plot \"" + name + "\"";
            }
            if (p.z !== undefined && p.z !== null && p.z !== "") {
               p.zData = rpc.result.data[i++];
               if (p.zData === null)
                  mp.mpg.error = "Invalid Z data \"" + p.z + "\" for scatter plot \"" + name + "\"";
            }

            if ((p.type === "scatter" || p.type === "histogram") && mp.mpg.error === null) {
               // generate X data for histograms
               if (p.xData === undefined || p.xData === null) {

                  if (p.type === "scatter") {
                     // scatter plot goes from 0 ... N
                     p.xMin = 0;
                     p.xMax = p.yData.length;
                     p.xData = Array.from({length: p.yData.length}, (v, i) => i);
                  } else {
                     // histogram goes from -0.5 ... N-0.5 to have bins centered over bin x-value
                     p.xMin = -0.5;
                     p.xMax = p.yData.length - 0.5;

                     let dx = (p.xMax - p.xMin) / p.yData.length;
                     let x0 = p.xMin + dx / 2;
                     p.xData = Array.from({length: p.yData.length}, (v, i) => x0 + i * dx);
                  }
               } else {
                  if (p.xMin === undefined) {
                     p.xMin = Math.min(...p.xData);
                     p.xMax = Math.max(...p.xData);
                  }
               }

               p.yMin = Math.min(...p.yData);
               p.yMax = Math.max(...p.yData);
            }

            if (p.type === "colormap" && mp.mpg.error === null) {
               p.zMin = Math.min(...p.zData);
               p.zMax = Math.max(...p.zData);

               if (p.xMin === undefined) {
                  p.xMin = -0.5;
                  p.xMax = p.nx - 0.5;
               }
               if (p.yMin === undefined) {
                  p.yMin = -0.5;
                  p.yMax = p.ny - 0.5;
               }

               let dx = (p.xMax - p.xMin) / p.nx;
               let x0 = p.xMin + dx/2;
               p.xData = Array.from({length: p.nx}, (v,i) => x0 + i*dx);

               let dy = (p.yMax - p.yMin) / p.ny;
               let y0 = p.yMin + dy/2;
               p.yData = Array.from({length: p.ny}, (v,i) => y0 + i*dy);
            }
         }
      }

      for (const mp of mPlot) {
         if (!mp.mpg.blockAutoScale)
            mp.mpg.calcMinMax();
         mp.mpg.redraw();
      }

      // refresh data once per second
      window.setTimeout(loadMPlotData, 1000);

   }).catch( (error) => {
      dlgAlert(error)
   });
}

MPlotGraph.prototype.setData = function (index, x, y) {

   if (index > this.param.plot.length) {
      dlgAlert("Wrong index \"" + index + "\" for graph \""+ this.param.title.text +"\"<br />" +
         "New index must be \"" + this.param.plot.length + "\"");
      return;
   }

   let p = this.param.plot[index];
   p.odbPath = ""; // prevent loading of ODB data

   if (index + 1 > this.param.plot.length) {
      // add new default plot
      this.param.plot.push(JSON.parse(JSON.stringify(defaultParam.plot[0])));
      p = this.param.plot[index];
      p.marker.lineColor = index;
      p.marker.fillColor = index;
      p.line.color = index;
   } else
      p = this.param.plot[index];

   if (p.type === "colormap") {
      p.zData = x; // 2D array of colormap plot

      p.zMin = Math.min(...p.zData);
      p.zMax = Math.max(...p.zData);

      if (p.xMin === undefined) {
         p.xMin = -0.5;
         p.xMax = p.nx - 0.5;
      }
      if (p.yMin === undefined) {
         p.yMin = -0.5;
         p.yMax = p.ny - 0.5;
      }

      let dx = (p.xMax - p.xMin) / p.nx;
      let x0 = p.xMin + dx/2;
      p.xData = Array.from({length: p.nx}, (v,i) => x0 + i*dx);

      let dy = (p.yMax - p.yMin) / p.ny;
      let y0 = p.yMin + dy/2;
      p.yData = Array.from({length: p.ny}, (v,i) => y0 + i*dy);
   }


   if (p.type === "histogram") {
      p.yData = x;
      p.line.color = "#404040";
      // generate X data for histograms
      if (p.xMin === undefined || p.xMax === undefined) {
         p.xMin = -0.5;
         p.xMax = p.yData.length - 0.5;
      }
      let dx = (p.xMax - p.xMin) / p.yData.length;
      let x0 = p.xMin + dx/2;
      if (p.xData === undefined || p.xData === null)
         p.xData = Array.from({length: p.yData.length}, (v,i) => x0 + i*dx);

      p.yMin = Math.min(...p.yData);
      p.yMax = Math.max(...p.yData);
   }

   if (p.type === "scatter" ) {
      p.xData = x;
      p.yData = y;
      p.xMin = Math.min(...p.xData);
      p.xMax = Math.max(...p.xData);
      p.yMin = Math.min(...p.yData);
      p.yMax = Math.max(...p.yData);
   }
   
   if (!this.blockAutoScale) {
      this.calcMinMax();
   }
   
   this.redraw();
}

MPlotGraph.prototype.resize = function () {
   this.canvas.width = this.parentDiv.clientWidth;
   this.canvas.height = this.parentDiv.clientHeight;

   this.redraw();
}

MPlotGraph.prototype.redraw = function () {
   let f = this.draw.bind(this);
   window.requestAnimationFrame(f);
}

MPlotGraph.prototype.xToScreen = function (x) {
   if (this.param.xAxis.log) {
      if (x <= 0)
         return this.x1;
      else
         return this.x1 + (Math.log(x) - Math.log(this.xMin)) /
            (Math.log(this.xMax) - Math.log(this.xMin)) * (this.x2 - this.x1);
   }
   return this.x1 + (x - this.xMin) / (this.xMax - this.xMin) * (this.x2 - this. x1);
}

MPlotGraph.prototype.yToScreen = function (y) {
   if (this.param.yAxis.log) {
      if (y <= 0)
         return this.y1;
      else
         return this.y1 - (Math.log(y) - Math.log(this.yMin)) /
            (Math.log(this.yMax) - Math.log(this.yMin)) * (this.y1 - this.y2);
   }
   return this.y1 - (y - this.yMin) / (this.yMax - this.yMin) * (this.y1 - this. y2);
}

MPlotGraph.prototype.screenToX = function (x) {
   if (this.param.xAxis.log) {
      let xl = (x - this.x1) / (this.x2 - this.x1) * (Math.log(this.xMax)-Math.log(this.xMin)) + Math.log(this.xMin);
      return Math.exp(xl);
   }
   return (x - this.x1) / (this.x2 - this.x1) * (this.xMax - this.xMin) + this.xMin;
};

MPlotGraph.prototype.screenToY = function (y) {
   if (this.param.yAxis.log) {
      let yl = (this.y1 - y) / (this.y1 - this.y2) * (Math.log(this.yMax)-Math.log(this.yMin)) + Math.log(this.yMin);
      return Math.exp(yl);
   }
   return (this.y1 - y) / (this.y1 - this.y2) * (this.yMax - this.yMin) + this.yMin;
};

MPlotGraph.prototype.calcMinMax = function () {

   // simple nx / ny for colormaps
   if (this.param.plot[0].type === "colormap") {
      this.nx = this.param.plot[0].nx;
      this.ny = this.param.plot[0].ny;

      if (this.param.zAxis.min !== undefined)
         this.zMin = this.param.zAxis.min;
      else
         this.zMin = this.param.plot[0].zMin;

      if (this.param.zAxis.max !== undefined)
         this.zMax = this.param.zAxis.max;
      else
         this.zMax = this.param.plot[0].zMax;

      this.xMin = this.param.plot[0].xMin;
      this.xMax = this.param.plot[0].xMax;
      this.yMin = this.param.plot[0].yMin;
      this.yMax = this.param.plot[0].yMax;

      this.xMin0 = this.xMin;
      this.xMax0 = this.xMax;
      this.yMin0 = this.yMin;
      this.yMax0 = this.yMax;
      return;
   }

   // determine min/max of overall plot
   let xMin = this.param.plot[0].xMin;
   for (const p of this.param.plot)
      if (p.xMin < xMin)
         xMin = p.xMin;
   if (this.param.xAxis.min !== undefined)
      xMin = this.param.xAxis.min;

   let xMax = this.param.plot[0].xMax;
   for (const p of this.param.plot)
      if (p.xMax > xMax)
         xMax = p.xMax;
   if (this.param.xAxis.max !== undefined)
      xMax = this.param.xAxis.max;

   let yMin = this.param.plot[0].yMin;
   for (const p of this.param.plot)
      if (p.yMin < yMin)
         yMin = p.yMin;
   if (this.param.yAxis.min !== undefined)
      yMin = this.param.yAxis.min;

   let yMax = this.param.plot[0].yMax;
   for (const p of this.param.plot)
      if (p.yMax > yMax)
         yMax = p.yMax;
   if (this.param.yAxis.max !== undefined)
      yMax = this.param.yAxis.max;

   // avoid min === max
   if (xMin === xMax) { xMin -= 0.5; xMax += 0.5; }
   if (yMin === yMax) { yMin -= 0.5; yMax += 0.5; }

   // add 5% on each side
   let dx = (xMax - xMin);
   let dy = (yMax - yMin);
   if (this.param.plot[0].type !== "histogram") {
      if (this.param.xAxis.min === undefined)
         xMin -= dx / 20;
      if (this.param.xAxis.max === undefined)
         xMax += dx / 20;
      if (this.param.yAxis.min === undefined)
         yMin -= dy / 20;
   }
   if (this.param.yAxis.max === undefined)
      yMax += dy / 20;

   this.xMin = xMin;
   this.xMax = xMax;
   this.yMin = yMin;
   this.yMax = yMax;

   this.xMin0 = xMin;
   this.xMax0 = xMax;
   this.yMin0 = yMin;
   this.yMax0 = yMax;
}

MPlotGraph.prototype.drawMarker = function(ctx, p, x, y) {
   if (typeof p.marker.lineColor === "string")
      ctx.strokeStyle = p.marker.lineColor;
   else if (typeof p.marker.lineColor === "number")
      ctx.strokeStyle = this.param.color.data[p.marker.lineColor];

   if (typeof p.marker.fillColor === "string")
      ctx.fillStyle = p.marker.fillColor;
   else if (typeof p.marker.fillColor === "number")
      ctx.fillStyle = this.param.color.data[p.marker.fillColor];

   let size = p.marker.size;
   ctx.lineWidth = p.marker.lineWidth;

   switch(p.marker.style) {
      case "circle":
         ctx.beginPath();
         ctx.arc(x, y, size / 2, 0, 2 * Math.PI);
         ctx.fill();
         ctx.stroke();
         break;
      case "square":
         ctx.fillRect(x - size / 2, y - size / 2, size, size);
         ctx.strokeRect(x - size / 2, y - size / 2, size, size);
         break;
      case "diamond":
         ctx.beginPath();
         ctx.moveTo(x, y - size / 2);
         ctx.lineTo(x + size / 2, y);
         ctx.lineTo(x, y + size / 2);
         ctx.lineTo(x - size / 2, y);
         ctx.lineTo(x, y - size / 2);
         ctx.fill();
         ctx.stroke();
         break;
      case "pentagon":
         ctx.beginPath();
         ctx.moveTo(x + size * 0.00, y - size * 0.50);
         ctx.lineTo(x + size * 0.48, y - size * 0.16);
         ctx.lineTo(x + size * 0.30, y + size * 0.41);
         ctx.lineTo(x - size * 0.30, y + size * 0.41);
         ctx.lineTo(x - size * 0.48, y - size * 0.16);
         ctx.lineTo(x + size * 0.00, y - size * 0.50);
         ctx.fill();
         ctx.stroke();
         break;
      case "triangle-up":
         ctx.beginPath();
         ctx.moveTo(x, y - size / 2);
         ctx.lineTo(x + size / 2, y + size / 2);
         ctx.lineTo(x - size / 2, y + size / 2);
         ctx.lineTo(x, y - size / 2);
         ctx.fill();
         ctx.stroke();
         break;
      case "triangle-down":
         ctx.beginPath();
         ctx.moveTo(x, y + size / 2);
         ctx.lineTo(x + size / 2, y - size / 2);
         ctx.lineTo(x - size / 2, y - size / 2);
         ctx.lineTo(x, y + size / 2);
         ctx.fill();
         ctx.stroke();
         break;
      case "triangle-left":
         ctx.beginPath();
         ctx.moveTo(x - size / 2, y);
         ctx.lineTo(x + size / 2, y - size / 2);
         ctx.lineTo(x + size / 2, y + size / 2);
         ctx.lineTo(x - size / 2, y);
         ctx.fill();
         ctx.stroke();
         break;
      case "triangle-right":
         ctx.beginPath();
         ctx.moveTo(x + size / 2, y);
         ctx.lineTo(x - size / 2, y - size / 2);
         ctx.lineTo(x - size / 2, y + size / 2);
         ctx.lineTo(x + size / 2, y);
         ctx.fill();
         ctx.stroke();
         break;
      case "cross":
         ctx.beginPath();
         ctx.moveTo(x - size / 2, y - size / 2);
         ctx.lineTo(x + size / 2, y + size / 2);
         ctx.moveTo(x - size / 2, y + size / 2);
         ctx.lineTo(x + size / 2, y - size / 2);
         ctx.stroke();
         break;
      case "plus":
         ctx.beginPath();
         ctx.moveTo(x - size / 2, y);
         ctx.lineTo(x + size / 2, y);
         ctx.moveTo(x, y + size / 2);
         ctx.lineTo(x, y - size / 2);
         ctx.stroke();
         break;
   }
}

MPlotGraph.prototype.draw = function () {

   //profile();

   if (!this.canvas)
      return;

   let ctx = this.canvas.getContext("2d");

   this.width = this.canvas.width;
   this.height = this.canvas.height;

   ctx.fillStyle = this.param.color.background;
   ctx.fillRect(0, 0, this.width, this.height);

   if (this.error !== null) {
      ctx.lineWidth = 1;
      ctx.font = "14px sans-serif";
      ctx.strokeStyle = "#808080";
      ctx.fillStyle = "#808080";
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(this.error, this.width / 2, this.height / 2);
      return;
   }

   if (this.param.plot[0].xData === undefined) {
      ctx.lineWidth = 1;
      ctx.font = "14px sans-serif";
      ctx.strokeStyle = "#808080";
      ctx.fillStyle = "#808080";
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText("No data-odb-path present and no setData() called", this.width / 2, this.height / 2);
      return;
   }

   if (this.height === undefined || this.width === undefined)
      return;
   if (this.param.plot[0].xMin === undefined || this.param.plot[0].xMax === undefined)
      return;

   ctx.font = this.param.yAxis.textSize + "px sans-serif";

   let axisLabelWidth = this.drawYAxis(ctx, 50, this.height - 25, this.height - 35,
      -4, -7, -10, -12, 0, this.yMin, this.yMax, this.param.yAxis.log, false);

   if (axisLabelWidth === undefined)
      return;

   if (this.param.yAxis.title.text && this.param.yAxis.title.text !== "")
      this.x1 = axisLabelWidth + 5 + 2.5*this.param.yAxis.title.textSize;
   else
      this.x1 = axisLabelWidth + 15;

   if (this.param.xAxis.title.text && this.param.xAxis.title.text !== "")
      this.y1 = this.height - this.param.xAxis.textSize - 1.5*this.param.xAxis.title.textSize - 10;
   else
      this.y1 = this.height - this.param.xAxis.textSize - 10;

   this.x2 = this.param.showMenuButtons ? this.width - 30 : this.width - 2;
   if (this.param.zAxis.title.text && this.param.zAxis.title.text !== "")
      this.x2 -= 1.0*this.param.zAxis.title.textSize;

   this.y2 = 6;

   if (this.param.showMenuButtons === false)
      this.x2 = this.width - 2;

   if (this.param.plot[0].type === "colormap" && this.param.plot[0].showZScale) {

      ctx.font = this.param.zAxis.textSize + "px sans-serif";
      axisLabelWidth = this.drawYAxis(ctx, this.x2 + 30, this.y1, this.y1 - this.y2,
         4, 7, 10, 12, 0, this.zMin, this.zMax, false, false);
      if (axisLabelWidth === undefined)
         return;

      if (this.param.zAxis.show) {
         let w = 5;  // left gap
         w += 10;    // color bar
         w += 12;    // tick width
         w += 5;

         this.x2 -= axisLabelWidth + w;
         this.param.zAxis.width = axisLabelWidth + w;
      }
   }

   // title
   if (this.param.title.text !== "") {
      ctx.strokeStyle = this.param.color.axis;
      ctx.fillStyle = "#F0F0F0";
      ctx.font = this.param.title.textSize + "px sans-serif";
      let h = this.param.title.textSize * 1.2;
      ctx.strokeRect(this.x1, 6, this.x2 - this.x1, h);
      ctx.fillRect(this.x1, 6, this.x2 - this.x1, h);
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillStyle = this.param.title.color;
      ctx.fillText(this.param.title.text, (this.x2 + this.x1) / 2, 6 + h/2);
      this.y2 = 6 + h;
   }

   // draw axis
   ctx.strokeStyle = this.param.color.axis;
   ctx.drawLine(this.x1, this.y2, this.x2, this.y2);
   ctx.drawLine(this.x2, this.y2, this.x2, this.y1);

   if (this.param.yAxis.log && this.yMin < 1E-20)
      this.yMin = 1E-20;
   if (this.param.yAxis.log && this.yMax < 1E-18)
      this.yMax = 1E-18;

   if (this.param.xAxis.title.text && this.param.xAxis.title.text !== "") {
      ctx.save();
      ctx.fillStyle = this.param.title.color;
      let s = this.param.xAxis.title.textSize;
      ctx.font = s + "px sans-serif";
      ctx.textAlign = "center";
      ctx.textBaseline = "top";
      ctx.fillText(this.param.xAxis.title.text, (this.x1 + this.x2)/2,
         this.y1 + this.param.xAxis.textSize + 10 + this.param.xAxis.title.textSize / 4);
      ctx.restore();
   }

   ctx.font = this.param.xAxis.textSize + "px sans-serif";
   let grid = this.param.xAxis.grid ? this.y2 - this.y1 : 0;
   this.drawXAxis(ctx, this.x1, this.y1, this.x2 - this.x1,
      4, 7, 10, 10, grid, this.xMin, this.xMax, this.param.xAxis.log);

   if (this.param.yAxis.title.text && this.param.yAxis.title.text !== "") {
      ctx.save();
      ctx.fillStyle = this.param.title.color;
      let s = this.param.yAxis.title.textSize;
      ctx.translate(s / 2, (this.y1 + this.y2) / 2);
      ctx.rotate(-Math.PI / 2);
      ctx.font = s + "px sans-serif";
      ctx.textAlign = "center";
      ctx.textBaseline = "top";
      ctx.fillText(this.param.yAxis.title.text, 0, 0);
      ctx.restore();
   }

   if (this.param.zAxis.title.text && this.param.zAxis.title.text !== "") {
      ctx.save();
      ctx.fillStyle = this.param.title.color;
      let s = this.param.zAxis.title.textSize;
      ctx.translate(s / 2, (this.y1 + this.y2) / 2);
      ctx.rotate(-Math.PI / 2);
      ctx.font = s + "px sans-serif";
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(this.param.zAxis.title.text, 0, this.x2 + this.param.zAxis.width);
      ctx.restore();
   }

   ctx.font = this.param.yAxis.textSize + "px sans-serif";
   grid = this.param.yAxis.grid ? this.x2 - this.x1 : 0;
   this.drawYAxis(ctx, this.x1, this.y1, this.y1 - this.y2,
      -4, -7, -10, -12, grid, this.yMin, this.yMax, this.param.yAxis.log, true);

   if (this.param.yAxis.title.text && this.param.yAxis.title.text !== "") {
      ctx.save();
      let s = this.param.yAxis.title.textSize;
      ctx.translate(s / 2, (this.y1 + this.y2) / 2);
      ctx.rotate(-Math.PI / 2);
      ctx.font = s + "px sans-serif";
      ctx.textAlign = "center";
      ctx.textBaseline = "top";
      ctx.fillText(this.param.yAxis.title.text, 0, 0);
      ctx.restore();
   }

   ctx.save();
   ctx.rect(this.x1, this.y2, this.x2 - this.x1, this.y1 - this.y2);
   ctx.clip();

   // draw graphs
   let noData = true;
   for (const p of this.param.plot) {
      if (p.xData === undefined || p.xData === null)
         continue;

      if (p.xData.length > 0)
         noData = false;

      ctx.globalAlpha = p.alpha;

      if (p.type === "scatter") {
         // draw lines
         if (p.line && p.line.draw ||
            p.line && p.line.fill) {

            if (typeof p.line.color === "string")
               ctx.fillStyle = p.line.color;
            else if (typeof p.line.color === "number")
               ctx.fillStyle = this.param.color.data[p.line.color];
            ctx.strokeStyle = ctx.fillStyle;

            // shaded area
            if (p.line.fill) {
               ctx.globalAlpha = 0.1;
               ctx.beginPath();
               ctx.moveTo(this.xToScreen(p.xData[0]), this.yToScreen(0));
               for (let i = 0; i < p.xData.length; i++) {
                  let x = this.xToScreen(p.xData[i]);
                  let y = this.yToScreen(p.yData[i]);
                  ctx.lineTo(x, y);
               }
               ctx.lineTo(this.xToScreen(p.xData[p.xData.length - 1]), this.yToScreen(0));
               ctx.lineTo(this.xToScreen(p.xData[0]), this.yToScreen(0));
               ctx.fill();
               ctx.globalAlpha = 1;
            }

            // draw line
            if (p.line.draw) {
               ctx.lineWidth = p.line.width;
               ctx.beginPath();
               for (let i = 0; i < p.xData.length; i++) {
                  let x = this.xToScreen(p.xData[i]);
                  let y = this.yToScreen(p.yData[i]);
                  if (i === 0)
                     ctx.moveTo(x, y);
                  else
                     ctx.lineTo(x, y);
               }
               ctx.stroke();
            }
         }

         // draw markers
         if (p.marker && p.marker.draw) {
            for (let i = 0; i < p.xData.length; i++) {

               let x = this.xToScreen(p.xData[i]);
               let y = this.yToScreen(p.yData[i]);

               this.drawMarker(ctx, p, x, y);
            }
         }
      }

      else if (p.type === "histogram") {
         let x, y;
         let dx = (p.xMax - p.xMin) / p.xData.length;
         let dxs = dx / (this.xMax - this.xMin) * (this.x2 - this. x1);

         if (p.length < 100)
            ctx.lineWidth = 2;
         else
            ctx.lineWidth = 1;

         if (typeof p.line.color === "string")
            ctx.fillStyle = p.line.color;
         else if (typeof p.line.color === "number")
            ctx.fillStyle = this.param.color.data[p.line.color];
         ctx.strokeStyle = ctx.fillStyle;

         ctx.beginPath();
         ctx.moveTo(this.xToScreen(p.xData[0])-dxs/2, this.yToScreen(0));
         for (let i = 0; i < p.xData.length; i++) {
            x = this.xToScreen(p.xData[i]);
            y = this.yToScreen(p.yData[i]);
            ctx.lineTo(x-dxs/2, y);
            ctx.lineTo(x+dxs/2, y);
         }
         ctx.lineTo(x+dxs/2, this.yToScreen(0));
         ctx.globalAlpha = 0.2;
         ctx.fill();
         ctx.globalAlpha = 1;
         ctx.stroke();
      }

      else if (p.type === "colormap") {
         let dx = (p.xMax - p.xMin) / this.nx;
         let dy = (p.yMax - p.yMin) / this.ny;

         let dxs = dx / (this.xMax - this.xMin) * (this.x2 - this. x1);
         let dys = dy / (this.yMax - this.yMin) * (this.y2 - this. y1);

         for (let i=0 ; i<p.ny ; i++) {
            for (let j=0 ; j<p.nx ; j++) {
               let x = this.xToScreen(j * dx + p.xMin);
               let y = this.yToScreen(i * dy + p.yMin);
               let v = this.param.plot[0].zData[j+i*p.nx] / this.zMax;
               ctx.fillStyle = 'hsl(' + Math.floor((1 - v) * 240) + ', 100%, 50%)';
               ctx.fillRect(Math.floor(x), Math.floor(y), Math.floor(dxs+1), Math.floor(dys-1));
            }
         }
         //profile("plot");
      }
   }

   ctx.restore(); // remove clipping

   // plot color scale
   if (this.param.plot[0].type === "colormap") {
      if (this.param.plot[0].showZScale) {

         for (let i=0 ; i<100 ; i++) {
            let v = i / 100;
            ctx.fillStyle = 'hsl(' +
               Math.floor(v * 240) + ', 100%, 50%)';
            ctx.fillRect(this.x2 + 5, this.y2 + i/100*(this.y1 - this.y2),
               10, (this.y1 - this.y2) / 100 + 1);
         }

         ctx.lineWidth = 1;
         ctx.strokeStyle = this.param.color.axis;
         ctx.beginPath();
         ctx.rect(this.x2 + 5, this.y2, 10, this.y1 - this.y2);
         ctx.stroke();

         ctx.font = this.param.zAxis.textSize + "px sans-serif";
         ctx.strokeStyle = this.param.color.axis;

         this.drawYAxis(ctx, this.x2 + 15, this.y1, this.y1 - this.y2,
            4, 7, 10, 12, 0, this.zMin, this.zMax, false, true);
      }
   }

   // plot legend
   let nLabel = 0;
   for (const p of this.param.plot)
      if (p.label && p.label !== "")
         nLabel++;

   if (this.param.legend && this.param.legend.show && nLabel > 0) {
      ctx.font = this.param.legend.textSize + "px sans-serif";

      let mw = 0;
      for (const p of this.param.plot) {
         if (ctx.measureText(p.label).width > mw) {
            mw = ctx.measureText(p.label).width;
         }
      }
      let w = 50 + mw + 5;
      let h = this.param.legend.textSize * 1.5;

      ctx.fillStyle = this.param.legend.backgroundColor;
      ctx.strokeStyle = this.param.legend.color;
      ctx.fillRect(this.x1 + 10, this.y2 + 10, w, h * this.param.plot.length);
      ctx.strokeRect(this.x1 + 10, this.y2 + 10, w, h  * this.param.plot.length);

      for (const [pi,p] of this.param.plot.entries()) {
         if (p.line) {
            ctx.beginPath();
            ctx.strokeStyle = this.param.color.data[p.line.color];
            ctx.lineWidth = p.line.width;
            ctx.beginPath();
            ctx.moveTo(this.x1 + 15, this.y2 + 10 + pi*h + h/2);
            ctx.lineTo(this.x1 + 45, this.y2 + 10 + pi*h + h/2);
            ctx.stroke();
         }
         if (p.marker) {
            this.drawMarker(ctx, p, this.x1 + 30, this.y2 + 10 + pi*h + h/2);
         }
         ctx.textAlign = "left";
         ctx.textBaseline = "middle";
         ctx.fillStyle = this.param.color.axis;
         ctx.fillText(p.label, this.x1 + 50, this.y2 + 10 + pi*h + h/2);
      }
   }

   // "empty window" notice
   if (noData) {
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
   if (this.param.showMenuButtons) {
      let y = 0;
      let buttonSize = 20;
      this.button.forEach(b => {
         b.x1 = this.width - buttonSize - 6;
         b.y1 = 6 + y * (buttonSize + 4);
         b.width = buttonSize + 4;
         b.height = buttonSize + 4;
         b.enabled = true;

         ctx.fillStyle = "#F0F0F0";
         ctx.strokeStyle = "#808080";
         ctx.fillRect(b.x1, b.y1, b.width, b.height);
         ctx.strokeRect(b.x1, b.y1, b.width, b.height);
         ctx.drawImage(b.img, b.x1 + 2, b.y1 + 2);

         y++;
      });
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
      if (this.param.plot[0].type !== "colormap") {
         ctx.beginPath();
         ctx.globalAlpha = 0.1;
         ctx.arc(this.marker.sx, this.marker.sy, 10, 0, 2 * Math.PI);
         ctx.fillStyle = "#000000";
         ctx.fill();
         ctx.globalAlpha = 1;

         ctx.beginPath();
         ctx.arc(this.marker.xs, this.marker.sy, 4, 0, 2 * Math.PI);
         ctx.fillStyle = "#000000";
         ctx.fill();
      }

      ctx.strokeStyle = "#A0A0A0";
      ctx.drawLine(this.marker.sx, this.y1, this.marker.sx, this.y2);
      ctx.drawLine(this.x1, this.marker.sy, this.x2, this.marker.sy);

      // text label
      ctx.font = "12px sans-serif";
      ctx.textAlign = "left";
      let s = this.marker.x.toPrecision(6).stripZeros() + " / " +
              this.marker.y.toPrecision(6).stripZeros();
      if (this.param.plot[0].type === "colormap")
         s += ": " + this.marker.z.toPrecision(6).stripZeros();
      let w = ctx.measureText(s).width + 6;
      let h = ctx.measureText("M").width * 1.2 + 6;
      let x = this.marker.mx + 10;
      let y = this.marker.my - 20;

      // move marker inside if outside plotting area
      if (x + w >= this.x2)
         x = this.marker.sx - 10 - w;

      ctx.strokeStyle = "#808080";
      ctx.fillStyle = "#F0F0F0";
      ctx.textBaseline = "middle";
      ctx.fillRect(x, y, w, h);
      ctx.strokeRect(x, y, w, h);
      ctx.fillStyle = "#404040";
      ctx.fillText(s, x + 3, y + h / 2);
   }

   // call optional user overlay function
   if (this.param.overlay) {

      // set default text
      ctx.textAlign = "left";
      ctx.textBaseline = "top";
      ctx.fillStyle = "black";
      ctx.strokeStyle = "black";
      ctx.font = "12px sans-serif";

      eval(this.param.overlay + "(this, ctx)");
   }

   //profile("end");
}

LN10 = 2.302585094;
LOG2 = 0.301029996;
LOG5 = 0.698970005;

MPlotGraph.prototype.drawXAxis = function (ctx, x1, y1, width, minor, major,
                                           text, label, grid, xmin, xmax, logaxis) {
   var dx, int_dx, frac_dx, x_act, label_dx, major_dx, x_screen, maxwidth;
   var tick_base, major_base, label_base, n_sig1, n_sig2, xs;
   var base = [1, 2, 5, 10, 20, 50, 100, 200, 500, 1000];

   if (xmin === undefined || xmax === undefined || isNaN(xmin) || isNaN(xmax))
      return;

   if (xmax <= xmin || width <= 0)
      return;

   ctx.textAlign = "center";
   ctx.textBaseline = "top";

   if (logaxis) {

      dx = Math.pow(10, Math.floor(Math.log(xmin) / Math.log(10)));
      if (isNaN(dx) || dx === 0) {
         xmin = 1E-20;
         dx = 1E-20;
      }
      label_dx = dx;
      major_dx = dx * 10;
      n_sig1 = 4;

   } else { // linear axis ----

      // use 10 as min tick distance
      dx = (xmax - xmin) / (width / 10);

      int_dx = Math.floor(Math.log(dx) / LN10);
      frac_dx = Math.log(dx) / LN10 - int_dx;

      if (frac_dx < 0) {
         frac_dx += 1;
         int_dx -= 1;
      }

      tick_base = frac_dx < LOG2 ? 1 : frac_dx < LOG5 ? 2 : 3;
      major_base = label_base = tick_base + 1;

      // rounding up of dx, label_dx
      dx = Math.pow(10, int_dx) * base[tick_base];
      major_dx = Math.pow(10, int_dx) * base[major_base];
      label_dx = major_dx;

      do {
         // number of significant digits
         if (xmin === 0)
            n_sig1 = 0;
         else
            n_sig1 = Math.floor(Math.log(Math.abs(xmin)) / LN10) - Math.floor(Math.log(Math.abs(label_dx)) / LN10) + 1;

         if (xmax === 0)
            n_sig2 = 0;
         else
            n_sig2 = Math.floor(Math.log(Math.abs(xmax)) / LN10) - Math.floor(Math.log(Math.abs(label_dx)) / LN10) + 1;

         n_sig1 = Math.max(n_sig1, n_sig2);

         // toPrecision displays 1050 with 3 digits as 1.05e+3, so increase precision to number of digits
         if (Math.abs(xmin) < 100000)
            n_sig1 = Math.max(n_sig1, Math.floor(Math.log(Math.abs(xmin)) / LN10) + 1);
         if (Math.abs(xmax) < 100000)
            n_sig1 = Math.max(n_sig1, Math.floor(Math.log(Math.abs(xmax)) / LN10) + 1);

         // determination of maximal width of labels
         let str = (Math.floor(xmin / dx) * dx).toPrecision(n_sig1);
         let ext = ctx.measureText(str);
         maxwidth = ext.width;

         str = (Math.floor(xmax / dx) * dx).toPrecision(n_sig1).stripZeros();
         ext = ctx.measureText(str);
         maxwidth = Math.max(maxwidth, ext.width);
         str = (Math.floor(xmax / dx) * dx + label_dx).toPrecision(n_sig1).stripZeros();
         maxwidth = Math.max(maxwidth, ext.width);

         // increasing label_dx, if labels would overlap
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

   x_act = Math.floor(xmin / dx) * dx;

   ctx.strokeStyle = this.param.color.axis;
   ctx.drawLine(x1, y1, x1 + width, y1);

   do {
      if (logaxis)
         x_screen = (Math.log(x_act) - Math.log(xmin)) /
            (Math.log(xmax) - Math.log(xmin)) * width + x1;
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
               // label tick mark
               ctx.strokeStyle = this.param.color.axis;
               ctx.drawLine(xs, y1, xs, y1 + text);

               // grid line
               if (grid !== 0 && xs > x1 && xs < x1 + width) {
                  ctx.strokeStyle = this.param.color.grid;
                  ctx.drawLine(xs, y1, xs, y1 + grid);
               }

               // label
               if (label !== 0) {
                  str = x_act.toPrecision(n_sig1).stripZeros();
                  ext = ctx.measureText(str);
                  if (xs - ext.width / 2 > x1 &&
                     xs + ext.width / 2 < x1 + width) {
                     ctx.strokeStyle = this.param.color.label;
                     ctx.fillStyle = this.param.color.label;
                     ctx.fillText(str, xs, y1 + label);
                  }
                  last_label_x = xs + ext.width / 2;
               }
            } else {
               // major tick mark
               ctx.strokeStyle = this.param.color.axis;
               ctx.drawLine(xs, y1, xs, y1 + major);

               // grid line
               if (grid !== 0 && xs > x1 && xs < x1 + width) {
                  ctx.strokeStyle = this.param.color.grid;
                  ctx.drawLine(xs, y1 - 1, xs, y1 + grid);
               }
            }

            if (logaxis) {
               dx *= 10;
               major_dx *= 10;
               label_dx *= 10;
            }

         } else {
            // minor tick mark
            ctx.strokeStyle = this.param.color.axis;
            ctx.drawLine(xs, y1, xs, y1 + minor);
         }

         if (logaxis) {
            // for log axis, also put grid lines on minor tick marks
            if (grid !== 0 && xs > x1 && xs < x1 + width) {
               ctx.strokeStyle = this.param.color.grid;
               ctx.drawLine(xs, y1 - 1, xs, y1 + grid);
            }

            // for log axis, also put labels on minor tick marks
            if (label !== 0) {
               let str;
               if (Math.abs(x_act) < 0.001 && Math.abs(x_act) > 1E-20)
                  str = x_act.toExponential(n_sig1).stripZeros();
               else
                  str = x_act.toPrecision(n_sig1).stripZeros();
               ext = ctx.measureText(str);
               if (xs - ext.width / 2 > x1 &&
                  xs + ext.width / 2 < x1 + width &&
                  xs - ext.width / 2 > last_label_x + 5) {
                  ctx.strokeStyle = this.param.color.label;
                  ctx.fillStyle = this.param.color.label;
                  ctx.fillText(str, xs, y1 + label);
               }

               last_label_x = xs + ext.width / 2;
            }
         }
      }

      x_act += dx;

      /* suppress 1.23E-17 ... */
      if (Math.abs(x_act) < dx / 100)
         x_act = 0;

   } while (1);
}


MPlotGraph.prototype.drawYAxis = function (ctx, x1, y1, height, minor, major,
                                           text, label, grid, ymin, ymax, logaxis, draw) {
   let dy, int_dy, frac_dy, y_act, label_dy, major_dy, y_screen;
   let tick_base, major_base, label_base, n_sig1, n_sig2, ys;
   let base = [1, 2, 5, 10, 20, 50, 100, 200, 500, 1000];

   if (ymin === undefined || ymax === undefined || isNaN(ymin) || isNaN(ymax))
      return;

   if (ymax <= ymin || height <= 0)
      return;

   if (label < 0)
      ctx.textAlign = "right";
   else
      ctx.textAlign = "left";
   ctx.textBaseline = "middle";
   let textHeight = parseInt(ctx.font.match(/\d+/)[0]);

   if (!isFinite(ymax - ymin) || ymax === Number.MAX_VALUE) {
      dy = Number.MAX_VALUE / 10;
      label_dy = dy;
      major_dy = dy;
      n_sig1 = 1;
   } else if (logaxis) {
      dy = Math.pow(10, Math.floor(Math.log(ymin) / Math.log(10)));
      if (isNaN(dy) || dy === 0) {
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
      ctx.strokeStyle = this.param.color.axis;
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
                  ctx.strokeStyle = this.param.color.axis;
                  ctx.drawLine(x1, ys, x1 + text, ys);
               }

               // grid line
               if (grid !== 0 && ys < y1 && ys > y1 - height)
                  if (draw) {
                     ctx.strokeStyle = this.param.color.grid;
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
                     ctx.strokeStyle = this.param.color.label;
                     ctx.fillStyle = this.param.color.label;
                     ctx.fillText(str, x1 + label, ys);
                  }
                  last_label_y = ys - textHeight / 2;
               }
            } else {
               // major tick mark
               if (draw) {
                  ctx.strokeStyle = this.param.color.axis;
                  ctx.drawLine(x1, ys, x1 + major, ys);
               }

               // grid line
               if (grid !== 0 && ys < y1 && ys > y1 - height)
                  if (draw) {
                     ctx.strokeStyle = this.param.color.grid;
                     ctx.drawLine(x1, ys, x1 + grid, ys);
                  }
            }

            if (logaxis) {
               dy *= 10;
               major_dy *= 10;
               label_dy *= 10;
            }

         } else {
            // minor tick mark
            if (draw) {
               ctx.strokeStyle = this.param.color.axis;
               ctx.drawLine(x1, ys, x1 + minor, ys);
            }
         }

         if (logaxis) {

            // for log axis, also put grid lines on minor tick marks
            if (grid !== 0 && ys < y1 && ys > y1 - height) {
               if (draw) {
                  ctx.strokeStyle = this.param.color.grid;
                  ctx.drawLine(x1+1, ys, x1 + grid - 1, ys);
               }
            }

            // for log axis, also put labels on minor tick marks
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
                     ctx.strokeStyle = this.param.color.label;
                     ctx.fillStyle = this.param.color.label;
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

MPlotGraph.prototype.download = function (mode) {

   let d = new Date();
   let filename = this.param.title.text + "-" +
      d.getFullYear() +
      ("0" + d.getUTCMonth() + 1).slice(-2) +
      ("0" + d.getUTCDate()).slice(-2) + "-" +
      ("0" + d.getUTCHours()).slice(-2) +
      ("0" + d.getUTCMinutes()).slice(-2) +
      ("0" + d.getUTCSeconds()).slice(-2);

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

      // title
      this.param.plot.forEach(p => {
         if (p.type === "scatter" || p.type === "histogram") {
            data += "X,";
            if (p.label === "")
               data += "Y";
            else
               data += p.label;
            data += '\n';

            // data
            for (let i = 0; i < p.xData.length; i++) {
               data += p.xData[i] + ",";
               data += p.yData[i] + "\n";
            }
            data += '\n';
         }

         if (p.type === "colormap") {
            data += "X  \  Y,";

            // X-header
            for (let i = 0; i < p.nx; i++)
               data += p.xData[i] + ",";
            data += '\n';

            for (let j = 0; j < p.ny; j++) {
               data += p.yData[j] + ",";
               for (let i = 0; i < p.nx; i++)
                  data += p.zData[i + j * p.nx] + ",";
               data += '\n';
            }
         }
      });

      let blob = new Blob([data], {type: "text/csv"});
      let url = window.URL.createObjectURL(blob);

      a.href = url;
      a.download = filename;
      a.click();
      window.URL.revokeObjectURL(url);
      dlgAlert("Data downloaded to '" + filename + "'");

   } else if (mode === "PNG") {
      filename += ".png";

      let smb = this.param.showMenuButtons;
      this.param.showMenuButtons = false;
      this.draw();

      let h = this;
      this.canvas.toBlob(function (blob) {
         let url = window.URL.createObjectURL(blob);

         a.href = url;
         a.download = filename;
         a.click();
         window.URL.revokeObjectURL(url);
         dlgAlert("Image downloaded to '" + filename + "'");

         h.param.showMenuButtons = smb;
         h.redraw();

      }, 'image/png');
   }

};

MPlotGraph.prototype.drawTextBox = function (ctx, text, x, y) {
   let line = text.split("\n");

   let mw = 0;
   for (const p of line)
      if (ctx.measureText(p).width > mw)
         mw = ctx.measureText(p).width;
   let w = 5 + mw + 5;
   let h = parseInt(ctx.font) * 1.5;

   let c = ctx.fillStyle;
   ctx.fillStyle = "white";
   ctx.fillRect(x, y, w, h * line.length);
   ctx.fillStyle = c;
   ctx.strokeRect(x, y, w, h * line.length);

   for (let i=0 ; i<line.length ; i++)
      ctx.fillText(line[i], x+5, y +  + 0.2*h + i*h);
}

MPlotGraph.prototype.mouseEvent = function (e) {

   // fix buttons for IE
   if (!e.which && e.button) {
      if ((e.button & 1) > 0) e.which = 1;      // Left
      else if ((e.button & 4) > 0) e.which = 2; // Middle
      else if ((e.button & 2) > 0) e.which = 3; // Right
   }

   let cursor = "default";
   let title = "";
   let cancel = false;

   // cancel dragging in case we did not catch the mouseup event
   if (e.type === "mousemove" && e.buttons === 0 &&
      (this.drag.active || this.zoom.x.active || this.zoom.y.active))
      cancel = true;

   if (e.type === "mousedown") {

      this.downloadSelector.style.display = "none";

      // check for buttons
      this.button.forEach(b => {
         if (e.offsetX > b.x1 && e.offsetX < b.x1 + b.width &&
            e.offsetY > b.y1 && e.offsetY < b.y1 + b.width &&
            b.enabled) {
            b.click(this);
         }
      });

      // check for dragging
      if (e.offsetX > this.x1 && e.offsetX < this.x2 &&
         e.offsetY > this.y2 && e.offsetY < this.y1) {
         this.drag.active = true;
         this.marker.active = false;
         this.drag.sxStart = e.offsetX;
         this.drag.syStart = e.offsetY;
         this.drag.xStart = this.screenToX(e.offsetX);
         this.drag.yStart = this.screenToY(e.offsetY);
         this.drag.xMinStart = this.xMin;
         this.drag.xMaxStart = this.xMax;
         this.drag.yMinStart = this.yMin;
         this.drag.yMaxStart = this.yMax;

         this.blockAutoScale = true;
      }

      // check for axis dragging
      if (e.offsetX > this.x1 && e.offsetX < this.x2 && e.offsetY > this.y1) {
         this.zoom.x.active = true;
         this.zoom.x.x1 = e.offsetX;
         this.zoom.x.x2 = undefined;
         this.zoom.x.t1 = this.screenToX(e.offsetX);
      }
      if (e.offsetY < this.y1 && e.offsetY > this.y2 && e.offsetX < this.x1) {
         this.zoom.y.active = true;
         this.zoom.y.y1 = e.offsetY;
         this.zoom.y.y2 = undefined;
         this.zoom.y.v1 = this.screenToY(e.offsetY);
      }

   } else if (cancel || e.type === "mouseup") {

      if (this.drag.active)
         this.drag.active = false;

      if (this.zoom.x.active) {
         if (this.zoom.x.x2 !== undefined &&
            Math.abs(this.zoom.x.x1 - this.zoom.x.x2) > 5) {
            let x1 = this.zoom.x.t1;
            let x2 = this.screenToX(this.zoom.x.x2);
            if (x1 > x2)
               [x1, x2] = [x2, x1];
            this.xMin = x1;
            this.xMax = x2;
         }
         this.zoom.x.active = false;
         this.blockAutoScale = true;
         this.redraw();
      }

      if (this.zoom.y.active) {
         if (this.zoom.y.y2 !== undefined &&
            Math.abs(this.zoom.y.y1 - this.zoom.y.y2) > 5) {
            let y1 = this.zoom.y.v1;
            let y2 = this.screenToY(this.zoom.y.y2);
            if (y1 > y2)
               [y1, y2] = [y2, y1];
            this.yMin = y1;
            this.yMax = y2;
         }
         this.zoom.y.active = false;
         this.blockAutoScale = true;
         this.redraw();
      }

   } else if (e.type === "mousemove") {

      if (this.drag.active) {

         // execute dragging
         cursor = "move";

         if (this.param.xAxis.log) {
            let dx = e.offsetX - this.drag.sxStart;

            this.xMin = Math.exp(((this.x1 - dx) - this.x1) / (this.x2 - this.x1) * (Math.log(this.drag.xMaxStart)-Math.log(this.drag.xMinStart)) + Math.log(this.drag.xMinStart));
            this.xMax = Math.exp(((this.x2 - dx) - this.x1) / (this.x2 - this.x1) * (Math.log(this.drag.xMaxStart)-Math.log(this.drag.xMinStart)) + Math.log(this.drag.xMinStart));

            if (this.xMin <= 0)
               this.xMin = 1E-20;
            if (this.xMax <= 0)
               this.xMax = 1E-18;
         } else {
            let dx = (e.offsetX - this.drag.sxStart) / (this.x2 - this.x1) * (this.xMax - this.xMin);
            this.xMin = this.drag.xMinStart - dx;
            this.xMax = this.drag.xMaxStart - dx;
         }

         if (this.param.yAxis.log) {
            let dy = e.offsetY - this.drag.syStart;

            this.yMin = Math.exp((this.y1 - (this.y1 - dy)) / (this.y1 - this.y2) * (Math.log(this.drag.yMaxStart)-Math.log(this.drag.yMinStart)) + Math.log(this.drag.yMinStart));
            this.yMax = Math.exp((this.y1 - (this.y2 - dy)) / (this.y1 - this.y2) * (Math.log(this.drag.yMaxStart)-Math.log(this.drag.yMinStart)) + Math.log(this.drag.yMinStart));

            if (this.param.yAxis.log && this.yMin <= 0)
               this.yMin = 1E-20;
            if (this.param.yAxis.log && this.yMax <= 0)
               this.yMax = 1E-18;
         } else {
            let dy = (this.drag.syStart - e.offsetY) / (this.y1 - this.y2) * (this.yMax - this.yMin);
            this.yMin = this.drag.yMinStart - dy;
            this.yMax = this.drag.yMaxStart - dy;
         }

         this.redraw();

      } else {

         // change cursor to pointer over buttons
         this.button.forEach(b => {
            if (e.offsetX > b.x1 && e.offsetY > b.y1 &&
               e.offsetX < b.x1 + b.width && e.offsetY < b.y1 + b.height) {
               cursor = "pointer";
               title = b.title;
            }
         });

         // execute axis zoom
         if (this.zoom.x.active) {
            this.zoom.x.x2 = Math.max(this.x1, Math.min(this.x2, e.offsetX));
            this.zoom.x.t2 = this.screenToX(e.offsetX);
            this.redraw();
         }
         if (this.zoom.y.active) {
            this.zoom.y.y2 = Math.max(this.y2, Math.min(this.y1, e.offsetY));
            this.zoom.y.v2 = this.screenToY(e.offsetY);
            this.redraw();
         }

         // check if cursor close to plot point
         if (this.param.plot[0].type === "scatter" || this.param.plot[0].type === "histogram") {
            let minDist = 10000;
            for (const [pi, p] of this.param.plot.entries()) {
               if (p.xData === undefined || p.xData === null)
                  continue;

               for (let i = 0; i < p.xData.length; i++) {
                  let x = this.xToScreen(p.xData[i]);
                  let y = this.yToScreen(p.yData[i]);
                  let d = (e.offsetX - x) * (e.offsetX - x) +
                     (e.offsetY - y) * (e.offsetY - y);
                  if (d < minDist) {
                     minDist = d;
                     this.marker.x = p.xData[i];
                     this.marker.y = p.yData[i];
                     this.marker.sx = x;
                     this.marker.sy = y;
                     this.marker.mx = e.offsetX;
                     this.marker.my = e.offsetY;
                     this.marker.plotIndex = pi;
                     this.marker.index = i;
                  }
               }
            }

            this.marker.active = Math.sqrt(minDist) < 10 && e.offsetX > this.x1 && e.offsetX < this.x2;
         }

         if (this.param.plot[0].type === "colormap") {
            let x = this.screenToX(e.offsetX);
            let y = this.screenToY(e.offsetY);
            let xMin = this.param.plot[0].xMin;
            let xMax = this.param.plot[0].xMax;
            let yMin = this.param.plot[0].yMin;
            let yMax = this.param.plot[0].yMax;
            let dx = (xMax - xMin) / this.nx;
            let dy = (yMax - yMin) / this.ny;
            if (x > this.xMin && x < this.xMax && y > this.yMin && y < this.yMax &&
               x > xMin && x < xMax && y > yMin && y < yMax) {
               let i = Math.floor((x - xMin) / dx);
               let j = Math.floor((y - yMin) / dy);

               this.marker.x = (i + 0.5) * dx + xMin;
               this.marker.y = (j + 0.5) * dy + yMin;
               this.marker.z = this.param.plot[0].zData[i + j * this.nx];

               this.marker.sx = this.xToScreen(this.marker.x);
               this.marker.sy = this.yToScreen(this.marker.y);
               this.marker.mx = e.offsetX;
               this.marker.my = e.offsetY;
               this.marker.plotIndex = 0;
               this.marker.active = true;
            } else {
               this.marker.active = false;
            }
         }

         this.draw();
      }

   } else if (e.type === "wheel") {

      let x = this.screenToX(e.offsetX);
      let y = this.screenToY(e.offsetY);
      let scale = e.deltaY * 0.01;

      if (this.param.xAxis.log) {

         scale *= 10;
         let f = (e.offsetX - this.x1) / (this.x2 - this.x1);
         let xMinOld = this.xMin;
         let xMaxOld = this.xMax;

         this.xMax *= 1 + scale * (1 - f);
         this.xMin /= 1 + scale * f;

         if (this.xMax <= this.xMin) {
            this.xMin = xMinOld;
            this.xMax = xMaxOld;
         }

      } else {
         let dx = (this.xMax - this.xMin) * scale;
         let f = (x - this.xMin) / (this.xMax - this.xMin);
         this.xMin = this.xMin - dx * f;
         this.xMax = this.xMax + dx * (1 - f);
      }

      if (this.param.yAxis.log) {

         scale *= 10;
         let f = (e.offsetY - this.y2) / (this.y1 - this.y2);
         let yMinOld = this.yMin;
         let yMaxOld = this.yMax;

         this.yMax *= 1 + scale * f;
         this.yMin /= 1 + scale * (1 - f);

         if (this.yMax <= this.yMin) {
            this.yMin = yMinOld;
            this.yMax = yMaxOld;
         }

      } else {
         let dy = (this.yMax - this.yMin) * scale;
         let f = (y - this.yMin) / (this.yMax - this.yMin);
         this.yMin = this.yMin - dy * f;
         this.yMax = this.yMax + dy * (1 - f);
      }

      this.blockAutoScale = true;

      this.draw();
   }


   this.parentDiv.title = title;
   this.parentDiv.style.cursor = cursor;

   e.preventDefault();
}

MPlotGraph.prototype.resetAxes = function () {
   this.xMin = this.xMin0;
   this.xMax = this.xMax0;
   this.yMin = this.yMin0;
   this.yMax = this.yMax0;

   this.blockAutoScale = false;

   this.redraw();
}
