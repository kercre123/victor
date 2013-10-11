function [imgUndist, xDistorted, yDistorted] = undistort(this, img, interpMethod)
% Undoes radial distortion of an image.
%
% [imgUndistorted, xDistorted, yDistorted] = camera.undistort(img)
%
%   Uses same radial/tangential distortion model as in Bouguet's Camera 
%   Calibration Toolbox.
%
% ------------
% Andrew Stein
%

if nargin<3 || isempty(interpMethod)
    interpMethod = 'bilinear';
end

img = im2double(img);

[nrows,ncols,nbands] = size(img);

if Camera.DistortionLUTres > 0
    xDistorted = this.xDistortedLUT;
    yDistorted = this.yDistortedLUT;
    
    if Camera.DistortionLUTres > 1
        [xgrid,ygrid] = meshgrid(1:ncols, 1:nrows);
        
        xDistorted = interp2(this.DistortionLUT_xCoords, ...
            this.DistortionLUT_yCoords, xDistorted, ...
            xgrid, ygrid, 'bilinear');
        
        yDistorted = interp2(this.DistortionLUT_xCoords, ...
            this.DistortionLUT_yCoords, yDistorted, ...
            xgrid, ygrid, 'bilinear');
    end
else
    [xgrid,ygrid] = meshgrid(1:ncols, 1:nrows);
    [xDistorted, yDistorted] = this.distort(xgrid, ygrid, true);
end

if nbands > 1
    imgUndist = cell(1, nbands);
    for i = 1:nbands
        imgUndist{i} = interp2(img(:,:,i), xDistorted, yDistorted, interpMethod);
    end
    imgUndist = cat(3, imgUndist{:});
else
    imgUndist = interp2(img, xDistorted, yDistorted, interpMethod);
end

if nargout==0
    subplot 121, imagesc(img), axis image off
    hold on, plot(this.center(1), this.center(2), 'r.');
    boundaryIndex = [1:nrows nrows:nrows:nrows*ncols ...
        nrows*ncols:-1:(nrows*(ncols-1)+1) ...
        (nrows*(ncols-1)+1):-nrows:1];
    plot(xDistorted(boundaryIndex), yDistorted(boundaryIndex), 'r');
    title('Input Distorted Image')
    
    subplot 122, imagesc(imgUndist), axis image off
    hold on, plot(this.center(1), this.center(2), 'r.');
    title('Undistorted Image')
    
    linkaxes
    
    clear imgUndist
end

end % FUNCTION undistortImage()