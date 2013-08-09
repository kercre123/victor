function createModel(this, firstMarkerID)
% Block origin is lower left corner of the "front" face (the one with
% lowest faceType value)
% The marker origin is the upper left inside corner of the square fiducial.

% Scale is [width length height], in *world* coordinates
%this.markers = containers.Map('KeyType', 'double', 'ValueType', 'any');
this.faceTypeToIndex = containers.Map('KeyType', 'double', 'ValueType', 'double');

 % Tiny spacing so markers draw in front of block face
e = .1;
    
switch(this.blockType)
    
    case 10 % Green 2x1x4 Block, two marker faces
        this.color = 'g';
        scale = [63 31 75];
        this.markers = cell(1,2);
        
        defineFaceHelper(this, 'front', 1, [19 -e 44.5], scale, firstMarkerID);
        defineFaceHelper(this, 'back', 5, [45.5 scale(2)+e 44.5], scale, firstMarkerID);
        
    case 15 % Blue 2x1x4 Block, two marker faces
        this.color = 'b';
        scale = [63 31 75];
        this.markers = cell(1,2);
        
        defineFaceHelper(this, 'front', 1, [19 -e 44], scale, firstMarkerID);
        defineFaceHelper(this, 'back', 5, [44 scale(2)+e 43], scale, firstMarkerID);
        
    case 20 % Red 2x1x4 Block, two marker faces
        this.color = 'r';
        scale = [63 31 75];
        this.markers = cell(1,2);
        
        defineFaceHelper(this, 'front', 1, [19 -e 44], scale, firstMarkerID);
        defineFaceHelper(this, 'back', 5, [44 scale(2)+e 43], scale, firstMarkerID);
        
    case 25 % Blue 2x1x5 Block, two marker faces
        this.color = 'b';
        scale = [63 31 94.5];
        this.markers = cell(1,2);
        
        defineFaceHelper(this, 'front', 1, [18 -e 44], scale, firstMarkerID);
        defineFaceHelper(this, 'back', 5, [45 scale(2)+e 44], scale, firstMarkerID);
            
    case 30 % Red 2x1x5 Block, two marker faces
        this.color = 'r';
        scale = [63 31 94.5];
        this.markers = cell(1,2);
        
        defineFaceHelper(this, 'front', 1, [17 -e 43.5], scale, firstMarkerID);
        defineFaceHelper(this, 'back', 5, [45.5 scale(2)+e 44], scale, firstMarkerID);
                
    case 35 % Red 2x2x3 Block
        this.color = 'r';
        scale = [63 63 56];
        this.markers = cell(1, 4);
        
        defineFaceHelper(this, 'front', 1, [19 -e 38], scale, firstMarkerID);
        defineFaceHelper(this, 'right', 5, [scale(1)+e 18 38], scale, firstMarkerID);
        defineFaceHelper(this, 'back',  8, [44.5 scale(2)+e 38.5], scale, firstMarkerID);
        defineFaceHelper(this, 'left', 10, [-e 45 38.5], scale, firstMarkerID);
                
    case 40 % Green 2x2x3 Block
        this.color = 'g';
        scale = [63 63 56];
        this.markers = cell(1, 4);
        
        defineFaceHelper(this, 'front', 1, [18.5 -e 38.5], scale, firstMarkerID);
        defineFaceHelper(this, 'right', 5, [scale(1)+e 18 38.5], scale, firstMarkerID);
        defineFaceHelper(this, 'back',  8, [44.5 scale(2)+e 38], scale, firstMarkerID);
        defineFaceHelper(this, 'left', 10, [-e 45 38.5], scale, firstMarkerID);
                
    case 45 % Blue 3x2x3 Block
        this.color = 'b';
        scale = [94 63 56];
        this.markers = cell(1, 4);
        
        defineFaceHelper(this, 'front', 1, [33.5 -e 38], scale, firstMarkerID);
        defineFaceHelper(this, 'right', 5, [scale(1)+e 18 38], scale, firstMarkerID);
        defineFaceHelper(this, 'back',  8, [61 scale(2)+e 38], scale, firstMarkerID);
        defineFaceHelper(this, 'left', 10, [-e 45 38.5], scale, firstMarkerID);
        
    case 50 % Yellow 4x2x3 Block
        this.color = 'y';
        scale = [126 63 56];
        this.markers = cell(1, 4);
        
        defineFaceHelper(this, 'front', 1, [48 -e 38.5], scale, firstMarkerID);
        defineFaceHelper(this, 'right', 5, [scale(1)+e 18 38], scale, firstMarkerID);
        defineFaceHelper(this, 'back',  8, [74 scale(2)+e 38], scale, firstMarkerID);
        defineFaceHelper(this, 'left', 10, [-e 44 39], scale, firstMarkerID);

    case 60 % Webot Simulated Red 1x1 Block
        scale = makeWebotBlockHelper(this, 'r', firstMarkerID);
        
    case 65 % Webot Simulated Green 1x1 Block
        scale = makeWebotBlockHelper(this, 'g', firstMarkerID);
        
    case 70 % Webot Simulated Blue 1x1 Block
        scale = makeWebotBlockHelper(this, 'b', firstMarkerID);
        
    otherwise
        error('No model defined for block type %d.', this.blockType);
    
end % SWITCH(blockType)

% Scale the canonical unit cube:
% (each 4 element column -- note the transpose -- is a face)
X = scale(1)*[0 1 1 0; 0 1 1 0; 0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0]';
Y = scale(2)*[0 0 1 1; 0 0 1 1; 0 0 1 1; 0 0 1 1; 1 1 1 1; 0 0 0 0]';
Z = scale(3)*[0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0; 0 0 1 1; 0 0 1 1]';

this.model = [X(:)-scale(1)/2 Y(:)-scale(2)/2 Z(:)-scale(3)/2];

end

function defineFaceHelper(this, whichFace, faceType, origin, scale, firstMarkerID)

switch(whichFace)
    case 'front'
        R = eye(3);
    case 'back'
        R = pi*[0 0 1];
    case 'right'
        R = pi/2*[0 0 1];
    case 'left'
        R = -pi/2*[0 0 1];
    case 'top'
        R = -pi/2*[1 0 0];
    case 'bottom'
        R = pi/2*[1 0 0];
    otherwise
        error('Unrecognized face "%s"', whichFace);
end

index = this.faceTypeToIndex.Count + 1;
this.faceTypeToIndex(faceType) = index;
pose = Pose(R, origin-scale/2);

assert(index <= length(this.markers), 'Index for markers too large.');
this.markers{index} = BlockMarker3D(this, faceType, pose, ...
    firstMarkerID + index);

end


function scale = makeWebotBlockHelper(this, color, firstMarkerID)
this.color = color;
scale = [60 60 60];
this.markers = cell(1, 6);

e = .1;
halfwidth = BlockMarker3D.Width / 2;
offset1 = scale(1)/2 - halfwidth;
offset2 = scale(1)/2 + halfwidth;

% Note: there is a coordinate change between Webot's world and BlockWorld.
% In BlockWorld, the ground is (x,y) with x point from left to right and y
% pointing away from you.  z is elevation off the ground.  However, in
% Webot's world, x points away from, z points left/right, and y is
% elevation off the ground.
defineFaceHelper(this, 'front',  1, [offset1 -e offset2], scale, firstMarkerID);
defineFaceHelper(this, 'back',   2, [offset2 scale(2)+e offset2], scale, firstMarkerID);
defineFaceHelper(this, 'top',    3, [offset1 offset2 scale(3)+e], scale, firstMarkerID);
defineFaceHelper(this, 'bottom', 4, [offset1 offset1 -e], scale, firstMarkerID);
defineFaceHelper(this, 'left',   5, [-e offset2 offset2], scale, firstMarkerID);
defineFaceHelper(this, 'right',  6, [scale(1)+e offset1 offset2], scale, firstMarkerID);
end