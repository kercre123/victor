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
        
        defineFaceHelper(this, 'front', 1, [19 -e 44.5], firstMarkerID);
        defineFaceHelper(this, 'back', 5, [45.5 scale(2)+e 44.5], firstMarkerID);
        
    case 15 % Blue 2x1x4 Block, two marker faces
        this.color = 'b';
        scale = [63 31 75];
        this.markers = cell(1,2);
        
        defineFaceHelper(this, 'front', 1, [19 -e 44], firstMarkerID);
        defineFaceHelper(this, 'back', 5, [44 scale(2)+e 43], firstMarkerID);
        
    case 20 % Red 2x1x4 Block, two marker faces
        this.color = 'r';
        scale = [63 31 75];
        this.markers = cell(1,2);
        
        defineFaceHelper(this, 'front', 1, [19 -e 44], firstMarkerID);
        defineFaceHelper(this, 'back', 5, [44 scale(2)+e 43], firstMarkerID);
        
    case 25 % Blue 2x1x5 Block, two marker faces
        this.color = 'b';
        scale = [63 31 94.5];
        this.markers = cell(1,2);
        
        defineFaceHelper(this, 'front', 1, [18 -e 44], firstMarkerID);
        defineFaceHelper(this, 'back', 5, [45 scale(2)+e 44], firstMarkerID);
            
    case 30 % Red 2x1x5 Block, two marker faces
        this.color = 'r';
        scale = [63 31 94.5];
        this.markers = cell(1,2);
        
        defineFaceHelper(this, 'front', 1, [17 -e 43.5], firstMarkerID);
        defineFaceHelper(this, 'back', 5, [45.5 scale(2)+e 44], firstMarkerID);
                
    case 35 % Red 2x2x3 Block
        this.color = 'r';
        scale = [63 63 56];
        this.markers = cell(1, 4);
        
        defineFaceHelper(this, 'front', 1, [19 -e 38], firstMarkerID);
        defineFaceHelper(this, 'right', 5, [scale(1)+e 18 38], firstMarkerID);
        defineFaceHelper(this, 'back',  8, [44.5 scale(2)+e 38.5], firstMarkerID);
        defineFaceHelper(this, 'left', 10, [-e 45 38.5], firstMarkerID);
                
    case 40 % Green 2x2x3 Block
        this.color = 'g';
        scale = [63 63 56];
        this.markers = cell(1, 4);
        
        defineFaceHelper(this, 'front', 1, [18.5 -e 38.5], firstMarkerID);
        defineFaceHelper(this, 'right', 5, [scale(1)+e 18 38.5], firstMarkerID);
        defineFaceHelper(this, 'back',  8, [44.5 scale(2)+e 38], firstMarkerID);
        defineFaceHelper(this, 'left', 10, [-e 45 38.5], firstMarkerID);
                
    case 45 % Blue 3x2x3 Block
        this.color = 'b';
        scale = [94 63 56];
        this.markers = cell(1, 4);
        
        defineFaceHelper(this, 'front', 1, [33.5 -e 38], firstMarkerID);
        defineFaceHelper(this, 'right', 5, [scale(1)+e 18 38], firstMarkerID);
        defineFaceHelper(this, 'back',  8, [61 scale(2)+e 38], firstMarkerID);
        defineFaceHelper(this, 'left', 10, [-e 45 38.5], firstMarkerID);
        
    case 50 % Yellow 4x2x3 Block
        this.color = 'y';
        scale = [126 63 56];
        this.markers = cell(1, 4);
        
        defineFaceHelper(this, 'front', 1, [48 -e 38.5], firstMarkerID);
        defineFaceHelper(this, 'right', 5, [scale(1)+e 18 38], firstMarkerID);
        defineFaceHelper(this, 'back',  8, [74 scale(2)+e 38], firstMarkerID);
        defineFaceHelper(this, 'left', 10, [-e 44 39], firstMarkerID);

    otherwise
        error('No model defined for block type %d.', this.blockType);
    
end % SWITCH(blockType)

% Scale the canonical unit cube:
% (each 4 element column -- note the transpose -- is a face)
X = scale(1)*[0 1 1 0; 0 1 1 0; 0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0]';
Y = scale(2)*[0 0 1 1; 0 0 1 1; 0 0 1 1; 0 0 1 1; 1 1 1 1; 0 0 0 0]';
Z = scale(3)*[0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0; 0 0 1 1; 0 0 1 1]';

this.model = [X(:) Y(:) Z(:)];

end

function defineFaceHelper(this, whichFace, faceType, origin, firstMarkerID)

switch(whichFace)
    case 'front'
        R = eye(3);
    case 'back'
        R = pi*[0 0 1];
    case 'right'
        R = pi/2*[0 0 1];
    case 'left'
        R = -pi/2*[0 0 1];
    otherwise
        error('Unrecognized face "%s"', whichFace);
end

index = this.faceTypeToIndex.Count + 1;
this.faceTypeToIndex(faceType) = index;
pose = Pose(R, origin);

assert(index <= length(this.markers), 'Index for markers too large.');
this.markers{index} = BlockMarker3D(this, faceType, pose, ...
    firstMarkerID + index);

end