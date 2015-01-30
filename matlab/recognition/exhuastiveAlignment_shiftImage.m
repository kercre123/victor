% function shiftedImage = exhuastiveAlignment_shiftImage(image, dy, dx, maxOffset)

function shiftedImage = exhuastiveAlignment_shiftImage(image, dy, dx, maxOffset)
    shiftedImage = zeros(size(image), 'int16');
    
    if ndims(shiftedImage) == 2
        shiftedImage((1+maxOffset):(end-maxOffset), (1+maxOffset):(end-maxOffset)) = image(((1+maxOffset):(end-maxOffset))+dy, ((1+maxOffset):(end-maxOffset))+dx);
    elseif ndims(shiftedImage) == 3
        shiftedImage((1+maxOffset):(end-maxOffset), (1+maxOffset):(end-maxOffset), :) = image(((1+maxOffset):(end-maxOffset))+dy, ((1+maxOffset):(end-maxOffset))+dx, :);
    else
        assert(false);
    end
end