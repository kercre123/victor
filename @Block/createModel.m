function createModel(this)
% Block origin is lower left corner of the "front" face (the one with
% lowest faceType value)
% The marker origin is the upper left inside corner of the square fiducial.

% Scale is [width length height], in *world* coordinates
 this.markers = containers.Map('KeyType', 'double', 'ValueType', 'any');

 % Tiny spacing so markers draw in front of block face
e = .1;

switch(this.blockType)
    
    case 10 % Green 2x1x4 Block, two marker faces
        this.color = 'g';
        scale = [63 31 75];
        
        % Front face = 1:
        frontFrame = Frame(eye(3), [19 -e 44.5]); % Relative to block frame
        this.markers(1) = BlockMarker3D(this, 1, frontFrame);
        
        % Back face = 5:
        backFrame = Frame(pi*[0 0 1], [45.5 scale(2)+e 44.5]);
        this.markers(5) = BlockMarker3D(this, 5, backFrame);
        
    case 15 % Blue 2x1x4 Block, two marker faces
        this.color = 'b';
        scale = [63 31 75];
        
        % Front face = 1:
        frontFrame = Frame(eye(3), [19 -e 44]); % Relative to block frame
        this.markers(1) = BlockMarker3D(this, 1, frontFrame);
        
        % Back face = 5:
        backFrame = Frame(pi*[0 0 1], [44 scale(2)+e 43]);
        this.markers(5) = BlockMarker3D(this, 5, backFrame);
        
    case 20 % Red 2x1x4 Block, two marker faces
        this.color = 'r';
        scale = [63 31 75];
        
        % Front face = 1:
        frontFrame = Frame(eye(3), [19 -e 44]); % Relative to block frame
        this.markers(1) = BlockMarker3D(this, 1, frontFrame);
        
        % Back face = 5:
        backFrame = Frame(pi*[0 0 1], [44 scale(2)+e 43]);
        this.markers(5) = BlockMarker3D(this, 5, backFrame);
        
    case 25 % Blue 2x1x5 Block, two marker faces
        this.color = 'b';
        scale = [63 31 94.5];
        
        % Front face = 1:
        frontFrame = Frame(eye(3), [18 -e 44]); % Relative to block frame
        this.markers(1) = BlockMarker3D(this, 1, frontFrame);
        
        % Back face = 5:
        backFrame = Frame(pi*[0 0 1], [45 scale(2)+e 44]);
        this.markers(5) = BlockMarker3D(this, 5, backFrame);
    
    case 30 % Red 2x1x5 Block, two marker faces
        this.color = 'r';
        scale = [63 31 94.5];
        
        % Front face = 1:
        frontFrame = Frame(eye(3), [17 -e 43.5]); % Relative to block frame
        this.markers(1) = BlockMarker3D(this, 1, frontFrame);
        
        % Back face = 5:
        backFrame = Frame(pi*[0 0 1], [45.5 scale(2)+e 44]);
        this.markers(5) = BlockMarker3D(this, 5, backFrame);
        
    case 35 % Red 2x2x3 Block
        this.color = 'r';
        scale = [63 63 56];
        
        % Front face = 1:
        frontFrame = Frame(eye(3), [19 -e 38]);
        this.markers(1) = BlockMarker3D(this, 1, frontFrame);
        
        % Right face = 5:
        rightFrame = Frame(pi/2*[0 0 1], [scale(1)+e 18 38]);
        this.markers(5) = BlockMarker3D(this, 5, rightFrame);
        
        % Back face = 8:
        backFrame = Frame(pi*[0 0 1], [44.5 scale(2)+e 38.5]);
        this.markers(8) = BlockMarker3D(this, 8, backFrame);
        
        % Left face = 10:
        leftFrame = Frame(-pi/2*[0 0 1], [-e 45 38.5]);
        this.markers(10) = BlockMarker3D(this, 10, leftFrame);
        
        
    case 40 % Green 2x2x3 Block
        this.color = 'g';
        scale = [63 63 56];
        
        % Front face = 1:
        frontFrame = Frame(eye(3), [18.5 -e 38.5]);
        this.markers(1) = BlockMarker3D(this, 1, frontFrame);
        
        % Right face = 5:
        rightFrame = Frame(pi/2*[0 0 1], [scale(1)+e 18 38.5]);
        this.markers(5) = BlockMarker3D(this, 5, rightFrame);
        
        % Back face = 8:
        backFrame = Frame(pi*[0 0 1], [44.5 scale(2)+e 38]);
        this.markers(8) = BlockMarker3D(this, 8, backFrame);
        
        % Left face = 10:
        leftFrame = Frame(-pi/2*[0 0 1], [-e 45 38.5]);
        this.markers(10) = BlockMarker3D(this, 10, leftFrame);
                
    case 45 % Blue 3x2x3 Block
        this.color = 'b';
        scale = [94 63 56];
        
        % Front face = 1:
        frontFrame = Frame(eye(3), [33.5 -e 38]);
        this.markers(1) = BlockMarker3D(this, 1, frontFrame);
        
        % Right face = 5:
        rightFrame = Frame(pi/2*[0 0 1], [scale(1)+e 18 38]);
        this.markers(5) = BlockMarker3D(this, 5, rightFrame);
        
        % Back face = 8:
        backFrame = Frame(pi*[0 0 1], [61 scale(2)+e 38]);
        this.markers(8) = BlockMarker3D(this, 8, backFrame);
        
        % Left face = 10:
        leftFrame = Frame(-pi/2*[0 0 1], [-e 45 38.5]);
        this.markers(10) = BlockMarker3D(this, 10, leftFrame);
        
    case 50 % Yellow 4x2x3 Block
        this.color = 'y';
        scale = [126 63 56];
        
        % Front face = 1:
        frontFrame = Frame(eye(3), [48 -e 38.5]);
        this.markers(1) = BlockMarker3D(this, 1, frontFrame);
        
        % Right face = 5:
        rightFrame = Frame(pi/2*[0 0 1], [scale(1)+e 18 38]);
        this.markers(5) = BlockMarker3D(this, 5, rightFrame);
        
        % Back face = 8:
        backFrame = Frame(pi*[0 0 1], [74 scale(2)+e 38]);
        this.markers(8) = BlockMarker3D(this, 8, backFrame);
        
        % Left face = 10:
        leftFrame = Frame(-pi/2*[0 0 1], [-e 44 39]);
        this.markers(10) = BlockMarker3D(this, 10, leftFrame);
        
                
    otherwise
        error('No model defined for block type %d.', this.blockType);
    
end % SWITCH(blockType)

% Scale the canonical unit cube:
this.Xmodel = scale(1)*[0 1 1 0; 0 1 1 0; 0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0]';
this.Ymodel = scale(2)*[0 0 1 1; 0 0 1 1; 0 0 1 1; 0 0 1 1; 1 1 1 1; 0 0 0 0]';
this.Zmodel = scale(3)*[0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0; 0 0 1 1; 0 0 1 1]';

end