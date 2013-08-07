function [imgRot] = matLocalization_step4_rotateImage(imgOrig, orient, imgCen, xgrid, ygrid, embeddedConversions, DEBUG_DISPLAY)

xgridRot =  xgrid*cos(orient) + ygrid*sin(orient) + imgCen(1);
ygridRot = -xgrid*sin(orient) + ygrid*cos(orient) + imgCen(2);

% Note that we use a value of 1 (white) for pixels outside the image, which
% will effectively give less weight to lines/squares that are closer to
% the image borders naturally. (Still true now that i'm using a derivative
% stencil?)
%imgRot = interp2(imgOrig, xgridRot, ygridRot, 'nearest', 1);
[nrows,ncols] = size(imgOrig);
inbounds = xgridRot >= 1 & xgridRot <= ncols & ygridRot >= 1 & ygridRot <= nrows;
index = round(ygridRot(inbounds)) + (round(xgridRot(inbounds))-1)*nrows;
imgRot = ones(nrows,ncols);
imgRot(inbounds) = imgOrig(index);