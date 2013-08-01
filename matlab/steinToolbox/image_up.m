function img_up = image_up(img,step)
% if(ndims(img)>6)
%     warning('High-dimensional image, some dimensions may be lost!');
% end

if(nargin==1)
    step=1;
end

if(step <0 )
    img_up = image_down(img, -step);
else
    %img_up = img([ones(1,step) 1:end-step],:,:,:,:,:);
    img_up = img([ones(1,step) 1:end-step],:,:);
    if ndims(img)>3
        img_up = reshape(img_up, size(img));
    end
end
