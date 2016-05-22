function gridMat(varargin)

% Parameters
squareSize = 10; % mm
lineWidth  = 1.5; % mm
matSize    = 1000; % mm
codePadding = 1; % mm
pageDims = [];
axesHandle = [];
mirrorX = false;
mirrorY = false;

parseVarargin(varargin{:});

if isempty(axesHandle)
    h_fig = namedFigure('GridMat', 'Units', 'centimeters');
    figure(h_fig);
    axesHandle = gca;
    cla(axesHandle);
    set(axesHandle, 'Units', 'centimeters');
end

if ~isempty(pageDims)
    rectangle('Pos', [-squareSize -squareSize pageDims], ...
        'LineStyle', '--', 'Parent', axesHandle);
end

% Grid
gridLines = 0:squareSize:matSize;
numLines = length(gridLines);
h_line = zeros(2, numLines);
mmPerPix_line = lineWidth/2;
mmPerPix_mat  = matSize/2;
for i = 1:numLines
    %h_line(1,i) = rectangle('Pos', [gridLines(i)-lineWidth/2 0 lineWidth matSize], 'Parent', axesHandle);
    %h_line(2,i) = rectangle('Pos', [0 gridLines(i)-lineWidth/2 matSize lineWidth], 'Parent', axesHandle);
    h_line(1,i) = imagesc(gridLines(i) + lineWidth/2*[-1 1] + mmPerPix_line/2*[1 -1], [0 matSize] + mmPerPix_mat/2*[1 -1], ...
        zeros(2,2,3), 'Parent', axesHandle); hold on
    h_line(2,i) = imagesc([0 matSize] + mmPerPix_mat/2*[1 -1], gridLines(i) + lineWidth/2*[-1 1] + mmPerPix_line/2*[1 -1], ...
        zeros(2,2,3), 'Parent', axesHandle);
end
%set(h_line, 'FaceColor', 'k');
hold(axesHandle, 'on');

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
        if mirrorX 
            j_pos = numCols - j + 1;
        else
            j_pos = j;
        end
        
        if mirrorY
            i_pos = numRows - i + 1;
        else
            i_pos = i;
        end
        
        code = MatMarker2D.encodeIDs(j_pos, i_pos);
        
        imagesc([xcen(i,j)-codePos xcen(i,j)+codePos], ...
            [ycen(i,j)-codePos ycen(i,j)+codePos], ~code, ...
            'Parent', axesHandle);
            
    end
end
axis(axesHandle, 'ij', 'equal');
colormap(axesHandle, gray)


end % FUNCTION gridMat()

