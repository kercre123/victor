% function binaryLK(im1, mask1, im2)

% im1 = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_0.png');
% im2 = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_3.png');

% quad1 = [242,170; 418,161; 429,336; 241,341];

% mask1 = roipoly(im1); imwrite(mask1, 'c:/tmp/mask1.png');
% mask1 = imread('c:/tmp/mask1.png');

% binaryLK(im1, quad1, im2)

function binaryLK(im1, quad1, im2)

numScales = 3;
maxIterations = 50;

useBinary = true;
% useBinary = false;

mask1 = roipoly(im1, quad1(:,1), quad1(:,2));

if useBinary
    im1p = computeBinaryExtrema(im1, 3, 1.0, 255, true, true);
    im2p = computeBinaryExtrema(im2, 3, 1.0, 255, true, true);
        
    g = fspecial('gaussian',[7 1], 3.0);
    gs = g / max(g(:));
    
    im1p = imfilter(imfilter(im1p, g), gs');
    
    imshows(im1p, im2p)
else
    im1p = im1;
    im2p = im2;
end

lkTracker_projective = LucasKanadeTracker(im1p, mask1, ...
    'Type', 'homography', ...
    'DebugDisplay', false, 'UseBlurring', false,...
    'UseNormalization', false, 'NumScales', numScales,...
    'ApproximateGradientMargins', false);

lkTracker_projective.track(im2p, 'MaxIterations', maxIterations, 'ConvergenceTolerance', .25);

disp(lkTracker_projective.tform)
plotResults(im2, quad1, lkTracker_projective.tform);

keyboard

function plotResults(im, corners, H)
    cen = mean(corners,1);

    order = [1,2,3,4,1];

    hold off;
    imshow(im);
    hold on;
    plot(corners(order,1), corners(order,2), 'k--', ...
                    'LineWidth', 2, 'Tag', 'TrackRect');

    tempx = ...
        H(1,1)*(corners(order,1)-cen(1)) + ...
        H(1,2)*(corners(order,2)-cen(2)) + ...
        H(1,3);

    tempy = ...
        H(2,1)*(corners(order,1)-cen(1)) + ...
        H(2,2)*(corners(order,2)-cen(2)) + ...
        H(2,3);

    tempw = ...
        H(3,1)*(corners(order,1)-cen(1)) + ...
        H(3,2)*(corners(order,2)-cen(2)) + ...
        H(3,3);

    plot((tempx./tempw) + cen(1), (tempy./tempw) + cen(2), 'b', ...
                    'LineWidth', 2, 'Tag', 'TrackRect');
    