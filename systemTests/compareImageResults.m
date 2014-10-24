% function compareImageResults(filenamePatterns, varargin)

% compareImageResults({'/Users/pbarnum/Documents/Anki/products-cozmo-large-files/systemTestsData/results/dateTime_2014-10-23_13-41-34/images/c-with-refinement/*.png', '/Users/pbarnum/Documents/Anki/products-cozmo-large-files/systemTestsData/results/dateTime_2014-10-23_13-41-34/images/c-with-refinement-binomial/*.png'});

function compareImageResults(filenamePatterns, varargin)
    global numPatterns;
    global curPattern;
    
    numPatterns = length(filenamePatterns);
    
    imageFilenames = cell(numPatterns, 1);
    flipSpeed = 1.0;
    numImageTiles = [4,7];
    originalImageSize = [580,640];
    resizedImageSize = round(originalImageSize * 0.4);
    
    parseVarargin(varargin{:});
    
    images = cell(numPatterns, 1);
    
    for iPattern = 1:numPatterns
        imageFilenames{iPattern} = rdir(filenamePatterns{iPattern});
        images{iPattern} = cell(length(imageFilenames{iPattern}));
    end
    
    %     for iPattern = 2:numPatterns
    %         if length(imageFilenames{1}) ~= length(imageFilenames{iPattern})
    %             disp('Different number of images in these directories')
    %             keyboard
    %         end
    %     end
    
    curImageIndex = 1;
    maxImageIndex = length(imageFilenames{1});
    
    listOfTimers = timerfindall;
    if ~isempty(listOfTimers)
        delete(listOfTimers(:));
    end
    
    isRunning = true;
    while isRunning
        for iPattern = 1:numPatterns
            figureHandle = figure(200 + iPattern);
            set(figureHandle, 'name', [sprintf('%d->%d ', curImageIndex, curImageIndex+prod(numImageTiles)-1), filenamePatterns{iPattern}])
            
            bigImage = zeros([resizedImageSize(1)*numImageTiles(1), resizedImageSize(2)*numImageTiles(2), 3], 'uint8');
            
            dImage = 0;
            for dImageY = 0:(numImageTiles(1)-1)
                for dImageX = 0:(numImageTiles(2)-1)
                    totalImageIndex = curImageIndex + dImage;
                    
                    if totalImageIndex <= length(imageFilenames{iPattern})
                        curFilename = imageFilenames{iPattern}(totalImageIndex).name;
                        
                        curImage = images{iPattern}{totalImageIndex};
                        
                        % If the image is not cached yet, load and cache it
                        if isempty(curImage)
                            curImage = imresize(imread(curFilename), resizedImageSize);
                            images{iPattern}{totalImageIndex} = curImage;
                        end
                        
                        bigImage((1 + resizedImageSize(1)*dImageY):(resizedImageSize(1)*(dImageY+1)), (1 + resizedImageSize(2)*dImageX):(resizedImageSize(2)*(dImageX+1)), :) = curImage;
                    end
                    
                    dImage = dImage + 1;
                end % for dImageX = 0:(numImageTiles(2)-1)
            end % for dImageY = 0:(numImageTiles(1)-1)
            
            imshow(bigImage);
            %             set(figureHandle, 'units','normalized','outerposition',[0 0 1 1]);
            
            slashIndex = find(curFilename == '/');
            title(curFilename((slashIndex(end)+1):end))
        end % for iPattern = 1:numPatterns
        
        curPattern = iPattern;
        
        curCharacter = -1;
        
        while isnan(curCharacter) || curCharacter < 0
            curCharacter = getkeywait(flipSpeed);
            
            for iPattern = 1:numPatterns
                if ~ishandle(200 + iPattern)
                    isRunning = false;
                    break;
                end
            end
            
            if curCharacter == int32('a')
                curImageIndex = curImageIndex - 1*prod(numImageTiles);
            elseif curCharacter == int32('s')
                curImageIndex = curImageIndex + 1*prod(numImageTiles);
            elseif curCharacter == int32('d')
                curImageIndex = curImageIndex - 10*prod(numImageTiles);
            elseif curCharacter == int32('f')
                curImageIndex = curImageIndex + 10*prod(numImageTiles);
            elseif curCharacter == int32('g')
                curImageIndex = curImageIndex - 100*prod(numImageTiles);
            elseif curCharacter == int32('h')
                curImageIndex = curImageIndex + 100*prod(numImageTiles);
            elseif curCharacter == int32('q')
                isRunning = false;
                break;
            elseif curCharacter == -1
                curPattern = curPattern + 1;
                
                if curPattern > numPatterns
                    curPattern = 1;
                end;
                
                if ishandle(200 + curPattern)
                    figure(200 + curPattern);
                end
            end
            
            if curImageIndex < 1
                curImageIndex = 1;
            elseif curImageIndex > (length(imageFilenames{1}) - prod(numImageTiles) + 1)
                curImageIndex = length(imageFilenames{1}) - prod(numImageTiles) + 1;
            end
        end % while isnan(curCharacter) || curCharacter < 0
    end % while isRunning
    
    
    