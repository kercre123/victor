function img_upright = image_upright(img, up_step, right_step)
% if(ndims(img)>6)
%     warning('High-dimensional image, some dimensions may be lost!');
% end

if nargin==1
    up_step=1;
end

if nargin<=2
    right_step = up_step;
end

% img_upright = img([ones(1,step) 1:(end-step)],[(step+1):end end*ones(1,step)],:,:,:,:); 
img_upright = img([ones(1,up_step) 1:(end-up_step)],[(right_step+1):end end*ones(1,right_step)],:,:,:,:); 

if ndims(img)>3
    img_upright = reshape(img_upright, size(img));
end
