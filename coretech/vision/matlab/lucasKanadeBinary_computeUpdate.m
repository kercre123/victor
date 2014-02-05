
% function lucasKanadeBinary_computeUpdate()

% im1 = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_0.png');
% im2 = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_5.png');

% lucasKanadeBinary_computeUpdate(im1, im2, eye(3), 1.0, 3, 1.0, 255, 15, 5.0);

function lucasKanadeBinary_computeUpdate(...
    image1, image2,...
    initialHomography, scale,...
    extremaFilterWidth, extremaFilterSigma, extremaDerivativeThreshold,...
    matchingFilterWidth, matchingFilterSigma)

assert(size(image1,1) == size(image2,1));
assert(size(image1,2) == size(image2,2));
assert(size(image1,3) == 1);
assert(size(image2,3) == 1);

imageHeight = size(image1, 1);
imageWidth = size(image1, 2);

imageResizedHeight = imageHeight / scale;
imageResizedWidth = imageWidth / scale;

image1resized = imresize(image1, [imageResizedHeight, imageResizedWidth]);
image2resized = imresize(image2, [imageResizedHeight, imageResizedWidth]);

[~, xMinima1, yMinima1] = computeBinaryExtrema(...
    image1, extremaFilterWidth, extremaFilterSigma, extremaDerivativeThreshold, true, false,...
    true, matchingFilterWidth, matchingFilterSigma);

[~, xMaxima1, yMaxima1] = computeBinaryExtrema(...
    image1, extremaFilterWidth, extremaFilterSigma, extremaDerivativeThreshold, false, true,...
    true, matchingFilterWidth, matchingFilterSigma);


[~, xMinima2, yMinima2] = computeBinaryExtrema(...
    image2, extremaFilterWidth, extremaFilterSigma, extremaDerivativeThreshold, true, false);

[~, xMaxima2, yMaxima2] = computeBinaryExtrema(...
    image2, extremaFilterWidth, extremaFilterSigma, extremaDerivativeThreshold, false, true);

[xgrid, ygrid] = meshgrid( ...
    linspace(-(imageWidth-1)/2,  (imageWidth-1)/2,  imageResizedWidth), ...
    linspace(-(imageHeight-1)/2, (imageHeight-1)/2, imageResizedHeight));

% [xMinima2List_y, xMinima2List_x] = find(xMinima2 ~= 0);
% [yMinima2List_y, yMinima2List_x] = find(yMinima2 ~= 0);
% [xMaxima2List_y, xMaxima2List_x] = find(xMaxima2 ~= 0);
% [yMaxima2List_y, yMaxima2List_x] = find(yMaxima2 ~= 0);

xMinima2inds = find(xMinima2 ~= 0);
yMinima2inds = find(yMinima2 ~= 0);
xMaxima2inds = find(xMaxima2 ~= 0);
yMaxima2inds = find(yMaxima2 ~= 0);

xMinima2List_y = ygrid(xMinima2inds);
xMinima2List_x = xgrid(xMinima2inds);

yMinima2List_y = ygrid(yMinima2inds);
yMinima2List_x = xgrid(yMinima2inds);

xMaxima2List_y = ygrid(xMaxima2inds);
xMaxima2List_x = xgrid(xMaxima2inds);

yMaxima2List_y = ygrid(yMaxima2inds);
yMaxima2List_x = xgrid(yMaxima2inds);


xMinima2(5,5) = 1;
figure(1);
hold off;
imshow(xMinima2);
hold on;
scatter(xMinima2List_x+imageWidth/2+0.5, xMinima2List_y+imageHeight/2+0.5);

scatter(5,5, 'r+')


