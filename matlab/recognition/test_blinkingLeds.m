% function test_blinkingLeds()

% [accuracy, results] = test_blinkingLeds();

function [accuracy, results] = test_blinkingLeds()
    
    filenamePatterns = {...
        {'~/Documents/Anki/products-cozmo-large-files/blinkingLights/dutyCycle1/images_%05d.png', [0,99], 1, 6},...
        {'~/Documents/Anki/products-cozmo-large-files/blinkingLights/dutyCycle2/images_%05d.png', [0,99], 1, 6},...
        {'~/Documents/Anki/products-cozmo-large-files/blinkingLights/dutyCycle3/images_%05d.png', [0,99], 1, 6},...
        {'~/Documents/Anki/products-cozmo-large-files/blinkingLights/dutyCycle4/images_%05d.png', [0,99], 1, 6},...
        {'~/Documents/Anki/products-cozmo-large-files/blinkingLights/dutyCycle5/images_%05d.png', [0,99], 1, 6},...
        {'~/Documents/Anki/products-cozmo-large-files/blinkingLights/dutyCycle6/images_%05d.png', [0,99], 1, 6},...
        {'~/Documents/Anki/products-cozmo-large-files/blinkingLights/dutyCycle7/images_%05d.png', [0,99], 1, 6},...
        {'~/Documents/Anki/products-cozmo-large-files/blinkingLights/dutyCycle8/images_%05d.png', [0,99], 1, 6},...
        {'~/Documents/Anki/products-cozmo-large-files/blinkingLights/dutyCycle9/images_%05d.png', [0,99], 1, 6},...
        {'~/Documents/Anki/products-cozmo-large-files/blinkingLights/dutyCycle10/images_%05d.png', [0,99], 1, 6},...
        {'~/Documents/Anki/products-cozmo-large-files/blinkingLights/dutyCycle11/images_%05d.png', [0,99], 1, 6},...
        {'~/Documents/Anki/products-cozmo-large-files/blinkingLights/dutyCycle12/images_%05d.png', [0,99], 1, 6}};
    
    numFramesToTest = 15;
    
    alignmentTypes = {'none', 'exhaustiveTranslation'};
%     parsingTypes = {'blur', 'histogram'};
    
%     alignmentTypes = {'none', 'exhaustiveTranslation', 'exhaustiveTranslation-double'};
%     parsingTypes = {'blur', 'histogram'};
    parsingTypes = {'blur', 'histogram', 'spatialBlur'};
    
    results = cell(length(filenamePatterns), length(parsingTypes), length(alignmentTypes));
    accuracy = cell(length(filenamePatterns), length(parsingTypes), length(alignmentTypes));
    
    testUnknownLedColor = true;
%     testUnknownLedColor = false;
    
    testKnownLedColor = true;
    
    numTestTypes = 2; % testUnknownLedColor and testKnownLedColor
    
    colorNames = {'red', 'green', 'blue'};
    
    for iAlignmentType = length(alignmentTypes):-1:1
        for iParsingType = length(parsingTypes):-1:1
%     for iAlignmentType = length(alignmentTypes)
%         for iParsingType = 1
%     for iAlignmentType = 2
%         for iParsingType = 3
            
            if iParsingType == 3
                processingSize = [120,160];
                lightSquareWidths = [80] * (processingSize(1) / 240);
            else
                processingSize = [120,160];
                lightSquareWidths = [40] * (processingSize(1) / 240);
            end
            
            for iFilenamePattern = 1:length(filenamePatterns)
%             for iFilenamePattern = 5
                whichFirstFrames = filenamePatterns{iFilenamePattern}{2}(1):(filenamePatterns{iFilenamePattern}{2}(2)-numFramesToTest+1);
                results{iFilenamePattern}{iParsingType}{iAlignmentType} = -1 * ones(length(whichFirstFrames), 2, numTestTypes);
                accuracy{iFilenamePattern}{iParsingType}{iAlignmentType} = -1 * ones(length(whichFirstFrames), numTestTypes);
                
                for iFirstFrame = 1:length(whichFirstFrames)
%                 for iFirstFrame = 1
                    firstFrame = whichFirstFrames(iFirstFrame);
                    
                    colorIndexes = -ones(numTestTypes, 1);
                    numPositives = -ones(numTestTypes, 1);
                    
                    outString = sprintf('test:%d.%d method%d.%d)', iFilenamePattern, iFirstFrame, iAlignmentType, iParsingType);
                    
                    % Unknown LED color
                    if testUnknownLedColor
                        [colorIndexes(1), numPositives(1)] = parseLedCode_dutyCycle(...
                            'showFigures', false,...
                            'numFramesToTest', numFramesToTest,...
                            'whichImages', firstFrame:(firstFrame+numFramesToTest-1),...
                            'cameraType', 'offline',...
                            'filenamePattern', filenamePatterns{iFilenamePattern}{1},...
                            'alignmentType', alignmentTypes{iAlignmentType},...
                            'parsingType', parsingTypes{iParsingType},...
                            'smallBlurKernel', fspecial('gaussian',[11,11],1.5),...
                            'processingSize', processingSize,...
                            'lightSquareWidths', lightSquareWidths);
                        
                        outString = [outString, sprintf(' unknown:%s %d', colorNames{colorIndexes(1)}, numPositives(1))];
                    end
                    
                    % Ground truth LED color
                    if testKnownLedColor
                        [colorIndexes(2), numPositives(2)] = parseLedCode_dutyCycle(...
                            'showFigures', false,...
                            'numFramesToTest', numFramesToTest,...
                            'whichImages', firstFrame:(firstFrame+numFramesToTest-1),...
                            'cameraType', 'offline',...
                            'filenamePattern', filenamePatterns{iFilenamePattern}{1},...
                            'alignmentType', alignmentTypes{iAlignmentType},...
                            'knownLedColor', filenamePatterns{iFilenamePattern}{3},...
                            'parsingType', parsingTypes{iParsingType},...
                            'smallBlurKernel', fspecial('gaussian',[11,11],1.5),...
                            'processingSize', processingSize,...
                            'lightSquareWidths', lightSquareWidths);
                        
                        outString = [outString, sprintf(' known:%s %d', colorNames{colorIndexes(2)}, numPositives(2))];
                    end
                    
                    disp(outString)
                    
                    for iType = 1:numTestTypes
                        results{iFilenamePattern}{iParsingType}{iAlignmentType}(iFirstFrame, :, iType) = [colorIndexes(iType), colorIndexes(iType)];
                        
                        if colorIndexes(iType) == filenamePatterns{iFilenamePattern}{3}
                            accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(iFirstFrame,iType) = abs(filenamePatterns{iFilenamePattern}{4} - numPositives(iType));
                        else
                            accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(iFirstFrame,iType) = numFramesToTest;
                        end
                    end % for iType = 1:numTestTypes
                end % for iFirstFrame = 1:length(whichFirstFrames)
                
                figureIndex = length(parsingTypes)*(iAlignmentType-1) + iParsingType;
                
                figureHandle = figure(figureIndex);
                subplot(ceil(sqrt(length(filenamePatterns))), ceil(sqrt(length(filenamePatterns))), iFilenamePattern);
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
    