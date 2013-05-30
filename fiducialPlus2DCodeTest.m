function markerImg = fiducialPlus2DCodeTest(blockType, faceType, varargin)

h_axes = [];
fiducialType = 'none'; % cornerDots, dockingDots, square or none
centerTargetType = 'circle'; % circle or checker
targetSize = 40;
fiducialSize = 5; % radius of dots or width of square
borderSpacing = 2;
n = 5; %numSquares, should be odd, so there's a center pixel we can use
useBarsBetweenDots = false; % only valid when centerTargetType=='cornerDots'

parseVarargin(varargin{:});

value = encodeIDs(blockType, faceType);

numBits = n^2 - 2*n + 1;
numValues = 2^numBits;

mid = (n+1)/2;

if isempty(h_axes)
    h_axes = gca;
end
cla(h_axes);
hold(h_axes, 'off')
rectangle('Pos', [0 0 targetSize targetSize], ...
    'EdgeColor', .8*ones(1,3), 'LineStyle', '--', ...
    'LineWidth', .5, 'Parent', h_axes);
title(sprintf('%d unique values possible', numValues))
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
            'FaceColor', 'k', 'EdgeColor', 'k', 'Parent', h_axes);
        
        h_fiducial(2) = rectangle('Pos', [(borderSpacing+fiducialSize)*[1 1] (targetSize-2*(fiducialSize+borderSpacing))*[1 1]], ...
            'FaceColor', 'w', 'EdgeColor', 'w', 'Parent', h_axes);
        
        %set(h_fiducial, 'EdgeColor', 'k', 'LineWidth', fiducialSize);
        
        lim = [0 targetSize];
        
        cornerSize = 2*borderSpacing + fiducialSize;
        
    case 'none'
        cornerSize = borderSpacing;
        
        lim = [0 targetSize];
        
    otherwise
        error('Unrecognized circle type: "%s"', fiducialType);
end
    

availableSlots = true(n,n);
availableSlots(:,mid) = false;
availableSlots(mid,:) = false;
%availableSlots(n,1) = false;
%availableSlots(n,n) = false;

code = zeros(n,n);
%code(1,1) = 1; % upper left always 1 for orientation
code((mid+1):end,mid) = 1;
code(mid,:) = 1;
code(mid,mid) = 0;
code(availableSlots) = dec2bin(value, numBits) == '1';

pixelWidth = (targetSize - 2*cornerSize)/n;
imPos = [cornerSize+pixelWidth/2 targetSize-cornerSize-pixelWidth/2];
h_code = imagesc(imPos, imPos, ~code, 'Parent', h_axes);
colormap(gray)
axis(h_axes, 'image', 'ij') 
set(h_axes, 'XLim', lim, 'YLim', lim);

% Center target:
switch(centerTargetType)
    case 'checker'
        h_cen(1) = rectangle('Pos', [targetSize/2-pixelWidth/3 targetSize/2-pixelWidth/3 pixelWidth/3 pixelWidth/3], 'Parent', h_axes);
        h_cen(2) = rectangle('Pos', [targetSize/2 targetSize/2 pixelWidth/3 pixelWidth/3], 'Parent', h_axes);
        rectangle('Pos', [targetSize/2-5*pixelWidth/12 targetSize/2-5*pixelWidth/12 5*pixelWidth/6 5*pixelWidth/6], 'LineWidth', 2, 'Parent', h_axes);
    case 'circle'
        h_cen = rectangle('Curvature', [1 1], ...
            'Pos', [(targetSize/2-pixelWidth/3)*[1 1] 2/3*pixelWidth*[1 1]], ...
            'Parent', h_axes);
    otherwise
        error('Unrecognized centerTargetType "%s"', centerTargetType);
end

set(h_cen, 'FaceColor', 'k');



if false
    h_orient(1) = rectangle('Pos', [fiducialSize/2-pixelWidth/2 fiducialSize/2-n/2*pixelWidth pixelWidth n*pixelWidth], 'Parent', h_axes);
    h_orient(2) = rectangle('Pos', [fiducialSize/2-n/2*pixelWidth fiducialSize/2-pixelWidth/2  n*pixelWidth pixelWidth], 'Parent', h_axes);
    
    set(h_orient, 'EdgeColor', 'r');
    
end

if nargout>0
    h_fig = get(h_axes, 'Parent');
    axis(h_axes, 'off');
    set(h_fiducial, 'Visible', 'off');
    set(h_fig, 'Color', 'w');
    truesize(h_fig, 5*[targetSize targetSize])
    markerImg = getframe;
    markerImg = markerImg.cdata;
    set(h_fiducial, 'Visible', 'on');
end
