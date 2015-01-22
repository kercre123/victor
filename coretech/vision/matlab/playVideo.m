% function playVideo(images, pauseSeconds, loopForever)

function playVideo(images, pauseSeconds, loopForever)
    
    if ~exist('pauseSeconds', 'var')
        pauseSeconds = 0.03;
    end
    
    if ~exist('loopForever', 'var')
        loopForever = false;
    end
    
    while true
        if iscell(images)
            for iImage = 1:length(images)
                imshows(images{iImage});
                pause(pauseSeconds);
            end
        else
            if ndims(images) == 3
                for iImage = 1:size(images, 3)
                    imshows(images(:,:,iImage));
                    pause(pauseSeconds);
                end
            else
                 for iImage = 1:size(images, 4)
                    imshows(images(:,:,:,iImage));
                    pause(pauseSeconds);
                end
            end
        end
        
        if ~loopForever
            break;
        end
    end % while true
   