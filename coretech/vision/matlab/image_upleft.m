function img_upleft = image_upleft(img, up_step, left_step)
% if(ndims(img)>6)
%     warning('High-dimensional image, some dimensions may be lost!');
% end

if nargin==1
    up_step = 1;
end

if nargin<=2
    left_step = up_step;
end

% img_upleft = img([ones(1,step) 1:(end-step)],[ones(1,step) 1:(end-step)],:,:,:,:);
img_upleft = img([ones(1,up_step) 1:(end-up_step)],[ones(1,left_step) 1:(end-left_step)],:);

if ndims(img)>3
    img_upleft = reshape(img_upleft, size(img));
end
