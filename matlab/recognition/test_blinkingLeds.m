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
    parsingTypes = {'blur', 'histogram'};
    
    results = cell(length(filenamePatterns), length(parsingTypes), length(alignmentTypes));
    accuracy = cell(length(filenamePatterns), length(parsingTypes), length(alignmentTypes));
    
    testUnknownLedColor = true;
    testKnownLedColor = true;
    
    numTestTypes = 2;
    
    for iAlignmentType = length(alignmentTypes):-1:1
        for iParsingType = 1:length(parsingTypes)
            for iPattern = 1:length(filenamePatterns)
%     for iAlignmentType = 2
%         for iParsingType = 1
%             for iPattern = 3
                whichFirstFrames = filenamePatterns{iPattern}{2}(1):(filenamePatterns{iPattern}{2}(2)-numFramesToTest+1);
                results{iPattern}{iParsingType}{iAlignmentType} = -1 * ones(length(whichFirstFrames), 2, numTestTypes);
                accuracy{iPattern}{iParsingType}{iAlignmentType} = -1 * ones(length(whichFirstFrames), numTestTypes);

                for iFirstFrame = 1:length(whichFirstFrames)
%                 for iFirstFrame = 1
                    firstFrame = whichFirstFrames(iFirstFrame);

                    colorIndexes = zeros(numTestTypes, 1);
                    numPositives = zeros(numTestTypes, 1);

                    % Unknown LED color
                    if testUnknownLedColor
                        [colorIndexes(1), numPositives(1)] = parseLedCode_dutyCycle(...
                            'showFigures', false,...
                            'numFramesToTest', numFramesToTest,...
                            'whichImages', firstFrame:(firstFrame+numFramesToTest-1),...
                            'cameraType', 'offline',...
                            'filenamePattern', filenamePatterns{iPattern}{1},...
                            'alignmentType', alignmentTypes{iAlignmentType},...
                            'parsingType', parsingTypes{iParsingType});
                    end

                    % Ground truth LED color
                    if testKnownLedColor
                        [colorIndexes(2), numPositives(2)] = parseLedCode_dutyCycle(...
                            'showFigures', false,...
                            'numFramesToTest', numFramesToTest,...
                            'whichImages', firstFrame:(firstFrame+numFramesToTest-1),...
                            'cameraType', 'offline',...
                            'filenamePattern', filenamePatterns{iPattern}{1},...
                            'alignmentType', alignmentTypes{iAlignmentType},...
                            'knownLedColor', filenamePatterns{iPattern}{3},...
                            'parsingType', parsingTypes{iParsingType});
                    end

                    for iType = 1:numTestTypes
                        results{iPattern}{iParsingType}{iAlignmentType}(iFirstFrame, :, iType) = [colorIndexes(iType), colorIndexes(iType)];

                        if colorIndexes(iType) == filenamePatterns{iPattern}{3}
                            accuracy{iPattern}{iParsingType}{iAlignmentType}(iFirstFrame,iType) = abs(filenamePatterns{iPattern}{4} - numPositives(iType));
                        else
                            accuracy{iPattern}{iParsingType}{iAlignmentType}(iFirstFrame,iType) = numFramesToTest;
                        end
                    end % for iType = 1:numTestTypes
                end % for iFirstFrame = 1:length(whichFirstFrames)

                figureIndex = length(parsingTypes)*(iAlignmentType-1) + iParsingType;
                
                figureHandle = figure(figureIndex);
                subplot(ceil(sqrt(length(filenamePatterns))), ceil(sqrt(length(filenamePatterns))), iPattern);
                plot(accuracy{iPattern}{iParsingType}{iAlignmentType})
                title(sprintf('FilenamePattern %d (unknown color)', iPattern))
                a = axis();
                axis([a(1:2),-1,numFramesToTest+1]);
                set(figureHandle, 'name', sprintf('ParsingType:%d AlignmentType:%d', iParsingType, iAlignmentType))
                figurePosition = get(figureHandle,'Position');
                set(figureHandle,'Position', [figurePosition(1:2),900,800])

%                 figure(2*iAlignmentType + 2);
%                 subplot(ceil(sqrt(length(filenamePatterns))), ceil(sqrt(length(filenamePatterns))), iPattern);
%                 plot(accuracy{iPattern}{iParsingType}{iAlignmentType}(:,3:4))
%                 title(sprintf('FilenamePattern %d (known color)', iPattern))
%                 a = axis();
%                 axis([a(1:2),-1,numFramesToTest+1]);

                pause(0.01);
            end % for iPattern = 1:length(filenamePatterns)
        end % for iParsingType = 1:length(parsingTypes)
    end % for iAlignmentType = 1:length(alignmentTypes)
    
    
    keyboard
    