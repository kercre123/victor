% function varargout = selectPoints(varargin)

% selectPoints();

function varargout = selectPoints(varargin)

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @selectPoints_OpeningFcn, ...
                   'gui_OutputFcn',  @selectPoints_OutputFcn, ...
                   'gui_LayoutFcn',  [] , ...
                   'gui_Callback',   []);
if nargin && ischar(varargin{1})
    gui_State.gui_Callback = str2func(varargin{1});
end

if nargout
    [varargout{1:nargout}] = gui_mainfcn(gui_State, varargin{:});
else
    gui_mainfcn(gui_State, varargin{:});
end
% End initialization code - DO NOT EDIT

function loadConfigFile()
    global jsonConfigFilename;
    global jsonData;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;
    global image;
    global dataPath;
    global imageFigureHandle;
    global imageHandle;
    global allHandles;

    imageFigureHandle = figure(100);
    
    jsonConfigFilename = get(allHandles.configFilename, 'String');
    jsonConfigFilename = strrep(jsonConfigFilename, '\', '/');
    jsonData = loadjson(jsonConfigFilename);
    curSequenceNumber = 1;
    maxSequenceNumber = length(jsonData.sequences);
    curFrameNumber = 1;
    maxFrameNumber = 0;
    image = rand([480,640]);

    slashIndexes = strfind(jsonConfigFilename, '/');
    dataPath = jsonConfigFilename(1:(slashIndexes(end)));

    for i = 1:maxSequenceNumber
%         if ~isfield(jsonData.sequences{i}, 'groundTruth')
%             jsonData.sequences{i}.groundTruth = [];
%         end
        
        if isfield(jsonData.sequences{i}, 'groundTruth') && ~iscell(jsonData.sequences{i}.groundTruth)
            jsonData.sequences{i}.groundTruth = {jsonData.sequences{i}.groundTruth};
        end
    end

    pointer = [
        nan, nan, nan, nan, 2,   2,   2,   2,   2,   2,   2,   2,   nan, nan, nan, nan;
        nan, nan, nan, 2,   nan, nan, nan, nan, nan, nan, nan, nan, 2,   nan, nan, nan;
        nan, nan, 2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2,   nan, nan;
        nan, 2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2,   nan;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        nan, 2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2,   nan;
        nan, nan, 2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2,   nan, nan;
        nan, nan, nan, 2,   nan, nan, nan, nan, nan, nan, nan, nan, 2,   nan, nan, nan;
        nan, nan, nan, nan, 2,   2,   2,   2,   2,   2,   2,   2,   nan, nan, nan, nan;];

    
    set(imageFigureHandle,'Pointer','custom','PointerShapeCData',pointer,'PointerShapeHotSpot',(size(pointer))/2)

    sequenceChanged(allHandles, true)

function selectPoints_OpeningFcn(hObject, eventdata, handles, varargin)
    % Choose default command line output for selectPoints
    handles.output = hObject;

    % Update handles structure
    guidata(hObject, handles);

    % UIWAIT makes selectPoints wait for user response (see UIRESUME)
    % uiwait(handles.figure1);

    global allHandles;
    allHandles = handles;
    
    loadConfigFile()

function index = findFrameNumberIndex(jsonData, sequenceNumberIndex, frameNumberIndex)
    index = -1;

    if ~isfield(jsonData.sequences{sequenceNumberIndex}, 'groundTruth')
        return;
    end
    
    for i = 1:length(jsonData.sequences{sequenceNumberIndex}.groundTruth)
        curFrameNumber = jsonData.sequences{sequenceNumberIndex}.groundTruth{i}.frameNumber;
        queryFrameNumber = jsonData.sequences{sequenceNumberIndex}.frameNumbers(frameNumberIndex);

    %     disp(sprintf('%d) %d %d', i, curFrameNumber, queryFrameNumber));

        if curFrameNumber == queryFrameNumber
            index = i;
            return;
        end
    end

    return;

function varargout = selectPoints_OutputFcn(hObject, eventdata, handles)
    % Get default command line output from handles structure
    varargout{1} = handles.output;

function previousSequence_Callback(hObject, eventdata, handles) %#ok<*INUSL>
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    if curSequenceNumber > 1
        curSequenceNumber = curSequenceNumber - 1;
    end

    sequenceChanged(handles, true);

function nextSequence_Callback(hObject, eventdata, handles)
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    if curSequenceNumber < maxSequenceNumber
        curSequenceNumber = curSequenceNumber + 1;
    end

    sequenceChanged(handles, true);

function curSequence_Callback(hObject, eventdata, handles)
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    curSequenceNumberNew = str2double(get(hObject,'String'));

    if curSequenceNumberNew <= maxSequenceNumber && curSequenceNumberNew >= 1
        curSequenceNumber = curSequenceNumberNew;
    end

    sequenceChanged(handles);

function previousImage_Callback(hObject, eventdata, handles) %#ok<*DEFNU>
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    if curFrameNumber > 1
        curFrameNumber = curFrameNumber - 1;
    end

    sequenceChanged(handles);

function nextImage_Callback(hObject, eventdata, handles)
    global curSequenceNumber; %#ok<*NUSED>
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    if curFrameNumber < maxFrameNumber
        curFrameNumber = curFrameNumber + 1;
    end

    sequenceChanged(handles);

function curImage_Callback(hObject, eventdata, handles)
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    curFrameNumberNew = str2double(get(hObject,'String'));

    if curFrameNumberNew <= maxSequenceNumber && curFrameNumberNew >= 1
        curFrameNumber = curFrameNumberNew;
    end

    sequenceChanged(handles);

function curSequence_CreateFcn(hObject, eventdata, handles) %#ok<*INUSD>
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
        set(hObject,'BackgroundColor','white');
    end

    set(hObject,'String',num2str(curSequenceNumber));

function curImage_CreateFcn(hObject, eventdata, handles)
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
        set(hObject,'BackgroundColor','white');
    end

    set(hObject,'String',num2str(curFrameNumber));

function maxSequence_CreateFcn(hObject, eventdata, handles)
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    set(hObject,'String',num2str(maxSequenceNumber));

function maxImage_CreateFcn(hObject, eventdata, handles)
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    set(hObject,'String',num2str(maxFrameNumber));

function sequenceChanged(handles, redoZoom)
    global jsonData;
    global dataPath;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;
    global image;
    global allHandles;
    global imageFigureHandle;
    global imageHandle;

    if ~exist('redoZoom', 'var')
        redoZoom = false;
    end

    allHandles = handles;

    if curSequenceNumber < 1
        curSequenceNumber = 1;
    end

    if curSequenceNumber > maxSequenceNumber
        curSequenceNumber = maxSequenceNumber;
    end

    maxFrameNumber = length(jsonData.sequences{1}.frameNumbers);

    if curFrameNumber < 1
        curFrameNumber = 1;
    end

    if curFrameNumber > maxFrameNumber
        curFrameNumber = maxFrameNumber;
    end

    curFilename = [dataPath, sprintf(jsonData.sequences{curSequenceNumber}.filenamePattern, jsonData.sequences{curSequenceNumber}.frameNumbers(curFrameNumber))];
    image = imread(curFilename);

    set(handles.curSequence, 'String', num2str(curSequenceNumber))
    set(handles.maxSequence, 'String', num2str(maxSequenceNumber))
    set(handles.curImage, 'String', num2str(curFrameNumber))
    set(handles.maxImage, 'String', num2str(maxFrameNumber))

    index = findFrameNumberIndex(jsonData, curSequenceNumber, curFrameNumber);

    imageFigureHandle = figure(100);
    
    hold off;
    
    originalXlim = xlim();
    originalYlim = ylim();

    imageHandle = imshow(image);

    if ~redoZoom
        xlim(originalXlim);
        ylim(originalYlim);
    end

    set(imageHandle,'ButtonDownFcn',@axes1_ButtonDownFcn);
    hold on;

    if index ~= -1
        allCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners;
        if ~iscell(allCorners)
            allCorners = {allCorners};
        end
        for i = 1:length(allCorners)
            scatterHandle = scatter(allCorners{i}.x+0.5, allCorners{i}.y+0.5, 'r+');
            set(scatterHandle, 'HitTest', 'off')
        end
    end

function configFilename_Callback(hObject, eventdata, handles)
    loadConfigFile()

function configFilename_CreateFcn(hObject, eventdata, handles)
    if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
        set(hObject,'BackgroundColor','white');
    end

function axes1_ButtonDownFcn(hObject, eventdata, handles)
    global jsonConfigFilename;
    global jsonData;
    global dataPath;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;
    global image;
    global allHandles;
    global imageFigureHandle;
    global imageHandle;

    axesHandle  = get(imageHandle,'Parent');
    imPosition = get(axesHandle, 'CurrentPoint') - 0.5;

    imPosition = imPosition(1,1:2);

    if imPosition(1) < 0 || imPosition(2) < 0 || imPosition(1) > size(image,2) || imPosition(2) > size(image,1)
        return;
    end

    index = findFrameNumberIndex(jsonData, curSequenceNumber, curFrameNumber);

    if index == -1
        if ~isfield(jsonData.sequences{curSequenceNumber}, 'groundTruth')
            jsonData.sequences{curSequenceNumber}.groundTruth = [];
        end
        
        allFrameNumbers = jsonData.sequences{curSequenceNumber}.frameNumbers;
        jsonData.sequences{curSequenceNumber}.groundTruth{end+1}.frameNumber = allFrameNumbers(curFrameNumber);
        jsonData.sequences{curSequenceNumber}.groundTruth{end}.corners = [];
        index = length(jsonData.sequences{curSequenceNumber}.groundTruth);
    end

    % disp(index)
    buttonType = get(imageFigureHandle,'selectionType');
    if strcmp(buttonType, 'normal') % left click
        newPoint.x = imPosition(1);
        newPoint.y = imPosition(2);

        if ~iscell(jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners)
            jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners = {jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners};
        end
        
        jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners{end+1} = newPoint;
        savejson('',jsonData,jsonConfigFilename);
    elseif strcmp(buttonType, 'alt') % right click
        minDist = Inf;
        minInd = -1;

        for i = 1:length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners)
            curCorner = jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners{i};
            dist = sqrt((curCorner.x - imPosition(1))^2 + (curCorner.y - imPosition(2))^2);
            if dist < minDist
                minDist = dist;
                minInd = i;
            end
        end

        
        if minInd ~= -1 && minDist < (min(size(image,1),size(image,2))/50)
            newCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners([1:(minInd-1),(minInd+1):end]);
            jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners = newCorners;
            savejson('',jsonData,jsonConfigFilename);
        end
    end

    sequenceChanged(allHandles);

function figure1_ButtonDownFcn(hObject, eventdata, handles)
