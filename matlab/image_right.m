function img_right = image_right(img,step)
% if(ndims(img)>6)
%     warning('High-dimensional image, some dimensions may be lost!');
% end

if(nargin==1)
    step=1;
end

if(step < 0)
    img_right = image_left(img, -step);
else
    %img_right = img(:,[(step+1):end end*ones(1,step)],:,:,:,:);
    img_right = img(:,[(step+1):end end*ones(1,step)],:);
    if ndims(img)>3
        img_right = reshape(img_right, size(img));
    end
end
