function image = probeTree2_probeValuesToImage(probeValues, imageIndex)
    numProbes = length(probeValues);
    probeImageWidth = sqrt(numProbes);
    
    image = zeros(numProbes, 1, 'uint8');
    
    for i = 1:numProbes
        image(i) = probeValues{i}(imageIndex);
    end
    
    image = reshape(image, [probeImageWidth, probeImageWidth]);