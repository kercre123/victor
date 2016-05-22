
function test_trackingDiverging(varargin)

%imagePath = '~/temp/trackingDiverging/';
imagePath = 'C:/Anki/blockImages/trackingDiverging/';
resaveImages = false;

parseVarargin(varargin{:});

if resaveImages
    frames = {};
    for i = 1:120
        try 
            filename = fullfile(imagePath, sprintf('trackingImage%03d.png', i));
            im = imread(filename);
            frames{end+1} = im;
        catch
        end   
    end

    save(fullfile(imagePath, 'allFrames.mat'), 'frames');
else 
    load(fullfile(imagePath, 'allFrames.mat'), 'frames');
end

templateRegionRectangle = [31, 50, 10, 30];
numPyramidLevels = 2;
transformType = bitshift(6,8);
ridgeWeight = 1e-3;
maxIterations = 50;
convergenceTolerance = 0.05;
homography = eye(3,3);

templateImage = frames{1};

corners = [
    templateRegionRectangle(1), templateRegionRectangle(3);
    templateRegionRectangle(2), templateRegionRectangle(3);
    templateRegionRectangle(2), templateRegionRectangle(4);
    templateRegionRectangle(1), templateRegionRectangle(4)];    

plotResults(templateImage, templateImage, corners, homography);

for i = 2:length(frames)
    newHomography = mexTrackLucasKanade(uint8(templateImage), double(templateRegionRectangle), double(numPyramidLevels), double(transformType), double(ridgeWeight), uint8(frames{i}), double(maxIterations), double(convergenceTolerance), double(homography))    
    
    plotResults(templateImage, frames{i}, corners, homography);
    
    pause(.1);
     
    homography = newHomography;
end

keyboard



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
