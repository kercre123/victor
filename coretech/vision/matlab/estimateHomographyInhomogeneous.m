% function homography = estimateHomographyInhomogeneous(originalPoints, transformedPoints)

% originalPoints = [0,0;1,0;0,1;1,1];
% transformedPoints = 2*originalPoints;
% homography = estimateHomographyInhomogeneous(originalPoints, transformedPoints)

function homography = estimateHomographyInhomogeneous(originalPoints, transformedPoints)

assert(size(originalPoints,2) == 2);
assert(size(transformedPoints,2) == 2);
assert(size(transformedPoints,1) == size(originalPoints,1));
assert(size(originalPoints,1) >= 4);

numPoints = size(originalPoints,1);

A = zeros(2*numPoints, 8);
b = zeros(8, 1);

for i = 1:numPoints
    xi = originalPoints(i,1);
    yi = originalPoints(i,2);
    
    xp = transformedPoints(i,1);
    yp = transformedPoints(i,2);
    
    A(2*(i-1) + 1, :) = [ 0,  0, 0, -xi, -yi, -1,  xi*yp,  yi*yp];
    A(2*(i-1) + 2, :) = [xi, yi, 1,   0,  0,   0, -xi*xp, -yi*xp];
    
    b(2*(i-1) + 1) = -yp;
    b(2*(i-1) + 2) = xp;
end

% The direct solution
% homography = A \ b;

% The cholesky solution
AtA = A'*A;
Atb = A'*b;

L = chol(AtA);

homography = L \ (L' \ Atb);
homography(9) = 1;

homography = reshape(homography, [3,3])';



% keyboard