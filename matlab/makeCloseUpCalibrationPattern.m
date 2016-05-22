function makeCloseUpCalibrationPattern(varargin)

gridSize = [12 16]; % number of squares
squareSize = 2; % mm
margin = 30; % mm

parseVarargin(varargin{:});

namedFigure('CloseUpCalibration', 'Units', 'centimeters');

cla
rectangle('Pos', [0 0 11 8.5]*25.4, 'LineStyle', '--');

for i = 1:gridSize(1)
    for j = (mod(i,2)+1):2:gridSize(2)
        
        rectangle('Pos', [(i-1)*squareSize+margin (j-1)*squareSize+margin ...
            squareSize squareSize], 'FaceColor', 'k', 'EdgeColor', 'none')
        
    end
end

axis equal