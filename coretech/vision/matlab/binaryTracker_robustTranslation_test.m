
function binaryTracker_robustTranslation_test()

% mask1 = imresize(imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_12_mask.png'), [240,320]);
mask1 = imread('C:\Anki\systemTestImages\cozmo_date2014_02_27_time14_56_37_frame0_mask.png');

%for i = 4:4:49
for i = 1:4:49
%     im1 = uint8(imresize(imread(sprintf('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_%d.png',i-4)), [240,320]));
%     im2 = uint8(imresize(imread(sprintf('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_%d.png',i)), [240,320]));
    im1 = uint8(imresize(imread(sprintf('C:/Anki/systemTestImages/cozmo_date2014_02_27_time14_56_37_frame%d.png',i-1)), [240,320]));
    im2 = uint8(imresize(imread(sprintf('C:/Anki/systemTestImages/cozmo_date2014_02_27_time14_56_37_frame%d.png',i)),   [240,320]));
    
    [dx, dy] = binaryTracker_robustTranslation(im1, im2, mask1, 100, 2, 40, 0.05);
    
    updatedHomography = binaryTracker_computeUpdate(im1, mask1, im2, eye(3,3), 1, [], [], [], 7, 'translation', true);
    
    homography = eye(3,3); homography(1,3) = -dx; homography(2,3) = -dy;
    warped2 = uint8(binaryTracker_warpWithHomography(im2, homography));
    
    warped2b = uint8(binaryTracker_warpWithHomography(im2, inv(updatedHomography)));
    
    imshows(im2, im1, warped2, warped2b, 'maximize');
    figure(1); text(50,50,sprintf('image %d', i));
    figure(2); text(50,50,sprintf('image %d', i));
    figure(3); text(50,50,sprintf('image %d', i));
    figure(4); text(50,50,sprintf('image %d', i)); 
    pause();
end



% for i = 1:4:49
%     im1 = uint8(imresize(imread(sprintf('C:/Anki/systemTestImages/cozmo_date2014_02_27_time14_56_37_frame%d.png',i-1)), [240,320]));
%     im2 = uint8(imresize(imread(sprintf('C:/Anki/systemTestImages/cozmo_date2014_02_27_time14_56_37_frame%d.png',i)),   [240,320]));
%     [dx, dy] = binaryTracker_robustTranslation(im1, im2, mask1, 100, 2, 40, 0.1);
%     homography = eye(3,3); homography(1,3) = -dx; homography(2,3) = -dy;
%     warped2 = uint8(binaryTracker_warpWithHomography(im2, homography));
%     imshows(im2, im1, warped2, 'maximize');
%     figure(1); text(50,50,sprintf('image %d', i))
%     figure(2); text(50,50,sprintf('image %d', i))
%     figure(3); text(50,50,sprintf('image %d', i))
%     pause();
% end