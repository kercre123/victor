% function lucasKanade_init()

% templateImage = zeros(16,16); for y=1:size(templateImage,1) templateImage(y,:) = y*(1:size(templateImage,2)); end;
% newImage = warpProjective(templateImage, [cos(.1), -sin(.1), 0; sin(.1), cos(.1), 0; 0,0,1], size(templateImage));
% templateQuad = [5,4;10,4;10,8;5,8];
% numScales = 2;

% [A_translationOnly, A_affine, templateImagePyramid] = lucasKanade_init(templateImage, templateQuad, numScales, false, true)

function [A_translationOnly, A_affine, templateImagePyramid] = lucasKanade_init(templateImage, templateQuad, numScales, useBlurring, estimateAffine)

assert(~useBlurring);

% Currently, the template quad must be a rectangle
assert(isQuadARectangle(templateQuad));

if find(round(size(templateImage)/2) == size(templateImage)/2) ~= 2
    disp('Non even input image size');
    assert(false);
end

templateImage = double(templateImage);
if ndims(templateImage) == 3
    templateImage = rgb2gray(templateImage);
end

templateImagePyramid = cell(numScales,1);

A_translationOnly = cell(numScales,1);

if estimateAffine
    A_affine = cell(numScales,1);
else
    A_affine = [];
end

for iScale = 1:numScales
    scale = 2^(iScale-1);

    curSize = size(templateImage) / scale;

    if find(round(curSize) == curSize) ~= 2
        disp('Too many scales');
        assert(false);
    end

    if useBlurring
%         imgBlur = separable_filter(firstImg, gaussian_kernel(spacing/3));
       assert(false);
    else
        if iScale == 1
            curTemplateImage = templateImage;
        else
            assert(all(mod(size(prevTemplateImageRaw),2) == 0));
            curTemplateImage = (prevTemplateImageRaw(1:2:end, 1:2:end) + prevTemplateImageRaw(2:2:end, 1:2:end) + prevTemplateImageRaw(1:2:end, 2:2:end) + prevTemplateImageRaw(2:2:end, 2:2:end)) / 4; %#ok<NODEF>
        end
    end

    templateImagePyramid{iScale} = curTemplateImage;

    derivativeX = imfilter(curTemplateImage, [-.5,0,.5]/scale);
    derivativeY = imfilter(curTemplateImage, ([-.5,0,.5]/scale)');

    % Gaussian weighting function to give more weight to center of target
    % TODO?

    % Always estimate translation
    A_translationOnly{iScale} = zeros(size(derivativeX,1), size(derivativeX,2), 2);
    A_translationOnly{iScale}(:,:,1) = derivativeX;
    A_translationOnly{iScale}(:,:,2) = derivativeY;

    if estimateAffine
        ys = scale * (0.5 + (1:size(curTemplateImage,1)));
        xs = scale * (0.5 + (1:size(curTemplateImage,2)));
        A_affine{iScale} = Inf * ones([size(curTemplateImage,1), size(curTemplateImage,2), 6]);

        for ix = 1:length(xs)
            x = xs(ix);
            for iy = 1:length(ys)
                y = ys(iy);
                curDerivativeX = derivativeX(iy,ix);
                curDerivativeY = derivativeY(iy,ix);

                A_affine{iScale}(iy,ix,:) = [x*curDerivativeX, y*curDerivativeX, curDerivativeX, x*curDerivativeY, y*curDerivativeY, curDerivativeY];
            end
        end
    end

    prevTemplateImageRaw = curTemplateImage;
end % for iScale = 1:numScales

% keyboard
