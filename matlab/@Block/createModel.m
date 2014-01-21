function createModel(this, varargin)
% Block origin is lower left corner of the "front" face (the one with
% lowest faceType value)
% The marker origin is the upper left inside corner of the square fiducial.

firstMarkerID = 1;
faceImages = {};

parseVarargin(varargin{:});

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
        makeWebotBlock(this, '1x1', 'r', firstMarkerID);
        
    case 65 % Webot Simulated Green 1x1 Block
        makeWebotBlock(this, '1x1', 'g', firstMarkerID);
        
    case 70 % Webot Simulated Blue 1x1 Block
        makeWebotBlock(this, '1x1', 'b', firstMarkerID);
        
    case 75 % Webot Simulated Purple 2x1 Block
        makeWebotBlock(this, '2x1', [0.5 0.1 0.75], firstMarkerID);
        
    case 'fuel' % Webots Simulated 1x1 block with battery markers on all sides
        makeWebotBlock(this, '1x1', 'r', firstMarkerID, faceImages);
        
    otherwise
        error('No model defined for block type %d.', this.blockType);
    
end % SWITCH(blockType)


if this.blockType < 60 % HACK: if non-webot block, finish making model
    % Scale the canonical unit cube:
    % (each 4 element column -- note the transpose -- is a face)
    X = scale(1)*[0 1 1 0; 0 1 1 0; 0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0]';
    Y = scale(2)*[0 0 1 1; 0 0 1 1; 0 0 1 1; 0 0 1 1; 1 1 1 1; 0 0 0 0]';
    Z = scale(3)*[0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0; 0 0 1 1; 0 0 1 1]';
    
    this.model = [X(:)-scale(1)/2 Y(:)-scale(2)/2 Z(:)-scale(3)/2];
    this.dims = scale;
end


end

function defineFaceHelper(this, whichFace, faceType, origin, scale, firstMarkerID)

originAdjustment = 0;
switch(whichFace)
    case 'front'
        R = [0 0 0];
        if BlockMarker2D.UseOutsideOfSquare
            originAdjustment = [-5 0 5];
        end
    case 'back'
        R = pi*[0 0 1];
        if BlockMarker2D.UseOutsideOfSquare
            originAdjustment = [5 0 5];
        end
    case 'right'
        R = pi/2*[0 0 1];
        if BlockMarker2D.UseOutsideOfSquare
            originAdjustment = [0 -5 5];
        end
    case 'left'
        R = -pi/2*[0 0 1];
        if BlockMarker2D.UseOutsideOfSquare
            originAdjustment = [0 5 5];
        end
    case 'top'
        R = -pi/2*[1 0 0];
        if BlockMarker2D.UseOutsideOfSquare
            originAdjustment = [-5 5 0];
        end
    case 'bottom'
        R = pi/2*[1 0 0];
        if BlockMarker2D.UseOutsideOfSquare
            originAdjustment = [-5 -5 0];
        end
    otherwise
        error('Unrecognized face "%s"', whichFace);
end

Rmat = rodrigues(R);
origin = origin(:) + originAdjustment(:) + Rmat*BlockMarker3D.ReferenceWidth/2*[1 0 -1]';

index = this.faceTypeToIndex.Count + 1;
this.faceTypeToIndex(faceType) = index;
pose = Pose(Rmat, origin-scale(:)/2);
pose.name = sprintf('%sBlockMarkerPose_Face%', ...
    [upper(whichFace(1)) lower(whichFace(2:end))]);
pose.parent = this.pose;

assert(index <= length(this.markers), 'Index for markers too large.');
this.markers{index} = BlockMarker3D(this, faceType, pose, ...
    firstMarkerID + index);

end % FUNCTION defineFaceHelper()


function makeWebotBlock(this, shape, color, firstMarkerID, faceImages)

this.color = color;

switch(shape)
    case '1x1'
        scale = [60 60 60];
    case '2x1'
        scale = [120 60 60];
    otherwise
        error('Unrecognized shape "%s".', shape);
end

% Define block faces in Webots coordinate system (where Y points up)
X = scale(1)*[0 1 1 0; 0 1 1 0; 0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0]' - scale(1)/2;
Y = scale(2)*[0 0 1 1; 0 0 1 1; 0 0 1 1; 0 0 1 1; 1 1 1 1; 0 0 0 0]' - scale(2)/2;
Z = scale(3)*[0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0; 0 0 1 1; 0 0 1 1]' - scale(3)/2;

% Assume x points right to left, z points away from you, and y points up

% Create Marker3Ds in the correct pose relative to Block origin (center)
for faceID = 1:6
    switch(faceID)
        case 1
            % Face 1: -x side (rotate around y +90 degrees)
            angle = pi/2;
            axis  = [0 1 0];
            offset = [-scale(1)/2 0 0];
        case 2
            % Face 2: +x side (rotate around y -90 degrees)
            angle = -pi/2;
            axis = [0 1 0];
            offset = [scale(1)/2 0 0];
        case 3
            % Face 3: +y side (rotate around x +90 degrees)
            angle = pi/2;
            axis = [1 0 0];
            offset = [0 scale(2)/2 0];
        case 4
            % Face 4: -y side (rotate around x -90 degrees)
            angle = -pi/2;
            axis = [1 0 0];
            offset = [0 -scale(2)/2 0];
        case 5
            % Face 5: -z side (no rotation needed)
            angle = 0;
            axis = [0 1 0];
            offset = [0 0 -scale(3)/2];
        case 6
            % Face 6: +z side (rotate around y 180 degrees)
            angle = pi;
            axis = [0 1 0];
            offset = [0 0 scale(3)/2];
        otherwise
            error('Unrecognized face ID "%d".', faceID);
    end % SWITCH(faceID)
        
    % Switch to Matlab coordinates (negate x, swap y and z)
    axis = [-axis(1) axis(3) axis(2)];
    offset = [-offset(1) offset(3) offset(2)];
    
    % Bookkeeping (still needed? this is really only needed for
    % bundleAdjustment)
    index = this.faceTypeToIndex.Count + 1;
    this.faceTypeToIndex(faceID) = index;
    
    % Build the corresponding 3D marker at that Pose 
    P = Pose(angle*axis, offset(:));
    P.name = sprintf('BlockMarkerPose_Face%d', faceID);
    P.parent = this.pose;
         
    if ischar(this.blockType)
        assert(nargin >= 5, 'You must pass in faceImages for this block type.');
        
        % TODO: load the code directly instead of having to compute it 
        % from an image each time
        %img = imread(sprintf('~/Box Sync/Cozmo SE/VisionMarkers/blocks/%s/face%d.png', this.blockType, faceID));
        img = faceImages{faceID};
        
        % Assume this was drawn with a gap around the outside
        % TODO: Size should be probably be block-specific and passed in somehow
        [nrows,ncols,~] = size(img);
        corner = ncols*VisionMarker.FiducialPaddingFraction*VisionMarker.SquareWidth;
        this.markers{index} = VisionMarker(img, ...
            'Corners', [corner corner; corner nrows-corner; ncols-corner corner; ncols-corner nrows-corner], ...
            'RefineCorners', false, ...
            'Pose', P, 'Size', 32, 'Name', upper(this.blockType)); %sprintf('%s-FACE%d', upper(this.blockType), faceID));
    else
        this.markers{index} = BlockMarker3D(this, faceID, P, ...
            firstMarkerID + index);
    end
end

this.model = [X(:) Z(:) Y(:)];
this.dims = scale([1 3 2]);

end % FUNCTION makeWebotBlock()