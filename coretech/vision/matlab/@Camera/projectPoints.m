function varargout = projectPoints(this, varargin)
% Project 3D world coordinates onto the image plane of a calibrated camera.
%
% [u,v] = camera.projectPoints(X,Y,Z, <Pose>)
% [p]   = camera.projectPoints(P, <Pose>)
%
%   Inputs are 3D (X,Y,Z) coordinates, separated or combined into 
%    P = [X(:) Y(:) Z(:)]
%
%   Outputs are image coordinates (u,v), separated or combined into 
%    p = [u(:) v(:)]
%
%   If a Pose is provided, points are first transformed by that Pose.
%   Otherwise (default), they are assumed to be in the camera's coordinate
%   frame, ready for projection.
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

pose = [];

origDims = [];
switch(nargin)
    case {2,3}
        assert(size(varargin{1},2)==3, 'P should be Nx3.');
        
        P = varargin{1};
        
        if nargin == 3
            pose = varargin{2};
        end
                
    case {4,5}
        X = varargin{1};
        Y = varargin{2};
        Z = varargin{3};
        
        origDims = size(X);
        assert(isequal(size(Y),origDims) && isequal(size(Z),origDims), ...
            'X, Y, and Z should be the same size.');
        
        P = [X(:) Y(:) Z(:)];
             
        if nargin == 5
            pose = varargin{4};
        end
        
    otherwise
        error('Unexpected number of inputs.');
end

if ~isempty(pose)
    P = pose.applyTo(P);
end

% Only keep Points in front of the camera (those with positive Z 
% coordinate, now that P is in the camera's frame)
invisible = P(:,3)<0;
    
if length(this.distortionCoeffs)>4 && this.distortionCoeffs(5)~=0
    warning('Ignoring fifth-order non-zero radial distortion');
end

a = P(:,1) ./ P(:,3);
b = P(:,2) ./ P(:,3);
[uDistorted, vDistorted] = this.distort(a, b);
u = this.focalLength(1) * uDistorted + this.center(1);
v = this.focalLength(2) * vDistorted + this.center(2);
    
u(invisible) = -1;
v(invisible) = -1;

if nargout==1
    varargout{1} = [u v P(:,3)];
elseif nargout>=2
    if ~isempty(origDims)
        u = reshape(u, origDims);
        v = reshape(v, origDims);
    end
    
    varargout{1} = u;
    varargout{2} = v;
    if nargout == 3
        w = P(:,3);
        if ~isempty(origDims)
            w = reshape(w, origDims);
        end
        varargout{3} = w;
    end
end

end