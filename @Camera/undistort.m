function varargout = undistort(this, varargin)
% Undoes radial distortion.  Can be applied to an image or 2D coordinates.
%
% imgUndistorted = camera.undistort(imgDistorted)
% [xUndistorted, yUndistorted] = camera.undistort(xDistorted, yDistorted)
%
%   Uses same radial/tangential distortion model as in Bouguet's Camera 
%   Calibration Toolbox.
%
% ------------
% Andrew Stein
%

assert(nargin == nargout+1, 'Unexpected number of input/output arguments.');

if nargin == 2
    img = varargin{1};
    [nrows,ncols,nbands] = size(img);
    [xgrid,ygrid] = meshgrid(1:ncols, 1:nrows);
    x_in = xgrid(:);
    y_in = ygrid(:);
    
elseif nargin == 3
    x_in = varargin{1};
    y_in = varargin{2};
    assert(isequal(size(x_in), size(y_in)), ...
        'X and Y inputs should be the same size.');
else
    error('Unexpected number of inputs.');
end

varargout = cell(1, nargout);

K = this.calibrationMatrix;

rays = K \ [(x_in(:)-1)'; (y_in(:)-1)'; ones(1, numel(x_in))];

x = rays(1,:)./rays(3,:);
y = rays(2,:)./rays(3,:);

% Add distortion:
r2 = x.^2 + y.^2;
r4 = r2.^2;
r6 = r2.^3;

% Radial distortion:
k = this.distortionCoeffs;
cdist = 1 + (k(1) * r2) + (k(2) * r4) + (k(5) * r6);

xd = x.*cdist;
yd = y.*cdist;

% Tangential distortion:
a1 = 2.*x.*y;
a2 = r2 + 2*x.^2;
a3 = r2 + 2*y.^2;

xd = xd + k(3)*a1 + k(4)*a2;
yd = yd + k(3)*a3 + k(4)*a1;

% Put back in pixel coordinates:
xi = this.focalLength(1)*(xd+this.alpha*yd) + this.center(1);
yi = this.focalLength(2)*yd + this.center(2);

if nargout == 1
    xi = reshape(xi, [nrows ncols]);
    yi = reshape(yi, [nrows ncols]);
    
    imgNew = cell(1, nbands);
    for i = 1:nbands
        imgNew{i} = interp2(img(:,:,i), xi, yi, 'bilinear');
    end
    imgNew = cat(3, imgNew{:});
    varargout{1} = imgNew;
else
    varargout{1} = reshape(xi, size(x_in));
    varargout{2} = reshape(yi, size(y_in));
end

end % FUNCTION undistortImage()