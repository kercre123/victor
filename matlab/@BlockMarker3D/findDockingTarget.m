function [xcen,ycen, u_box,v_box,img] = findDockingTarget(this, img, varargin)
% Find a BlockMarker3D's docking target in an image.

assert(strcmp(BlockMarker3D.DockingTarget, 'FourDots'), ...
    'findDockingTarget() is currently only written to support a "FourDots" target.');

mask = [];
squareDiagonal = [];
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


[xcen, ycen] = findFourDotTarget(img, 'uMask', u_box, 'vMask', v_box, ...
    'squareDiagonal', squareDiagonal, ...
    'TrueSquareWidth', BlockMarker3D.CodeSquareWidth, ...
    'TrueDotWidth', BlockMarker3D.DockingDotWidth, ...
    'TrueDotSpacing ', BlockMarker3D.DockingDotSpacing);

   
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
