% function test_lucasKanade(im1, im2)

% im1 = im2double(rgb2gray(imread('C:\Anki\blockImages\blockImages00020.png')));
% im2 = im2double(rgb2gray(imread('C:\Anki\blockImages\blockImages00021.png')));

% test_lucasKanade(im1, im2)

function test_lucasKanade(im1, im2)

% close all

templateQuad = [95, 78;
        371, 83;
        361, 368;
        83, 354];

x0 = min(templateQuad(:,2));
x1 = max(templateQuad(:,2));
y0 = min(templateQuad(:,1));
y1 = max(templateQuad(:,1));

templateRect = [x0,y0;x1,y0;x1,y1;x0,y1];

mask1 = roipoly(im1, templateRect(:,1), templateRect(:,2));

scale = 2;
numScales = 5;

im1Small = imresize(im1, size(im1)/scale);
im2Small = imresize(im2, size(im2)/scale);
mask1Small = imresize(mask1, size(mask1)/scale);

templateQuad = templateQuad / scale;
templateRect = templateRect / scale;

corners = templateQuad;

% Test original tracker
LKtracker = LucasKanadeTracker(im1Small, mask1Small, 'Type', 'homography', 'DebugDisplay', false, 'UseBlurring', false, 'UseNormalization', false, 'NumScales', numScales);
converged = LKtracker.track(im2Small, 'MaxIterations', 50, 'ConvergenceTolerance', .25);
disp(LKtracker.tform);

% Test expanded tracker
[A_translationOnly, A_affine, templateImagePyramid] = lucasKanade_init(im1Small, templateRect, numScales, false, false);
%
H = eye(3);

for i = 1:2
    [update, ~, ~] = lucasKanade_computeUpdate(templateImagePyramid{5}, templateRect, A_translationOnly{5}, imresize(im2Small, size(im2Small)/16), H, 16, false)
    H = H - [0,0,update(1);0,0,update(2);0,0,0];
    figure(); plotResults(im1Small, im2Small, corners, H);
end

% for i = 1:3
for i = 1:2
    [update, ~, ~] = lucasKanade_computeUpdate(templateImagePyramid{3}, templateRect, A_translationOnly{3}, imresize(im2Small, size(im2Small)/4), H, 4, false)
    H = H - [0,0,update(1);0,0,update(2);0,0,0];
    figure(); plotResults(im1Small, im2Small, corners, H);
end

% for i = 1:3
% for i = 1
%     [update, ~, ~] = lucasKanade_computeUpdate(templateImagePyramid{2}, templateRect, A_translationOnly{2}, imresize(im2Small, size(im2Small)/2), H, 2, false);
%     H = H - [0,0,update(1);0,0,update(2);0,0,0];
%     figure(); plotResults(im1Small, im2Small, corners, H);
% end

for i = 1:2
    [update, ~, ~] = lucasKanade_computeUpdate(templateImagePyramid{1}, templateRect, A_translationOnly{1}, im2Small, H, 1, false);
    H = H - [0,0,update(1);0,0,update(2);0,0,0];
    figure(); plotResults(im1Small, im2Small, corners, H);
end

% [update2, ~, ~] = lucasKanade_computeUpdate(templateImagePyramid{2}, templateRect, A_translationOnly{2}, imresize(im2Small, size(im2Small)/2), eye(3), 2, false);
% [update3, ~, ~] = lucasKanade_computeUpdate(templateImagePyramid{3}, templateRect, A_translationOnly{3}, imresize(im2Small, size(im2Small)/(2^2)), eye(3), 2^2, false);
% [update4, ~, ~] = lucasKanade_computeUpdate(templateImagePyramid{4}, templateRect, A_translationOnly{4}, imresize(im2Small, size(im2Small)/(2^3)), eye(3), 2^3, false);
%
% disp(update1)
% disp(update2)
% disp(update3)
% disp(update4)

% h_axes = figure(1);
figure(100); plotResults(im1Small, im2Small, corners, LKtracker.tform);
% figure(2); plotResults(im1Small, im2Small, corners, eye(3) - [0,0,update1(1);0,0,update1(2);0,0,0]);
% figure(2); plotResults(im1Small, im2Small, corners, H);
% figure(3); plotResults(im1Small, im2Small, corners, eye(3) - [0,0,update2(1);0,0,update2(2);0,0,0]);
% figure(4); plotResults(im1Small, im2Small, corners, eye(3) - [0,0,update3(1);0,0,update3(2);0,0,0]);
% figure(5); plotResults(im1Small, im2Small, corners, eye(3) - [0,0,update4(1);0,0,update4(2);0,0,0]);


end % function test_lucasKanade()

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

