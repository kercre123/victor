function outputImg = Draw(varargin)

Figure = [];
ShowProbes = false;
NumCorners = 1;
Image = [];
Scale = []; % in mm
%Margin = 2.54; % in cm
Origin = [0 0]; % in mm
Margin = [10 10]; % in mm
Add = false;
PageSize = [11 8.5] * 2.54;
AddPadding = true;

parseVarargin(varargin{:});

if isempty(Figure)
    Figure = gcf;
end

singleCode = false;
if isempty(Scale)
    singleCode = true;
    Scale = 10;
    Margin = [0 0];
end

% Convert to cm
Scale = Scale/10;
Origin = Origin/10 + Margin/10;


if ~Add
    clf(Figure)
    set(Figure, 'Color', 'w')
    Axes = [];
else
    Axes = findobj(Figure, 'Tag', 'MarkerPageAxes');
end

if isempty(Axes)
    Axes = axes('Units', 'centimeters', 'Pos', [0 0 PageSize], ...
        'Parent', Figure, 'Box', 'on', 'Color', 'none', ...
        'XGrid', 'off', 'YGrid', 'off', ...
        'XLim', [0 PageSize(1)], 'YLim', [0 PageSize(2)], ...
        'XTick', 0:PageSize(1), 'YTick', 0:PageSize(2), ...
        'Tag', 'MarkerPageAxes');
end

rectangle('Pos', [Origin Scale*[1 1]], 'FaceColor', 'k', 'Parent', Axes);
hold(Axes, 'on')
rectangle('Pos', [Origin+Scale*VisionMarker.SquareWidth*[1 1] ...
    Scale*(1-2*VisionMarker.SquareWidth)*[1 1]], ...
    'FaceColor', 'w', 'Parent', Axes);

if AddPadding
    border = VisionMarker.FiducialPaddingFraction*VisionMarker.SquareWidth;
    rectangle('Pos', [Origin-border*Scale*[1 1] Scale*(1+2*border)*[1 1]], ...
        'LineStyle', '--', 'Parent', Axes);
else
    border = 0;
end

if ~isempty(Image)
    if ischar(Image)
        if ~exist(Image, 'file')
            error('Image file "%s" does not exist.');
        end
        Image = imread(Image);
    end
    
    assert(size(Image,1)==size(Image,2), 'Expecting square image.');
    pixelWidth = (1-(2+2*VisionMarker.FiducialPaddingFraction)*VisionMarker.SquareWidth)/size(Image,1);
    
    P = (1+VisionMarker.FiducialPaddingFraction)*VisionMarker.SquareWidth + pixelWidth/2;
    imgPos = [Scale*P Scale*(1-P)];
    imagesc(Origin(1)+imgPos, Origin(2)+imgPos, Image, 'Parent', Axes)
end

% Corner triangle(s)
int = 2*VisionMarker.SquareWidth;
%int = 2*(.5 + VisionMarker.FiducialPaddingFraction + ...
%    .5*VisionMarker.ProbeParameters.WidthFraction)*VisionMarker.SquareWidth;
x_tri = Scale*([0 int 0 0] + VisionMarker.SquareWidth);
y_tri = Scale*([0 0 int 0] + VisionMarker.SquareWidth);

patch(Origin(1)+x_tri, Origin(2)+Scale-y_tri, 'k', 'Parent', Axes); % LowerLeft

if NumCorners > 1
    patch(Origin(1)+Scale-x_tri, Origin(2)+Scale-y_tri, 'k', 'Parent', Axes); % LowerRight
end
if NumCorners > 2
    patch(Origin(1)+x_tri, Origin(2)+y_tri, 'k', 'Parent', Axes); % UpperLeft
end
if NumCorners > 3
    warning('Only creating a maximum of 3 corner markers.');
end

if ShowProbes
    plot(Origin(1)+Scale*VisionMarker.XProbes, ...
        Origin(2)+Scale*VisionMarker.YProbes, 'r.', ...
        'MarkerSize', 10, 'Parent', Axes);
end

axis(Axes, 'image', 'ij')
if singleCode
    set(Axes, 'XLim', [-border 1+border], 'YLim', [-border 1+border], 'Box', 'on');
else
    set(Axes, 'XLim', [0 PageSize(1)], 'YLim', [0 PageSize(2)], 'Box', 'on');
end

colormap(Axes, 'gray')

if nargout > 0
    try 
        outputImg = getframe(Axes);
    catch
        currentFigPos = get(Figure, 'Pos');
        set(Figure, 'Pos', [10 10 currentFigPos(3:4)]);
        outputImg = getframe(Axes);
        set(Figure, 'Pos', currentFigPos);
    end
        
    outputImg = outputImg.cdata(:,:,1);
end

end % Function Draw()