function markerImg = fiducialPlus2DCodeTest(blockType, faceType, varargin)

h_axes = [];
circleType = 'none'; % corners, docking, none
fiducialSize = 40;
circleRadius = 5;
borderSpacing = 2;
n = 5; %numSquares, should be odd, so there's a center pixel we can use

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
h_fiducial = rectangle('Pos', [0 0 fiducialSize fiducialSize], 'Parent', h_axes);
title(sprintf('%d unique values possible', numValues))
hold(h_axes, 'on');

h_circle = [];
switch(circleType)
    case 'docking'
        h_circle(1) = rectangle('Pos', [fiducialSize/2-circleRadius -borderSpacing-2*circleRadius 2*circleRadius 2*circleRadius], 'Parent', h_axes);
        h_circle(2) = rectangle('Pos', [-borderSpacing-2*circleRadius fiducialSize/2-circleRadius 2*circleRadius 2*circleRadius], 'Parent', h_axes);
        h_circle(3) = rectangle('Pos', [fiducialSize/2-circleRadius fiducialSize+borderSpacing 2*circleRadius 2*circleRadius], 'Parent', h_axes);
        h_circle(4) = rectangle('Pos', [fiducialSize+borderSpacing fiducialSize/2-circleRadius 2*circleRadius 2*circleRadius], 'Parent', h_axes);
        
        cornerSize = borderSpacing;
        
        lim = [-2*circleRadius-2*borderSpacing fiducialSize+2*circleRadius+2*borderSpacing];
        
    case 'corners'
        h_circle(1) = rectangle('Pos', [borderSpacing borderSpacing 2*circleRadius 2*circleRadius], 'Parent', h_axes);
        h_circle(2) = rectangle('Pos', [fiducialSize-borderSpacing-2*circleRadius borderSpacing 2*circleRadius 2*circleRadius], 'Parent', h_axes);
        h_circle(3) = rectangle('Pos', [borderSpacing fiducialSize-borderSpacing-2*circleRadius 2*circleRadius 2*circleRadius], 'Parent', h_axes);
        h_circle(4) = rectangle('Pos', [fiducialSize-borderSpacing-2*circleRadius fiducialSize-borderSpacing-2*circleRadius 2*circleRadius 2*circleRadius], 'Parent', h_axes);

        cornerSize = 2*circleRadius + 2*borderSpacing;
       
        lim = [0 fiducialSize];
        
    case 'none'
        cornerSize = borderSpacing;
        
        lim = [0 fiducialSize];
        
    otherwise
        error('Unrecognized circle type: "%s"', circleType);
end

if ~isempty(h_circle)
    set(h_circle, 'Curvature', [1 1], 'FaceColor', 'k');
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

pixelWidth = (fiducialSize - 2*cornerSize)/n;
imPos = [cornerSize+pixelWidth/2 fiducialSize-cornerSize-pixelWidth/2];
h_code = imagesc(imPos, imPos, ~code, 'Parent', h_axes);
colormap(gray)
axis(h_axes, 'image', 'ij') 
set(h_axes, 'XLim', lim, 'YLim', lim);


% Center target:
h_cen(1) = rectangle('Pos', [fiducialSize/2-pixelWidth/3 fiducialSize/2-pixelWidth/3 pixelWidth/3 pixelWidth/3], 'Parent', h_axes);
h_cen(2) = rectangle('Pos', [fiducialSize/2 fiducialSize/2 pixelWidth/3 pixelWidth/3], 'Parent', h_axes);

set(h_cen, 'FaceColor', 'k');

rectangle('Pos', [fiducialSize/2-5*pixelWidth/12 fiducialSize/2-5*pixelWidth/12 5*pixelWidth/6 5*pixelWidth/6], 'LineWidth', 2, 'Parent', h_axes);

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
    truesize(h_fig, 5*[fiducialSize fiducialSize])
    markerImg = getframe;
    markerImg = markerImg.cdata;
    set(h_fiducial, 'Visible', 'on');
end
