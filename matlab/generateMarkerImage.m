function markerImg = generateMarkerImage(blockType, faceType, varargin)

h_axes = [];
fiducialType = 'none'; % cornerDots, dockingDots, square or none
centerTargetType = 'circle'; % circle or checker
targetSize = 3.84; % in cm
imgSize = 255; 
fiducialSize = .5; % radius of dots or width of square
borderSpacing = .25; % in cm
useBarsBetweenDots = false; % only valid when centerTargetType=='cornerDots'

parseVarargin(varargin{:});

%value = encodeIDs(blockType, faceType);
code = BlockMarker2D.encodeIDs(blockType, faceType);

n = size(BlockMarker2D.Layout, 1);

% mid = (n+1)/2;

if isempty(h_axes)
    h_axes = gca;
end
cla(h_axes);
set(get(h_axes, 'Parent'), 'Units', 'centimeters')
set(h_axes, 'Units', 'centimeters');
hold(h_axes, 'off')
rectangle('Pos', [0 0 targetSize targetSize], ...
    'EdgeColor', .8*ones(1,3), 'LineStyle', '--', ...
    'LineWidth', .5, 'Parent', h_axes);
hold(h_axes, 'on');

h_fiducial = [];
switch(fiducialType)
    case 'dockingDots'
        h_fiducial(1) = rectangle('Pos', [targetSize/2-fiducialSize -borderSpacing-2*fiducialSize 2*fiducialSize 2*fiducialSize], 'Parent', h_axes);
        h_fiducial(2) = rectangle('Pos', [-borderSpacing-2*fiducialSize targetSize/2-fiducialSize 2*fiducialSize 2*fiducialSize], 'Parent', h_axes);
        h_fiducial(3) = rectangle('Pos', [targetSize/2-fiducialSize targetSize+borderSpacing 2*fiducialSize 2*fiducialSize], 'Parent', h_axes);
        h_fiducial(4) = rectangle('Pos', [targetSize+borderSpacing targetSize/2-fiducialSize 2*fiducialSize 2*fiducialSize], 'Parent', h_axes);
        
        cornerSize = borderSpacing;
        
        lim = [-2*fiducialSize-2*borderSpacing targetSize+2*fiducialSize+2*borderSpacing];
        
        set(h_fiducial, 'Curvature', [1 1], 'FaceColor', 'k');
        
    case 'cornerDots'
        h_fiducial(1) = rectangle('Pos', [borderSpacing borderSpacing 2*fiducialSize 2*fiducialSize], 'Parent', h_axes);
        h_fiducial(2) = rectangle('Pos', [targetSize-borderSpacing-2*fiducialSize borderSpacing 2*fiducialSize 2*fiducialSize], 'Parent', h_axes);
        h_fiducial(3) = rectangle('Pos', [borderSpacing targetSize-borderSpacing-2*fiducialSize 2*fiducialSize 2*fiducialSize], 'Parent', h_axes);
        h_fiducial(4) = rectangle('Pos', [targetSize-borderSpacing-2*fiducialSize targetSize-borderSpacing-2*fiducialSize 2*fiducialSize 2*fiducialSize], 'Parent', h_axes);

        cornerSize = 2*fiducialSize + 2*borderSpacing;
       
        lim = [0 targetSize];
        
        if useBarsBetweenDots 
            barStart = borderSpacing + 3*fiducialSize;
            barLength = targetSize - 2*barStart;
            barWidth  = fiducialSize;
            h_bar(1) = rectangle('Parent', h_axes, 'Pos', ...
                [barStart (borderSpacing+fiducialSize)-barWidth/2 barLength barWidth]);
            h_bar(2) = rectangle('Parent', h_axes, 'Pos', ...
                [barStart targetSize-(borderSpacing+fiducialSize)-barWidth/2 barLength barWidth]);
            h_bar(3) = rectangle('Parent', h_axes, 'Pos', ...
                [(borderSpacing+fiducialSize)-barWidth/2 barStart barWidth barLength]);
            h_bar(4) = rectangle('Parent', h_axes, 'Pos', ...
                [targetSize-(borderSpacing+fiducialSize)-barWidth/2 barStart barWidth barLength]);
            
            set(h_bar, 'EdgeColor', 'k', 'FaceColor', 'k')
            
        end
        
        set(h_fiducial, 'Curvature', [1 1], 'FaceColor', 'k');
        
    case 'square'
        h_fiducial(1) = rectangle('Pos', [borderSpacing*[1 1] (targetSize-2*borderSpacing)*[1 1]], ...
            'FaceColor', 'k', 'EdgeColor', 'none', 'Parent', h_axes);
        
        h_fiducial(2) = rectangle('Pos', [(borderSpacing+fiducialSize)*[1 1] (targetSize-2*(fiducialSize+borderSpacing))*[1 1]], ...
            'FaceColor', 'w', 'EdgeColor', 'none', 'Parent', h_axes);
        
        %set(h_fiducial, 'EdgeColor', 'k', 'LineWidth', fiducialSize);
        
        lim = [0 targetSize];
        
        cornerSize = 2*borderSpacing + fiducialSize;
        
    case 'none'
        cornerSize = borderSpacing;
        
        lim = [0 targetSize];
        
    otherwise
        error('Unrecognized circle type: "%s"', fiducialType);
end
    

% availableSlots = true(n,n);
% availableSlots([1 end],mid) = false;
% availableSlots(mid,[1 end]) = false;
% availableSlots(mid, mid)    = false;
% %availableSlots(n,1) = false;
% %availableSlots(n,n) = false;
% 
% code = zeros(n,n);
% %code(1,1) = 1; % upper left always 1 for orientation
% code(end,mid) = 1;
% code(mid,[1 end]) = 1;
% code(availableSlots) = dec2bin(value, numBits) == '1';

pixelWidth = (targetSize - 2*cornerSize)/n;
%imPos = [cornerSize+pixelWidth/2 targetSize-cornerSize-pixelWidth/2];
%h_code = imagesc(imPos, imPos, ~code, 'Parent', h_axes);

% Draw the squres one by one as rectangles, so that they scale better when
% printing a sheet:
for i = 1:n
    for j = 1:n
        if code(i,j)
           rectangle('Pos',  [cornerSize+(j-1)*pixelWidth ...
               cornerSize+(i-1)*pixelWidth pixelWidth pixelWidth], ...
               'EdgeColor', 'none', 'FaceColor', 'k');
        end
    end
end
colormap(gray)
axis(h_axes, 'image', 'ij') 
set(h_axes, 'XLim', lim, 'YLim', lim);

% Center target:
switch(centerTargetType)
    case 'checker'
        w = .8*pixelWidth/3;
        h_cen(1) = rectangle('Pos', [targetSize/2-1.5*w targetSize/2-1.5*w w w], 'Parent', h_axes);
        h_cen(2) = rectangle('Pos', [targetSize/2-1.5*w targetSize/2+0.5*w w w], 'Parent', h_axes);
        h_cen(3) = rectangle('Pos', [targetSize/2-0.5*w targetSize/2-0.5*w w w], 'Parent', h_axes);
        h_cen(4) = rectangle('Pos', [targetSize/2+0.5*w targetSize/2-1.5*w w w], 'Parent', h_axes);
        h_cen(5) = rectangle('Pos', [targetSize/2+0.5*w targetSize/2+0.5*w w w], 'Parent', h_axes);
        
        %rectangle('Pos', [targetSize/2-5*pixelWidth/12 targetSize/2-5*pixelWidth/12 5*pixelWidth/6 5*pixelWidth/6], 'LineWidth', 2, 'Parent', h_axes);
        
        set(h_cen, 'FaceColor', 'k', 'EdgeColor', 'none');
    case 'circle'
        h_cen = rectangle('Curvature', [1 1], ...
            'Pos', [(targetSize/2-pixelWidth/3)*[1 1] 2/3*pixelWidth*[1 1]], ...
            'Parent', h_axes);
        
        set(h_cen, 'FaceColor', 'k', 'EdgeColor', 'none');
        
    case 'square'
        w1 = 1*pixelWidth;
        h_cen(1) = rectangle('Pos', [(targetSize/2-w1/2)*[1 1] w1 w1], ...
            'Parent', h_axes, 'EdgeColor', 'none', 'FaceColor', 'k');
        w2 = .6*pixelWidth;
        h_cen(2) = rectangle('Pos', [(targetSize/2-w2/2)*[1 1] w2 w2], ...
            'Parent', h_axes, 'EdgeColor', 'none', 'FaceColor', 'w');
        
    case 'dots'
        spacing = .5*pixelWidth;
        radius  = .125*pixelWidth;
        
        h_dot(1) = rectangle('Curvature', [1 1], 'Parent', h_axes, ...
            'Pos', [(targetSize/2-radius)+spacing/2*[-1 -1] 2*radius*[1 1]]);
        h_dot(2) = rectangle('Curvature', [1 1], 'Parent', h_axes, ...
            'Pos', [(targetSize/2-radius)+spacing/2*[-1  1] 2*radius*[1 1]]);
        h_dot(3) = rectangle('Curvature', [1 1], 'Parent', h_axes, ...
            'Pos', [(targetSize/2-radius)+spacing/2*[ 1 -1] 2*radius*[1 1]]);
        h_dot(4) = rectangle('Curvature', [1 1], 'Parent', h_axes, ...
            'Pos', [(targetSize/2-radius)+spacing/2*[ 1  1] 2*radius*[1 1]]);
        
        set(h_dot, 'EdgeColor', 'none', 'FaceColor', 'k');
        
    otherwise
        error('Unrecognized centerTargetType "%s"', centerTargetType);
end





if false
    h_orient(1) = rectangle('Pos', [fiducialSize/2-pixelWidth/2 fiducialSize/2-n/2*pixelWidth pixelWidth n*pixelWidth], 'Parent', h_axes);
    h_orient(2) = rectangle('Pos', [fiducialSize/2-n/2*pixelWidth fiducialSize/2-pixelWidth/2  n*pixelWidth pixelWidth], 'Parent', h_axes);
    
    set(h_orient, 'EdgeColor', 'r');
    
end

if nargout>0
    h_fig = get(h_axes, 'Parent');
    axis(h_axes, 'off');
    %set(h_fiducial, 'Visible', 'off');
    set(h_fig, 'Color', 'w');
    truesize(h_fig, [imgSize imgSize])
    markerImg = getframe;
    markerImg = markerImg.cdata;
    %set(h_fiducial, 'Visible', 'on');
end
