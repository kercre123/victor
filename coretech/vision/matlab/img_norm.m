function [img_new, min_vals, max_vals] = img_norm(img, high, low, link_channels)
%
% [img_new, mins, maxes] = img_norm(img, high, low, <link_channels>)
%
% Normalizes an image so that its values range between 'low' and 'high'.
% 'low' defaults to 0. 'high' defaults to 1.
%
% If image is 3D (color or vector-valued image, or grayscale image sequence), 
% each channel is normalized separately.  Similarly, if image is 4D (color
% or vector-valued image sequence), each frame and channel are normalized
% separately.  Images with greater than four dimensions are not supported.
% If, however, link_channels is true, the min and max values for
% normalization will be computed from all channels simultaneously (but
% still separately for each frame in the 4D case).
%
% 'min_vals' and 'max_vals' will be the minimum and maximum values for each
% frame/channel of the input image.  Thus, the following call would "undo"
% the normalization:
%
%   img = img_norm(img_new, max_vals, min_vals)
%

if nargin < 4 || isempty(link_channels)
    link_channels = false;
end

if ndims(img)>4
    error('Input image must be no more than 4-dimensional!');
end

img = double(img);

if nargout > 1
    min_vals = zeros(size(img,3), size(img,4));
    max_vals = zeros(size(img,3), size(img,4));
end

img_new = zeros(size(img));

if link_channels
    for j = 1:size(img,4)
        min_val = min( column(img(:,:,:,j)) );
        max_val = max( column(img(:,:,:,j)) );
        
        if min_val ~= max_val
            img_new(:,:,:,j) = (img(:,:,:,j)-min_val)/(max_val-min_val);
        else
            img_new(:,:,:,j) = img(:,:,:,j);
        end
        
        if nargout > 1
            min_vals(1,j) = min_val;
            max_vals(1,j) = max_val;
        end
    end
else
    for i=1:size(img,3)
        for j=1:size(img,4)
            min_val = min( column(img(:,:,i,j)) );
            max_val = max( column(img(:,:,i,j)) );
            if min_val ~= max_val
                img_new(:,:,i,j) = (img(:,:,i,j) - min_val)/(max_val - min_val);
            else
                %             if size(img,4)>1
                %                 warning('Min and max value the same (%f) at channel %d of frame %d.  No change made.', min_val, i, j);
                %             elseif size(img,3)>1
                %                 warning('Min and max value the same (%f) at level %d.  No change made.', min_val, i);
                %             end
                img_new(:,:,i,j) = img(:,:,i,j);
            end

            if nargout > 1
                min_vals(i,j) = min_val;
                max_vals(i,j) = max_val;
            end
        end
    end
end


if nargin>1 && ~isempty(low) && ~isempty(high) 
    
    if nargin==3 && size(low,1)==size(img,3) && size(low,2)==size(img,4) && ...
            size(high,1)==size(img,3) && size(high,2)==size(img,4)
        
        if all(high(:) < low(:))
            temp = low;
            low = high;
            high = temp;
        end
        
        if link_channels 
            for j=1:size(img,4)
                img_new(:,:,:,j) = (high(1,j)-low(1,j))*img_new(:,:,:,j) + low(1,j);
            end
        else
            for i=1:size(img,3)
                for j=1:size(img,4)
                    img_new(:,:,i,j) = (high(i,j)-low(i,j))*img_new(:,:,i,j) + low(i,j);
                end
            end
        end
        
    else
        
        if nargin<3
            low = 0;
            if nargin<2
                high = 1;
            end
        end

        if high < low
            temp = low;
            low = high;
            high = temp;
        end

        img_new = (high-low)*img_new + low;
    end
end