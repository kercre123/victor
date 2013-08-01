function img_downright = image_downright(img, down_step, right_step)
% if(ndims(img)>6)
%     warning('High-dimensional image, some dimensions may be lost!');
% end

if nargin==1
    down_step=1;
end

if nargin<=2
    right_step = down_step;
end

% img_downright = img([(step+1):end end*ones(1,step)],[(step+1):end end*ones(1,step)],:,:,:,:); 
img_downright = img([(down_step+1):end end*ones(1,down_step)],[(right_step+1):end end*ones(1,right_step)],:); 

if ndims(img)>3
    img_downright = reshape(img_downright, size(img));
end

% nrows = size(img,1);
% ncols = size(img,2);
% index = {[(step+1):nrows nrows*ones(1,step)],[(step+1):ncols ncols*ones(1,step)]};
% N = ndims(img);
% if N>2
%     index = [index cellstr(repmat(':',[N-2 1]))'];
% end
% 
% img_downright = img(index{:});