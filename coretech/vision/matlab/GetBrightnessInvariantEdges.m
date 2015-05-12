function mag = GetBrightnessInvariantEdges(probeValues)

probeValues = double(probeValues);

minVal = min(probeValues(:));
maxVal = max(probeValues(:));
probeValues = (probeValues - minVal) / (maxVal - minVal) + 1;

Ix = image_right(probeValues)./image_left(probeValues);
Ix(Ix>1) = 1./Ix(Ix>1);

subplot 221, imagesc(Ix), axis image

Iy = image_down(probeValues)./image_up(probeValues);
Iy(Iy>1) = 1./Iy(Iy>1);

subplot 222, imagesc(Iy), axis image

%subplot 223, imagesc(sqrt(Ix.^2 + Iy.^2)), axis image
subplot 223, imagesc(probeValues), axis image

mag = im2uint8(2*(min(Ix,Iy)-0.5));
subplot 224, imagesc(mag), axis image

