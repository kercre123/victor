% function test_lucasKanade2()

% im1 = rgb2gray(imresize(imread('C:\Anki\blockImages\blockImages00189.png'), [480,640]/8));
% im2 = rgb2gray(imresize(imread('C:\Anki\blockImages\blockImages00190.png'), [480,640]/8));
% % templateQuad = [20.2659574468085,21.6276595744681;35.3297872340426,29.0319148936170;28.1808510638298,43.9680851063830;12.7340425531915,37.0744680851064]
% templateQuad = [20,22;35,29;28,44;13,37];
% test_lucasKanade2(im1, im2, templateQuad)

% im101 = rgb2gray(imresize(imread('C:\Anki\blockImages\blockImages00100.png'), [480,640]/8));
% im102 = rgb2gray(imresize(imread('C:\Anki\blockImages\blockImages00101.png'), [480,640]/8));
% templateQuad = [8.5213, 30.3085; 27.9255, 30.5638; 26.7766, 49.8404; 8.3936, 49.2021];
% test_lucasKanade2(im101, im102, templateQuad)

% im101 = rgb2gray(imresize(imread('C:\Anki\blockImages\blockImages00100.png'), [480,640]/2));
% im102 = rgb2gray(imresize(imread('C:\Anki\blockImages\blockImages00101.png'), [480,640]/2));
% templateQuad = [8.5213, 30.3085; 27.9255, 30.5638; 26.7766, 49.8404; 8.3936, 49.2021]*4;
% test_lucasKanade2(im101, im102, templateQuad)

% im315 = rgb2gray(imresize(imread('C:\Anki\blockImages\blockImages00315.png'), [480,640]/8));
% im316 = rgb2gray(imresize(imread('C:\Anki\blockImages\blockImages00316.png'), [480,640]/8));
% templateQuad = [52.0936, 115.3606; 85.7988, 122.2928; 79.5837, 156.9542; 45.1614, 150.2610]/4;
% test_lucasKanade2(im315, im316, templateQuad)

function test_lucasKanade2(im1, im2, templateQuad)

% close all
        
x0 = min(templateQuad(:,2));
x1 = max(templateQuad(:,2));
y0 = min(templateQuad(:,1));
y1 = max(templateQuad(:,1));

templateRect = round([x0,y0;x1,y0;x1,y1;x0,y1])
templateRect_cFormat = [x0,x1+1,y0,y1+1];

% [im1, im2, templateRect] = drawExample([60,80], false);

% imshows(im1, im2)

% mask1 = roipoly(im1, templateQuad(:,1), templateQuad(:,2));
mask1 = roipoly(im1, templateRect(:,2), templateRect(:,1));

numScales = 2;
maxIterations = 50;
ridgeWeight = 1e-3;

figure(100); subplot(1,2,2); imshow(im1);

% translation only
LKtracker = LucasKanadeTracker(im1, mask1, 'Type', 'translation', 'DebugDisplay', true, 'UseBlurring', false, 'UseNormalization', false, 'NumScales', numScales, 'ApproximateGradientMargins', false);
converged = LKtracker.track(im2, 'MaxIterations', maxIterations, 'ConvergenceTolerance', .05);
disp('Matlab LK translation-only:');
disp(LKtracker.tform);
figure(101); plotResults(im1, im2, templateQuad, LKtracker.tform);

homography_translate = mexTrackLucasKanade(uint8(im1), double(templateRect_cFormat), double(numScales), double(1), double(ridgeWeight), uint8(im2), double(maxIterations), double(0.05), double(eye(3,3)));
disp('C++ LK translation-only:');
disp(homography_translate);
figure(102); plotResults(im1, im2, templateQuad, homography_translate);

% projective
LKtracker = LucasKanadeTracker(im1, mask1, 'Type', 'homography', 'DebugDisplay', false, 'UseBlurring', false, 'UseNormalization', false, 'NumScales', numScales, 'ApproximateGradientMargins', false);
converged = LKtracker.track(im2, 'MaxIterations', maxIterations, 'ConvergenceTolerance', .25);
disp('Matlab LK projective:');
disp(LKtracker.tform);
figure(201); plotResults(im1, im2, templateQuad, LKtracker.tform);

homography_projective = mexTrackLucasKanade(uint8(im1), double(templateRect_cFormat), double(numScales), double(2), double(ridgeWeight), uint8(im2), double(maxIterations), double(0.05), double(eye(3,3)));
disp('C++ LK projective:');
disp(homography_projective);
figure(202); plotResults(im1, im2, templateQuad, homography_projective);

end % function test_lucasKanade()

function [im1, im2, templateRect] = drawExample(imageSize, translationOnly)
    im1 = zeros(imageSize);
    im2 = zeros(imageSize);
    
    im1(10:50, 10:40) = 1;
    im1(15:45, 15:35) = 0;
    
    templateRect = [8,8; 42,8; 42,52; 8,52];

    if translationOnly
        im2(10:50, 12:42) = 1;
%         im2(15:45, 17:37) = 0;
        im2(15:45, 17:37) = repmat(linspace(), []);
    else 
        distortion = 32;
        
        for x = 12:42
            im2(round(10+(x-16)/distortion):round(50-(x-16)/distortion), x) = 1;
        end
        
        for x = 17:37
            im2(round(15+(x-16)/distortion):round(45-(x-16)/distortion), x) = 0;
        end
    end
end % function drawExample()

function plotResults(im1, im2, corners, H)

    cen = mean(corners,1);

    order = [1,2,3,4,1];

    subplot(1,2,1);
    hold off;
    imshow(im1)

    subplot(1,2,2);
    hold off;
    imshow(im2);
    hold on;
    plot(corners(order,1), corners(order,2), 'b--', ...
                    'LineWidth', 2, 'Tag', 'TrackRect');

    tempx = H(1,1)*(corners(:,1)-cen(1)) + ...
        H(1,2)*(corners(:,2)-cen(2)) + ...
        H(1,3) + cen(1);

    tempy = H(2,1)*(corners(:,1)-cen(1)) + ...
        H(2,2)*(corners(:,2)-cen(2)) + ...
        H(2,3) + cen(2);

    plot(tempx, tempy, 'r', ...
                    'LineWidth', 2, 'Tag', 'TrackRect');
end % function plotResults()

