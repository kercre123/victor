% function compositeVideo = test_createCompositeSampleVideo(filenamePatterns)

function compositeVideo = test_createCompositeSampleVideo(filenamePatterns)
    numImages = Inf;
    for iFilenamePattern = 1:length(filenamePatterns)
        imageNumbers = filenamePatterns{iFilenamePattern}{2}(1):filenamePatterns{iFilenamePattern}{2}(2);
        numImages = min(numImages, length(imageNumbers));
    end
    
    compositeVideo = cell(numImages, 1);
    for iImage = 1:numImages
        images = cell(length(filenamePatterns), 1);
        for iFilenamePattern = 1:length(filenamePatterns)
            imageNumbers = filenamePatterns{iFilenamePattern}{2}(1):filenamePatterns{iFilenamePattern}{2}(2);
            curFilename = sprintf(filenamePatterns{iFilenamePattern}{1}, imageNumbers(iImage));
            images{iFilenamePattern} = imread(curFilename);
        end
        
        compositeVideo{iImage} = drawCollage(images);
    end
end % function test_createCompositeSampleVideo()


