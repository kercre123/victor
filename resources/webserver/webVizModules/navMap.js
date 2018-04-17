/*  
 *  NavMap viz
 *  ross
 *  april 13 2018
 *  Copyright Anki, Inc. 2018
 */

(function(myMethods, sendData) {

  // for debugging:
  var dumpInput = false; // print all input the engine sends
  var showFakeDataUponDisconnect = false; 

  // helper classes
  function Vector( x, y, z ) {
    this.x = 1.0*x;
    this.y = 1.0*y;
    this.z = 1.0*z;
    this.clone = function() {
      return new Vector( this.x, this.y, this.z );
    }
    this.getLength = function() {
      return Math.sqrt( this.x*this.x + this.y*this.y + this.z*this.z );
    }
    this.makeUnitLength = function() {
      var length = this.getLength();
      this.x = length == 0.0 ? 0.0 : this.x/length;
      this.y = length == 0.0 ? 0.0 : this.y/length;
      this.z = length == 0.0 ? 0.0 : this.z/length;
      return this;
    }
    this.cross = function( v ) {
      return new Vector( this.y*v.z - this.z*v.y, this.z*v.x - this.x*v.z, this.x*v.y - this.y*v.x  );
    }
    this.dot = function( v ) {
      return this.x*v.x + this.y*v.y + this.z*v.z;
    }
    this.getScaled = function(a) {
      return this.clone().scale(a);
    }
    this.getAfterAdd = function(v) {
      return this.clone().add(v);
    }
    this.scale = function(a) {
      this.x *= a;
      this.y *= a;
      this.z *= a;
      return this;
    }
    this.add = function(v) {
      this.x += v.x;
      this.y += v.y;
      this.z += v.z;
      return this;
    }
    this.getRotatedAbout = function(u, theta) {
      var unitThis = this.clone().makeUnitLength();
      var length = this.getLength();
      var cosTheta = Math.cos( theta );
      var sinTheta = Math.sin( theta );
      var c1 = unitThis.getScaled( cosTheta );
      var c2 = u.getScaled( u.dot( unitThis )*(1.0-cosTheta) );
      var c3 = u.cross( unitThis ).scale( sinTheta );
      return c1.add( c2.add( c3 ) ).scale( length );
    }
  }
  function Point(x,y) {
    this.x = x;
    this.y = y;
  }
  function Color(r,g,b,a) {
    this.r = r;
    this.g = g;
    this.b = b;
    if( typeof a === 'undefined' ) {
      this.a = 255;
    }
    else {
      this.a = a;
    }
  }

  // dom vars and methods

  var updateBtn;
  var canvasContainer;
  var legendContainer;
  var noteDiv;
  var autoUpdate = false;
  var waitingOnData = false;
  var is3D = true;

  function callUpdate() {
    waitingOnData = true;
    updateBtn.prop( 'disabled', true );
    var payload = { 'update' : true };
    sendData( payload );
    if( $('#status').length && 
        ($('#status').text() != "Connected") &&
        showFakeDataUponDisconnect && 
        (typeof noteDiv !== 'undefined') ) 
    {
        noteDiv.text('DISCONNECTED: DISPLAYING FAKE DATA');
        fakeData();
    }
  }

  // quadtree/robot/objects data and methods

  var memoryMapQuadInfoVectorMapIncoming = {}; // map from {origin => map of {sequence # => list of quads}}
  var memoryMapInfo = {}; // map from {origin => map info}
  var quadTreeQuads = [];
  var dataExtentsInfo = {};
  var cubeData;
  var faceData = {};
  var robotPosition;

  function SimpleQuad( center, sideSize, color ) {
    this.center = center;
    this.sideSize = sideSize;
    this.color = color;
  }
  function getQuadColor( content ) {
    var color = new Color( 0, 0, 0 );
    switch( content )
    {
      case 'Unknown'                : { color = new Color(  77,  77,  77,  51 ); break; } // DARKGRAY  alpha=0.2
      case 'ClearOfObstacle'        : { color = new Color(   0, 255,   0, 127 ); break; } // GREEN     alpha=0.5
      case 'ClearOfCliff'           : { color = new Color(   0, 127,   0, 204 ); break; } // DARKGREEN alpha=0.8
      case 'ObstacleCube'           : { color = new Color( 255,   0,   0, 127 ); break; } // RED       alpha=0.5
      case 'ObstacleCharger'        : { color = new Color( 255, 127,   0, 127 ); break; } // ORANGE    alpha=0.5
      case 'ObstacleProx'           : { color = new Color(   0, 255, 255, 127 ); break; } // CYAN      alpha=0.5
      case 'ObstacleProxExplored'   : { color = new Color(   0,   0, 255, 255 ); break; } // BLUE      alpha=1.0
      case 'ObstacleUnrecognized'   : { color = new Color(   0,   0,   0, 127 ); break; } // BLACK     alpha=0.5
      case 'Cliff'                  : { color = new Color(   0,   0,   0, 204 ); break; } // BLACK     alpha=0.8
      case 'InterestingEdge'        : { color = new Color( 255,   0, 255, 127 ); break; } // MAGENTA   alpha=0.5
      case 'NotInterestingEdge'     : { color = new Color( 255,  20, 148, 204 ); break; } // PINK      alpha=0.8
    };
    return color;
  }
  // duplicates the code in physVizController
  function MemoryMapNode( depth, size_m, center ) {
    this.depth = depth;
    this.size_m = size_m;
    this.center = center;
    this.nextChild = 0;
    this.children = [];

    this.AddChild = function( destSimpleQuads, extentsInfo, content, depth ) {
      if( this.depth == depth ) {
        var color = getQuadColor( content );
        if( this.center.x - 0.5*size_m < extentsInfo.minX ) {
          extentsInfo.minX = this.center.x - 0.5*size_m;
        }
        if( this.center.x + 0.5*size_m > extentsInfo.maxX ) {
          extentsInfo.maxX = this.center.x + 0.5*size_m;
        }
        if( this.center.y - 0.5*size_m < extentsInfo.minY ) {
          extentsInfo.minY = this.center.y - 0.5*size_m;
        }
        if( this.center.y + 0.5*size_m > extentsInfo.maxY ) {
          extentsInfo.maxY = this.center.y + 0.5*size_m;
        }
        destSimpleQuads.push( new SimpleQuad( this.center, this.size_m, color ) );
        return true;
      }
      
      if( this.children.length == 0 ) {
        var nextDepth = this.depth - 1;
        var nextSize = this.size_m * 0.5;
        var offset = nextSize * 0.5;

        var center1 = new Point( this.center.x + offset, this.center.y + offset );
        var center2 = new Point( this.center.x + offset, this.center.y - offset );
        var center3 = new Point( this.center.x - offset, this.center.y + offset );
        var center4 = new Point( this.center.x - offset, this.center.y - offset );
        
        this.children.push( new MemoryMapNode( nextDepth, nextSize, center1 ) );
        this.children.push( new MemoryMapNode( nextDepth, nextSize, center2 ) );
        this.children.push( new MemoryMapNode( nextDepth, nextSize, center3 ) );
        this.children.push( new MemoryMapNode( nextDepth, nextSize, center4 ) );
      }
      
      if( this.children[this.nextChild].AddChild( destSimpleQuads, extentsInfo, content, depth) ) {
        // All children below this child have been processed
        ++this.nextChild;
      }
      
      return (this.nextChild > 3);
    }
  }

  // viz vars and methods
  var myp5;
  var vizDirty = false;
  var shouldDrawRobot = true;
  var shouldDrawCubes = true;
  var shouldDrawFaces = true;
  var kKnownTypes = ['Unknown','ClearOfObstacle','ClearOfCliff','ObstacleCube','ObstacleCharger','ObstacleProx','ObstacleProxExplored','ObstacleUnrecognized','Cliff','InterestingEdge','NotInterestingEdge'];
  
  var sketch = function( p ) {
    var kCanvasWidth = 700; // note: container is ~800
    var kCanvasHeight = 600;
    var kInitialMargin = 50; // padding on either side for initial draw
    var kDataScaleFactor = 1000; // 1 mm resolution in 3d (this is auto calculated in 2d)
    var kFovAngle = Math.PI / 3; // the default for p5, but repeated here bc it's used in calculations
    // needs different colors in 2d and 3d bc of transparency rendering
    var kQuadBorderColor3D = p.color('rgba(255,255,255,0.5)');
    var kQuadBorderColor2D = p.color('rgba(255,255,255,0.1)');

    function Camera(x,y,z,a,b,c,p,q,r) {
      // camera pos
      this.x=x;
      this.y=y;
      this.z=z;
      // subject pos
      this.a=a;
      this.b=b;
      this.c=c;
      // components of up vector
      this.p=p;
      this.q=q;
      this.r=r;
      this.clone = function() {
        return new Camera( this.x, this.y, this.z, this.a, this.b, this.c, this.p, this.q, this.r );
      }
      this.getLookVector = function() {
        return new Vector( this.a - this.x, this.b - this.y, this.c - this.z );
      }
      this.getUpVector = function() {
        return new Vector( this.p, this.q, this.r );
      }
      // translate the camera position and subject position by v
      this.move = function( v ) {
        this.x += v.x;
        this.y += v.y;
        this.z += v.z;
        this.a += v.x;
        this.b += v.y;
        this.c += v.z;
      }
      // rotate the camera around vector (p,q,r) originating from (x,y,z) by amount a
      this.rotateAbout = function( x, y, z, rotAxis, a ) {
        var upVec = new Vector( this.p, this.q, this.r );
        var cameraVec = new Vector( this.x - x, this.y - y, this.z - z );
        var lookVec = this.getLookVector();
        // rotate
        lookVec = lookVec.getRotatedAbout( rotAxis, a );
        cameraVec = cameraVec.getRotatedAbout( rotAxis, a );
        upVec = upVec.getRotatedAbout( rotAxis, a );
        // set the new params
        this.x = x + cameraVec.x;
        this.y = y + cameraVec.y;
        this.z = z + cameraVec.z;
        this.a = this.x + lookVec.x;
        this.b = this.y + lookVec.y;
        this.c = this.z + lookVec.z;
        this.p = upVec.x;
        this.q = upVec.y;
        this.r = upVec.z;
      }
      // rotate by some right/left amount, up/down amount, keeping the camera center in place
      this.rotate = function( x, y ) {
        var up = new Vector( this.p, this.q, this.r );
        var to = this.getLookVector().makeUnitLength();
        var upHat = up.clone().makeUnitLength();
        var toHat = to.clone().makeUnitLength();
        var rightHat = toHat.cross( upHat );
        var rotAxis = rightHat.getScaled( y ).getAfterAdd( upHat.getScaled( x ) );
        rotAxis.makeUnitLength();
        // rotate the vector looking outward from the camera 
        var theta = Math.sqrt( x*x + y*y ) * Math.PI / 10;
        var newTo = to.getRotatedAbout( rotAxis, theta );
        this.a = this.x + newTo.x;
        this.b = this.y + newTo.y;
        this.c = this.z + newTo.z;
        // rotate the up vector
        var newUp = up.getRotatedAbout( rotAxis, theta );
        this.p = newUp.x;
        this.q = newUp.y;
        this.r = newUp.z;
      }
    }
    var camera;

    var dragging = false;
    var draggingInfo = {};
    

    var scaleFactor2D;
    var scaleFactor2D0; // initial
    var xOffset2D;
    var yOffset2D;

    p.setup = function() {
      p.createCanvas( kCanvasWidth, kCanvasHeight, (is3D ? p.WEBGL : p.P2D) );
      var loadCallback = function() {
        if( (typeof camera !== 'undefined') || (typeof scaleFactor2D !== 'undefined') ) {
          // scene was loaded at least once. need to redraw
          vizDirty = true;
        }
      };
      if( is3D ) {
        robot = p.loadModel( 'webVizModules/cozmo.obj', loadCallback );
        cube = p.loadModel( 'webVizModules/cube.obj', loadCallback );
        faceImg = p.loadImage( 'webVizModules/face01.png',loadCallback );
      } else {
        robotImg = p.loadImage( 'webVizModules/robot.png', loadCallback );
        cubeImg = p.loadImage( 'webVizModules/cube.png', loadCallback );
      }
    };

    function drawRect2D( centerX, centerY, width, height, fillColor, borderColor ) {
      centerX = Math.round( centerX );
      centerY = Math.round( centerY );
      width = Math.round( width );
      height = Math.round( height );
      p.push();
      p.translate( centerX, centerY, 0 );
      p.stroke( borderColor );
      p.fill( fillColor );
      p.rect( 0, 0, width, height );
      p.pop();
    }
    function drawRect3D( centerX, centerY, width, height, fillColor, borderColor ) {
      centerX = Math.round( centerX );
      centerY = Math.round( centerY );
      width = Math.round( width );
      height = Math.round( height );
      p.push();
      p.rectMode( p.CENTER );
      p.translate( centerX, centerY, 0 );

      // a transparent rect will show its triangulation in the stroke color, so there 
      // has to be no stroke, so we have to draw the lines manually
      p.stroke( borderColor );
      p.line( -width/2, -height/2, width/2, -height/2 );
      p.line( -width/2, height/2, width/2, height/2 );
      p.line( -width/2, -height/2, -width/2, height/2 );
      p.line( width/2, -height/2, width/2, height/2 );
      
      p.fill( fillColor );
      p.noStroke();
      
      p.rect( 0, 0, width, height );
      p.pop();
    }
    function drawRobot3D() {
      p.push()
      var scaleFactor = kDataScaleFactor*1.0/1000;
      p.translate( scaleFactor*robotPosition.x, scaleFactor*robotPosition.y, scaleFactor*robotPosition.z );
      var euler = calcEuler( robotPosition.qW, robotPosition.qX, robotPosition.qY, robotPosition.qZ );
      p.rotateZ( euler.z );
      p.rotateX( euler.x );
      p.rotateY( euler.y );
      p.scale( 7.0 ) // arbitrary til it looks right
      p.normalMaterial();
      p.model( robot );
      p.pop();
    }
    function drawCubes3D() {
      if( typeof cubeData === 'undefined' ) {
        return;
      }
      for( var idx=0; idx<cubeData.length; ++idx ) {
        var cubePos = cubeData[idx];
        p.push()
        var scaleFactor = kDataScaleFactor*1.0/1000;
        p.translate( scaleFactor*cubePos.x, scaleFactor*cubePos.y, scaleFactor*cubePos.z );
        p.rotateZ( cubePos.angle );
        p.scale( 7.0 ); // arbitrary til it looks right
        p.normalMaterial();
        p.model( cube );
        p.pop();
      }
    }
    function drawFaces3D() {
      for( var faceId in faceData ) {
        var facePose = faceData[faceId]["pose"]
        p.push()
        var scaleFactor = kDataScaleFactor*1.0/1000;
        var imageHeight = 100;
        var imageWidth = 100;
        p.translate( scaleFactor*facePose.x, scaleFactor*facePose.y - imageHeight/2, scaleFactor*facePose.z + imageHeight/2 );
        var euler = calcEuler( facePose.qW, facePose.qX, facePose.qY, facePose.qZ );
        p.rotateZ( euler.z );
        p.rotateX( euler.x );
        p.rotateY( euler.y );
        p.scale( 2.0 ); // arbitrary til it looks right
        p.texture( faceImg );
        p.rect( 0, 0, imageHeight, imageWidth );
        p.pop();
      }
    }
    function drawRobot2D() {
      p.push()
      var x = scaleFactor2D*(0.001*robotPosition.x - xOffset2D);
      var y = scaleFactor2D*(0.001*robotPosition.y - yOffset2D);
      var euler = calcEuler( robotPosition.qW, robotPosition.qX, robotPosition.qY, robotPosition.qZ );
      var robotLength = 30.0*scaleFactor2D/scaleFactor2D0; // in pixels
      var robotWidth = 20.0*scaleFactor2D/scaleFactor2D0;
      p.translate( x - 0.75*robotLength, y - 0.5*robotWidth );
      p.rotate( euler.z );
      p.image(robotImg, 0,0, robotLength, robotWidth,0,0);
      p.pop();
    }
    function drawCubes2D() {
      if( typeof cubeData === 'undefined' ) {
        return;
      }
      for( var idx=0; idx<cubeData.length; ++idx ) {
        var cubePos = cubeData[idx];
        p.push();
        var x = scaleFactor2D*(0.001*cubePos.x - xOffset2D);
        var y = scaleFactor2D*(0.001*cubePos.y - yOffset2D);
        var cubeSide = 15.0*scaleFactor2D/scaleFactor2D0; // in pixels
        p.translate( x - 0.5*cubeSide, y - 0.5*cubeSide );
        p.rotate( cubePos.angle );
        p.image( cubeImg, 0, 0, cubeSide, cubeSide, 0, 0 );
        p.pop();
      }
    }
    // transforms from (imageX, imageY) to (worldX, worldY, 0)
    function imageToWorld( imageX, imageY ) {
      var lookVec = camera.getLookVector().makeUnitLength();
      var upVec = camera.getUpVector().makeUnitLength();
      var rightVec = lookVec.cross( upVec );
      var size = p.width > p.height ? p.width : p.height;
      var imageZ = 0.5 * size / Math.tan( 0.5*kFovAngle );
      var imageVec = rightVec.getScaled( imageX - 0.5*p.width )
                       .add( upVec.getScaled( imageY - 0.5*p.height ) )
                       .add( lookVec.getScaled( imageZ ) );
      imageVec = imageVec.makeUnitLength();
      var plane = new Vector( 0, 0, 1 );
      if( Math.abs( plane.dot( imageVec ) ) < 0.001 ) { 
        // nearly perpendicular, never intersect or fully contained
        return;
      } else {
        var t = -camera.z / imageVec.z;
        imageVec.scale( t );
        var z = camera.z + imageVec.z; // should be 0
        return new Vector( camera.x + imageVec.x, camera.y + imageVec.y, 0.0 );
      }
    }
    // quaternion to euler Z-X-Y
    function calcEuler( w, x, y, z ) {
      var threeaxisrot = function( r11, r12, r21, r31, r32 ) {
        return new Vector( Math.atan2( r31, r32 ), Math.asin ( r21 ), Math.atan2( r11, r12 ) );
      };
      return threeaxisrot( -2*(x*y - w*z),
                           w*w - x*x + y*y - z*z,
                           2*(y*z + w*x),
                           -2*(x*z - w*y),
                           w*w - x*x - y*y + z*z );
    }

    var robot;
    var cube;

    p.draw = function() {
      
      if( (quadTreeQuads.length == 0) || !vizDirty ) {
        return;
      }

      if( is3D && (typeof camera === 'undefined') ) {
        var camX = 0.5*kDataScaleFactor*(dataExtentsInfo.maxX + dataExtentsInfo.minX);
        var camY = 0.5*kDataScaleFactor*(dataExtentsInfo.maxY + dataExtentsInfo.minY);
        // find a z so that it fits with a margin
        var camZa = (0.5*kDataScaleFactor*(dataExtentsInfo.maxX - dataExtentsInfo.minX) + kInitialMargin) / Math.tan( kFovAngle / 2 );
        var camZb = (0.5*kDataScaleFactor*(dataExtentsInfo.maxY - dataExtentsInfo.minY) + kInitialMargin) / Math.tan( kFovAngle / 2 );
        var camZ = camZa > camZb ? camZa : camZb;
        camera = new Camera( camX, camY, camZ, camX, camY, 0, 0, 1, 0 );
      }
      if( !is3D && (typeof scaleFactor2D === 'undefined') ) {
        var scaleX = (dataExtentsInfo.maxX-dataExtentsInfo.minX) / (kCanvasWidth - 2*kInitialMargin);
        var scaleY = (dataExtentsInfo.maxY-dataExtentsInfo.minY) / (kCanvasHeight - 2*kInitialMargin);
        if( scaleX > 0 || scaleY > 0 ) {
          scaleFactor2D = (scaleX > scaleY) ? 1.0/scaleX : 1.0/scaleY;
        } else {
          scaleFactor2D = 500;
        }
        scaleFactor2D0 = scaleFactor2D;
        xOffset2D = dataExtentsInfo.minX - (1.0*kInitialMargin)/scaleFactor2D;
        yOffset2D = dataExtentsInfo.minY - (1.0*kInitialMargin)/scaleFactor2D;
      }


      p.clear();
      
      if( is3D ) {
        p.camera( camera.x, camera.y, camera.z, camera.a, camera.b, camera.c, camera.p, camera.q, camera.r );
      }

      for( var quadIdx = 0; quadIdx < quadTreeQuads.length; ++ quadIdx ) {
        var quad = quadTreeQuads[quadIdx];
        var colorStr = 'rgba(' + quad.color.r + ',' + quad.color.g + ',' + quad.color.b + ',' + ((quad.color.a*1.0)/255) + ')';
        var color = p.color( colorStr );
        if( is3D ) {
          var x = kDataScaleFactor*quad.center.x;
          var y = kDataScaleFactor*quad.center.y;
          var side = kDataScaleFactor * quad.sideSize;
          drawRect3D( x, y, side, side, color, kQuadBorderColor3D );
        } else {
          var x = scaleFactor2D*(quad.center.x - 0.5*quad.sideSize - xOffset2D);
          var y = scaleFactor2D*(quad.center.y - 0.5*quad.sideSize - yOffset2D);
          var side = scaleFactor2D * quad.sideSize;
          drawRect2D( x, y, side, side, color, kQuadBorderColor2D );
        }
      }
      if( is3D ) {
        if( shouldDrawRobot ) {
         drawRobot3D();
        }
        if( shouldDrawCubes ) {
          drawCubes3D();
        }
        if( shouldDrawFaces ) {
          drawFaces3D();
        }
      } else {
        if( shouldDrawRobot ) {
          drawRobot2D();
        }
        if( shouldDrawCubes ) {
          drawCubes2D();
        }
      }
      vizDirty = false;
      
    };

    var mouseWithinCanvas = function() {
      return (p.mouseX >= 0) && (p.mouseX < kCanvasWidth) && (p.mouseY >= 0) && (p.mouseY < kCanvasHeight);
    };
    p.mousePressed = function( event ) {
      if( quadTreeQuads.length == 0 ) {
        return true;
      }
      if( mouseWithinCanvas() ) {
        dragging = true;
        draggingInfo = {};
        draggingInfo.startX = p.mouseX;
        draggingInfo.startY = p.mouseY;
        if( is3D ) {
          var clickedWorld = imageToWorld( draggingInfo.startX, draggingInfo.startY );
          draggingInfo.clickedData = false;
          if( typeof clickedWorld !== 'undefined' ) {
            if( (clickedWorld.x >= dataExtentsInfo.minX*kDataScaleFactor) &&
                (clickedWorld.x <= dataExtentsInfo.maxX*kDataScaleFactor) &&
                (clickedWorld.y >= dataExtentsInfo.minY*kDataScaleFactor) &&
                (clickedWorld.y <= dataExtentsInfo.maxY*kDataScaleFactor) ) 
            {
              draggingInfo.clickedData = true;
              draggingInfo.startDataX = clickedWorld.x;
              draggingInfo.startDataY = clickedWorld.y;
            }
          }
          draggingInfo.startCamera = camera.clone();
        } else { 
          draggingInfo.startXOffset = xOffset2D;
          draggingInfo.startYOffset = yOffset2D;
        }
      } else {
        dragging = false;
      }
      return !dragging; // consume the click if within canvas
    };
    p.mouseReleased = function( event ) {
      if( quadTreeQuads.length == 0 ) {
        return true;
      }
      var oldDragging = dragging;
      dragging = false;
      return !oldDragging; 
    }
    p.mouseDragged = function( event ) {
      if( quadTreeQuads.length == 0 ) {
        return true;
      }
      if( dragging ) {
        var dx = p.mouseX - draggingInfo.startX;
        var dy = p.mouseY - draggingInfo.startY;
        if( is3D ) {
          dx *= 0.01;
          dy *= 0.01;
          camera = draggingInfo.startCamera.clone();
          if( !draggingInfo.clickedData ) {
            camera.rotate( dx, -dy );
          } else {
            // mouse movement left/right rotates about z, and up/down about the camera right direction
            var rotAxisX = new Vector( 0, 0, -1.0 );
            var rotAxisY = camera.getLookVector().cross( camera.getUpVector() ).makeUnitLength();
            camera.rotateAbout( draggingInfo.startDataX, draggingInfo.startDataY, 0.0, rotAxisY, dy );
            camera.rotateAbout( draggingInfo.startDataX, draggingInfo.startDataY, 0.0, rotAxisX, -dx );
          }
        } else {
          xOffset2D = draggingInfo.startXOffset - dx/scaleFactor2D;
          yOffset2D = draggingInfo.startYOffset - dy/scaleFactor2D;
        }
        vizDirty = true;
      }
      return !dragging; // consume the drag if started in canvas
    }
    
    p.mouseWheel = function( event ) {
      if( !dragging && event.isTrusted && mouseWithinCanvas() && (quadTreeQuads.length != 0) ) {
        if( is3D ) {
          var delta = -5*event.delta; // this is max(deltaX,deltaY)
          delta = Math.min( Math.max( delta, -100 ), 100 );
          var v = camera.getLookVector().makeUnitLength();
          camera.move( v.scale( delta ) );
        } else {
          var delta = 0.5*event.delta; // this is max(deltaX,deltaY)
          var prevScaleFactor = scaleFactor2D;
          scaleFactor2D *= (100 - delta)/100; // todo: exponential?
          // change offset so it keep the point under the mouse stationary
          xOffset2D += p.mouseX * (1.0/prevScaleFactor - 1.0/scaleFactor2D);
          yOffset2D += p.mouseY * (1.0/prevScaleFactor - 1.0/scaleFactor2D);
        }
        vizDirty = true;
        return false;
      } else {
        // forward the mouse wheel call
        return true;
      }
      
    };
  };

  // webviz methods

  myMethods.init = function(elem) {
    updateBtn = $('<input type="button" value="Update"/>');
    updateBtn.click( function(){
      if( dumpInput ) {
        $('#pastebin').html();
      }
      callUpdate();
    });
    updateBtn.appendTo( elem );

    var chkAuto = $('<input />', { type: 'checkbox', id: 'chkAuto'}).appendTo( elem );
    $('<label />', { for: 'chkAuto', text: 'Auto-update' }).appendTo( elem );
    var chk3D = $('<input />', { type: 'checkbox', id: 'chk3D'}).appendTo(elem).prop('checked', true);
    $('<label />', { for: 'chk3D', text: '3D' }).appendTo( elem );
    var chkRobot = $('<input />', { type: 'checkbox', id: 'chkRobot'}).appendTo(elem).prop('checked', true);
    $('<label />', { for: 'chkRobot', text: 'Show robot' }).appendTo( elem );
    var chkCubes = $('<input />', { type: 'checkbox', id: 'chkCubes'}).appendTo(elem).prop('checked', true);
    $('<label />', { for: 'chkCubes', text: 'Show cubes' }).appendTo( elem );
    var chkFaces = $('<input />', { type: 'checkbox', id: 'chkFaces'}).appendTo(elem).prop('checked', true);
    $('<label />', { for: 'chkFaces', text: 'Show faces' }).appendTo( elem );
    chkAuto.change( function() {
      autoUpdate = $(this).is(':checked');
      updateBtn.prop('disabled', autoUpdate);
    });
    chkRobot.change( function() {
      var old = shouldDrawRobot;
      shouldDrawRobot = $(this).is(':checked');
      vizDirty = vizDirty || (old != shouldDrawRobot);
    });
    chkCubes.change( function() {
      var old = shouldDrawCubes;
      shouldDrawCubes = $(this).is(':checked');
      vizDirty = vizDirty || (old != shouldDrawCubes);
    });
    chkFaces.change( function() {
      var old = shouldDrawFaces;
      shouldDrawFaces = $(this).is(':checked');
      vizDirty = vizDirty || (old != shouldDrawFaces);
    });
    chk3D.change( function() {
      var old = is3D;
      is3D = $(this).is(':checked');
      if( old != is3D ) {
        // 2d faces not supported yet
        if( is3D ) {
          chkFaces.show(); 
          $('label[for="chkFaces"]').show()
        } else { 
          chkFaces.hide(); 
          $('label[for="chkFaces"]').hide()
        }
        if( typeof myp5 !== 'undefined' ) {
          // destroy canvas and start again
          myp5.remove();
          myp5 = undefined;
          if( typeof canvasContainer !== 'undefined' ) {
            canvasContainer.remove();
          }
          canvasContainer = undefined;
          legendContainer.remove();
          scaleFactor2D = undefined;
        }
        if( !waitingOnData ) {
          timeTilAutoUpdate = kAutoUpdatePeriod_s;
          callUpdate();

        }
      }
    })
    // todo: remove when fixed
    noteDiv = $('<div>NOTE: the y axis is flipped (e.g., when the robot turns right it will look like it turned left here), and face poses aren\'t correct yet</div>').appendTo( elem );

    callUpdate();
  };

  function initializeSketch(elem) {
    canvasContainer = $('<div></div>', {id: 'navMapContainer'}).appendTo( elem );
    myp5 = new p5(sketch, 'navMapContainer');

    legendContainer = $('<div></div>', {id: 'legendContainer'}).appendTo( elem );
    for( var idx=0; idx<kKnownTypes.length; ++idx ) {
      legendContainer.append( '<span class="navMapLegendEntry" data-quadtype="' + kKnownTypes[idx] + '">' + kKnownTypes[idx] + '</span');
    }
    if( dumpInput ) {
      $('<div id="pastebin"></div>').appendTo( elem );
    }
  }
  myMethods.onData = function( data, elem ) {
    
    if( typeof canvasContainer === 'undefined' ) {
      initializeSketch( elem );
    }
    
    if( dumpInput ) {
      $('#pastebin').html( $('#pastebin').html() + '\n\n************************************\n\n' + JSON.stringify(data) );
    }

    var type = data["type"];
    var originId = data["originId"];
    if( type == 'MemoryMapMessageVizBegin' ) {
      // clear for the incoming origin
      memoryMapQuadInfoVectorMapIncoming[originId] = {};
      memoryMapInfo[originId] = data["mapInfo"];
    }
    else if( type == "MemoryMapMessageViz" ) {
      var dest = memoryMapQuadInfoVectorMapIncoming[originId];
      dest[data["seqNum"]] = data["quadInfos"];
    }
    else if( type == "MemoryMapMessageVizEnd" ) {

      quadTreeQuads = [];
      dataExtentsInfo = {minX: Number.MAX_VALUE, maxX: -Number.MAX_VALUE, minY: Number.MAX_VALUE, maxY: -Number.MAX_VALUE};

      centerX_m = 0.001 * memoryMapInfo[originId].rootCenterX;
      centerY_m = 0.001 * memoryMapInfo[originId].rootCenterY;
      depth = memoryMapInfo[originId].rootDepth;
      rootSize = 0.001 * memoryMapInfo[originId].rootSize_mm;
      
      var root = new MemoryMapNode( depth, rootSize, new Point( centerX_m, centerY_m ) );
      var expectedSeqNum = 0; // u32
      var srcQuadInfos = memoryMapQuadInfoVectorMapIncoming[originId];
      for( var seqNum in srcQuadInfos ){
        if( seqNum != expectedSeqNum ) {
          console.log('DROPPED VIZ MESSAGE. map will be incorrect');
          break;
        } else {  
          if( srcQuadInfos.hasOwnProperty( seqNum ) ) {
            var quadInfo = srcQuadInfos[seqNum];
            for( var idx=0; idx<quadInfo.length; ++idx ) {
              var quad = quadInfo[idx];
              root.AddChild( quadTreeQuads, dataExtentsInfo, quad["content"], quad["depth"] );
            }
            ++expectedSeqNum;
          }
        }
      }

      delete memoryMapQuadInfoVectorMapIncoming[ originId ];
      delete memoryMapInfo[ originId ];

      robotPosition = data["robot"];

      vizDirty = true;
      updateBtn.prop('disabled', autoUpdate);
      waitingOnData = false;
    } 
    else if( type == "MemoryMapCubes" ) {
      var newCubeData = data["cubes"];
      if( typeof cubeData === 'undefined' || (newCubeData.length != cubeData.length) ) {
        cubeData = newCubeData;
      }
    } 
    else if( type == "MemoryMapFace" ) {
      var id = data["faceID"];
      if( typeof faceData[id] === 'undefined' ) {
        faceData[id] = data;
      }
    }
    else if( type == "RobotDeletedFace" ) {
      var id = data["faceID"];
      if( typeof faceData[id] !== 'undefined' ) {
        delete faceData[id];
      }
    }
  };

  var kAutoUpdatePeriod_s = 5.0; 
  var timeTilAutoUpdate = kAutoUpdatePeriod_s;
  myMethods.update = function( dt, elem ) {
    timeTilAutoUpdate -= dt;
    if( (timeTilAutoUpdate < 0) 
        && autoUpdate 
        && !waitingOnData ) 
    {
      callUpdate();
      timeTilAutoUpdate = kAutoUpdatePeriod_s;
    }
  };

  myMethods.getStyles = function() {
    var styles = `
      span.navMapLegendEntry {
        display: block;
        margin: 1px 3px 0px 0px;
        font-size:10px;
      }
      span.navMapLegendEntry:before {
        content: "";
        display: inline-block;
        width: 12px;
        height: 12px;
        margin-right: 5px;
      }
      input[type=checkbox] {
        margin-left:20px;
        margin-right:5px;
      }
    `;
    for( var idx=0; idx<kKnownTypes.length; ++idx ) {
      var color = getQuadColor( kKnownTypes[idx] );
      styles += 'span.navMapLegendEntry[data-quadtype="' + kKnownTypes[idx] + '"]:before {';
      styles +=    'background: rgba(' + color.r + ',' + color.g + ',' + color.b + ',' + ((1.0*color.a)/255) + ')';
      styles += '}';
    }
    return styles;
  };

  // for debugging when there's no engine connected
  function fakeData() {
    var msgs = [];
    msgs.push( '{"mapInfo":{"identifier":"QuadTree_0x7fdb4d531d80","rootCenterX":560,"rootCenterY":240,"rootCenterZ":1,"rootDepth":7,"rootSize_mm":1280},"originId":1,"type":"MemoryMapMessageVizBegin"}' );
    msgs.push( '{"originId":1,"quadInfos":[{"content":"Unknown","depth":6},{"content":"Unknown","depth":5},{"content":"Unknown","depth":5},{"content":"Unknown","depth":4},{"content":"Unknown","depth":4},{"content":"Unknown","depth":4},{"content":"Unknown","depth":3},{"content":"Unknown","depth":3},{"content":"Unknown","depth":2},{"content":"Unknown","depth":2},{"content":"Unknown","depth":2},{"content":"Unknown","depth":1},{"content":"Unknown","depth":1},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":3},{"content":"Unknown","depth":5},{"content":"Unknown","depth":6},{"content":"Unknown","depth":4},{"content":"Unknown","depth":2},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":2},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":2},{"content":"Unknown","depth":2},{"content":"Unknown","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":2},{"content":"Unknown","depth":2},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":2},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":2},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":2},{"content":"Unknown","depth":4},{"content":"Unknown","depth":2},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":2},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":2},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":2},{"content":"Unknown","depth":2},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":2},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":2},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":2},{"content":"Unknown","depth":5},{"content":"Unknown","depth":4},{"content":"Unknown","depth":2},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":2},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfCliff","depth":0},{"content":"ClearOfCliff","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":2},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfCliff","depth":0},{"content":"ClearOfCliff","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":2},{"content":"Unknown","depth":2},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfCliff","depth":1},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfCliff","depth":1},{"content":"Unknown","depth":2},{"content":"Unknown","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"Unknown","depth":2},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"Unknown","depth":2},{"content":"Unknown","depth":4},{"content":"Unknown","depth":2},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"Unknown","depth":2},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"ClearOfCliff","depth":1},{"content":"ClearOfObstacle","depth":1},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ClearOfObstacle","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"ObstacleCharger","depth":0},{"content":"Unknown","depth":0},{"content":"Unknown","depth":1},{"content":"Unknown","depth":5}],"seqNum":0,"type":"MemoryMapMessageViz"}' );
    msgs.push( '{"originId":1,"type":"MemoryMapMessageVizEnd","robot": {"x": 0, "y": 0, "z": 0, "qW":1.0,"qX":0.0,"qY":0.0,"qZ":0.0}}' );
    msgs.push( '{"faceID":1,"pose":{"qW":0.7038945423784015,"qX":-0.06860959401309058,"qY":0.06858566274866171,"qZ":-0.7036484944093795,"x":768.1229248046875,"y":-10.172940254211426,"z":174.9200439453125},"timestamp":35655,"type":"MemoryMapFace"}' );
    msgs.push( '{"cubes":[{"angle":0.049684006720781326,"x":99.4178695678711,"y":0.0902092456817627,"z":29.857250213623047}],"type":"MemoryMapCubes"}' );
    
    for( var idx=0; idx<msgs.length; ++idx ) {
      myMethods.onData( JSON.parse( msgs[idx] ), $('#tab-navmap') );
    }
  }

})(moduleMethods, moduleSendDataFunc);
