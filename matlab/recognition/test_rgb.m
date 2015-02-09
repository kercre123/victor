% function test_rgb()

% [accuracy, results] = test_rgb();

function [accuracy, results] = test_rgb()
    
    % format: {filenamePattern, [minFrame,maxFrame], [whichLeds], [numOnFrames]}
%     filenamePatterns = {...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/rgb1/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/rgb2/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/rgb3/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/rgb4/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/rgb5/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/rgb6/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/rgb7/images_%05d.png', [0,99], [1,2,3], [5,5,2]}};

    filenamePatterns = {...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb1/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb2/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb3/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb4/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb5/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb6/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb7/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb8/images_%05d.png', [0,99], [1,2,3], [5,5,2]}};
    
    numFramesToTest = 15;
    
    %     compositeVideo = test_createCompositeSampleVideo(filenamePatterns);
    %     save /Users/pbarnum/Documents/Anki/drive-ar-large-files/blinkingLights/lightsVideo.mat compositeVideo
    %     load /Users/pbarnum/Documents/Anki/drive-ar-large-files/blinkingLights/lightsVideo.mat
    
    alignmentTypes = {'none', 'exhaustiveTranslation'}; % {'none', 'exhaustiveTranslation', 'exhaustiveTranslation-double'};
    parsingTypes = {'spatialBlur'};
    
    results = cell(length(filenamePatterns), length(parsingTypes), length(alignmentTypes));
    accuracy = cell(length(filenamePatterns), length(parsingTypes), length(alignmentTypes));
    
    testUnknownLedColor = true;
    %     testUnknownLedColor = false;
    
    %     testKnownLedColor = true;
    testKnownLedColor = false;
    
    numTestTypes = 2; % testUnknownLedColor and testKnownLedColor
    
    %     colorNames = {'red', 'green', 'blue'};
    colorNames = {'R', 'G', 'B'};
    
    %     for iAlignmentType = length(alignmentTypes):-1:1
    for iAlignmentType = length(alignmentTypes)
        for iParsingType = length(parsingTypes):-1:1
            processingSize = [120,160];
            lightSquareWidth = [80] * (processingSize(1) / 240);
            
            for iFilenamePattern = 1:length(filenamePatterns)
%             for iFilenamePattern = 8
                whichFirstFrames = filenamePatterns{iFilenamePattern}{2}(1):(filenamePatterns{iFilenamePattern}{2}(2)-numFramesToTest+1);
                results{iFilenamePattern}{iParsingType}{iAlignmentType} = -1 * ones(length(whichFirstFrames), 6, numTestTypes);
                accuracy{iFilenamePattern}{iParsingType}{iAlignmentType} = -1 * ones(length(whichFirstFrames), numTestTypes);
                
                for iFirstFrame = 1:length(whichFirstFrames)
%                 for iFirstFrame = 1
                    firstFrame = whichFirstFrames(iFirstFrame);
                    
                    whichColors = -ones(3, numTestTypes);
                    numPositives = -ones(3, numTestTypes);
                    
                    outString = sprintf('test:(%d,%d) method:(%d,%d)', iFilenamePattern, iFirstFrame, iAlignmentType, iParsingType);
                    
                    % Unknown LED color
                    if testUnknownLedColor
                        [whichColors(:,1), numPositives(:,1)] = parseLedCode_rgb(...
                            'showFigures', false,...
                            'numFramesToTest', numFramesToTest,...
                            'whichImages', firstFrame:(firstFrame+numFramesToTest-1),...
                            'cameraType', 'offline',...
                            'filenamePattern', filenamePatterns{iFilenamePattern}{1},...
                            'alignmentType', alignmentTypes{iAlignmentType},...
                            'parsingType', parsingTypes{iParsingType},...
                            'processingSize', processingSize,...
                            'lightSquareWidth', lightSquareWidth);
                        
%                         'smallBlurKernel', fspecial('gaussian',[11,11],1.5),...
                        
                        %                             sprintf(' unknown:(%s%0.1f %s%0.1f %s%0.1f)',...
                        outString = [outString,...
                            sprintf(' unknown:(%s%d %s%d %s%d)',...
                            colorNames{whichColors(1,1)}, numPositives(1,1),...
                            colorNames{whichColors(2,1)}, numPositives(2,1),...
                            colorNames{whichColors(3,1)}, numPositives(3,1))];
                    end
                    
                    %                     % Ground truth LED color
                    %                     if testKnownLedColor
                    %                         [whichColors(2), numPositives(2)] = parseLedCode_rgb(...
                    %                             'showFigures', false,...
                    %                             'numFramesToTest', numFramesToTest,...
                    %                             'whichImages', firstFrame:(firstFrame+numFramesToTest-1),...
                    %                             'cameraType', 'offline',...
                    %                             'filenamePattern', filenamePatterns{iFilenamePattern}{1},...
                    %                             'alignmentType', alignmentTypes{iAlignmentType},...
                    %                             'knownLedColor', filenamePatterns{iFilenamePattern}{3},...
                    %                             'parsingType', parsingTypes{iParsingType},...
                    %                             'smallBlurKernel', fspecial('gaussian',[11,11],1.5),...
                    %                             'processingSize', processingSize,...
                    %                             'lightSquareWidth', lightSquareWidth);
                    %
                    %                         outString = [outString, sprintf(' known:(%s,%0.1f)', colorNames{whichColors(2)}, numPositives(2))];
                    %                     end
                    
                    isBadValue = false;
                    for iType = [1,2]
                        if iType == 1 && ~testUnknownLedColor
                            continue;
                        end
                        
                        if iType == 2 && ~testKnownLedColor
                            continue;
                        end
                        
                        results{iFilenamePattern}{iParsingType}{iAlignmentType}(iFirstFrame, :, iType) = [whichColors(:,iType)', numPositives(:,iType)'];
                        
                        groundTruthColors = filenamePatterns{iFilenamePattern}{3};
                        if whichColors(1,iType)==1 && whichColors(2,iType)==1 && whichColors(3,iType)==1
                            accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(iFirstFrame,iType) = 10;
                            
                            isBadValue = true;
                        elseif whichColors(1,iType)==groundTruthColors(1) && whichColors(2,iType)==groundTruthColors(2) && whichColors(3,iType)==groundTruthColors(3)
                            
                            curAccuracy = max([abs(filenamePatterns{iFilenamePattern}{4}(1) - numPositives(1,iType)),...
                                abs(filenamePatterns{iFilenamePattern}{4}(2) - numPositives(2,iType)),...
                                abs(filenamePatterns{iFilenamePattern}{4}(3) - numPositives(3,iType))]);
                            
                            accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(iFirstFrame,iType) = curAccuracy;
                            
                            if curAccuracy > 1
                                isBadValue = true;
                            end
                        else
                            accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(iFirstFrame,iType) = numFramesToTest;
                            isBadValue = true;
                        end
                    end % for iType = 1:numTestTypes
                    
                    if isBadValue
                        outString = [outString, ' *****']; %#ok<AGROW>
                    end
                    
                    disp(outString)
                    
                end % for iFirstFrame = 1:length(whichFirstFrames)
                
                figureIndex = length(parsingTypes)*(iAlignmentType-1) + iParsingType;
                
                figureHandle = figure(figureIndex);
                subplotHandle = subplot(ceil(sqrt(length(filenamePatterns))), ceil(sqrt(length(filenamePatterns))), iFilenamePattern);
                %                 plot(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType})
                hold off
                if testUnknownLedColor
                    markerSizes1 = 20 * ones(size(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType},1),1);
                    scatter(1:size(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType},1), accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(:,1), markerSizes1, 'bo', 'filled');
                    hold on;
                end
                
                if testKnownLedColor
                    markerSizes2 = 5 * ones(size(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType},1),1);
                    scatter(1:size(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType},1), accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(:,2), markerSizes2, 'go', 'filled');
                end
                
                set(subplotHandle, 'XTick', [0:10:100]);
                title(sprintf('FilenamePattern %d', iFilenamePattern))
                a = axis();
                axis([a(1:2),-1,numFramesToTest+1]);
                set(figureHandle, 'name', sprintf('ParsingType:%d AlignmentType:%d', iParsingType, iAlignmentType))
                figurePosition = get(figureHandle,'Position');
                set(figureHandle,'Position', [figurePosition(1:2),900,800])
                
                %                 figure(2*iAlignmentType + 2);
                %                 subplot(ceil(sqrt(length(filenamePatterns))), ceil(sqrt(length(filenamePatterns))), iFilenamePattern);
                %                 plot(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(:,3:4))
                %                 title(sprintf('FilenamePattern %d (known color)', iFilenamePattern))
                %                 a = axis();
                %                 axis([a(1:2),-1,numFramesToTest+1]);
                
                pause(0.01);
            end % for iFilenamePattern = 1:length(filenamePatterns)
        end % for iParsingType = 1:length(parsingTypes)
    end % for iAlignmentType = 1:length(alignmentTypes)
    
    
    keyboard
end % function test_rgb()
