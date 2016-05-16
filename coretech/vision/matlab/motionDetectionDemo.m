function motionDetectionDemo(img, h_axes, h_img, varargin)

persistent imgBuffer;

if length(imgBuffer) < 3
  imgBuffer{end+1} = img;
  return;
else
  imgBuffer(1:2) = imgBuffer(2:3);
  imgBuffer{3} = img;
  
  % diff 1 and 2
  diff12 = ratioHelper(imgBuffer{1}, imgBuffer{2}); %max(imabsdiff(imgBuffer{1},imgBuffer{2}),[],3);
  %diff12 = 1;
  
  % diff 2 and 3
  diff23 = ratioHelper(imgBuffer{2}, imgBuffer{3}); %max(imabsdiff(imgBuffer{2},imgBuffer{3}),[],3);
  
  % sum the two diffs
  diffTotal = min(diff12,diff23);
  
%   set(h_img, 'CData', [diff12 diff23 diffTotal]);
% set(h_axes, 'CLim', [1 2]);

[xgrid,ygrid] = meshgrid(1:size(img,2), 1:size(img,1));
diff12(diff12<1.5) = 0;
xcen(1) = sum(column(diff12.*xgrid)) / sum(diff12(:));
ycen(1) = sum(column(diff12.*ygrid)) / sum(diff12(:));
diff23(diff23<1.5) = 0;
xcen(2) = sum(column(diff23.*xgrid)) / sum(diff23(:));
ycen(2) = sum(column(diff23.*ygrid)) / sum(diff23(:));
diffTotal(diffTotal<1.5) = 0;
xcen(3) = sum(column(diffTotal.*xgrid)) / sum(diffTotal(:));
ycen(3) = sum(column(diffTotal.*ygrid)) / sum(diffTotal(:));

dispHelper = @(img,d)repmat(max(0,min(1,d-1)),[1 1 3]).*im2double(img);
set(h_img, 'CData', [dispHelper(imgBuffer{2},diff12) dispHelper(imgBuffer{2},diff23) dispHelper(imgBuffer{2},diffTotal)]);


    hold(h_axes, 'on');
    delete(findobj(h_axes, 'Tag', 'MotionCentroid'))
    plot(xcen+size(img,2)*[0 1 2], ycen, ...
      'rx', 'LineWidth', 2, 'Parent', h_axes, 'Tag', 'MotionCentroid');
    plot(xcen(1)+[2 2]*size(img,2), ycen(1:2), 'gx', 'Tag', 'MotionCentroid');
    
    
  stats = regionprops(diffTotal > 1.5, {'Area', 'Centroid', 'PixelIdxList'});
  
  [maxArea, largestIndex] = max([stats.Area]);
  
  if maxArea > .05^2 * size(img,1) * size(img,2)
    
%     % Get a model of the appearance (color) in the region
%     img_cols = reshape(double(img), [], 3);
%     mu = mean(img_cols(stats(largestIndex).PixelIdxList,:), 1);
%     sigma = var(img_cols(stats(largestIndex).PixelIdxList,:), 0, 1);
%     
%     p = bsxfun(@minus, img_cols, mu);
%     p = bsxfun(@rdivide, p.^2, 2*sigma);
%     p = reshape(min(exp(-p), [], 2), size(img,1), size(img,2));
    

%     temp = zeros(size(img,1), size(img,2));
%     temp(stats(largestIndex).PixelIdxList) = 255;%diffTotal(stats(largestIndex).PixelIdxList);;
%     set(h_img, 'CData', [img repmat(temp, [1 1 3])]);
%     hold(h_axes, 'on');
%     delete(findobj(h_axes, 'Tag', 'MotionCentroid'))
%     plot(stats(largestIndex).Centroid(1)+[0 size(img,2)], stats(largestIndex).Centroid(2)*[1 1], ...
%       'rx', 'LineWidth', 2, 'Parent', h_axes, 'Tag', 'MotionCentroid');
   end
  
end

end

function R = ratioHelper(img1,img2)

A = max(img1,img2);
B = min(img1,img2);

R = max(double(A) ./ double(max(1,B)), [], 3);

end