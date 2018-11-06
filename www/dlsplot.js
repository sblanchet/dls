/** dls: copy from flotr2 lines
    spezial plotting for dls data
**/

Flotr.addType('dls', {
  options: {
    show: false,           // => setting to true will show lines, false will hide
    lineWidth: 1,          // => line width in pixels
    fill: false,           // => true to fill the area from the line to the x axis, false for (transparent) no fill
    fillBorder: false,     // => draw a border around the fill
    fillColor: null,       // => fill color
    fillOpacity: 0.4,      // => opacity of the fill color, set to 1 for a solid fill, 0 hides the fill
    steps: false           // => draw steps
  },


  /**
   * Draws lines series in the canvas element.
   * @param {Object} options
   */
  draw : function (options) {

    var
      context     = options.context,
      lineWidth   = options.lineWidth,
      offset;

    context.save();
    context.lineJoin = 'round';

    context.lineWidth = lineWidth;
    context.strokeStyle = options.color;

    this.plot(options);

    context.restore();
  },

  /**
     here it happens
   */
  plot : function (options) {

    var
      context   = options.context,
      width     = options.width, 
      height    = options.height,
      xScale    = options.xScale,
      yScale    = options.yScale,
      data      = options.data.data, 
      blocks    = data.length;


    
      /*  
	  data has an array of blocks
	  metatyp: gen -> normal lines
	  metatyp: min,max ->filled polypon  
      */

//      console.log(JSON.stringify(data,null, " "));

      if (blocks < 1) return;

//      console.log("blocks: " + blocks);

      for (var i = 0; i < blocks; i++) {
	  if(data[i].meta != 'undefined'  &&
	     data[i].block != 'undefined' && 
	     data[i].block.length > 1) {
	      context.beginPath();
	      if(data[i].meta === "gen" || 
		 data[i].meta === "min") { //generic data und min
		  context.moveTo(xScale(data[i].block[0][0]/1e3),
				 yScale(data[i].block[0][1]));
		  for(var j=1; j < data[i].block.length;j++) {
		      context.lineTo(xScale(data[i].block[j][0]/1e3),
				     yScale(data[i].block[j][1]));
		  }
	      }
	      if(data[i].meta === "min") { //um den Pfad zu schließen,
		  //muß es einen Block mit meta === max geben, 
		  //der die gleiche Startzeit hat;
		  //diesen jetzt suchen ... und zeichnen
		  for (var k = 0; k < blocks; k++) {
		      if(data[k].meta != 'undefined'  &&
			 data[k].block != 'undefined' && 
			 data[k].meta === "max" &&
			 data[k].start_time === data[i].start_time &&
			 data[k].block.length > 1) {
			  for(var j=data[i].block.length-1; j>0; j--) {
			      context.lineTo(xScale(data[k].block[j][0]/1e3),
					     yScale(data[k].block[j][1]));
			  }
			  break;
		      }
		  }
		  context.fillStyle = options.fillStyle;
		  context.closePath();
		  context.fill();
	      }
	      context.stroke();
	  }
      }

  },

 extendXRange : function (axis, data) {
//     console.log("xrange data:"+ JSON.stringify(data,null, " "));
     var
     data      = data.data, 
     blocks    = data.length,
     xmin,xmax;
     
     if( blocks > 0) {
	 xmin = data[0].start_time/1e3;
	 var n = data[blocks-1].block.length;
	 xmax = data[blocks-1].block[n-1][0]/1e3;
     if(xmax > axis.max) {axis.max = xmax};
     if(xmin < axis.min) {axis.min = xmin};
     }

 },

 extendYRange : function (axis, data) {
     console.log("yrange");

     var
     ymin = Number.MAX_VALUE,
     ymax = -Number.MAX_VALUE,
     data      = data.data, 
     blocks    = data.length,
     y;

     for(var i=0;i<blocks;i++) {
	 if(data[i].block != 'undefined') {
	     for(var j=0;j<data[i].block.length;j++) {
		 y = data[i].block[j][1];
		 if(y < ymin) { ymin = y; }
		 if(y > ymax) { ymax = y; }
	     }
	 }
	 
     }
     if(ymax > axis.max) {axis.max = ymax};
     if(ymin < axis.min) {axis.min = ymin};
 }

  // Perform any pre-render precalculations (this should be run on data first)
  // - Pie chart total for calculating measures
  // - Stacks for lines and bars
  // precalculate : function () {
  // }
  //
  //
  // Get any bounds after pre calculation (axis can fetch this if does not have explicit min/max)
  // getBounds : function () {
  // }
  // getMin : function () {
  // }
  // getMax : function () {
  // }
  //
  //
  // Padding around rendered elements
  // getPadding : function () {
  // }

/* FIXME Later
  extendYRange : function (axis, data, options, lines) {

    var o = axis.options;

    if (options.steps) {

      this.hit = function (options) {
        var
          data = options.data,
          args = options.args,
          yScale = options.yScale,
          mouse = args[0],
          length = data.length,
          n = args[1],
          x = options.xInverse(mouse.relX),
          relY = mouse.relY,
          i;

        for (i = 0; i < length - 1; i++) {
          if (x >= data[i][0] && x <= data[i+1][0]) {
            if (Math.abs(yScale(data[i][1]) - relY) < 8) {
              n.x = data[i][0];
              n.y = data[i][1];
              n.index = i;
              n.seriesIndex = options.index;
            }
            break;
          }
        }
      };


      this.drawHit = function (options) {
        var
          context = options.context,
          args    = options.args,
          data    = options.data,
          xScale  = options.xScale,
          index   = args.index,
          x       = xScale(args.x),
          y       = options.yScale(args.y),
          x2;

        if (data.length - 1 > index) {
          x2 = options.xScale(data[index + 1][0]);
          context.save();
          context.strokeStyle = options.color;
          context.lineWidth = options.lineWidth;
          context.beginPath();
          context.moveTo(x, y);
          context.lineTo(x2, y);
          context.stroke();
          context.closePath();
          context.restore();
        }
      };

      this.clearHit = function (options) {
        var
          context = options.context,
          args    = options.args,
          data    = options.data,
          xScale  = options.xScale,
          width   = options.lineWidth,
          index   = args.index,
          x       = xScale(args.x),
          y       = options.yScale(args.y),
          x2;

        if (data.length - 1 > index) {
          x2 = options.xScale(data[index + 1][0]);
          context.clearRect(x - width, y - width, x2 - x + 2 * width, 2 * width);
        }
      };
    }
  }
*/

});
