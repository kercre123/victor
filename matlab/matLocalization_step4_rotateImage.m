function [imgRot] = matLocalization_step4_rotateImage(imgOrig, orient1, imgCen, xgrid, ygrid, embeddedConversions, DEBUG_DISPLAY)

xgridRot =  xgrid*cos(orient1) + ygrid*sin(orient1) + imgCen(1);
ygridRot = -xgrid*sin(orient1) + ygrid*cos(orient1) + imgCen(2);

% Note that we use a value of 1 (white) for pixels outside the image, which
% will effectively give less weight to lines/squares that are closer to
% the image borders naturally. (Still true now that i'm using a derivative
% stencil?)
imgRot = interp2(imgOrig, xgridRot, ygridRot, 'nearest', 1);
