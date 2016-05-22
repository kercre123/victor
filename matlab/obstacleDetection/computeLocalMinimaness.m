function localMinimaness = computeLocalMinimaness(im, mask, windowSize)
    windowSizeFilter = ones(windowSize) / prod(windowSize);
    
%     mask(1:windowSize(2), :) = 0;
%     mask((end-windowSize(2)):end, :) = 0;
%     mask(1:windowSize(1), :) = 0;
%     mask((end-windowSize(1)):end, :) = 0;
    
    imDouble = double(im);
    
    imDouble(~mask) = nan;    
    imDouble(1,:) = nan;
    imDouble(end,:) = nan;
    imDouble(:,1) = nan;
    imDouble(:,end) = nan;
    
    dxNeg = imfilter(abs(imfilter(imDouble, [-0.5,0.5,0])), windowSizeFilter, 0);
    dxPos = imfilter(abs(imfilter(imDouble, [0,0.5,-0.5])), windowSizeFilter, 0);
    dyNeg = imfilter(abs(imfilter(imDouble, [-0.5,0.5,0]')), windowSizeFilter, 0);
    dyPos = imfilter(abs(imfilter(imDouble, [0,0.5,-0.5]')), windowSizeFilter, 0);
    
    localMinimaness = min(min(min(dxNeg, dxPos), dyNeg), dyPos);
end