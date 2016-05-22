% function image = runLengthDecode(compressed)

function image = runLengthDecode(compressed, imageSize)

image = zeros(imageSize, 'uint8')';

curPixel = 1;
for iByte = 1:length(compressed)
    byte = compressed(iByte);
    
    if byte > 127
        numPixels = uint32(byte - 128) + 1;
        curColor = 1;
    else
        numPixels = uint32(byte) + 1;
        curColor = 0;
    end
    
    image(curPixel:(curPixel+numPixels-1)) = curColor;
    
    curPixel = curPixel + numPixels;
end

image = image';