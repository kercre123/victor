function grabs = CameraCapture(varargin)

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

parseVarargin(varargin{:});

grabs = appendGrabs;

% Set up display figure
h_fig = namedFigure(figureName, 'CurrentCharacter', '~');
keypressID = iptaddcallback(h_fig, 'KeyPressFcn', @keypress);
clf(h_fig);
figure(h_fig);

if isempty(displayAxes)
    if ~isempty(processFcn)
        if isempty(processAxes)
            displayAxes = axes('Pos', [0 .51 1 .45], 'Parent', h_fig);
            processAxes = axes('Pos', [0 .03 1 .45], 'Parent', h_fig);
        else
            assert(~isempty(displayAxes), 'If processAxes are specified, so must be displayAxes.');
        end
    else
        displayAxes = subplot(1,1,1, 'Parent', h_fig);
    end
end

try
    
    % Initialize and grab first frame
    frame = mexCameraCapture(device, resolution(1), resolution(2));
    
    h_img = imshow(frame, 'Parent', displayAxes);
    if ~isempty(processFcn)
        h_imgProc = imshow(frame, 'Parent', processAxes);
    end
    
    %set(h_img, 'EraseMode', 'none');
    pause(1/fps)
    
    % Capture remaining frames
    i_frame = 1;
    while i_frame < numFrames && get(h_fig, 'CurrentCharacter') ~= 27 % ESCAPE
        t = tic;
        frame = mexCameraCapture;
        frame = frame(:,:,[3 2 1]); % BGR to RGB
        
        set(h_img, 'CData', frame);
        if ~isempty(processFcn)
            if doContinuousProcessing || get(h_fig, 'CurrentCharacter') ~= '~' 
                disp('Processing frame')
                processFcn(frame, processAxes, h_imgProc); %#ok<NOEFF>
                set(h_fig, 'CurrentCharacter', '~');
            end
        end
        
        i_frame = i_frame + 1;
        
        if ~isempty(grabInc) && mod(i_frame, grabInc)==0
            grabs{end+1} = frame; %#ok<AGROW>
        end
        
        drawnow
        pause(max(0, 1/fps-toc(t)))
        title(displayAxes, sprintf('%.1f FPS', 1/toc(t)));
    end
    
catch E
    iptremovecallback(h_fig, 'KeyPressFcn', keypressID);
    mexCameraCapture(-1);
    rethrow(E)
end

mexCameraCapture(-1);
iptremovecallback(h_fig, 'KeyPressFcn', keypressID);


if nargout==0
    clear grabs;
end


    function keypress(~, edata)
        switch(edata.Key)
            case 'space'
                disp('Frame grabbed.');
                grabs{end+1} = frame;
            
%             case {'return', 'enter'}
%                 if ~isempty(processFcn)
%                     processFcn(frame);
%                 end
                
        end
    end

end




