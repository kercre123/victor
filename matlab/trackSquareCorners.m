
function updatedCorners = trackSquareCorners(image, initialCorners, harris_width, harris_sigma, grayvalueMatch_width, grayvalueMatch_offset, maxCornerDelta)

assert(mod(grayvalueMatch_width,2) == 1);
assert(mod(harris_width,2) == 1);

gvm_kernel = ones(grayvalueMatch_width,grayvalueMatch_width);
gvm_kernel = gvm_kernel / sum(gvm_kernel(:));

gvm_filteredImage = imfilter(image, gvm_kernel);

gvm_ulImage = zeros(size(image));
offset = 5;
for y = (1+offset):(size(image,1)-offset)
    for x = (1+offset):(size(image,2)-offset)
        gvm_ulImage(y,x) = gvm_filteredImage(y-grayvalueMatch_offset, x-grayvalueMatch_offset)/3 +...
                           gvm_filteredImage(y-grayvalueMatch_offset, x)/3 +...
                           gvm_filteredImage(y, x-grayvalueMatch_offset)/3 -...
                           gvm_filteredImage(y, x);
    end
end
gvm_ulImage = abs(gvm_ulImage / 255);

harris_kernel = fspecial('gaussian',[harris_width 1], harris_sigma);
harris_filteredImage = cornermetric(image, 'Harris', 'FilterCoefficients', harris_kernel);
% cp = c - min(c(:));
% cp = cp / max(cp(:));

for i = 1:length(initialCorners)
    limitsY = [initialCorners(2)+maxCornerDelta, initialCorners(2)-maxCornerDelta];
    limitsX = [initialCorners(1)+maxCornerDelta, initialCorners(1)-maxCornerDelta];
    
    maxValue = -Inf;
    maxInds = [-1, -1];
    for y = limitsY(1):limitsY(2)
        for x = limitsX(1):limitsX(2)
            if harris_filteredImage(y,x) > maxValue
                maxValue = harris_filteredImage(y,x);
                maxInds = [y,x];
            end
        end
    end
    
end