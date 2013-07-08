function [xMat,yMat,orient1] = matLocalization(img, varargin)

%% Params
derivSigma = 1;
pixPerMM = 6;
squareWidth = 10;
lineWidth = 1.5;
codePadding = 1;

parseVarargin(varargin{:});

%% Init
if isa(img, 'uint8')
    img = double(img)/255;
end

if size(img,3)>1
    img = mean(img,3);
end

assert(min(img(:))>=0 && max(img(:))<=1, ...
    'Image should be on the interval [0 1].');

%% Image Orientation
[Ix,Iy] = smoothgradient(img, derivSigma);
mag = sqrt(Ix.^2 + Iy.^2);
orient = atan2(Iy, Ix);
orient(orient < 0) = pi + orient(orient<0);

% Bin into 1-degree bins, using linear interpolation
orient = 180/pi*orient(:);
leftBin = floor(orient);
leftBinWeight = 1-(orient-leftBin);
rightBin = leftBin + 1;
rightBin(rightBin==181) = 1;
%counts = row(accumarray(round(180/pi*orient(:))+1, mag(:)));
assert(all(leftBinWeight>=0 & leftBinWeight <= 1), ...
    'Bin weights should be [0,1]');
counts = row(accumarray(leftBin+1, leftBinWeight.*mag(:), [181 1]));
counts = counts + row(accumarray(rightBin+1, (1-leftBinWeight).*mag(:), [181 1]));
%counts = imfilter(counts, gaussian_kernel(1), 'circular');

% Find peaks
localMaxima = find(counts > counts([end 1:end-1]) & counts > counts([2:end 1]));
if length(localMaxima) < 2
    error('Could not find at least 2 peaks in orientation histogram.');
end

[~,sortIndex] = sort(counts(localMaxima), 'descend');
localMaxima = localMaxima(sortIndex(1:2));

orient1 = -(localMaxima(1)-1)*pi/180; % subtract 1 because of +1 in accumarray above
orient2 = -(localMaxima(2)-1)*pi/180;
if orient2 > orient1
    temp = orient1;
    orient1 = orient2;
    orient2 = temp;
end
orientDiff = orient1 - orient2; 
if abs(orientDiff - pi/2) > 5*pi/180
    error('Found two orientation peaks, but they are not ~90 degrees apart.');
end

% Fit a parabola to the peak orientation to get sub-bin orientation
% precision
assert(length(counts)==181, 'Expecting 181 orientation bins');
if localMaxima(1)==1
    bins = [181 1 2]';
elseif localMaxima(1)==181
    bins = [180 181 1]';
else
    bins = localMaxima(1) + [-1 0 1]';
end
% solve for parameters of parabola:
A = [bins.^2 bins ones(3,1)];
p = A \ counts(bins)';

% Find max location of the parabola *in terms of bins*.  Remember to
% subtract 1 again because of +1 in accumarray above.  Then convert to
% radians.
orient1 = -(-p(2)/(2*p(1)) - 1) * pi/180; 

%% Grid Square localization
[nrows,ncols] = size(img);
[xgrid,ygrid] = meshgrid(linspace(-ncols/2,ncols/2,ncols), ...
    linspace(-nrows/2,nrows/2,nrows));

xgridRot =  xgrid*cos(orient1) + ygrid*sin(orient1) + ncols/2;
ygridRot = -xgrid*sin(orient1) + ygrid*cos(orient1) + nrows/2;

imgRot = interp2(img, xgridRot, ygridRot, 'nearest', -1);
weights = double(imgRot >= 0);

weights(:,all(weights==0,1)) = 1;
weights(all(weights==0,2),:) = 1;

assert(all(any(weights > 0, 1)) && all(any(weights > 0, 2)), ...
    'There should be no all-zero rows or cols in weight image.');

colMeans = sum(weights.*imgRot,1) ./ sum(weights,1);
rowMeans = sum(weights.*imgRot,2) ./ sum(weights,2);

x = linspace(-squareWidth,squareWidth,pixPerMM*2*squareWidth);
gridStencil = max(exp(-(x+squareWidth/2).^2/(2*(lineWidth/2)^2)), ...
    exp(-(x-squareWidth/2).^2/(2*(lineWidth/2)^2)));

[~,xcenIndex] = max(conv2(1-colMeans, gridStencil, 'same'));
[~,ycenIndex] = max(conv2(1-rowMeans', gridStencil, 'same'));

xcen = xgridRot(ycenIndex,xcenIndex);
ycen = ygridRot(ycenIndex,xcenIndex);

%% Square Identification
codeWidth = (squareWidth - lineWidth - 2*codePadding)*pixPerMM;
codeImg = imgRot(round(ycenIndex + (-codeWidth/2 : codeWidth/2)), ...
    round(xcenIndex + (-codeWidth/2 : codeWidth/2)) );

insideSquareWidth = squareWidth - lineWidth;
 
s = diag(insideSquareWidth * pixPerMM * [1 1]);
R = [cos(orient1) sin(orient1); -sin(orient1) cos(orient1)];
t = [xcen; ycen] - R*diag(s)/2;
tform = maketform('affine', [s*R t; 0 0 1]');

corners = [0 0; ...
           0 1; ...
           1 0; ...
           1 1];
        
corners = tformfwd(tform, corners);
    
marker = MatMarker2D(img, corners, maketform('affine', tform.tdata.Tinv));

% Get camera/image position on mat, in mm:
xMat = marker.X * squareWidth - squareWidth/2 + (ncols/2-xcen)/pixPerMM;
yMat = marker.Y * squareWidth - squareWidth/2 + (nrows/2-ycen)/pixPerMM;

if nargout == 0
    subplot 221
    hold off, imagesc(img), axis image, hold on
    plot(xcen, ycen, 'r.');
    title(sprintf('Orientation = %.1f degrees', orient1*180/pi));
    
    x = insideSquareWidth/2 * [-1 1 1 -1 -1] * pixPerMM;
    y = insideSquareWidth/2 * [-1 -1 1 1 -1] * pixPerMM;
    xRot = x*cos(orient1) + y*sin(orient1) + xcen;
    yRot = -x*sin(orient1) + y*cos(orient1) + ycen;
    
    plot(xRot, yRot, 'r');
    
    subplot 222
    imagesc(codeImg), axis image
    title(sprintf('X = %d, Y = %d', marker.X, marker.Y));
    
    subplot 212
    gridMat('matSize', 200);
    hold on
    h = plot(xMat, yMat, 'r.');
    xImg = nrows/2*[-1 1 1 -1 -1] / pixPerMM;
    yImg = ncols/2*[-1 -1 1 1 -1] / pixPerMM;
    xImgRot = xImg*cos(-orient1) + yImg*sin(-orient1) + xMat;
    yImgRot = -xImg*sin(-orient1) + yImg*cos(-orient1) + yMat;
    plot(xImgRot, yImgRot, 'r');
end



% keyboard