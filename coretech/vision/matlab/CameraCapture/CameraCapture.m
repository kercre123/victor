function grabs = CameraCapture(varargin)
% Read / display / process images from a camera or stored sequence.
%
% frameGrabs = CameraCapture(<name/value pairs...>)
%  
%  Opens a USB device or saved image sequence and displays it frame by
%  frame, with optional processing applied.  Pressing the spacebar will
%  "grab" the current frame and append it to the output cell array
%  "frameGrabs".  
%  
%  Parameter names and [default values] provided below:
%
%  'fps' [25]
%    Frames per second for the camera or for reading files.  Note that this
%    is the max; the actual speed may be slower, depending on I/O or
%    processing.
%
%  'device', [0]
%    The USB device number to open, a filename pattern to read, (such as
%    '~/some/path/frame*.png'), or a SerialCamera object handle.
%
%  'resolution' [640 480]
%    The resolution of captured frames, [resX resY].  If saved files are
%    being used, they will be resized to this resolution after reading.
%
%  'cropFactor' [1.0]
%    If less than one, then only this fraction of the specified resolution,
%    cropped around the center of the image will be used.
% 
%  'numFrames' [Inf]
%    Total number of frames to capture before returning.  If Inf, the
%    camera capture will run contuniously or until the ESC is pressed, or
%    until the files run out.
%
%  'grabInc' []
%    Scalar integer. If non-empty, every 'grabInc' frames will be returned
%    in the output cell array grabFrames.  If empty, grabs will occur when
%    the user presses the spacebar.
%
%  'appendGrabs' {}
%    To add grabFrames to an initial cell array of previous grabbed frames,
%    pass that initial cell array as this parameter.
%
%  'figureName' ['CameraCapture']
%    The name to give the figure showing the frames.
%
%  'processFcn' []
%    A handle to a function of the form:
%       processFcn(frame, h_axesProc, h_imgProc); 
%    Where 'frame' is the current image data, 'h_axesProc' is the handle of
%    the axes where that frame is to be displayed, and 'h_imgProc' is the
%    handle to the processed image displayed on those axes.  So your
%    processing function can do set(h_imgProc, 'CData', someFcn(frame)),
%    for example.
%
%  'doContinuousProcessing' [false]
%    Whether to process every frame (true) or only process when a key is
%    pressed (false).  (When processFcn is provided.)
%
% Relies on mexCameraCapture for interfacing to USB cameras.
% ------------
% Andrew Stein
% 

fps = 25;
device = 0;
resolution = [640 480];
numFrames = inf;
grabInc = [];
appendGrabs = {};
figureName = mfilename;
processFcn = [];
doContinuousProcessing = false;
displayAxes = [];
processAxes = [];
cropFactor = 1;

parseVarargin(varargin{:});

grabs = appendGrabs;
escapePressed = false;
processKeyPressed = false;

% Set up display figure
h_fig = namedFigure(figureName, 'CurrentCharacter', '~');
keypressID = iptaddcallback(h_fig, 'KeyPressFcn', @keypress);
keypressCleanup = onCleanup(@()iptremovecallback(h_fig, 'KeyPressFcn', keypressID));

clf(h_fig);
figure(h_fig);
colormap(h_fig, gray);

if isempty(displayAxes)
    if ~isempty(processFcn)
        if strcmp(processAxes, 'none')
            displayAxes = subplot(1,1,1, 'Parent', h_fig);
            processAxes = [];
        elseif isempty(processAxes)
            displayAxes = axes('Pos', [0 .53 1 .45], 'Parent', h_fig);
            processAxes = axes('Pos', [0 .03 1 .45], 'Parent', h_fig);
        else
            assert(~isempty(displayAxes), 'If processAxes are specified, so must be displayAxes.');
        end
    else
        displayAxes = subplot(1,1,1, 'Parent', h_fig);
    end
end

frame = [];

OPEN  = 0;
GRAB  = 1;
CLOSE = 2;

    function frame = readFrameFromFile(i_frame, framePath, frameName)
        frame = imread(fullfile(framePath, frameName{i_frame}));
        if size(frame,1)~=resolution(2) || size(frame,2) ~= resolution(1)
            frame = imresize(frame, resolution([2 1]));
        end
    end

    function frame = readFrameFromCamera(~)
        frame = mexCameraCapture(GRAB, device);
        frame = frame(:,:,[3 2 1]); % BGR to RGB
    end

    function frame = readFrameFromWifiCamera(~)
        frame = [];
        while isempty(frame)
            frame = mexWifiCameraCapture(GRAB);
            pause(1/fps);
        end     
        %frame = im2uint8( im2double(frame) .^ (1/2.2));
    end

% Default close camera function doesn't need to do anything.
% Can be redefined below depending on device type.
closeCameraFcn = @()[];

% try
    
    % Initialize and grab first frame
    if ischar(device)
        % see if it looks like an IP address
        if ~isempty(regexp(device, '\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}', 'once'))
            %deviceType = 'wifiCamera';
            frame = mexWifiCameraCapture(OPEN, device);
            getFrameFcn = @readFrameFromWifiCamera;
            closeCameraFcn = @()mexWifiCameraCapture(CLOSE);
        else
            % assume it's a filename/pattern
            %deviceType = 'file';
            [framePath, pattern, patternExt] = fileparts(device);
            frameList = getfnames(framePath, [pattern patternExt]);
            if isempty(frameList)
                error('No frames found at %s.\n', device);
            else
              fprintf('Using %d frames found in %s.\n', length(frameList), framePath);
            end
            numFrames = min(numFrames, length(frameList));
            frame = readFrameFromFile(1, framePath, frameList);
            getFrameFcn = @(i_frame)readFrameFromFile(i_frame, framePath, frameList);
        end
    elseif isa(device, 'SerialCamera')
        %deviceType = 'serialCamera';
        getFrameFcn = @(i_frame)getFrame(device);
        frame = getFrameFcn();
        if size(frame,3)==1
            colormap(h_fig, gray);
        end
    else
        %deviceType = 'usbCamera';
        frame = mexCameraCapture(OPEN, device, resolution(1), resolution(2));
        getFrameFcn = @readFrameFromCamera;
        closeCameraFcn = @()mexCameraCapture(CLOSE);
    end
    
    if cropFactor < 1
        xcen = resolution(1)/2;
        ycen = resolution(2)/2;
        
        xmin = xcen - cropFactor*xcen + 1;
        ymin = ycen - cropFactor*ycen + 1;
        xmax = xcen + cropFactor*xcen;
        ymax = ycen + cropFactor*ycen;
        cropRect = round([xmin ymin xmax-xmin+1 ymax-ymin+1]);
        
        frame = imcrop(frame, cropRect);
    end
    
    h_img = imagesc(frame, 'Parent', displayAxes);
    axis(displayAxes, 'image', 'off');
    if ~isempty(processFcn) && ~isempty(processAxes)
        h_imgProc = imagesc(frame, 'Parent', processAxes);
        axis(processAxes, 'image', 'off');
    else
        h_imgProc = [];
    end
    
    %set(h_img, 'EraseMode', 'none');
    if fps > 0
      pause(1/fps)
    end
    
    if ~isempty(grabInc) 
        grabs = {frame};
    end
        
    % Capture remaining frames
    i_frame = 1;
    totalProcessTime = 0;
    numProcessCalls = 0;
    while i_frame <= numFrames && ~escapePressed
        t = tic;
        frame = getFrameFcn(i_frame);
        
        if cropFactor < 1
           frame = imcrop(frame, cropRect); 
        end
        
        set(h_img, 'CData', frame);
        if ~isempty(processFcn)
            if doContinuousProcessing || processKeyPressed
                if ~doContinuousProcessing
                    disp('Processing frame')
                end
                %try
                tic
                processFcn(frame, processAxes, h_imgProc); %#ok<NOEFF>
                totalProcessTime = totalProcessTime + toc;
                numProcessCalls = numProcessCalls + 1;
                %catch E
                 %   warning(E.message)
                %end
                   
                processKeyPressed = false;
            end
        end
        
        i_frame = i_frame + 1;
        
        if ~isempty(grabInc) && mod(i_frame-1, grabInc)==0
            grabs{end+1} = frame; %#ok<AGROW>
        end
        
        drawnow
        if fps == 0
          pause
        else
          pause(max(0, 1/fps-toc(t)))
        end
        
        title(displayAxes, sprintf('%.1f FPS', 1/toc(t)));
    end
    
% catch E
% 
%     closeCameraFcn();    
%     rethrow(E)
%     
% end

closeCameraFcn();

fprintf('Average processing function time = %.3fsec (%d calls)\n', ...
  totalProcessTime / numProcessCalls, numProcessCalls);

if nargout==0
    clear grabs;
end

    function keypress(~, edata)
        if isempty(edata.Modifier)
        switch(edata.Key)
            case 'escape'
                escapePressed = true;
                
            case 'space'
                disp('Frame grabbed.');
                grabs{end+1} = frame;
                
                %             case {'return', 'enter'}
                %                 if ~isempty(processFcn)
                %                     processFcn(frame);
                %                 end
                
            otherwise
                processKeyPressed = true;
                
        end
        end
    end

end




