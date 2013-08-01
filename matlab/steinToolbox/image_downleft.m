function img_downleft = image_downleft(img, down_step, left_step)
% if(ndims(img)>6)
%     warning('High-dimensional image, some dimensions may be lost!');
% end

if nargin==1
    down_step=1;
end

if nargin<=2
    left_step = down_step;
end

% img_downleft = img([(step+1):end end*ones(1,step)],[ones(1,step) 1:(end-step)],:,:,:,:);
img_downleft = img([(down_step+1):end end*ones(1,down_step)],[ones(1,left_step) 1:(end-left_step)],:);

if ndims(img)>3
    img_downleft = reshape(img_downleft, size(img));
end
