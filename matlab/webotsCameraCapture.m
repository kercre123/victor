% function image = webotsCameraCapture()

% To capture images from the webots process, do the following:
%
% 1. Create a 50mb ramdisk on OSX at "/Volumes/RamDisk/" by typing:
%    diskutil erasevolume HFS+ 'RamDisk' `hdiutil attach -nomount ram://100000`
%
% 2. Type "mkdir /Volumes/RamDisk/image_captures"
%
% 3. Create a symbolic link from the ramdisk, to the image_captures directory, by typing something like: "ln -s ~/Documents/Anki/cozmo-game/simulator/controllers/blockworld_controller/image_captures/ /Volumes/RamDisk/"
%
% 4. In the webots window, type shift-e to start saving images
%
% 5. In Matlab, run "image = webotsCameraCapture();". It blocks until
%    webots received a new image
%
%
% Example:
% while true image = webotsCameraCapture(); imshow(image); pause(0.003); end;

function image = webotsCameraCapture(varargin)
    
    persistent files;
    
    filenameDirectory = '/Volumes/RamDisk/image_captures/';
    filenamePattern = [filenameDirectory, '*.jpg'];
    
    allowLag = false; % If false, delete everything up to the second-to-last image
    
    % TODO: add timeout
    % timeout = 0;
    
    parseVarargin(varargin{:});
    
    if length(files) < 2
        while true
            files = dir(filenamePattern);

            if length(files) >= 2
                break;
            end

            pause(0.001);
        end
    end
    
    timestamps = zeros(length(files), 2);
    
    for i = 1:length(files)
        timestamps(i,:) = [sscanf(files(i).name, 'cozmo1_img_%d.jpg'), i];
    end
    
    sortedTimestamps = sortrows(timestamps, 1);
    
    if allowLag
        goodIndex = sortedTimestamps(1,2);
        
        fullFilename = [filenameDirectory, files(goodIndex).name];
    
        image = imread(fullFilename);

        system(['rm ', fullFilename]);
    
        files = [files(1:(firstIndex-1)), files((firstIndex+1):end)];
    else
        goodIndex = sortedTimestamps(end-1,2);
        
        fullFilename = [filenameDirectory, files(goodIndex).name];
    
        image = imread(fullFilename);

        fullFilenames = '';
        for i = 1:(size(sortedTimestamps, 1) - 1)
            fullFilenames = [fullFilenames, ' ', filenameDirectory, files(sortedTimestamps(i,2)).name]; %#ok<AGROW>
        end
        
        system(['rm ', fullFilenames]);
        
        files = [];
    end
    
    
end % webotsCameraCapture()

