function img_dn = image_down(img,step)
% if(ndims(img)>6)
%     warning('High-dimensional image, some dimensions may be lost!');
% end

if(nargin==1)
    step=1;
end

if(step < 0)
    img_dn = image_up(img, -step);
else
    %     img_dn = img([(1+step):end end*ones(1,step)],:,:,:,:,:);
    img_dn = img([(1+step):end end*ones(1,step)],:,:);
    
    if ndims(img)>3
        img_dn = reshape(img_dn, size(img));
    end
end
