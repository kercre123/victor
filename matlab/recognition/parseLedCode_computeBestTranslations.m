
function [bestDys, bestDxs, offsetImageScale] = parseLedCode_computeBestTranslations(images, offsetImageSize, maxOffset, validRegion, twoPass)
    middleImageIndex = ceil(size(images,4) / 2);
    smallImages = imresize(images, offsetImageSize);
    
    offsetImageScale = offsetImageSize(1) / size(images,1);
    
    bestDys = zeros(size(images,4), 1);
    bestDxs = zeros(size(images,4), 1);
    for iImage = 1:size(images,4)
        
        if iImage == middleImageIndex
            bestDy = 0;
            bestDx = 0;
        else
            % First, find the best for the small image
            scaledMaxOffset = round(offsetImageScale*maxOffset);
            [coarseBestDy, coarseBestDx, ~, ~, ~] = exhaustiveAlignment(smallImages(:,:,:,middleImageIndex), smallImages(:,:,:,iImage), (-scaledMaxOffset):scaledMaxOffset, (-scaledMaxOffset):scaledMaxOffset, scaledMaxOffset, round(offsetImageScale*validRegion));
            coarseBestDy = (1/offsetImageScale)*coarseBestDy(1);
            coarseBestDx = (1/offsetImageScale)*coarseBestDx(1);
            
            if twoPass
                % Next, use the full size image to compute the one-pixel offset
                fineOffsetsY = (coarseBestDy-ceil(0.5/offsetImageScale)):(coarseBestDy+ceil(0.5/offsetImageScale));
                fineOffsetsX = (coarseBestDx-ceil(0.5/offsetImageScale)):(coarseBestDx+ceil(0.5/offsetImageScale));
                [bestDy, bestDx, ~, ~, ~] = exhaustiveAlignment(images(:,:,:,middleImageIndex), images(:,:,:,iImage), fineOffsetsY, fineOffsetsX, (1/offsetImageScale)*maxOffset, round(offsetImageScale*validRegion));
            else
                bestDy = coarseBestDy;
                bestDx = coarseBestDx;
            end
        end
        
        %         bestDys(iImage) = (1/offsetImageScale)*bestDy(1);
        %         bestDxs(iImage) = (1/offsetImageScale)*bestDx(1);
        bestDys(iImage) = bestDy(1);
        bestDxs(iImage) = bestDx(1);
    end
end % function parseLedCode_computeBestTranslations()
