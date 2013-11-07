
% function lucasKande_warpGroundTruth()

% im: input image (must be type double)
% H: 3x3 projective transform matrix
% sz: A 2x1 vector of the size of result, such as sz=[768,1024]
% interpMethod: Optional interpolation method, one of {'nearest','linear','spline','cubic'}
% result: output image
%
% If a transformed point is outside of the volume, NaN is used

% result = lucasKande_warpGroundTruth(ones(480,640), [cos(.1), -sin(.1), 250; sin(.1), cos(.1), 250; 0,0,1], 2*[480,640], 'linear');

function result = lucasKande_warpGroundTruth(im, H, sz, interpMethod)

sz=double(sz);

if ~exist('interpMethod','var')
   interpMethod='linear'; 
end

% Compute coordinates corresponding to input 
% and transformed coordinates for result

[xo,yo] = meshgrid(1:size(im,2), 1:size(im,1));
[x,y] = meshgrid(1:sz(2), 1:sz(1));

coords = [x(:)'; y(:)'];

homogeneousCoords = [coords; ones(1,prod(sz))];
warpedCoords = inv(H)*homogeneousCoords;

xprime = warpedCoords(1,:) ./ warpedCoords(3,:);
yprime = warpedCoords(2,:) ./ warpedCoords(3,:);

if ndims(im)==3
    result = zeros([sz(1), sz(2), 3]);
    for i=1:3
        resultTmp = interp2(xo, yo, double(im(:,:,i)), xprime, yprime, interpMethod);
        result(:,:,i) = reshape(resultTmp, sz);
    end
else
    result = interp2(xo, yo, double(im), xprime, yprime, interpMethod);
    result = reshape(result, sz);
end
