function [edgeCenterPairs, symmetryPairs] = computeProbeLocations(varargin)

blockHalfWidth = 5;
numSymmetryProbes = 4;
numEdgeCenterProbes = 8;
minElevationAngle = 30;

parseVarargin(varargin{:});

%% Initialize
dotRadius = blockHalfWidth - 1;
blockSize = 2*blockHalfWidth + 1;
centerRadius = cos(minElevationAngle*pi/180) * dotRadius;

%% Center/Edge Probes
edgeCenterAngles = linspace(0, 2*pi, numEdgeCenterProbes+1);
edgeCenterPairs = zeros(numEdgeCenterProbes,2);
for i = 1:numEdgeCenterProbes
    edgeX = round(blockHalfWidth*cos(edgeCenterAngles(i)));
    edgeY = round(blockHalfWidth*sin(edgeCenterAngles(i)));
    
    cenX = round(centerRadius/2*cos(edgeCenterAngles(i)));
    cenY = round(centerRadius/2*sin(edgeCenterAngles(i)));
    
    edgeCenterPairs(i,:) = sub2ind([blockSize blockSize], ...
        [edgeY cenY]+blockHalfWidth+1, [edgeX cenX]+blockHalfWidth+1);
        
end


%% Symmetry Probes
symmetryRadius = (dotRadius + centerRadius)/2;
symmetryAngles = linspace(0, pi, numSymmetryProbes+1);
symmetryPairs = zeros(numSymmetryProbes, 2);
for i = 1:numSymmetryProbes
    symX = round(symmetryRadius*cos(symmetryAngles(i)+[0 pi]));
    symY = round(symmetryRadius*sin(symmetryAngles(i)+[0 pi]));
    
    symmetryPairs(i,:) = sub2ind([blockSize blockSize], ...
        symY+blockHalfWidth+1, symX+blockHalfWidth+1);
end

    