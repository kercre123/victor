function varargout = projectPoints(varargin)
% Project 3D world coordinates onto the image plane of a calibrated camera.
%
% [u,v] = projectPoints(X,Y,Z, calibration)
% [p]   = projectPoints(P, calibration)
%
%   Inputs are 3D (X,Y,Z) coordinates, separated or combined into 
%    P = [X(:) Y(:) Z(:)]
%
%   Outputs are image coordinates (u,v), separated or combined into 
%    p = [u(:) v(:)]
%
%   (This exactly follows the projection model in Bouguet's camera 
%    calibration toolbox.)
%
%   1. Pinhole projection to (a,b) on the plane, where a=x/z and b=y/z.
%   2. Radial distortion.  Let r^2 = a^2 + b^2.  Then the distorted
%      point coordinates (ud,vd) are:
%
%      ud = a * (1 + kc(1)*r^2 + kc(2)*r^4) + 2*kc(3)*a*b + kc(4)*(r^2 + 2*a^2);
%      vd = b * (1 + kc(1)*r^2 + kc(2)*r^4) + kc(3)*(r^2 + 2*b^2) + 2*kc(4)*a*b;
%
%      The left terms correspond to radial distortion, the right terms
%      correspond to tangential distortion. 'kc' are the calibrated
%      radial distortion parameters.
%
%   3. Convertion into pixel coordinates, according to focal lengths and
%      center of projection:
%
%      u = f(1)*ud + c(1)
%      v = f(2)*vd + c(2)
%
% ------------
% Andrew Stein
%

switch(nargin)
    case 2
        assert(size(varargin{1},2)==3, 'P should be Nx3.');
        
        P = varargin{1};
        calibration = varargin{2};
        
    case 4
        assert(all(cellfun(@isvector, varargin(1:3))), ...
            'X, Y, and Z should be vectors.');
        
        P = [varargin{1}(:) varargin{2}(:) varargin{3}(:)];
        calibration = varargin{4};
        
    otherwise
        error('Unexpected number of inputs.');
end

assert(isstruct(calibration), 'Calibration data should be a struct.');
assert(all(isfield(calibration, {'kc', 'fc', 'cc'})), ...
    'Calibration struct should have fields "kc", "fc", and "cc".');

if length(calibration.kc)>4 && calibration.kc(5)~=0
    warning('Ignoring fifth-order non-zero radial distortion');
end
k = [calibration.kc(1:4); 1];
a = P(:,1) ./ P(:,3);
b = P(:,2) ./ P(:,3);
a2 = a.^2;
b2 = b.^2;
ab = a.*b;
r2 = a2 + b2;
r4 = r2.^2;
uDistorted = [a.*r2  a.*r4  2*ab    2*a2+r2 a] * k;
vDistorted = [b.*r2  b.*r4  r2+2*b2 2*ab    b] * k;
u = calibration.fc(1) * uDistorted + calibration.cc(1);
v = calibration.fc(2) * vDistorted + calibration.cc(2);

if nargout==1
    varargout{1} = [u v];
elseif nargout==2
    varargout{1} = u;
    varargout{2} = v;
end

end