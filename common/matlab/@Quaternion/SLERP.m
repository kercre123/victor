function Qi = SLERP(P, Q, t, shortestPath)
% SLERP - Spherical Linear Interpolation of two Quaternions.

if nargin < 4
    shortestPath = true;
end

assert(isa(P, 'Quaternion') && isa(Q, 'Quaternion') && ...
    isa(t, 'double') && isscalar(t) && t>=0 && t<=1, ...
    'Expecting two Quaternions and a scalar double, [0,1].');

fCos = dot(P, Q);

if shortestPath && fCos < 0
   % Invert rotation
   fCos = -fCos;
   Q = -Q;
end

fSin   = sqrt(1 - fCos*fCos);
angle  = atan2(fSin, fCos);
invSin = 1/fSin;
coeff1 = sin((1-t) * angle) * invSin;
coeff2 = sin(  t   * angle) * invSin;

Qi = coeff1*P + coeff2*Q;

end
