function img_left = image_left(img, step)
% if(ndims(img)>6)
%     warning('High-dimensional image, some dimensions may be lost!');
% end

if(nargin==1)
    step=1;
end

if(step < 0)
    img_left = image_right(img, -step);
else
    % img_left = img(:,[ones(1,step) 1:(end-step)],:,:,:,:);
    img_left = img(:,[ones(1,step) 1:(end-step)],:);
    
    if ndims(img)>3
        img_left = reshape(img_left, size(img));
    end
end
