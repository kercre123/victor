
% function updatedCorners = trackSquareCorners(image, initialCorners, harris_width, harris_sigma, grayvalueMatch_width, grayvalueMatch_offset, maxCornerDelta)

% image = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_38.png');
% image = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_0.png');

% initialCorners = {[0,0], [0,0]}; maxCornerDelta = 30;
% harris_width = 9; harris_sigma = 2.5;
% grayvalueMatch_width = 3; grayvalueMatch_offset = 3;
% updatedCorners = trackSquareCorners(imresize(image,[120,160]), initialCorners, harris_width, harris_sigma, grayvalueMatch_width, grayvalueMatch_offset, maxCornerDelta);

function updatedCorners = trackSquareCorners(image, initialCorners, harris_width, harris_sigma, grayvalueMatch_width, grayvalueMatch_offset, maxCornerDelta)

assert(mod(grayvalueMatch_width,2) == 1);
assert(mod(harris_width,2) == 1);

gvm_kernel = ones(grayvalueMatch_width,grayvalueMatch_width);
gvm_kernel = gvm_kernel / sum(gvm_kernel(:));

gvm_filteredImage = imfilter(double(image), gvm_kernel);

ix = imfilter(image, [-1,0,1]);
iy = imfilter(image, [-1,0,1]');
imshows(image,ix,iy);

gvm_tlImage = zeros(size(image));
gvm_trImage = zeros(size(image));
gvm_blImage = zeros(size(image));
gvm_brImage = zeros(size(image));
offset = 5;
% tic
for y = (1+offset):(size(image,1)-offset)
    xs = (1+offset):(size(image,2)-offset);
%     for x = (1+offset):(size(image,2)-offset)
        gvm_tlImage(y,xs) = (gvm_filteredImage(y-grayvalueMatch_offset, xs-grayvalueMatch_offset) +...
                            gvm_filteredImage(y-grayvalueMatch_offset, xs) +...
                            gvm_filteredImage(y, xs-grayvalueMatch_offset))/3 -...
                            gvm_filteredImage(y, xs);
                       
        gvm_trImage(y,xs) = (gvm_filteredImage(y-grayvalueMatch_offset, xs+grayvalueMatch_offset) +...
                           gvm_filteredImage(y-grayvalueMatch_offset, xs) +...
                           gvm_filteredImage(y, xs+grayvalueMatch_offset))/3 -...
                           gvm_filteredImage(y, xs);
                       
        gvm_blImage(y,xs) = (gvm_filteredImage(y+grayvalueMatch_offset, xs-grayvalueMatch_offset) +...
                           gvm_filteredImage(y+grayvalueMatch_offset, xs) +...
                           gvm_filteredImage(y, xs-grayvalueMatch_offset))/3 -...
                           gvm_filteredImage(y, xs);
                       
        gvm_brImage(y,xs) = (gvm_filteredImage(y+grayvalueMatch_offset, xs+grayvalueMatch_offset) +...
                           gvm_filteredImage(y+grayvalueMatch_offset, xs) +...
                           gvm_filteredImage(y, xs+grayvalueMatch_offset))/3 -...
                           gvm_filteredImage(y, xs);
%     end
end
% toc
% gvm_tlImage = (gvm_tlImage / 255);
% gvm_trImage = (gvm_trImage / 255);
% gvm_blImage = (gvm_blImage / 255);
% gvm_brImage = (gvm_brImage / 255);

gvm_tlImage = (gvm_tlImage / max(gvm_tlImage(:)));
gvm_trImage = (gvm_trImage / max(gvm_trImage(:)));
gvm_blImage = (gvm_blImage / max(gvm_blImage(:)));
gvm_brImage = (gvm_brImage / max(gvm_brImage(:)));

% imshows(image, gvm_tlImage, gvm_trImage, gvm_blImage, gvm_brImage);
figure(1); imshow(image);
% figure(2); imshow(gvm_tlImage); title('gvm_tlImage');
% figure(3); imshow(gvm_trImage); title('gvm_trImage');
% figure(4); imshow(gvm_blImage); title('gvm_blImage');
figure(5); imshow(gvm_brImage); title('gvm_brImage');

harris_kernel = fspecial('gaussian',[harris_width 1], harris_sigma);
harris_filteredImage = cornermetric(image, 'Harris', 'FilterCoefficients', harris_kernel);
cp = harris_filteredImage - min(harris_filteredImage(:));
cp = cp / max(cp(:)) * 3;

corners = corner(image, 'Harris', 'FilterCoefficients', harris_kernel);

figure(6); 
hold off;
imshow(cp); title('harris');
hold on;
scatter(corners(:,1), corners(:,2));


for i = 1:length(initialCorners)
    limitsY = [initialCorners(2)+maxCornerDelta, initialCorners(2)-maxCornerDelta];
    limitsX = [initialCorners(1)+maxCornerDelta, initialCorners(1)-maxCornerDelta];
    
    maxValue = -Inf;
    maxInds = [-1, -1];
    for y = limitsY(1):limitsY(2)
        for x = limitsX(1):limitsX(2)
            if harris_filteredImage(y,x) > maxValue
                maxValue = harris_filteredImage(y,x);
                maxInds = [y,x];
            end
        end
    end
    
end