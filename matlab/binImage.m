function bins = binImage(img, numBins)

if size(img,3) > 1
    img = mean(img,3);
end

bins = round((numBins-1)*im2double(img)) + 1;

end
