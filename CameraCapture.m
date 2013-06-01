function grabs = CameraCapture(varargin)

fps = 25;
device = 0;
resolution = [640 480];
numFrames = inf;
grabInc = [];
figureName = mfilename;
processFcn = [];

parseVarargin(varargin{:});

grabs = {};

% Set up display figure
h_fig = namedFigure(figureName, 'CurrentCharacter', 'a');
keypressID = iptaddcallback(h_fig, 'KeyPressFcn', @keypress);
clf(h_fig);
figure(h_fig);

try
    
    % Initialize and grab first frame
    frame = mexCameraCapture(device, resolution(1), resolution(2));
    
    h_img = imshow(frame);
    %set(h_img, 'EraseMode', 'none');
    pause(1/fps)
    
    % Capture remaining frames
    i_frame = 1;
    while i_frame < numFrames && get(h_fig, 'CurrentCharacter') ~= 27 % ESCAPE
        t = tic;
        frame = mexCameraCapture;
        set(h_img, 'CData', frame);
        if ~isempty(processFcn)
            processFcn(frame);
        end        
        
        i_frame = i_frame + 1;
        
        if ~isempty(grabInc) && mod(i_frame, grabInc)==0
            grabs{end+1} = frame; %#ok<AGROW>
        end
        
        pause(max(0, (1-toc(t))/fps))
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
        end
    end

end




