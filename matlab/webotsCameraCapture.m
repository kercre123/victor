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
    
    firstIndex = Inf;
    firstTimestamp = Inf;
    
    for i = 1:length(files)
        curTimestamp = sscanf(files(i).name, 'cozmo1_img_%d.jpg');
        
        if curTimestamp < firstTimestamp
            firstTimestamp = curTimestamp;
            firstIndex = i;
        end
    end
    
    assert(~isinf(firstIndex));
    
    fullFilename = [filenameDirectory, files(firstIndex).name];
    
    image = imread(fullFilename);

    system(['rm ', fullFilename]);
    
    files = [files(1:(firstIndex-1)), files((firstIndex+1):end)];
end % webotsCameraCapture()

