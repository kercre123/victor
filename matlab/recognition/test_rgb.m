% function test_rgb()

% [accuracy, results] = test_rgb();

function [accuracy, results] = test_rgb()
    
    % format: {filenamePattern, [minFrame,maxFrame], [whichLeds], [numOnFrames]}

%     redBlueOnly = false;
%     filenamePatterns = {...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb1/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb2/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb3/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb4/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb5/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb6/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb7/images_%05d.png', [0,99], [1,2,3], [5,5,2]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/carRgb8/images_%05d.png', [0,99], [1,2,3], [5,5,2]}};

    redBlueOnly = true;
    filenamePatterns = {...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue1/images_%05d.png', [0,99], [1,2], [6,6]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue2/images_%05d.png', [0,99], [1,2], [6,6]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue3/images_%05d.png', [0,99], [1,2], [6,6]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue4/images_%05d.png', [0,99], [1,2], [6,6]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue5/images_%05d.png', [0,99], [1,2], [6,6]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue6/images_%05d.png', [0,99], [1,2], [6,6]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue7/images_%05d.png', [0,99], [2,1], [8,4]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue8/images_%05d.png', [0,99], [2,1], [8,4]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue9/images_%05d.png', [0,99], [2,1], [8,4]}};
    
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
    if redBlueOnly
        colorNames = {'R', 'B'};
    else
        colorNames = {'R', 'G', 'B'};
    end
    
    %     for iAlignmentType = length(alignmentTypes):-1:1
    for iAlignmentType = length(alignmentTypes)
        for iParsingType = length(parsingTypes):-1:1
            processingSize = [120,160];
            lightSquareWidth = [80] * (processingSize(1) / 240);
            
            for iFilenamePattern = 1:length(filenamePatterns)
%             for iFilenamePattern = 7:9
                whichFirstFrames = filenamePatterns{iFilenamePattern}{2}(1):(filenamePatterns{iFilenamePattern}{2}(2)-numFramesToTest+1);              
                accuracy{iFilenamePattern}{iParsingType}{iAlignmentType} = -1 * ones(length(whichFirstFrames), numTestTypes);
                
                if redBlueOnly
                    results{iFilenamePattern}{iParsingType}{iAlignmentType} = -1 * ones(length(whichFirstFrames), 4, numTestTypes);
                else
                    results{iFilenamePattern}{iParsingType}{iAlignmentType} = -1 * ones(length(whichFirstFrames), 6, numTestTypes);
                end
                
                for iFirstFrame = 1:length(whichFirstFrames)
%                 for iFirstFrame = 1
                    firstFrame = whichFirstFrames(iFirstFrame);
                    
                    if redBlueOnly
                        whichColors = -ones(2, numTestTypes);
                        numPositives = -ones(2, numTestTypes);
                    else
                        whichColors = -ones(3, numTestTypes);
                        numPositives = -ones(3, numTestTypes);
                    end
                    
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
                            'lightSquareWidth', lightSquareWidth,...
                            'redBlueOnly', redBlueOnly);
                        
%                         'smallBlurKernel', fspecial('gaussian',[11,11],1.5),...
                        
                        if length(whichColors) == 2
                            outString = [outString,...
                                sprintf(' unknown:(%s%d %s%d)',...
                                colorNames{whichColors(1,1)}, numPositives(1,1),...
                                colorNames{whichColors(2,1)}, numPositives(2,1))];
                        elseif length(whichColors) == 3
                            outString = [outString,...
                                sprintf(' unknown:(%s%d %s%d %s%d)',...
                                colorNames{whichColors(1,1)}, numPositives(1,1),...
                                colorNames{whichColors(2,1)}, numPositives(2,1),...
                                colorNames{whichColors(3,1)}, numPositives(3,1))];
                        end
                    end
                    
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
                        if whichColors(1,iType)==1 && whichColors(2,iType)==1
                            accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(iFirstFrame,iType) = 10;
                            
                            isBadValue = true;
                        elseif length(filenamePatterns{iFilenamePattern}{4}) == 2 && whichColors(1,iType)==groundTruthColors(1) && whichColors(2,iType)==groundTruthColors(2)
                            
                            curAccuracy = max([abs(filenamePatterns{iFilenamePattern}{4}(1) - numPositives(1,iType)),...
                                abs(filenamePatterns{iFilenamePattern}{4}(2) - numPositives(2,iType))]);
                            
                            accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(iFirstFrame,iType) = curAccuracy;
                            
                            if curAccuracy > 1
                                isBadValue = true;
                            end                     
                        elseif length(filenamePatterns{iFilenamePattern}{4}) == 3 && whichColors(1,iType)==groundTruthColors(1) && whichColors(2,iType)==groundTruthColors(2) && whichColors(3,iType)==groundTruthColors(3)
                            
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
