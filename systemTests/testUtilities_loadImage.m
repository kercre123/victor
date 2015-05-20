function [image, fileInfo] = testUtilities_loadImage(filename, imageCompression, preprocessingFunction, basicStatsFilename, varargin)
    image = imread(filename);
    
    if ~isempty(preprocessingFunction)
        image = preprocessingFunction(image);
    end
    
    pixelShift = [0, 0];
    
    parseVarargin(varargin{:});
    
    if min(pixelShift == [0,0]) == 0
        if pixelShift(1) > 0
            image(:, (1+pixelShift(1)):end, :) = image(:, 1:(end-pixelShift(1)), :);
        else
            image(:, 1:(end+pixelShift(1)), :) = image(:, (1-pixelShift(1)):end, :);
        end
        
        if pixelShift(2) > 0
            image((1+pixelShift(1)):end, :, :) = image(1:(end-pixelShift(1)), :, :);
        else
            image(1:(end+pixelShift(2)), :, :) = image((1-pixelShift(2)):end, :, :);
        end
    end % if min(pixelShift == [0,0]) == 0
    
    if strcmp(imageCompression{1}, 'none') || strcmp(imageCompression{1}, 'png')
        fileInfo = dir(filename);
    elseif strcmp(imageCompression{1}, 'jpg') || strcmp(imageCompression{1}, 'jpeg')
        tmpFilename = [basicStatsFilename, 'tmpImage.jpg'];
        
        % Note: Matlab's jpeg writer is not as good as ImageMagick's at low bitrates
        imwrite(image, tmpFilename, 'jpg', 'Quality', imageCompression{2});
        
        image = imread(tmpFilename);
        
        fileInfo = dir(tmpFilename);
    elseif strcmp(imageCompression{1}, 'nathanJpg') || strcmp(imageCompression{1}, 'nathanJpeg')
        nathanJpegExecutableFilename = '~/Documents/Anki/products-cozmo/systemTests/enjpeg-test'; % If this doesn't exist, compile it with: gcc enjpeg.cpp enjpeg-test.cpp -o enjpeg-test
        
        tmpBmpFilename = [basicStatsFilename, 'tmpImage.bmp'];
        tmpRawFilename = [basicStatsFilename, 'tmpImage.gray'];
        tmpJpgFilename = [basicStatsFilename, 'tmpImage.jpg'];
        
        imwrite(image, tmpBmpFilename, 'bmp');
        
        system(sprintf('/usr/local/bin/convert %s %s', tmpBmpFilename, tmpRawFilename));
        
        quality = imageCompression{2};
        if quality < 1
            quality = 1;
        elseif quality > 100
            quality = 100;
        end
        
        system(sprintf('%s %s %s %dx%d %d', nathanJpegExecutableFilename, tmpRawFilename, tmpJpgFilename, size(image,2), size(image,1), quality));
        
        image = imread(tmpJpgFilename);
        
        fileInfo = dir(tmpJpgFilename);
    else
        assert(false);
    end
end % runTests_loadImage()