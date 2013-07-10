function [imgUndist, xDistorted, yDistorted] = undistort(this, img, interpMethod)
% Undoes radial distortion of an image.
%
% [imgUndistorted, xUndistorted, yUndistorted] = camera.undistort(img)
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
[xgrid,ygrid] = meshgrid(1:ncols, 1:nrows);

[xDistorted, yDistorted] = this.distort(xgrid, ygrid, true);
  
imgUndist = cell(1, nbands);
for i = 1:nbands
    imgUndist{i} = interp2(img(:,:,i), xDistorted, yDistorted, interpMethod);
end
imgUndist = cat(3, imgUndist{:});

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