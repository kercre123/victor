% function test_lucasKanade3()

function test_lucasKanade3()

% showTranslationOnly = true;
showTranslationOnly = false;

directoryName = 'C:/Anki/blockImages/testLKtrackSequence/';
files = dir([directoryName, '*.png']);

im1 = rgb2gray(imresize(imread([directoryName,files(1).name]), [480,640]/8));

% Use this commented code to pick a new templateQuad
% im1 = rgb2gray(imread([directoryName,files(1).name]));
% imshow(im1)
% [x,y] = ginput(4)
% templateQuad = round([x,y]/8)

templateQuad = [15, 21;
                54, 20;
                55, 57;
                19, 55];

x0 = min(templateQuad(:,2));
x1 = max(templateQuad(:,2));
y0 = min(templateQuad(:,1));
y1 = max(templateQuad(:,1));

templateRect = round([x0,y0;x1,y0;x1,y1;x0,y1]);
templateRect_cFormat = [x0,x1+1,y0,y1+1];

mask1 = roipoly(im1, templateRect(:,2), templateRect(:,1));

numScales = 2;
maxIterations = 50;
ridgeWeight = 1e-3;

% close all
figure(100); 
subplot(1,2,1); imshow(im1);
subplot(1,2,2); imshow(im1);

LKtracker_translation = LucasKanadeTracker(im1, mask1, 'Type', 'translation', 'DebugDisplay', false, 'UseBlurring', false, 'UseNormalization', false, 'NumScales', numScales, 'ApproximateGradientMargins', false);
LKtracker_affine = LucasKanadeTracker(im1, mask1, 'Type', 'affine', 'DebugDisplay', false, 'UseBlurring', false, 'UseNormalization', false, 'NumScales', numScales, 'ApproximateGradientMargins', false);
LKtracker_projective = LucasKanadeTracker(im1, mask1, 'Type', 'homography', 'DebugDisplay', false, 'UseBlurring', false, 'UseNormalization', false, 'NumScales', numScales, 'ApproximateGradientMargins', false);

homography_translate = eye(3);
homography_affine = eye(3);
homography_projective = eye(3);

for iFile = 2:length(files)
    im2 = rgb2gray(imresize(imread([directoryName,files(iFile).name]), [480,640]/8));

    if showTranslationOnly
        LKtracker_translation.track(im2, 'MaxIterations', maxIterations, 'ConvergenceTolerance', .05);
        disp('Matlab LK translation-only:');
        disp(LKtracker_translation.tform);

        homography_translate = mexTrackLucasKanade(uint8(im1), double(templateRect_cFormat), double(numScales), double(bitshift(2,8)), double(ridgeWeight), uint8(im2), double(maxIterations), double(0.05), double(homography_translate));
        disp('C++ LK translation-only:');
        disp(homography_translate);
    end
    
    LKtracker_affine.track(im2, 'MaxIterations', maxIterations, 'ConvergenceTolerance', .25);
    disp('Matlab LK affine:');
    disp(LKtracker_affine.tform);
    
    homography_affine= mexTrackLucasKanade(uint8(im1), double(templateRect_cFormat), double(numScales), double(bitshift(6,8)), double(ridgeWeight), uint8(im2), double(maxIterations), double(0.05), double(homography_affine));
    disp('C++ LK affine:');
    disp(homography_affine);
    
    LKtracker_projective.track(im2, 'MaxIterations', maxIterations, 'ConvergenceTolerance', .25);
    disp('Matlab LK projective:');
    disp(LKtracker_projective.tform);
    
    homography_projective = mexTrackLucasKanade(uint8(im1), double(templateRect_cFormat), double(numScales), double(bitshift(8,8)), double(ridgeWeight), uint8(im2), double(maxIterations), double(0.05), double(homography_projective));
    disp('C++ LK projective:');
    disp(homography_projective);

    if showTranslationOnly
        figure(201); 
        subplot(1,2,1); plotResults(im1, im2, templateQuad, LKtracker_translation.tform);
        subplot(1,2,2); plotResults(im1, im2, templateQuad, homography_translate);
    end
    
    figure(601); 
    subplot(1,2,1); plotResults(im1, im2, templateQuad, LKtracker_affine.tform);
    subplot(1,2,2); plotResults(im1, im2, templateQuad, homography_affine);
    
    figure(801); 
    subplot(1,2,1); plotResults(im1, im2, templateQuad, LKtracker_projective.tform);
    subplot(1,2,2); plotResults(im1, im2, templateQuad, homography_projective);
    
    pause(.5);
%     pause();
    
end % for iFile = 1:length(files)

end % function test_lucasKanade3()

function plotResults(im1, im2, corners, H)

    cen = mean(corners,1);

    order = [1,2,3,4,1];

%     subplot(1,2,1);
%     hold off;
%     imshow(im1)

%     subplot(1,2,2);
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

