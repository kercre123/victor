% function samples = loadOpencvTrainingSamples(filename)

% samples = loadOpencvTrainingSamples('~/Documents/Anki/products-cozmo-large-files/faceDetection/fddbFerretWebfacesFaces_20x24.vec', 20, 24);

function samples = loadOpencvTrainingSamples(filename, sampleWidth, sampleHeight)
    
    fileId = fopen(filename, 'r');
    
    count = fread(fileId, [1,1], 'int32');
    vecsize = fread(fileId, [1,1], 'int32');
    tmp = fread(fileId, [1,1], 'int32');
    
    assert(vecsize == (sampleWidth*sampleHeight));
    
    data = fread(fileId, Inf, 'uint8=>uint8');
        
    fclose(fileId);
    
%     assert((floor(length(data)/480/2)-2) == count);
    
    samples = cell(count,1);
    
    ci = 2;
    for iSample = 1:count
        endIndex = ci + sampleWidth*sampleHeight*2 - 2;
        
        samples{iSample} = reshape(data(ci:2:endIndex), [sampleWidth,sampleHeight])';
        
        ci = ci + sampleWidth*sampleHeight*2 + 1;
    end % for iSample = 1:count
    

