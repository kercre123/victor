function [edgeCenterPairs, symmetryPairs] = drawFiducialPlusProbes(varargin)

blockHalfWidth = 5;
dotRadius = blockHalfWidth - 1;
viewAngle = 60;
rotationAngle = 20;
maxAngle = 70;
numSymmetryProbes = 4;
numEdgeCenterProbes = 8;

parseVarargin(varargin{:});

blockSize = 2*blockHalfWidth + 1;

cla
hold off
h_rect = rectangle('Pos', ...
    [-blockHalfWidth-.5 -blockHalfWidth-.5 blockSize blockSize], ...
    'LineStyle', '--');
hold on

%h_dot = rectangle('Pos', dotRadius*[-1 -1 2 2], 'Curvature', [1 1], ...
%    'FaceColor', 'k');
t = linspace(0, 2*pi, 200);
x = dotRadius*cos(viewAngle*pi/180)*cos(t);
y = dotRadius*sin(t);
temp = [cos(rotationAngle*pi/180) sin(rotationAngle*pi/180); ...
    -sin(rotationAngle*pi/180) cos(rotationAngle*pi/180)] * [x; y];
h_dot = plot(temp(1,:), temp(2,:), 'k', 'LineWidth', 2);

centerRadius = cos(maxAngle*pi/180) * dotRadius;
h_center = rectangle('Pos', centerRadius*[-1 -1 2 2], ...
    'Curvature', [1 1], 'EdgeColor', 'r');    

axis image xy

grid on
set(gca, 'XTick', -(blockHalfWidth+.5):(blockHalfWidth+.5), ...
    'YTick', -(blockHalfWidth+.5):(blockHalfWidth+.5));

% Center/Edge Probes
edgeCenterAngles = linspace(0, 2*pi, numEdgeCenterProbes+1);
edgeCenterPairs = zeros(numEdgeCenterProbes,2);
for i = 1:numEdgeCenterProbes
    edgeX = round(blockHalfWidth*cos(edgeCenterAngles(i)));
    edgeY = round(blockHalfWidth*sin(edgeCenterAngles(i)));
    rectangle('Pos', [edgeX-.5 edgeY-.5 1 1], 'EdgeColor', 'b');
    
    cenX = round(centerRadius/2*cos(edgeCenterAngles(i)));
    cenY = round(centerRadius/2*sin(edgeCenterAngles(i)));
    rectangle('Pos', [cenX-.5 cenY-.5 1 1], 'EdgeColor', 'b');
    
    edgeCenterPairs(i,:) = sub2ind([blockSize blockSize], ...
        [edgeY cenY]+blockHalfWidth+1, [edgeX cenX]+blockHalfWidth+1);
        
end


% Symmetry Probes
symmetryRadius = (dotRadius + centerRadius)/2;
symmetryAngles = linspace(0, pi, numSymmetryProbes+1);
symmetryPairs = zeros(numSymmetryProbes, 2);
for i = 1:numSymmetryProbes
    symX = round(symmetryRadius*cos(symmetryAngles(i)+[0 pi]));
    symY = round(symmetryRadius*sin(symmetryAngles(i)+[0 pi]));
    
    rectangle('Pos', [symX(1)-.5 symY(1)-.5 1 1], 'EdgeColor', 'g');
    rectangle('Pos', [symX(2)-.5 symY(2)-.5 1 1], 'EdgeColor', 'g');
    
    symmetryPairs(i,:) = sub2ind([blockSize blockSize], ...
        symY+blockHalfWidth+1, symX+blockHalfWidth+1);
end

set(gca, 'XLim', (blockHalfWidth+1)*[-1 1], 'YLim', (blockHalfWidth+1)*[-1 1]);
    