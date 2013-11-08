% function lucasKanade_iterativelyOptimize()

% im1 = imresize(im2double(rgb2gray(imread('C:\Anki\blockImages\blockImages00020.png'))), [240,320]);
% im2 = imresize(im2double(rgb2gray(imread('C:\Anki\blockImages\blockImages00021.png'))), [240,320]);

% templateQuad = round([95, 78; 371, 83; 361, 368; 83, 354] / 2);
% x0 = min(templateQuad(:,2)); x1 = max(templateQuad(:,2));
% y0 = min(templateQuad(:,1)); y1 = max(templateQuad(:,1));
% templateRect = [x0,y0;x1,y0;x1,y1;x0,y1];

% initialHomography = eye(3);
% whichScales = 1:4;
% maxIterations = 10;
% convergenceThreshold = Inf;
% debugDisplay = true;

% [A_translationOnly, A_affine, templateImagePyramid] = lucasKanade_init(im1, templateRect, max(whichScales), false, true);

% homography = lucasKanade_iterativelyOptimize(A_translationOnly, A_affine, templateImagePyramid, templateRect, im2, initialHomography, whichScales, maxIterations, convergenceThreshold, debugDisplay)



% mask1 = roipoly(im1, templateRect(:,1), templateRect(:,2));
% LKtracker = LucasKanadeTracker(im1, mask1, 'Type', 'affine', 'DebugDisplay', false, 'UseBlurring', false, 'UseNormalization', false, 'NumScales', max(whichScales));
% converged = LKtracker.track(im2, 'MaxIterations', 50, 'ConvergenceTolerance', .25);
% disp(LKtracker.tform);

function homography = lucasKanade_iterativelyOptimize(A_translationOnly, A_affine, templateImagePyramid, templateRect, newImage, initialHomography, whichScales, maxIterations, convergenceThreshold, debugDisplay)

whichScales = sort(whichScales,'descend');

homography = initialHomography;

for iScale = whichScales
    newImageSmall = imresize(newImage, size(newImage)/(2^(iScale-1)));
    
    if debugDisplay
        figure(100 + iScale); 
    end
    
    if ~isempty(A_translationOnly)
        for iteration = 1:maxIterations
            [update, ~, ~] = lucasKanade_computeUpdate(templateImagePyramid{iScale}, templateRect, A_translationOnly{iScale}, newImageSmall, homography, 2^iScale, false);
            homography = homography - [0,0,update(1);0,0,update(2);0,0,0];
        end
    end
    
    if debugDisplay
        subplot(1,2,1); plotResults(newImage, templateRect, homography);
    end
    
    if ~isempty(A_affine)
        for iteration = 1:maxIterations
            [update, ~, ~] = lucasKanade_computeUpdate(templateImagePyramid{iScale}, templateRect, A_affine{iScale}, newImageSmall, homography, 2^iScale, false);
            tformUpdate = eye(3) + [update(1:3)'; update(4:6)'; zeros(1,3)];
            homography = homography / tformUpdate;
        end
    end
       
    if debugDisplay
        subplot(1,2,2); plotResults(newImage, templateRect, homography);
    end
    
    % TODO: check for convergence
end

end % function lucasKanade_iterativelyOptimize()

function plotResults(image, corners, homography)

    cen = mean(corners,1);

    order = [1,2,3,4,1];

    hold off;
    imshow(image);
    hold on;
    plot(corners(order,1), corners(order,2), 'b--', ...
                    'LineWidth', 2, 'Tag', 'TrackRect');

    tempx = homography(1,1)*(corners(:,1)-cen(1)) + ...
        homography(1,2)*(corners(:,2)-cen(2)) + ...
        homography(1,3) + cen(1);

    tempy = homography(2,1)*(corners(:,1)-cen(1)) + ...
        homography(2,2)*(corners(:,2)-cen(2)) + ...
        homography(2,3) + cen(2);

    plot(tempx, tempy, 'r', ...
                    'LineWidth', 2, 'Tag', 'TrackRect');
end % function plotResults()

