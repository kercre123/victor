function [xMat,yMat,orient1] = matLocalization(img, varargin)

%% Params
derivSigma = 2;
pixPerMM = 6;
squareWidth = 10;
lineWidth = 1.5;
camera = [];

parseVarargin(varargin{:});

%% Init
if isa(img, 'uint8')
    img = double(img)/255;
end

if size(img,3)>1
    img = mean(img,3);
end

[nrows,ncols] = size(img);

assert(min(img(:))>=0 && max(img(:))<=1, ...
    'Image should be on the interval [0 1].');

%% Image Orientation
imgOrig = img;
if ~isempty(camera)
    % Undo radial distortion according to camera calibration info:
    assert(isa(camera, 'Camera'), 'Expecting a Camera object.');
    [img, xgrid, ygrid] = camera.undistort(img);
    imgCen = camera.center;    
    xgrid = xgrid - imgCen(1);
    ygrid = ygrid - imgCen(2);
    
    %[xgrid,ygrid] = meshgrid((1:ncols)-imgCen(1), (1:nrows)-imgCen(2));
else
    imgCen = [ncols nrows]/2;
    [xgrid,ygrid] = meshgrid((1:ncols)-imgCen(1), (1:nrows)-imgCen(2));
end

[Ix,Iy] = smoothgradient(img, derivSigma);
mag = sqrt(Ix.^2 + Iy.^2);
orient = atan2(Iy, Ix);
orient(orient < 0) = pi + orient(orient < 0);

% Bin into 1-degree bins, using linear interpolation
orient = 180/pi*orient(:);
leftBin = floor(orient);
leftBinWeight = 1-(orient-leftBin);
rightBin = leftBin + 1;
rightBin(rightBin==181) = 1;
%counts = row(accumarray(round(180/pi*orient(:))+1, mag(:)));
assert(all(leftBinWeight>=0 & leftBinWeight <= 1), ...
    'Bin weights should be [0,1]');
counts = row(accumarray(leftBin+1, leftBinWeight.*mag(:), [181 1]));
counts = counts + row(accumarray(rightBin+1, (1-leftBinWeight).*mag(:), [181 1]));
%counts = imfilter(counts, gaussian_kernel(1), 'circular');

% First and last bin (0 and 180) are same thing
counts(1) = counts(1) + counts(181);
counts = counts(1:180);

% Find peaks
localMaxima = find(counts > counts([end 1:end-1]) & counts > counts([2:end 1]));
if length(localMaxima) < 2
    error('Could not find at least 2 peaks in orientation histogram.');
end

[~,sortIndex] = sort(counts(localMaxima), 'descend');
localMaxima = localMaxima(sortIndex(1:2));

orient1 = -(localMaxima(1)-1)*pi/180; % subtract 1 because of +1 in accumarray above
orient2 = -(localMaxima(2)-1)*pi/180;
if orient2 > orient1
    temp = orient1;
    orient1 = orient2;
    orient2 = temp;
end
orientDiff = orient1 - orient2; 
if abs(orientDiff - pi/2) > 5*pi/180
    error('Found two orientation peaks, but they are not ~90 degrees apart.');
end

% Fit a parabola to the peak orientation to get sub-bin orientation
% precision
assert(length(counts)==180, 'Expecting 180 orientation bins');
if localMaxima(1)==1
    bins = [180 1 2]';
elseif localMaxima(1)==180
    bins = [179 180 1]';
else
    bins = localMaxima(1) + [-1 0 1]';
end
% solve for parameters of parabola:
A = [bins.^2 bins ones(3,1)];
p = A \ counts(bins)';

% Find max location of the parabola *in terms of bins*.  Remember to
% subtract 1 again because of +1 in accumarray above.  Then convert to
% radians.
orient1 = -(-p(2)/(2*p(1)) - 1) * pi/180; 

%% Grid Square localization   
xgridRot =  xgrid*cos(orient1) + ygrid*sin(orient1) + imgCen(1);
ygridRot = -xgrid*sin(orient1) + ygrid*cos(orient1) + imgCen(2);

% if ~isempty(camera)
%     % Undo radial distortion according to camera calibration info:
%     assert(isa(camera, 'Camera'), 'Expecting a Camera object.');
%     [xgridRot, ygridRot] = camera.undistort(xgridRot, ygridRot);
% end

% Note that we use a value of 1 (white) for pixels outside the image, which
% will effectively give less weight to lines/squares that are closer to
% the image borders naturally.
imgRot = interp2(imgOrig, xgridRot, ygridRot, 'nearest', 1);

% Now using stencil that looks for edges of the stripes by using
% derivatives instead.  The goal is to be less sensitive to lighting
x = linspace(-squareWidth,squareWidth,pixPerMM*2*squareWidth);
gridStencil = max(exp(-(x+squareWidth/2).^2/(2*(lineWidth/3)^2)), ...
    exp(-(x-squareWidth/2).^2/(2*(lineWidth/3)^2)));
gridStencil = image_right(gridStencil) - gridStencil;

colDeriv = mean(image_right(imgRot)-imgRot,1);
rowDeriv = mean(image_down(imgRot)-imgRot,2);

[~,xcenIndex] = max(conv2(colDeriv, gridStencil, 'same'));
[~,ycenIndex] = max(conv2(rowDeriv', gridStencil, 'same'));


%% Square Identification
% codeWidth = (squareWidth - lineWidth - 2*codePadding)*pixPerMM;
% codeImg = imgRot(round(ycenIndex + (-codeWidth/2 : codeWidth/2)), ...
%     round(xcenIndex + (-codeWidth/2 : codeWidth/2)) );

insideSquareWidth = squareWidth - lineWidth;
 
% s = diag(insideSquareWidth * pixPerMM * [1 1]);
% R = [cos(orient1) sin(orient1); -sin(orient1) cos(orient1)];
% t = [xcen; ycen] - R*diag(s)/2;
% tform = maketform('affine', [s*R t; 0 0 1]');
% 
% corners = [0 0; ...
%            0 1; ...
%            1 0; ...
%            1 1];
%         
% corners = tformfwd(tform, corners);
%     
% marker = MatMarker2D(img, corners, maketform('affine', tform.tdata.Tinv));
s = diag(insideSquareWidth * pixPerMM * [1 1]);
R = eye(2);
t = [xcenIndex; ycenIndex] - diag(s)/2;
tform = maketform('affine', [s*R t; 0 0 1]');

corners = [0 0; ...
           0 1; ...
           1 0; ...
           1 1];
        
corners = tformfwd(tform, corners);

% Note we're using the rotated, undistorted image for identifying the code
marker = MatMarker2D(imgRot, corners, maketform('affine', tform.tdata.Tinv));

if marker.isValid
    % Get camera/image position on mat, in mm:
    xvec = imgCen(1)-xcenIndex;
    yvec = imgCen(2)-ycenIndex;
    xvecRot =  xvec*cos(-marker.upAngle) + yvec*sin(-marker.upAngle);
    yvecRot = -xvec*sin(-marker.upAngle) + yvec*cos(-marker.upAngle);
    xMat = xvecRot/pixPerMM + marker.X*squareWidth - squareWidth/2;
    yMat = yvecRot/pixPerMM + marker.Y*squareWidth - squareWidth/2;

    orient1 = orient1 + marker.upAngle;
    
else
    xMat = -1;
    yMat = -1;
    xvecRot = -1;
    yvecRot = -1;
end

if nargout == 0
    % Location of the center of the square in original (distorted) image
    % coordinates:
    xcen = xgridRot(ycenIndex,xcenIndex);
    ycen = ygridRot(ycenIndex,xcenIndex);

    namedFigure('MatLocalization');
    subplot 221
    hold off, imagesc(imgOrig), axis image, hold on
    plot(xcen, ycen, 'bx');
    plot(imgCen(1), imgCen(2), 'r.');
    
    h_axes = subplot(222);
    hold off, imagesc(imgRot), axis image, hold on
    draw(marker, 'where', h_axes, 'drawTextLabels', 'short');
    plot(imgCen(1), imgCen(2), 'r.', 'MarkerSize', 12);
    title(sprintf('Orientation = %.1f degrees', orient1*180/pi));
    
    subplot 234
    meansRot = imrotate(marker.means, -marker.upAngle * 180/pi, 'nearest');
    if marker.isValid
        imagesc(meansRot > marker.threshold, [0 1]), axis image
        title(sprintf('X = %d, Y = %d', marker.X, marker.Y));
    else
        imagesc(meansRot, [0 1]), axis image
        title('Invalid Marker')
    end
    
    % Draw image boundaries on the mat:
    xImg = ncols/2*[-1 1 1 -1 -1] / pixPerMM;
    yImg = nrows/2*[-1 -1 1 1 -1] / pixPerMM;
    xImgRot = xImg*cos(-orient1) + yImg*sin(-orient1) + xMat;
    yImgRot = -xImg*sin(-orient1) + yImg*cos(-orient1) + yMat;
    
    % Compute the marker center location on the mat, in mm:
    xMarkerCen = xMat - xvecRot/pixPerMM;
    yMarkerCen = yMat - yvecRot/pixPerMM;
    xMarker = squareWidth/2*[-1 1 1 -1 -1] + xMarkerCen;
    yMarker = squareWidth/2*[-1 -1 1 1 -1] + yMarkerCen;
    
    h_matAxes = subplot(2,3,[5 6]);
    if ~strcmp(get(h_matAxes, 'Tag'), 'GridMat')
        gridMat('matSize', 200, 'axesHandle', h_matAxes);
        set(h_matAxes, 'Tag', 'GridMat');
        hold on
        plot(xMat, yMat, 'r.', 'MarkerSize', 12, ...
            'Tag', 'MatPosition', 'Parent', h_matAxes);
        plot(xImgRot, yImgRot, 'r', 'LineWidth', 2, ...
            'Tag', 'MatPositionRect', 'Parent', h_matAxes);
        plot(xMarkerCen, yMarkerCen, 'bx', 'LineWidth', 2, ...
            'Tag', 'MatMarkerCen', 'Parent', h_matAxes);
        plot(xMarker, yMarker, 'g', 'LineWidth', 3, ...
            'Tag', 'MatMarker', 'Parent', h_matAxes);
    else 
        set(findobj(h_matAxes, 'Tag', 'MatPosition'), ...
            'XData', xMat, 'YData', yMat);
        set(findobj(h_matAxes, 'Tag', 'MatPositionRect'), ...
            'XData', xImgRot, 'YData', yImgRot);
        set(findobj(h_matAxes, 'Tag', 'MatMarkerCen'), ...
            'XData', xMarkerCen, 'YData', yMarkerCen);
        set(findobj(h_matAxes, 'Tag', 'MatMarker'), ...
            'XData', xMarker, 'YData', yMarker);
    end
end



% keyboard