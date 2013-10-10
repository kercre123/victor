function binaryImage = computeBinaryCharacteristicScaleImage_lowMemory2(imageBlahBlah, numLevels)

DEBUG_DISPLAY = false;
% DEBUG_DISPLAY = true;

assert(size(img,3)==1, 'Image should be scalar-valued.');
[nrows,ncols] = size(img);
scaleImage = img;
DoG_max = zeros(nrows,ncols);

if nargout > 1 || nargout == 0
    whichScale = ones(nrows,ncols);
end

imgPyr = cell(1, numLevels+1);
imgPyr{1} = separable_filter(img, kernel);

for k = 2:numLevels+1

    blurred = separable_filter(imgPyr{k-1}, kernel);
    
    if computeDogAtFullSize
        DoG = abs( imresize(blurred,[nrows,ncols],'bilinear') - imresize(imgPyr{k-1},[nrows,ncols],'bilinear') );
    else
        DoG = abs(blurred - imgPyr{k-1});
        DoG = imresize(DoG, [nrows ncols], 'bilinear');
    end
    
    larger = DoG > DoG_max;
    if any(larger(:))
        DoG_max(larger) = DoG(larger);
        imgPyr_upsample = imresize(imgPyr{k-1}, [nrows ncols], 'bilinear');
        scaleImage(larger) = imgPyr_upsample(larger);
        if nargout > 1 || nargout == 0
            whichScale(larger) = k-1;
        end
    end
    
%     disp(sprintf('min:%f max:%f', min(DoG(:)), max(DoG(:))));

    if DEBUG_DISPLAY
%         figureHandle = figure(100+k); imshow(DoG(150:190,260:300)*5);
%         figureHandle = figure(100+k); subplot(2,2,1); imshow(imgPyr{k-1}); subplot(2,2,2); imshow(blurred); subplot(2,2,3); imshow(DoG*5); subplot(2,2,4); imshow(scaleImage);
        figureHandle = figure(100+k); subplot(2,4,1); imshow(imgPyr{k-1}); subplot(2,4,3); imshow(blurred); subplot(2,4,5); imshow(DoG*5); subplot(2,4,7); imshow(scaleImage);
        set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1]) 
    end
    
    imgPyr{k} = imresize_bilinear(blurred, floor(size(blurred)/2));
end

if nargout == 0
    subplot 131
    imshow(img)
    subplot 132
    imshow(scaleImage)
    subplot 133
    imagesc(whichScale), axis image

    clear scaleImage
end

end % FUNCTION computeCharacteristicScaleImage()