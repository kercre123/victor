% function playVideo(images, pauseSeconds, loopForever)

function playVideo(images, pauseSeconds, loopForever, varargin)
    
    if ~exist('pauseSeconds', 'var') || isempty(pauseSeconds)
        pauseSeconds = 0.03;
    end
    
    if ~exist('loopForever', 'var') || isempty(loopForever)
        loopForever = true;
    end
    
    showFrameNumber = true;
    
    parseVarargin(varargin);
    
    while true
        if iscell(images)
            for iImage = 1:length(images)
                imshows(images{iImage});
                set(gcf, 'name', sprintf('Frame %d/%d', iImage, length(images)), 'NumberTitle','off')
                pause(pauseSeconds);
            end
        else
            if ndims(images) == 3
                for iImage = 1:size(images, 3)
                    imshows(images(:,:,iImage));
                    set(gcf, 'name', sprintf('Frame %d/%d', iImage, size(images, 3)), 'NumberTitle','off')
                    pause(pauseSeconds);
                end
            else
                 for iImage = 1:size(images, 4)
                    imshows(images(:,:,:,iImage));
                    set(gcf, 'name', sprintf('Frame %d/%d', iImage, size(images, 4)), 'NumberTitle','off')
                    pause(pauseSeconds);
                end
            end
        end
        
        if ~loopForever
            break;
        end
    end % while true
   