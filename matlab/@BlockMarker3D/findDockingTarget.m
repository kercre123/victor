function [xcen,ycen, u_box,v_box,img] = findDockingTarget(this, img, varargin)
% Find a BlockMarker3D's docking target in an image.

assert(strcmp(BlockMarker3D.DockingTarget, 'FourDots'), ...
    'findDockingTarget() is currently only written to support a "FourDots" target.');

mask = [];
squareDiagonal = [];
spacingTolerance = 0.25; % can be this fraction away from expected, i.e. +/- 10%
simulateDefocusBlur = true;
boxPadding = 1.5;
findBoundingBox = false;

parseVarargin(varargin{:});

if isa(img, 'Camera')
    camera = img;
    img = im2double(camera.image);
else
    camera = [];
    img = im2double(img);
end

[nrows,ncols,nbands] = size(img);
if nbands>1
    img = mean(img,3);
end

if simulateDefocusBlur
    % Simulate blur from defocus
    img = separable_filter(img, gaussian_kernel(squareDiagonal/16));
end

if isempty(mask)
    % If we don't have a mask to tell us where to look...
    if ~isempty(camera)
        % ... but we have a camera, project our Docking Target's bounding
        % box into the image to create the mask.
        dockingBox3D = this.getPosition(camera.pose, 'DockingTargetBoundingBox');
        [u_box, v_box] = camera.projectPoints(dockingBox3D);
        
        % Pad the target a bit:
        u_cen = mean(u_box);
        v_cen = mean(v_box);
        u_boxPad = boxPadding*(u_box-u_cen) + u_cen;
        v_boxPad = boxPadding*(v_box-v_cen) + v_cen;
        
        if findBoundingBox
            [u_box, v_box] = this.findDockingTargetBoundingBox(img, u_box, v_box);
        end

        mask = roipoly(img, u_boxPad([1 2 4 3]), v_boxPad([1 2 4 3]));
        squareDiagonal = sqrt((u_box(1)-u_box(4))^2 + (v_box(1)-v_box(4))^2);
    else
        % ... get the user to draw one for us.
        mask = roipoly(img);
    end
end

if isempty(squareDiagonal)
    squareDiagonal = sqrt(2)*sqrt(sum(mask(:)));
end

dotRadiusFraction = (BlockMarker3D.DockingDotWidth/2) / (sqrt(2)*BlockMarker3D.CodeSquareWidth);
dotRadius = squareDiagonal * dotRadiusFraction;
sigma = dotRadius; 
% dotKernel = gaussian_kernel(sigma);
% dotKernel = dotKernel(:)*dotKernel;
% dotKernel = 1 - dotKernel/max(dotKernel(:));
% dotKernel = image_downright(dotKernel) - dotKernel;

hsize = ceil(6*sigma + 1);
LoG = fspecial('log', hsize, sigma);

%imgFilter = imfilter(image_downright(img) - img, dotKernel);
imgFilter = imfilter(img, LoG);

step = 1; %max(1, round((squareWidth * dotRadiusMultiplier)/12))
localMaxima = find(mask &  ...
    imgFilter > image_right(imgFilter,step) & ...
    imgFilter > image_left(imgFilter,step) & ...
    imgFilter > image_down(imgFilter,step) & ...
    imgFilter > image_up(imgFilter,step));
[localMaxima_y, localMaxima_x] = ind2sub([nrows ncols], localMaxima);

% hold off
% imagesc(img), axis image
% hold on
% plot(localMaxima_x, localMaxima_y, 'b.', 'MarkerSize', 12);

xcen = zeros(1,4);
ycen = zeros(1,4);
for i = 1:4
    % Find the largest local maximum response, then zero out the area around it
    % (so we don't find another maximum in the same dot) and repeat.
    [~,index] = max(imgFilter(localMaxima));
    if isempty(index)
        error('FindDockingTarget:TooFewLocalMaxima', ...
            'No local maxima left in searching for docking target!');
    end
    [ycen(i),xcen(i)] = ind2sub([nrows ncols], localMaxima(index));
    
    toRemove = (localMaxima_x-xcen(i)).^2 + (localMaxima_y-ycen(i)).^2 <= (1.5*dotRadius)^2;
    
    % plot(xcen(i), ycen(i), 'go');
    % plot(localMaxima_x(toRemove), localMaxima_y(toRemove), 'rx');
    
    localMaxima(toRemove) = [];
    localMaxima_x(toRemove) = [];
    localMaxima_y(toRemove) = [];
    
    % pause
end

theta = cart2pol(xcen-mean(xcen),ycen-mean(ycen));
[~,sortIndex] = sort(theta);

xcen = xcen(sortIndex);
ycen = ycen(sortIndex);

% Return as upper left, lower left, upper right, lower right:
xcen = xcen([1 4 2 3]);
ycen = ycen([1 4 2 3]);

% Make sure the distances between dots are "reasonable":

expectedSpacingFrac = BlockMarker3D.DockingDotSpacing/BlockMarker3D.CodeSquareWidth;

leftLengthFrac = sqrt( (xcen(2)-xcen(1))^2 + (ycen(2)-ycen(1))^2 ) / ...
    sqrt( (u_box(2)-u_box(1))^2 + (v_box(2)-v_box(1))^2);

topLengthFrac = sqrt( (xcen(3)-xcen(1))^2 + (ycen(3)-ycen(1))^2 ) / ...
    sqrt( (u_box(3)-u_box(1))^2 + (v_box(3)-v_box(1))^2);

rightLengthFrac = sqrt( (xcen(3)-xcen(4))^2 + (ycen(3)-ycen(4))^2 ) / ...
    sqrt( (u_box(3)-u_box(4))^2 + (v_box(3)-v_box(4))^2);

bottomLengthFrac = sqrt( (xcen(2)-xcen(4))^2 + (ycen(2)-ycen(4))^2 ) / ...
    sqrt( (u_box(2)-u_box(4))^2 + (v_box(2)-v_box(4))^2);

if ~all(abs(expectedSpacingFrac - ...
        [leftLengthFrac topLengthFrac rightLengthFrac bottomLengthFrac]) / ...
        expectedSpacingFrac < spacingTolerance)
    
    desktop
    keyboard
    
    error('FindDockingTarget:BadSpacing', ...
        'Detected dots not within spacing tolerances.');
end

if nargout == 0
    hold off, imagesc(img), axis image, hold on
    plot(xcen, ycen, 'r.', 'MarkerSize', 12);
    plot(xcen([1 2 4 3 1]), ycen([1 2 4 3 1]), 'r');
    
    clear xcen ycen
end

end % FUNCTION findDots()
