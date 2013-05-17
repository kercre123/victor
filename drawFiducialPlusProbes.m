function drawFiducialPlusProbes(varargin)

blockHalfWidth = 5;
dotRadius = blockHalfWidth - 1;
viewAngle = 60;
rotationAngle = 20;
maxAngle = 70;
numSymmetryProbes = 8;
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
for i = 1:numEdgeCenterProbes
    rectangle('Pos', ...
        [round(blockHalfWidth*cos(edgeCenterAngles(i)))-.5 ...
        round(blockHalfWidth*sin(edgeCenterAngles(i)))-.5 ...
        1 1], 'EdgeColor', 'b');
    
    rectangle('Pos', ...
        [round(centerRadius/2*cos(edgeCenterAngles(i)))-.5 ...
        round(centerRadius/2*sin(edgeCenterAngles(i)))-.5 ...
        1 1], 'EdgeColor', 'b');
end


% Symmetry Probes
symmetryRadius = (dotRadius + centerRadius)/2;
symmetryAngles = linspace(0, 2*pi, numSymmetryProbes+1);
for i = 1:numSymmetryProbes
    symmetryProbes(i) = rectangle('Pos', ...
        [round(symmetryRadius*cos(symmetryAngles(i)))-.5 ...
        round(symmetryRadius*sin(symmetryAngles(i)))-.5 ...
        1 1], 'EdgeColor', 'g');
end

set(gca, 'XLim', (blockHalfWidth+1)*[-1 1], 'YLim', (blockHalfWidth+1)*[-1 1]);
    