function gridMat(varargin)

% Parameters
squareSize = 10; % mm
lineWidth  = 1.5; % mm
matSize    = 1000; % mm
codePadding = 1; % mm

parseVarargin(varargin{:});

h_fig = namedFigure('GridMat');
figure(h_fig);
cla

% Grid
gridLines = 0:squareSize:matSize;
numLines = length(gridLines);
h_line = zeros(2, numLines);
for i = 1:numLines
    h_line(1,i) = rectangle('Pos', [gridLines(i)-lineWidth/2 0 lineWidth matSize]);
    h_line(2,i) = rectangle('Pos', [0 gridLines(i)-lineWidth/2 matSize lineWidth]);
end
set(h_line, 'FaceColor', 'k');
hold on

% % Draw grid lines 
% % Problem: can't control width precisely
% h_vertLines = line(gridLines(ones(2,1),:), [zeros(1,numLines); matSize*ones(1,numLines)]);
% h_horzLines = line([zeros(1,numLines); matSize*ones(1,numLines)], gridLines(ones(2,1),:));
% set([h_vertLines;h_horzLines], 'Color', 'k', 'LineWidth', 3);

% Square Centers
[xcen,ycen] = meshgrid(squareSize/2:squareSize:matSize-squareSize/2);
% h_cen = plot(xcen, ycen, 'b.');

% Codes
codePos = squareSize/2 - lineWidth/2 - codePadding - .5;
[numRows, numCols] = size(xcen);
for i = 1:numRows
    for j = 1:numCols
        code = MatMarker2D.encodeIDs(j, i);
        
        imagesc([xcen(i,j)-codePos xcen(i,j)+codePos], ...
            [ycen(i,j)-codePos ycen(i,j)+codePos], ~code);
            
    end
end
axis ij equal
colormap(gray)


end % FUNCTION gridMat()

