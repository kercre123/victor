% function image = webotsCameraCapture() 

% To capture images from the webots process, do the following: 
% 
% 1. In basestation/src/messageHandler.cpp, uncomment 
%    "//#define STREAM_IMAGES_VIA_FILESYSTEM 1"
%
% 2. If you're using a single robot, in
%    "robot/simulator/controllers/robot_advertisement_controller/robot_advertisement_controller.cpp",
%    change FORCE_ADD_ROBOT to true, and change forcedRobotIP to the correct
%    IP. Then recompile.
%
% 3. Create a 50mb ramdisk on OSX at "/Volumes/RamDisk/" by typing:
%    diskutil erasevolume HFS+ 'RamDisk' `hdiutil attach -nomount
%    ram://100000`
%
% 4. In the webots window, type shift-i to start streaming images
%
% 5. In Matlab, run "image = webotsCameraCapture();". It blocks until 
%    webots received a new image

function image = webotsCameraCapture(varargin)
    
    filenameDirectory = '/Volumes/RamDisk/';
    filenamePattern = [filenameDirectory, 'robotImage%d.bmp'];
    
    % TODO: add timeout
    % timeout = 0;
    
    parseVarargin(varargin{:});

    persistent lastIndex;
    persistent lastTimestamp;
    
    if isempty(lastIndex)
        lastIndex = 100000;
    end
    
    if isempty(lastTimestamp)
        lastTimestamp = 0;
    end
     
    while true
        files = dir([filenameDirectory, '*.bmp']);

        datenums = zeros(length(files), 1);
        for i = 1:length(files)
            datenums(i) = files(i).datenum;
        end
        
        % Find next image
        foundNewImage = false;
        for iImageIndex = [(lastIndex+1):length(files), 1:(lastIndex-1)]
            if datenums(iImageIndex) < lastTimestamp
                % Wait until we get a new image
                break;
            elseif datenums(iImageIndex) >= lastTimestamp
                foundNewImage = true;
                lastIndex = iImageIndex;
                break;
            end
        end
                
        if foundNewImage
            lastTimestamp = datenums(lastIndex);
            inputFilename = [filenameDirectory, files(lastIndex).name];
            disp(sprintf('Loading %s', inputFilename));
            image = imread(inputFilename);
            break;
        end
        
        pause(0.005);
    end
