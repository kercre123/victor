% function lucasKanade_init()

function lucasKanade_init(templateImage, templateQuad, numScales, useBlurring, estimateAffine)

assert(~useBlurring);

assert(size(templateQuad,1) == 4);

% Currently, the template quad must be a rectangle
minTemplateQuadX = min(templateQuad(:,1));
maxTemplateQuadX = max(templateQuad(:,1));
minTemplateQuadY = min(templateQuad(:,2));
maxTemplateQuadY = max(templateQuad(:,2));
assert(length(find(templateQuad(:,1)==minTemplateQuadX)) == 2);
assert(length(find(templateQuad(:,1)==maxTemplateQuadX)) == 2);
assert(length(find(templateQuad(:,2)==minTemplateQuadY)) == 2);
assert(length(find(templateQuad(:,2)==maxTemplateQuadY)) == 2);

if find(round(size(templateImage)/2) == size(templateImage)/2) ~= 2
    disp('Non even input image size');
    assert(false);
end

templateImage = double(templateImage);
if ndims(templateImage) == 3
    templateImage = rgb2gray(templateImage);
end

% originalTemplateSize = [maxTemplateQuadX - minTemplateQuadX + 1,...
%                         maxTemplateQuadY - minTemplateQuadY + 1];

A_translationOnly = cell(numScales,1);

if estimateAffine
    A_affine = cell(numScales,1);
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
            curTemplateImageRaw = templateImage;
        else
            curTemplateImageRaw = (prevTemplateImageRaw(1:2:end, 1:2:end) + prevTemplateImageRaw(2:2:end, 1:2:end) + prevTemplateImageRaw(1:2:end, 2:2:end) + prevTemplateImageRaw(2:2:end, 2:2:end)) / 4; %#ok<NODEF>
        end
    end
     
    curTemplateQuad = round(templateQuad / scale);
    minCurTemplateQuadX = min(curTemplateQuad(:,1));
    maxCurTemplateQuadX = max(curTemplateQuad(:,1));
    minCurTemplateQuadY = min(curTemplateQuad(:,2));
    maxCurTemplateQuadY = max(curTemplateQuad(:,2));
    
    curTemplateImage = curTemplateImageRaw(minCurTemplateQuadY:maxCurTemplateQuadY, minCurTemplateQuadX:maxCurTemplateQuadX);
    
    derivativeX = imfilter(curTemplateImage, scale*[-.5,0,.5]);
    derivativeY = imfilter(curTemplateImage, scale*[-.5,0,.5]');
    
    % Gaussian weighting function to give more weight to center of target
    % TODO?
    
    % Always estimate translation
    A_translationOnly{iScale} = [derivativeX(:) derivativeY(:)];
    
    if estimateAffine
        ys = scale * (0.5 + (minCurTemplateQuadY:maxCurTemplateQuadY));
        xs = scale * (0.5 + (minCurTemplateQuadX:maxCurTemplateQuadX));
        A_affine{iScale} = Inf * ones(length(ys)*length(xs), 6);
        iPixel = 1;
   
        for ix = 1:length(xs)
            x = xs(ix);
            for iy = 1:length(ys)
                y = ys(iy);
                curDerivativeX = derivativeX(iy,ix);
                curDerivativeY = derivativeY(iy,ix);
                
                A_affine{iScale}(iPixel,:) = [x*curDerivativeX, y*curDerivativeX, curDerivativeX, x*curDerivativeY, y*curDerivativeY, curDerivativeY];
                
                iPixel = iPixel + 1;
            end
        end
    end
    
    prevTemplateImageRaw = curTemplateImageRaw;
end % for iScale = 1:numScales

% keyboard



