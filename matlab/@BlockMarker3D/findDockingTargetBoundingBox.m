function [u_box, v_box] = findDockingTargetBoundingBox(~, img, ...
    u_boxInit, v_boxInit)

% Find a BlockMarker3D's docking target bounding box in an image.

assert(strcmp(BlockMarker3D.DockingTarget, 'FourDots'), ...
    'findDockingTarget() is currently only written to support a "FourDots" target.');

[nrows,ncols,nbands] = size(img);

assert(nbands==1, 'Scalar-valued image required.');

squareHeight = ((v_boxInit(2)-v_boxInit(1)) + (v_boxInit(4)-v_boxInit(3)))/2;
squareWidth  = ((u_boxInit(3)-u_boxInit(1)) + (u_boxInit(4)-u_boxInit(2)))/2;

% We expect significant perspective distortion (i.e. foreshortening) as we
% get close, so keep track of vertical and horizontal radii separately
% (i.e. treat the dots as ellipses in a squashed rectangle, rather than
% circles in a square)
dotRadiusFraction = (BlockMarker3D.DockingDotWidth/2) / BlockMarker3D.CodeSquareWidth;
dotRadiusVert = dotRadiusFraction * squareHeight;
dotRadiusHor  = dotRadiusFraction * squareWidth;

dotSpacingFraction = BlockMarker3D.DockingDotSpacing / BlockMarker3D.CodeSquareWidth;
dotSpacingHor = dotSpacingFraction * squareWidth;
dotSpacingVert = dotSpacingFraction * squareHeight;

halfWidthHor = ceil(squareWidth/2);
t = -halfWidthHor: halfWidthHor;
bump1 = gaussian_kernel(dotRadiusHor/2, t-dotSpacingHor/2);
bump2 = gaussian_kernel(dotRadiusHor/2, t+dotSpacingHor/2);
kernelHor = max(bump1, bump2);
kernelHor = 1 - kernelHor/max(kernelHor);
kernelHor = image_right(kernelHor) - kernelHor;

halfWidthVert = ceil(squareHeight/2);
t = -halfWidthVert:halfWidthVert;
bump1 = gaussian_kernel(dotRadiusVert/2, t-dotSpacingVert/2);
bump2 = gaussian_kernel(dotRadiusVert/2, t+dotSpacingVert/2);
kernelVert = max(bump1, bump2);
kernelVert = 1 - kernelVert/max(kernelVert);
kernelVert = image_right(kernelVert) - kernelVert;

imgFilterHor = imfilter(image_right(img) - img, row(kernelHor));
imgFilterVert = imfilter(image_down(img) - img, kernelVert(:));

%{
imgFilter = max(0, imgFilterHor.*imgFilterVert);

subplot(231), imagesc(img), axis image, overlay_image(max(0,imgFilterHor), 'r')
subplot(232), imagesc(img), axis image, overlay_image(max(0, imgFilterVert), 'r')
subplot(233), imagesc(img), axis image, overlay_image(imgFilter, 'r')
%}

% only find peaks near the initialization
u_cen = mean(u_boxInit);
v_cen = mean(v_boxInit);
u_boxPad = 2*(u_boxInit-u_cen)+u_cen;
v_boxPad = 2*(v_boxInit-v_cen)+v_cen;
maskHor = true(1, ncols);
maskHor(floor(u_boxPad(1)):ceil(u_boxPad(3))) = false;
maskVert = true(1, nrows);
maskVert(floor(v_boxPad(1)):ceil(v_boxPad(2))) = false;
imgFilterHor(:,maskHor) = 0;
imgFilterVert(maskVert,:) = 0;

imgFilterHor = max(max(0, imgFilterHor),[],1);
imgFilterVert = max(max(0, imgFilterVert),[],2);

%{
imgFilter = imgFilterVert*imgFilterHor;
subplot(234), plot(imgFilterHor)
subplot(235), plot(imgFilterVert)
subplot(236), hold off, imagesc(img), axis image, overlay_image(imgFilter, 'r')
hold on
%}

[~,xTargetCen] = max(imgFilterHor);
[~,yTargetCen] = max(imgFilterVert);

%{
plot(xTargetCen, yTargetCen, 'y.', 'MarkerSize', 16);
rectangle('Pos', [xTargetCen-squareWidth/2 yTargetCen-squareHeight/2 squareWidth squareHeight], ...
    'EdgeColor', 'y');
%}

u_box = [xTargetCen-squareWidth/2 xTargetCen-squareWidth/2 ...
    xTargetCen+squareWidth/2 xTargetCen+squareWidth/2]';
v_box = [yTargetCen-squareHeight/2 yTargetCen+squareHeight/2 ...
    yTargetCen-squareHeight/2 yTargetCen+squareHeight/2];

end % FUNCTION findDockingTargetBoundingBox()
