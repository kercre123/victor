function detections = naiveBayes(posHist, negHist, img, blockSize)

img = im2double(img);

img = mean(img,3);

scoreLUT = log(posHist) - log(negHist);

numBins = size(posHist,1);

mid = blockSize/2;

numScales = 5;
scaleFactor = 1.5;

scoreImg = cell(1, numScales);

for i = 1:numScales
    binnedImg = floor((numBins-1)*img) + 1;
    
    scoreImg{i} = nlfilter(binnedImg, [blockSize blockSize], ...
        @(block)computeBlockScore(block, scoreLUT, mid));
    
    subplot(1,numScales,i)
    imshow(img)
    overlay_image(scoreImg{i}, 'r');
    
    img = max(0, min(1, imresize(img, 1/scaleFactor, 'lanczos3')));
    
end

keyboard

end

function score = computeBlockScore(block, scoreLUT, mid)

if all(block(:)>0)
    centerBin = block(mid,mid); % NOTE: not doing 2x2 average like in training
    
    edgeBins = get_border(block);
    
    index = edgeBins + (centerBin-1)*size(scoreLUT,1);
    
    score = min(scoreLUT(index));
else
    score = 0;
end

end