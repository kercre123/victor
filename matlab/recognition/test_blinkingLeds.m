% function test_blinkingLeds()

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
    
    results = cell(length(filenamePatterns), 1);
    accuracy = cell(length(filenamePatterns), 1);
    
    numTestTypes = 4;
    
    for iPattern = 1:length(filenamePatterns)
        whichFirstFrames = filenamePatterns{iPattern}{2}(1):(filenamePatterns{iPattern}{2}(2)-numFramesToTest+1);
        results{iPattern} = zeros(length(whichFirstFrames), 2, numTestTypes);
        accuracy{iPattern} = zeros(length(whichFirstFrames), numTestTypes);
        
%         for iFirstFrame = 1:length(whichFirstFrames)
        for iFirstFrame = 1:10
            firstFrame = whichFirstFrames(iFirstFrame);
            
            colorIndexes = zeros(numTestTypes, 1);
            numPositives = zeros(numTestTypes, 1);
            
            % Unknown LED color
            [colorIndexes(1), numPositives(1), colorIndexes(2), numPositives(2)] = parseLedCode_dutyCycle(...
                'showFigures', false,...
                'numFramesToTest', numFramesToTest,...
                'whichImages', firstFrame:(firstFrame+numFramesToTest-1),...
                'cameraType', 'offline',...
                'filenamePattern', filenamePatterns{iPattern}{1});
            
            % Ground truth LED color
            [colorIndexes(3), numPositives(3), colorIndexes(4), numPositives(4)] = parseLedCode_dutyCycle(...
                'showFigures', false,...
                'numFramesToTest', numFramesToTest,...
                'whichImages', firstFrame:(firstFrame+numFramesToTest-1),...
                'cameraType', 'offline',...
                'filenamePattern', filenamePatterns{iPattern}{1},...
                'knownLedColor', filenamePatterns{iPattern}{3});
            
            for iType = 1:numTestTypes
                results{iPattern}(iFirstFrame, :, iType) = [colorIndexes(iType), colorIndexes(iType)];
                
                if colorIndexes(iType) == filenamePatterns{iPattern}{3}
                    accuracy{iPattern}(iFirstFrame,iType) = abs(filenamePatterns{iPattern}{4} - numPositives(iType));
                else
                    accuracy{iPattern}(iFirstFrame,iType) = numFramesToTest;
                end
            end
        end % for iFirstFrame = 1:length(whichFirstFrames)
        
        figure(1);
        subplot(ceil(sqrt(length(filenamePatterns))), ceil(sqrt(length(filenamePatterns))), iPattern);
        plot(accuracy{iPattern}(:,1:2))
        title(sprintf('FilenamePattern %d (unknown color)', iPattern))
        a = axis();
        axis([a(1:2),-1,numFramesToTest+1]);
        
        figure(2);
        subplot(ceil(sqrt(length(filenamePatterns))), ceil(sqrt(length(filenamePatterns))), iPattern);
        plot(accuracy{iPattern}(:,3:4))
        title(sprintf('FilenamePattern %d (known color)', iPattern))
        a = axis();
        axis([a(1:2),-1,numFramesToTest+1]);
        
        pause(0.01);
    end % for iPattern = 1:length(filenamePatterns)
    
    
    keyboard
    