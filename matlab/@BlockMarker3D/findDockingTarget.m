function [xcen,ycen, u_box,v_box,img] = findDockingTarget(this, img, varargin)
% Find a BlockMarker3D's docking target in an image.

assert(strcmp(BlockMarker3D.DockingTarget, 'FourDots'), ...
    'findDockingTarget() is currently only written to support a "FourDots" target.');

mask = [];
squareDiagonal = [];
spacingTolerance = 0.25; % can be this fraction away from expected, i.e. +/- 10%
simulateDefocusBlur = true;
boxPadding = 1.25;
findBoundingBox = false;

parseVarargin(varargin{:});

if isa(img, 'Camera')
    camera = img;
    img = im2double(camera.image);
else
    camera = [];
    img = im2double(img);
end

%[nrows,ncols,nbands] = size(img);
%if nbands>1
if size(img,3)>1
    img = mean(img,3);
end

u_box = [];
v_box = [];

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
            % NOTE: this will currently get the UNBLURRED image, even if
            % defocus blur simulation is turned on, since that is dependent
            % on the image diagonal...
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

if isempty(u_box) 
    [vMask, uMask] = find(mask);
    uMin = min(uMask);
    vMin = min(vMask);
    uMax = max(uMask);
    vMax = max(vMask);
    
    u_box = [uMin uMin uMax uMax];
    v_box = [vMin vMax vMin vMax];
end
   

if simulateDefocusBlur
    % Simulate blur from defocus
    img = separable_filter(img, gaussian_kernel(squareDiagonal/16));
end

% Create a filter for finding dots of the expected size (this is just a 2D
% Laplacian-of-Gaussian filter)
dotRadiusFraction = (BlockMarker3D.DockingDotWidth/2) / (sqrt(2)*BlockMarker3D.CodeSquareWidth);
dotRadius = squareDiagonal * dotRadiusFraction;
sigma = dotRadius; 
hsize = ceil(6*sigma + 1);
LoG = fspecial('log', hsize, sigma);

% Extract and filter just the image inside the docking target's bounding
% box.  We'll use a padded crop in order to avoid boundary effects.
padding = ceil((hsize+1)/2);
squareWidth = ceil(u_box(3)-u_box(1));
squareHeight = ceil(v_box(2)-v_box(1));
cropRectanglePad = [floor([u_box(1) v_box(1)]-padding) ...
    squareWidth+2*padding squareHeight+2*padding];
imgCrop = imcrop(img, cropRectanglePad);

% NOTE: this Laplacian-of-Gaussian filtering step can and should be
% approximated by a difference of two Gaussian filtering steps, since the
% individual Gaussian filtering steps can be done with 1D separable
% filters for speed.
imgFilter = imfilter(imgCrop, LoG);

% Get rid of the padding and normalize to be b/w 0 and 1:
cropRectangle = [padding padding squareWidth squareHeight];
imgFilter = imcrop(imgFilter, cropRectangle);
imgFilter = imgFilter - min(imgFilter(:));
imgFilter = imgFilter/max(imgFilter(:));


% Get local maxima of the filter result, which should correspond to
% potential dot centers
step = 1; %max(1, round((squareWidth * dotRadiusMultiplier)/12))
localMaxima = find( ... maskCrop &  ...
    imgFilter > image_right(imgFilter,step) & ...
    imgFilter > image_left(imgFilter,step) & ...
    imgFilter > image_down(imgFilter,step) & ...
    imgFilter > image_up(imgFilter,step));

if length(localMaxima)<4
    error('FindDockingTarget:TooFewLocalMaxima', ...
        'We need to find at least 4 local maxima for potential dot locations.');
end

[localMaxima_y, localMaxima_x] = ind2sub(size(imgFilter), localMaxima);


% We still don't know _which_ 4 potential dot centers belong together and
% make up the docking pattern.  To find the best configuration, we 
% effectively filter with this sparse mask:
%    [+1 ..0.. -1 ..0.. +1] 
%    [        ..0..       ]
%    [-1 ..0.. -1 ..0.. -1]
%    [        ..0..       ]
%    [+1 ..0.. -1 ..0.. +1]
% where the spacing between those non-zero filter components corresponds to
% the expected horizontal/vertical spacing between dots.

squareWidth  = u_box(3)-u_box(1); 
squareHeight = v_box(2)-v_box(1);
dotSpacingFrac = BlockMarker3D.DockingDotSpacing/BlockMarker3D.CodeSquareWidth;
dotSpacingHor  = dotSpacingFrac * squareWidth; 
dotSpacingVert = dotSpacingFrac * squareHeight;
stepH = round(dotSpacingHor/2);
stepV = round(dotSpacingVert/2);

imgFilter2 = -imgFilter + ...
    -image_left(imgFilter, stepH) - image_right(imgFilter, stepH) + ...
    -image_up(imgFilter, stepV) - image_down(imgFilter, stepV) + ...
    image_upleft(imgFilter, stepV, stepH) + image_downleft(imgFilter, stepV, stepH) + ...
    image_upright(imgFilter, stepV, stepH) + image_downright(imgFilter, stepV, stepH);

% The peak response of this filter will be the center of the target. We
% will then find the four local maxima from the original filter that are
% closest to the expected dot locations, given that center:
[~,targetCenterIndex] = max(imgFilter2(:)); % This max() still kinda worries me in terms of robustness...
[targetCenter_y, targetCenter_x] = ind2sub(size(imgFilter), targetCenterIndex);
distUL = (localMaxima_x - (targetCenter_x-dotSpacingHor/2)).^2 + ...
    (localMaxima_y - (targetCenter_y-dotSpacingVert/2)).^2;
distUR = (localMaxima_x - (targetCenter_x+dotSpacingHor/2)).^2 + ...
    (localMaxima_y - (targetCenter_y-dotSpacingVert/2)).^2;
distLL = (localMaxima_x - (targetCenter_x-dotSpacingHor/2)).^2 + ...
    (localMaxima_y - (targetCenter_y+dotSpacingVert/2)).^2;
distLR = (localMaxima_x - (targetCenter_x+dotSpacingHor/2)).^2 + ...
    (localMaxima_y - (targetCenter_y+dotSpacingVert/2)).^2;

[~, indexUL] = min(distUL);
[~, indexUR] = min(distUR);
[~, indexLL] = min(distLL);
[~, indexLR] = min(distLR);

if length(unique([indexUL indexUR indexLL indexLR]))~=4
    desktop
    keyboard
    error('FindDockingTarget:DuplicateMaximaSelected', ...
        'The same point should not be chosen for multiple dots.');
end

% Select the closest local maxima and put them back in origina, uncropped
% coordinates:
xcen = localMaxima_x([indexUL indexLL indexUR indexLR]) + padding + cropRectanglePad(1) - 2;
ycen = localMaxima_y([indexUL indexLL indexUR indexLR]) + padding + cropRectanglePad(2) - 2;

% Make sure the distances between dots are "reasonable":
% (Is this still useful, given the way I am finding/assigning dots now?  Or
% is it sorta guaranteed by the algorithm?)
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

end % FUNCTION findDockingTarget()



% Experimenting with filtering with template image:
% 
% imgFilterIntegral = cumsum(cumsum(imgFilter,2),1);
% 
% squareWidth = u_box(3)-u_box(1); 
% squareHeight = v_box(2)-v_box(1);
% [xgrid,ygrid] = meshgrid(-squareWidth/2:squareWidth/2, -squareHeight/2:squareHeight/2);
% 
% dotSpacingFrac = BlockMarker3D.DockingDotSpacing/BlockMarker3D.CodeSquareWidth;
% dotSpacingHor = dotSpacingFrac * squareWidth; 
% dotSpacingVert = dotSpacingFrac * squareHeight;
% 
% bumpUL = exp(-.5*( (xgrid+dotSpacingHor/2).^2 + (ygrid+dotSpacingVert/2).^2)/(sigma^2));
% bumpUR = exp(-.5*( (xgrid-dotSpacingHor/2).^2 + (ygrid+dotSpacingVert/2).^2)/(sigma^2));
% bumpLL = exp(-.5*( (xgrid+dotSpacingHor/2).^2 + (ygrid-dotSpacingVert/2).^2)/(sigma^2));
% bumpLR = exp(-.5*( (xgrid-dotSpacingHor/2).^2 + (ygrid-dotSpacingVert/2).^2)/(sigma^2));
% template = 1 - max(cat(3, bumpUL, bumpUR, bumpLL, bumpLR), [], 3);
% template = image_downright(template) - template;
