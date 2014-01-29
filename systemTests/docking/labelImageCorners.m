

% function corners = labelImageCorners(image)

function corners = labelImageCorners(im)

corners = [];

% while length(corners) ~= 2
% 
%     [corners, ~] = cpselect(image, image, 'Wait', true);
% 
%     if length(corners) ~= 2
%         disp('Must select two point on both images. The ones on the right can be anywhere, as they are ignored.');
%     end
% end

% scale = 3;
% halfImage = im(1:round(end/2), :);

scale = 2;

handle = figure(1);
% imshow(imresize(halfImage, size(halfImage)*scale));
imshow(imresize(im, size(im)*scale));
truesize(handle);

corners = zeros(2,0);

while size(corners,2) ~= 2
    [x,y,c] = ginput(1);
    if c == 115 % s
        disp(sprintf('Selected point at (%f,%f)', x/scale, y/scale));
        corners(:,end+1) = [x,y]/scale;
    elseif c == 104 % h
        disp('Position the mouse, and press the s key to select a point')
    end
end