% function shiftedImage = exhuastiveAlignment_shiftImage(image, dy, dx, borderWidth)

function shiftedImage = exhuastiveAlignment_shiftImage(image, dy, dx, borderWidth)
    shiftedImage = zeros(size(image), 'int16');
    
    if ndims(shiftedImage) == 2
        shiftedImage((1+borderWidth):(end-borderWidth), (1+borderWidth):(end-borderWidth)) = image(((1+borderWidth):(end-borderWidth))+dy, ((1+borderWidth):(end-borderWidth))+dx);
    elseif ndims(shiftedImage) == 3
        shiftedImage((1+borderWidth):(end-borderWidth), (1+borderWidth):(end-borderWidth), :) = image(((1+borderWidth):(end-borderWidth))+dy, ((1+borderWidth):(end-borderWidth))+dx, :);
    else
        assert(false);
    end
end