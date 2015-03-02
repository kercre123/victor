% function test_rgb()

% [accuracy, results] = test_rgb();

function [accuracy, results] = test_rgb(varargin)
    
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
    
%     redBlueOnly = true;
%     filenamePatterns = {...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue1/images_%05d.png', [0,99], [1,2], [6,6]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue2/images_%05d.png', [0,99], [1,2], [6,6]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue3/images_%05d.png', [0,99], [1,2], [6,6]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue4/images_%05d.png', [0,99], [1,2], [6,6]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue5/images_%05d.png', [0,99], [1,2], [6,6]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue6/images_%05d.png', [0,99], [1,2], [6,6]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue7/images_%05d.png', [0,99], [2,1], [8,4]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue8/images_%05d.png', [0,99], [2,1], [8,4]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue9/images_%05d.png', [0,99], [2,1], [8,4]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue10/images_%05d.png', [0,99], [1,2], [8,4]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue10/images_%05d.png', [100,199], [1,2], [8,4]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue10/images_%05d.png', [200,299], [1,2], [8,4]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue10/images_%05d.png', [300,399], [1,2], [8,4]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue10/images_%05d.png', [400,499], [1,2], [8,4]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue10/images_%05d.png', [500,599], [1,2], [8,4]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue10/images_%05d.png', [600,699], [1,2], [8,4]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue10/images_%05d.png', [700,799], [1,2], [8,4]},...
%         {'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue10/images_%05d.png', [800,899], [1,2], [8,4]}};
    
%     redBlueOnly = true;
%     filenamePatterns = {{'~/Documents/Anki/drive-ar-large-files/blinkingLights/redBlue10/images_%05d.png', [0,999], [1,2], [8,4]}};
    
    redBlueOnly = true;
    filenamePatterns = {...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/headlights1/images_%05d.png', [0,99], [1,2], [10,2]},...
        {'~/Documents/Anki/drive-ar-large-files/blinkingLights/headlights2/images_%05d.png', [0,99], [1,2], [10,2]}};

    displayIndividualResults = false;
    
    if ispc()
        for i = 1:length(filenamePatterns)
            filenamePatterns{i}{1} = strrep(filenamePatterns{i}{1}, '~/Documents/', 'c:/');
        end
    end
    
    findHeadlights = true;
    
    testCExecutable = true;
    cExecutable = 'C:/Anki/ARGarage/parseLedCode/parseLedCode.msvc2012/Release/test_parseLedCode.exe ';
    outputFilename = 'c:/tmp/testRgbOut.txt';
    
    numFramesToTest = 15;
    
    parseVarargin(varargin);
    
    %     compositeVideo = test_createCompositeSampleVideo(filenamePatterns);
    %     save /Users/pbarnum/Documents/Anki/drive-ar-large-files/blinkingLights/lightsVideo.mat compositeVideo
    %     load /Users/pbarnum/Documents/Anki/drive-ar-large-files/blinkingLights/lightsVideo.mat
    
    alignmentTypes = {'none', 'exhaustiveTranslation'}; % {'none', 'exhaustiveTranslation', 'exhaustiveTranslation-double'};
    parsingTypes = {'spatialBlur'};
    scaleDetectionMethod = 'temporalGaussian'; % { 'originalGaussian', 'temporalGaussian', 'temporalBox'}
    
    results = cell(length(filenamePatterns), length(parsingTypes), length(alignmentTypes));
    accuracy = cell(length(filenamePatterns), length(parsingTypes), length(alignmentTypes));
    
    ledSearch_roi = [30, 20, 21, 21]; % opencv format
    ledSearch_scale = 0.5;
    
    %     colorNames = {'red', 'green', 'blue'};
    if redBlueOnly
        colorNames = {'R', 'B'};
    else
        colorNames = {'R', 'G', 'B'};
    end
    
    %     for iAlignmentType = length(alignmentTypes):-1:1
    for iAlignmentType = length(alignmentTypes)
        for iParsingType = length(parsingTypes):-1:1
%             processingSize = [60,80];
%             lightSquareWidth = [80] * (processingSize(1) / 240);
            
            for iFilenamePattern = 1:length(filenamePatterns)
                %             for iFilenamePattern = 7:9
                whichFirstFrames = filenamePatterns{iFilenamePattern}{2}(1):(filenamePatterns{iFilenamePattern}{2}(2)-numFramesToTest+1);
                accuracy{iFilenamePattern}{iParsingType}{iAlignmentType} = -1 * ones(length(whichFirstFrames), 1);
                
                if redBlueOnly
                    results{iFilenamePattern}{iParsingType}{iAlignmentType} = -1 * ones(length(whichFirstFrames), 4);
                else
                    results{iFilenamePattern}{iParsingType}{iAlignmentType} = -1 * ones(length(whichFirstFrames), 6);
                end
                
                if testCExecutable
                    commandString = [...
                        cExecutable, ' ',...
                        filenamePatterns{iFilenamePattern}{1}, ' ',...
                        sprintf('%d %d ', filenamePatterns{iFilenamePattern}{2}(1), filenamePatterns{iFilenamePattern}{2}(2)),...
                        outputFilename];
                    
                    system(commandString);
                    
                    file = fopen(outputFilename);
                    
                    numbers = fscanf(file, '%d');
                    
                    fclose(file);
                    
                    numFrames = numbers(1);
                    
                    assert(numFrames == length(whichFirstFrames));
                    
                    numbers = reshape(numbers(2:end), [10, numFrames]);
                    numbers(2:3, :) = numbers(2:3, :) + 1;
                    numbers(6:8, :) = numbers(6:8, :) + 1;
                    
                    whichColors = numbers(2:3, :);
                    numPositives = numbers(4:5, :);
                    locationX = numbers(6, :);
                    locationY = numbers(7, :);
                    locationScaleIndex = numbers(8, :);
                    locationScale = numbers(9, :);
                    firstIndex = numbers(10, :);
                    
                    if findHeadlights
                        for iImage = filenamePatterns{iFilenamePattern}{2}(1):filenamePatterns{iFilenamePattern}{2}(2)
                            tmpIm = imread(sprintf(filenamePatterns{iFilenamePattern}{1}, iImage));
                            ims = zeros([size(tmpIm), 15], 'uint8');
                            for di = 0:14
                                ims(:,:,:,di+1) = imread(sprintf(filenamePatterns{iFilenamePattern}{1}, iImage+di));
                            end
                            
                            parseLedCode_findHeadlights(ims, whichColors(:, iImage+1), numPositives(:, iImage+1), locationX(iImage+1), locationY(iImage+1), locationScale(iImage+1), firstIndex(iImage+1), ledSearch_roi, ledSearch_scale);
                        end
                    end % if findHeadlights
                else % if testCExecutable
                    if redBlueOnly
                        whichColors = -ones(2, length(whichFirstFrames));
                        numPositives = -ones(2, length(whichFirstFrames));
                    else
                        whichColors = -ones(3, length(whichFirstFrames));
                        numPositives = -ones(3, length(whichFirstFrames));
                    end
                    
                    for iFirstFrame = 1:length(whichFirstFrames)
                        firstFrame = whichFirstFrames(iFirstFrame);
                        
                        outString = sprintf('test:(%d,%d) method:(%d,%d)', iFilenamePattern, iFirstFrame, iAlignmentType, iParsingType);
                        
                        [whichColors(:,iFirstFrame), numPositives(:,iFirstFrame)] = parseLedCode_rgb(...
                            'showFigures', false,...
                            'numFramesToTest', numFramesToTest,...
                            'whichImages', firstFrame:(firstFrame+numFramesToTest-1),...
                            'cameraType', 'offline',...
                            'filenamePattern', filenamePatterns{iFilenamePattern}{1},...
                            'alignmentType', alignmentTypes{iAlignmentType},...
                            'parsingType', parsingTypes{iParsingType},...
                            'processingSize', processingSize,...
                            'lightSquareWidth', lightSquareWidth,...
                            'redBlueOnly', redBlueOnly,...
                            'scaleDetectionMethod', scaleDetectionMethod);
                        
                        if size(whichColors,1) == 2
                            outString = [outString,...
                                sprintf(' (%s%d %s%d)',...
                                colorNames{whichColors(1,iFirstFrame)}, numPositives(1,iFirstFrame),...
                                colorNames{whichColors(2,iFirstFrame)}, numPositives(2,iFirstFrame))];
                        elseif size(whichColors,1) == 3
                            outString = [outString,...
                                sprintf(' (%s%d %s%d %s%d)',...
                                colorNames{whichColors(1,iFirstFrame)}, numPositives(1,iFirstFrame),...
                                colorNames{whichColors(2,iFirstFrame)}, numPositives(2,iFirstFrame),...
                                colorNames{whichColors(3,iFirstFrame)}, numPositives(3,iFirstFrame))];
                        end
                        
                        if displayIndividualResults
                            disp(outString);
                        end
                    end % for iFirstFrame = 1:length(whichFirstFrames)
                end % if testCExecutable
                
                for iFirstFrame = 1:size(whichColors,2)
                    outString = sprintf('test:(%d,%d) method:(%d,%d)', iFilenamePattern, iFirstFrame, iAlignmentType, iParsingType);
                    
                    if size(whichColors,1) == 2
                        outString = [outString,...
                            sprintf(' (%s%d %s%d)',...
                            colorNames{whichColors(1,iFirstFrame)}, numPositives(1,iFirstFrame),...
                            colorNames{whichColors(2,iFirstFrame)}, numPositives(2,iFirstFrame))];
                    elseif size(whichColors,1) == 3
                        outString = [outString,...
                            sprintf(' (%s%d %s%d %s%d)',...
                            colorNames{whichColors(1,iFirstFrame)}, numPositives(1,iFirstFrame),...
                            colorNames{whichColors(2,iFirstFrame)}, numPositives(2,iFirstFrame),...
                            colorNames{whichColors(3,iFirstFrame)}, numPositives(3,iFirstFrame))];
                    end
                                        
                    results{iFilenamePattern}{iParsingType}{iAlignmentType}(iFirstFrame, :) = [whichColors(:,iFirstFrame)', numPositives(:,iFirstFrame)'];
                    
                    groundTruthColors = filenamePatterns{iFilenamePattern}{3};
                    if whichColors(1,iFirstFrame) == whichColors(2,iFirstFrame)
                        accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(iFirstFrame) = 20;
                        outString = [outString, ' *']; %#ok<AGROW>
                    elseif length(filenamePatterns{iFilenamePattern}{4}) == 2 && whichColors(1,iFirstFrame)==groundTruthColors(1) && whichColors(2,iFirstFrame)==groundTruthColors(2)
                        
                        curAccuracy = max([abs(filenamePatterns{iFilenamePattern}{4}(1) - numPositives(1,iFirstFrame)),...
                            abs(filenamePatterns{iFilenamePattern}{4}(2) - numPositives(2,iFirstFrame))]);
                        
                        accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(iFirstFrame) = curAccuracy;
                        
                        if curAccuracy > 1
                            outString = [outString, ' XXXXX']; %#ok<AGROW>
                        end
                    elseif length(filenamePatterns{iFilenamePattern}{4}) == 3 && whichColors(1,iFirstFrame)==groundTruthColors(1) && whichColors(2,iFirstFrame)==groundTruthColors(2) && whichColors(3,iFirstFrame)==groundTruthColors(3)
                        
                        curAccuracy = max([abs(filenamePatterns{iFilenamePattern}{4}(1) - numPositives(1,iFirstFrame)),...
                            abs(filenamePatterns{iFilenamePattern}{4}(2) - numPositives(2,iFirstFrame)),...
                            abs(filenamePatterns{iFilenamePattern}{4}(3) - numPositives(3,iFirstFrame))]);
                        
                        accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(iFirstFrame) = curAccuracy;
                        
                        if curAccuracy > 1
                            outString = [outString, ' XXXXX']; %#ok<AGROW>
                        end
                    else
                        accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(iFirstFrame) = numFramesToTest;
                        outString = [outString, ' ZZZZZ']; %#ok<AGROW>
                    end
                    
                    if displayIndividualResults
                        disp(outString)
                    end
                end % for iFirstFrame = 1:size(whichColors,2)
                
                figureIndex = length(parsingTypes)*(iAlignmentType-1) + iParsingType;
                
                figureHandle = figure(figureIndex);
                subplotHandle = subplot(ceil(sqrt(length(filenamePatterns))), ceil(sqrt(length(filenamePatterns))), iFilenamePattern);
                %                 plot(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType})
                hold off
                
%                 markerSizes = 5 * ones(size(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType},1), 1);
%                 scatter(1:size(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType},1), accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(:,1), markerSizes, 'bo', 'filled');

                allInds = 1:size(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType},1);

                % Detected invalid                
                inds = find(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(:,1) == 20);
                markerSizes = 5 * ones(length(inds), 1);
                scatter(allInds(inds), 10*ones(length(inds),1), markerSizes, 'bo', 'filled');

                hold on;
                
                % Good
                showGood = false;
                if showGood
                    inds = find(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(:,1) <= 1);
                    markerSizes = 5 * ones(length(inds), 1);
                    scatter(allInds(inds), accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(inds,1), markerSizes, 'go', 'filled');
                end
                
                % Bad
                inds = find(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(:,1) > 1 & accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(:,1) ~= 20);
                markerSizes = 5 * ones(length(inds), 1);
                scatter(allInds(inds), accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}(inds,1), markerSizes, 'ro', 'filled');
                
                if length(allInds) <= 100
                    set(subplotHandle, 'XTick', [0:10:100]);
                else
                    set(subplotHandle, 'XTick', [0:round(length(allInds)/20):length(allInds)]);
                end
                
                title(sprintf('FilenamePattern %d', iFilenamePattern))
                a = axis();
                axis([a(1:2),-1,numFramesToTest+1]);
                set(figureHandle, 'name', sprintf('ParsingType:%d AlignmentType:%d', iParsingType, iAlignmentType))
                figurePosition = get(figureHandle,'Position');
                set(figureHandle,'Position', [figurePosition(1),100,900,800])
                
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
    
    disp('Overall accuracy:');
    for iAlignmentType = 1:length(alignmentTypes)
        for iParsingType = 1:length(parsingTypes)
            for iFilenamePattern = 1:length(filenamePatterns)
                numGood = length(find(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType}<=1)) ;
                numSkipped = length(find(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType} == 20));
                numErrors = length(find(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType} ~= 20 & accuracy{iFilenamePattern}{iParsingType}{iAlignmentType} > 1));
                numTotal = length(accuracy{iFilenamePattern}{iParsingType}{iAlignmentType});
                
                percentCorrect = numGood / numTotal;
                percentSkipped = numSkipped / numTotal;
                percentBadErrors = numErrors / numTotal;
                disp(sprintf('%d %d %d) good:%d/%d=%0.2f%%    \tskipped:%d/%d=%0.2f%%   \tveryBad:%d/%d=%0.2f%%',...
                    iAlignmentType, iParsingType, iFilenamePattern,...
                    numGood, numTotal, 100*percentCorrect,...
                    numSkipped, numTotal, 100*percentSkipped,...
                    numErrors, numTotal, 100*percentBadErrors));
            end
        end
    end
    
    keyboard
end % function test_rgb()
