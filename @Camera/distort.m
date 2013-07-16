function [uDistorted, vDistorted] = distort(this, a, b, normalize)
% Apply radial distortion to coordinates.
%
% [uDistorted, vDistorted] = camera.distort(this, a, b, <normalize>)
%
%  Computes radial distortion.  Let r^2 = a^2 + b^2.  Then the distorted
%  point coordinates (ud,vd) are:
%
%    ud = a * (1 + kc(1)*r^2 + kc(2)*r^4) + 2*kc(3)*a*b + kc(4)*(r^2 + 2*a^2);
%    vd = b * (1 + kc(1)*r^2 + kc(2)*r^4) + kc(3)*(r^2 + 2*b^2) + 2*kc(4)*a*b;
%
%  The left terms correspond to radial distortion, the right terms
%  correspond to tangential distortion. 'kc' are the calibrated
%  radial distortion parameters.
%
%  If normalize is true (default is false), focal length and camera center 
%  will be removed first, then distortion will be applied, and then focal 
%  length and camera center will be added back.  Otherwise, this is the 
%  caller's responsibility.
%
% ------------
% Andrew Stein
%

if nargin < 4
    normalize = false;
end

k = [this.distortionCoeffs(1:4); 1];

dims = size(a);
a = a(:);
b = b(:);

if normalize
    K = this.calibrationMatrix;
    temp = K \ [a b ones(length(a),1)]';
    a = temp(1,:)';
    b = temp(2,:)';
end

a2 = a.^2;
b2 = b.^2;
ab = a.*b;
r2 = a2 + b2;
r4 = r2.^2;
uDistorted = [a.*r2  a.*r4  2*ab    2*a2+r2 a] * k;
vDistorted = [b.*r2  b.*r4  r2+2*b2 2*ab    b] * k;

if normalize
    temp = K * [uDistorted vDistorted ones(length(a),1)]';
    uDistorted = temp(1,:)';
    vDistorted = temp(2,:)';
end

uDistorted = reshape(uDistorted, dims);
vDistorted = reshape(vDistorted, dims);


end