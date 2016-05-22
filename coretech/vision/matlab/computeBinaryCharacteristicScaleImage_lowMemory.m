function binaryImage = computeBinaryCharacteristicScaleImage_lowMemory(imageBlahBlah, numLevels)

DEBUG_DISPLAY = false;
% DEBUG_DISPLAY = true;

assert(numLevels <= 8);

assert(size(imageBlahBlah,3)==1, 'Image should be scalar-valued.');

imageBlahBlah = im2uint8(imageBlahBlah); % UQ8.0

[nrows,ncols] = size(imageBlahBlah);

% scaleImage = uint32(image)*(2^8); % UQ16.16
% DoG_max = zeros(nrows,ncols,'uint32'); % UQ16.16
% imagePyramid = cell(1, numLevels+1);

binaryImage = zeros(size(imageBlahBlah));

scaleFactors = int32(2.^[0:(numLevels)]); %#ok<NBRAK>

combinedWindows = cell(numLevels+1, 1);
originalWindows = cell(numLevels+1, 1);
curLinesToSum = zeros(numLevels+1, 1);
maxLinesToSum = zeros(numLevels+1, 1);
endIndex = ones(numLevels+1, 1);
curCombinedIndex = ones(numLevels+1, 1);

for pyramidLevel = 1:(numLevels+1)
    curWidth = ncols / (2^(pyramidLevel-1));
    originalWindows{pyramidLevel} = zeros(5, curWidth, 'int32');
    maxLinesToSum(pyramidLevel) = 2^(pyramidLevel-1);
    
    combinedWindows{pyramidLevel} = zeros(nrows / (2^(pyramidLevel-1)), curWidth, 'uint8');
end

for y = 0:(nrows-1)
    newLine = int32(imageBlahBlah(y+1, :));
    
    curLinesToSum(:) = curLinesToSum(:) + 1;
    
    % If it's time to scroll any of the pyramid levels, do so
    % TODO: do the computation for the scale image
    for pyramidLevel = 1:(numLevels+1)
        if curLinesToSum(pyramidLevel) > maxLinesToSum(pyramidLevel)
            % The last line of the originalWindow is a sum. Scale the sum
            % to be a mean.
            originalWindows{pyramidLevel}(endIndex(pyramidLevel), :) = originalWindows{pyramidLevel}(endIndex(pyramidLevel), :) / ((2^(pyramidLevel-1))^2);
            
            combinedWindows{pyramidLevel}(curCombinedIndex(pyramidLevel), :) = uint8(originalWindows{pyramidLevel}(endIndex(pyramidLevel), :));
            curCombinedIndex(pyramidLevel) = curCombinedIndex(pyramidLevel) + 1;
            
            
            % If we've filled up this window, then shift the rows up before
            % adding the new one
            if endIndex(pyramidLevel) == 5
                originalWindows{pyramidLevel}(1:(end-1), :) = originalWindows{pyramidLevel}(2:end, :);
                originalWindows{pyramidLevel}(end, :) = 0;
            else
                endIndex(pyramidLevel) = endIndex(pyramidLevel) + 1;
            end
            
%             curLinesToSum(pyramidLevel) = curLinesToSum(pyramidLevel) - scaleFactors(pyramidLevel);
            curLinesToSum(pyramidLevel) = 1;            
        end
    end

    % Add the current line to the window for each level of the pyramid
    for pyramidLevel = 1:(numLevels+1)
        curDivisor = (2^(pyramidLevel-1));
        curWidth = ncols / curDivisor;
        summedNewLine = zeros(1, curWidth, 'int32');
        
        for iSum = 0:(curDivisor-1)
            summedNewLine = summedNewLine + newLine(1, (1+iSum):curDivisor:end);
        end
        
%         keyboard
        
        originalWindows{pyramidLevel}(endIndex(pyramidLevel), :) = originalWindows{pyramidLevel}(endIndex(pyramidLevel), :) + summedNewLine;
%         figure(pyramidLevel); imshow(imresize(double(originalWindows{pyramidLevel})/255, [5*25, curWidth], 'nearest'));
    end
    
    if mod(y, 25) == 0
        for pyramidLevel = 1:(numLevels+1)
            figure(4 + pyramidLevel); imshow(combinedWindows{pyramidLevel});
        end
        keyboard
    end
end % for y = 1:size(image,1)


% % keyboard
% if filterWithMex
%     imagePyramid{1} = mexBinomialFilter(image);
% else
%     imagePyramid{1} = binomialFilter_loopsAndFixedPoint(image);
% end
% 
% for pyramidLevel = 2:numLevels+1
%     curPyramidLevel = imagePyramid{pyramidLevel-1}; %UQ8.0
%     if filterWithMex
%         curPyramidLevelBlurred = mexBinomialFilter(curPyramidLevel); %UQ8.0
%     else
%         curPyramidLevelBlurred = binomialFilter_loopsAndFixedPoint(curPyramidLevel); %UQ8.0
%     end
% 
%     if DEBUG_DISPLAY
%         if computeDogAtFullSize
%             DoGorig = abs( imresize(double(curPyramidLevelBlurred),[nrows,ncols],'bilinear') - imresize(double(curPyramidLevel),[nrows,ncols],'bilinear') );
%         else
%             DoGorig = abs(double(curPyramidLevelBlurred) - double(curPyramidLevel));
%             DoGorig = imresize(DoGorig, [nrows ncols], 'bilinear');
%         end
%     end
%     
%     largeDoG = zeros([nrows,ncols],'uint32'); % UQ16.16
%     
%     if pyramidLevel == 2
%         for y = 1:nrows
%             for x = 1:ncols
%                 DoG = uint32(abs(int32(curPyramidLevelBlurred(y,x)) - int32(curPyramidLevel(y,x)))) * (2^16); % abs(UQ8.0 - UQ8.0) -> UQ16.16;
%                 largeDoG(y,x) = DoG;
% 
%                 % The first level is always correct
%                 DoG_max(y,x) = DoG;
%                 scaleImage(y,x) = uint32(curPyramidLevel(y,x)) * (2^16); % UQ8.0 -> UQ16.16
% 
%                 % The first level is not always correct
% %                 if DoG > DoG_max(y,x)
% %                     DoG_max(y,x) = DoG;
% %                     scaleImage(y,x) = uint32(curPyramidLevel(y,x)) * (2^16); % UQ8.0 -> UQ16.16
% %                 end
%             end
%         end
% %         keyboard
%     elseif pyramidLevel <= 5 % if pyramidLevel == 2
%         largeYIndexRange = int32(1 + [scaleFactors(pyramidLevel-1)/2, nrows-scaleFactors(pyramidLevel-1)/2-1]); % SQ31.0
%         largeXIndexRange = int32(1 + [scaleFactors(pyramidLevel-1)/2, ncols-scaleFactors(pyramidLevel-1)/2-1]); % SQ31.0
% 
%         alphas = int32((2^8)*(.5 + double(0:(scaleFactors(pyramidLevel-1)-1)))); % SQ23.8
%         maxAlpha = int32((2^8)*scaleFactors(pyramidLevel-1)); % SQ31.0 -> SQ23.8
% 
%         largeY = largeYIndexRange(1);
%         for smallY = 1:(size(curPyramidLevel,1)-1)
%             for iAlphaY = 1:length(alphas)
%                 alphaY = alphas(iAlphaY);
%                 alphaYinverse = maxAlpha - alphas(iAlphaY);
% 
%                 largeX = largeXIndexRange(1);
% 
%                 for smallX = 1:(size(curPyramidLevel,2)-1)
%                     if computeDogAtFullSize
%                         curPyramidLevel_00 = int32(curPyramidLevel(smallY, smallX)); % SQ31.0
%                         curPyramidLevel_01 = int32(curPyramidLevel(smallY, smallX+1)); % SQ31.0
%                         curPyramidLevel_10 = int32(curPyramidLevel(smallY+1, smallX)); % SQ31.0
%                         curPyramidLevel_11 = int32(curPyramidLevel(smallY+1, smallX+1)); % SQ31.0
% 
%                         curPyramidLevelBlurred_00 = int32(curPyramidLevelBlurred(smallY, smallX)); % SQ31.0
%                         curPyramidLevelBlurred_01 = int32(curPyramidLevelBlurred(smallY, smallX+1)); % SQ31.0
%                         curPyramidLevelBlurred_10 = int32(curPyramidLevelBlurred(smallY+1, smallX)); % SQ31.0
%                         curPyramidLevelBlurred_11 = int32(curPyramidLevelBlurred(smallY+1, smallX+1)); % SQ31.0
%                     else
%                         curPyramidLevel_00 = int32(curPyramidLevel(smallY, smallX)); % SQ31.0
%                         curPyramidLevel_01 = int32(curPyramidLevel(smallY, smallX+1)); % SQ31.0
%                         curPyramidLevel_10 = int32(curPyramidLevel(smallY+1, smallX)); % SQ31.0
%                         curPyramidLevel_11 = int32(curPyramidLevel(smallY+1, smallX+1)); % SQ31.0
%                         
%                         DoG_00 = abs(int32(curPyramidLevel(smallY,   smallX))   - int32(curPyramidLevelBlurred(smallY,   smallX))); % SQ31.0
%                         DoG_01 = abs(int32(curPyramidLevel(smallY,   smallX+1)) - int32(curPyramidLevelBlurred(smallY,   smallX+1))); % SQ31.0
%                         DoG_10 = abs(int32(curPyramidLevel(smallY+1, smallX))   - int32(curPyramidLevelBlurred(smallY+1, smallX))); % SQ31.0
%                         DoG_11 = abs(int32(curPyramidLevel(smallY+1, smallX+1)) - int32(curPyramidLevelBlurred(smallY+1, smallX+1))); % SQ31.0
%                     end
% 
%                     for iAlphaX = 1:length(alphas)
%                         alphaX = alphas(iAlphaX);
%                         alphaXinverse = maxAlpha - alphas(iAlphaX);
% 
%                         if computeDogAtFullSize
%                             curPyramidLevelInterpolated = interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse); % SQ15.16
%                             curPyramidLevelBlurredInterpolated = interpolate2d(curPyramidLevelBlurred_00, curPyramidLevelBlurred_01, curPyramidLevelBlurred_10, curPyramidLevelBlurred_11, alphaY, alphaYinverse, alphaX, alphaXinverse); % SQ15.16
%                             DoG = uint32( abs(curPyramidLevelInterpolated - curPyramidLevelBlurredInterpolated) / (maxAlpha^2/(2^16)) ); % (SQ?.? - SQ?.?) -> UQ16.16
%                         else
%                             DoG = uint32( interpolate2d(DoG_00, DoG_01, DoG_10, DoG_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^16)) ); % SQ?.? -> UQ16.16
%                         end
%                         
%                         largeDoG(largeY,largeX) = DoG;
%                         
% %                         if largeY == 219 && largeX == 334
% %                             disp(sprintf('DoGorig(%d,%d) = %f', largeY, largeX, DoGorig(largeY,largeX)));
% %                             disp(sprintf('DoG(%d,%d) = %f', largeY, largeX, double(DoG)/(2^16)));
% %                             keyboard
% %                         end
%                         
%                         if DoG > DoG_max(largeY,largeX)
%                             DoG_max(largeY,largeX) = DoG;
%                             curPyramidLevelInterpolated = uint32( interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^16)) );  % SQ?.? -> UQ16.16
%                             scaleImage(largeY,largeX) = curPyramidLevelInterpolated;
%                         end
%                         
%                         largeX = largeX + 1;
%                     end % for iAlphaX = 1:length(alphas)
%                 end % for smallX = 1:(size(curPyramidLevel,2)-1)
% 
%                 largeY = largeY + 1;
%             end % for iAlphaY = 1:length(alphas)
%         end % for smallY = 1:(size(curPyramidLevel,1)-1)
%     else % if pyramidLevel == 2 ... elseif pyramidLevel <= 5
%         largeYIndexRange = int32(1 + [scaleFactors(pyramidLevel-1)/2, nrows-scaleFactors(pyramidLevel-1)/2-1]); % SQ31.0
%         largeXIndexRange = int32(1 + [scaleFactors(pyramidLevel-1)/2, ncols-scaleFactors(pyramidLevel-1)/2-1]); % SQ31.0
% 
%         alphas = int64((2^8)*(.5 + double(0:(scaleFactors(pyramidLevel-1)-1)))); % SQ55.8
%         maxAlpha = int64((2^8)*scaleFactors(pyramidLevel-1)); % SQ31.0 -> SQ55.8
% 
%         largeY = largeYIndexRange(1);
%         for smallY = 1:(size(curPyramidLevel,1)-1)
%             for iAlphaY = 1:length(alphas)
%                 alphaY = alphas(iAlphaY);
%                 alphaYinverse = maxAlpha - alphas(iAlphaY);
% 
%                 largeX = largeXIndexRange(1);
% 
%                 for smallX = 1:(size(curPyramidLevel,2)-1)
%                     if computeDogAtFullSize
%                         curPyramidLevel_00 = int64(curPyramidLevel(smallY, smallX)); % SQ61.0
%                         curPyramidLevel_01 = int64(curPyramidLevel(smallY, smallX+1)); % SQ61.0
%                         curPyramidLevel_10 = int64(curPyramidLevel(smallY+1, smallX)); % SQ61.0
%                         curPyramidLevel_11 = int64(curPyramidLevel(smallY+1, smallX+1)); % SQ61.0
% 
%                         curPyramidLevelBlurred_00 = int64(curPyramidLevelBlurred(smallY, smallX)); % SQ61.0
%                         curPyramidLevelBlurred_01 = int64(curPyramidLevelBlurred(smallY, smallX+1)); % SQ61.0
%                         curPyramidLevelBlurred_10 = int64(curPyramidLevelBlurred(smallY+1, smallX)); % SQ61.0
%                         curPyramidLevelBlurred_11 = int64(curPyramidLevelBlurred(smallY+1, smallX+1)); % SQ61.0
%                     else
%                         curPyramidLevel_00 = int64(curPyramidLevel(smallY, smallX)); % SQ61.0
%                         curPyramidLevel_01 = int64(curPyramidLevel(smallY, smallX+1)); % SQ61.0
%                         curPyramidLevel_10 = int64(curPyramidLevel(smallY+1, smallX)); % SQ61.0
%                         curPyramidLevel_11 = int64(curPyramidLevel(smallY+1, smallX+1)); % SQ61.0
%                         
%                         DoG_00 = abs(int64(curPyramidLevel(smallY,   smallX))   - int64(curPyramidLevelBlurred(smallY,   smallX))); % SQ61.0
%                         DoG_01 = abs(int64(curPyramidLevel(smallY,   smallX+1)) - int64(curPyramidLevelBlurred(smallY,   smallX+1))); % SQ61.0
%                         DoG_10 = abs(int64(curPyramidLevel(smallY+1, smallX))   - int64(curPyramidLevelBlurred(smallY+1, smallX))); % SQ61.0
%                         DoG_11 = abs(int64(curPyramidLevel(smallY+1, smallX+1)) - int64(curPyramidLevelBlurred(smallY+1, smallX+1))); % SQ61.0
%                     end
% 
%                     for iAlphaX = 1:length(alphas)
%                         alphaX = alphas(iAlphaX);
%                         alphaXinverse = maxAlpha - alphas(iAlphaX);
% 
%                         if computeDogAtFullSize
%                             curPyramidLevelInterpolated = interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse); % SQ15.16
%                             curPyramidLevelBlurredInterpolated = interpolate2d(curPyramidLevelBlurred_00, curPyramidLevelBlurred_01, curPyramidLevelBlurred_10, curPyramidLevelBlurred_11, alphaY, alphaYinverse, alphaX, alphaXinverse); % SQ15.16
%                             DoG = uint32( abs(curPyramidLevelInterpolated - curPyramidLevelBlurredInterpolated) / (maxAlpha^2/(2^16)) ); % (SQ?.? - SQ?.?) -> UQ16.16
%                         else
%                             DoG = uint32( interpolate2d(DoG_00, DoG_01, DoG_10, DoG_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^16)) ); % SQ?.? -> UQ16.16
%                         end
%                         
%                         largeDoG(largeY,largeX) = DoG;
%                         
% %                         if largeY == 219 && largeX == 334
% %                             disp(sprintf('DoGorig(%d,%d) = %f', largeY, largeX, DoGorig(largeY,largeX)));
% %                             disp(sprintf('DoG(%d,%d) = %f', largeY, largeX, double(DoG)/(2^16)));
% %                             keyboard
% %                         end
%                         
%                         if DoG > DoG_max(largeY,largeX)
%                             DoG_max(largeY,largeX) = DoG;
%                             curPyramidLevelInterpolated = uint32( interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^16)) );  % SQ?.? -> UQ16.16
%                             scaleImage(largeY,largeX) = curPyramidLevelInterpolated;
%                         end
%                         
%                         largeX = largeX + 1;
%                     end % for iAlphaX = 1:length(alphas)
%                 end % for smallX = 1:(size(curPyramidLevel,2)-1)
% 
%                 largeY = largeY + 1;
%             end % for iAlphaY = 1:length(alphas)
%         end % for smallY = 1:(size(curPyramidLevel,1)-1)
%     end % if k == 2 ... else
%     
%     if DEBUG_DISPLAY
% %         figureHandle = figure(350+pyramidLevel); imshow(double(largeDoG)/(2^16)/255*5);
% %         figureHandle = figure(300+pyramidLevel); subplot(2,2,1); imshow(curPyramidLevel); subplot(2,2,2); imshow(curPyramidLevelBlurred); subplot(2,2,3); imshow(double(largeDoG)/(2^16)/255*5); subplot(2,2,4); imshow(double(scaleImage)/(2^16));
%         figureHandle = figure(100+pyramidLevel); subplot(2,4,2); imshow(curPyramidLevel); subplot(2,4,4); imshow(curPyramidLevelBlurred); subplot(2,4,6); imshow(double(largeDoG)/(2^16)/255*5); subplot(2,4,8); imshow(double(scaleImage)/(255*2^16));
% 
%         % figureHandle = figure(300+pyramidLevel); imshow(curPyramidLevelBlurred);
% %         set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1]) 
%         jFig = get(handle(figureHandle), 'JavaFrame'); 
%         jFig.setMaximized(true);
%         pause(.1);
%     end
% 
%     %imagePyramid{pyramidLevel} = uint8(imresize_bilinear(curPyramidLevelBlurred, size(curPyramidLevelBlurred)/2)); % TODO: implement fixed point version
%     imagePyramid{pyramidLevel} = uint8(downsampleByFactor(curPyramidLevelBlurred, 2));
%     
% %     keyboard;
% end
% 
% end % FUNCTION computeCharacteristicScaleImage()
% 
% % pixel* are UQ8.0
% % alpha* are UQ?.? depending on the level of the pyramid
% function interpolatedPixel = interpolate2d(pixel00, pixel01, pixel10, pixel11, alphaY, alphaYinverse, alphaX, alphaXinverse)
%     interpolatedTop = alphaXinverse*pixel00 + alphaX*pixel01;
%     interpolatedBottom = alphaXinverse*pixel10 + alphaX*pixel11;
% 
%     interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;
% end
% 
