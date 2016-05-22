% function playVideo(images, pauseSeconds, loopForever)

function playVideo(images, pauseSeconds, loopForever)
    
    if ~exist('pauseSeconds', 'var') || isempty(pauseSeconds)
        pauseSeconds = 0.03;
    end
    
    if ~exist('loopForever', 'var') || isempty(loopForever)
        loopForever = true;
    end
    
    % If the inside array is a cell array, the format it {{sequence1}, {sequence2}}
    isNestedCellArray = false;
    
    if iscell(images)
        for i = 1:length(images)
            if iscell(images{i}) || ndims(images{i})==4 || (ndims(images{i})==3 && size(images{i},3)~=3)
                isNestedCellArray = true;
                break;
            end
        end
    end
    
    if ~isNestedCellArray
        images = {images};
    end
    
    numImages = zeros(length(images), 1);
    for iCell = 1:length(images)
        if iscell(images{iCell})
            numImages(iCell) = length(images{iCell});
        else
            numImages(iCell) = size(images{iCell}, ndims(images{iCell}));
        end
    end
    
    curImages = zeros(length(images), 1);
    while true
        textTitle = 'Frame ';
        
        toShowImages = cell(length(images), 1);
        for iCell = 1:length(images)
            % Increment the frame number
            curImages(iCell) = curImages(iCell) + 1;
            if curImages(iCell) > numImages(iCell)
                if loopForever
                    curImages(iCell) = 1;
                else
                    return;
                end
            end
            
            textTitle = [textTitle, sprintf('%d/%d ', curImages(iCell), numImages(iCell))]; %#ok<AGROW>
            
            if iscell(images{iCell})
                toShowImages{iCell} = images{iCell}{curImages(iCell)};
            else
                if ndims(images{iCell}) == 3
                    toShowImages{iCell} = images{iCell}(:,:,curImages(iCell));
                elseif ndims(images{iCell}) == 4
                    toShowImages{iCell} = images{iCell}(:,:,:,curImages(iCell));
                else
                    assert(false);
                end
            end
        end % for iCell = 1:length(images)
        
        imshows(toShowImages);
        
        set(gcf, 'name', textTitle, 'NumberTitle', 'off')
        
        if pauseSeconds >= 0
            pause(pauseSeconds);
        else
            pause();
        end
    end % while true
end % function playVideo()
