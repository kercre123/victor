function varargout = selectPoints(varargin)
% SELECTPOINTS MATLAB code for selectPoints.fig
%      SELECTPOINTS, by itself, creates a new SELECTPOINTS or raises the existing
%      singleton*.
%
%      H = SELECTPOINTS returns the handle to a new SELECTPOINTS or the handle to
%      the existing singleton*.
%
%      SELECTPOINTS('CALLBACK',hObject,eventData,handles,...) calls the local
%      function named CALLBACK in SELECTPOINTS.M with the given input arguments.
%
%      SELECTPOINTS('Property','Value',...) creates a new SELECTPOINTS or raises the
%      existing singleton*.  Starting from the left, property value pairs are
%      applied to the GUI before selectPoints_OpeningFcn gets called.  An
%      unrecognized property name or invalid value makes property application
%      stop.  All inputs are passed to selectPoints_OpeningFcn via varargin.
%
%      *See GUI Options on GUIDE's Tools menu.  Choose "GUI allows only one
%      instance to run (singleton)".
% 
%      selectPoints('C:/Anki/systemTestImages/test_1.json')
%
% See also: GUIDE, GUIDATA, GUIHANDLES

% Edit the above text to modify the response to help selectPoints

% Last Modified by GUIDE v2.5 27-Jan-2014 17:32:38

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


% --- Executes just before selectPoints is made visible.
function selectPoints_OpeningFcn(hObject, eventdata, handles, varargin)
% This function has no output args, see OutputFcn.
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% varargin   command line arguments to selectPoints (see VARARGIN)

% Choose default command line output for selectPoints
handles.output = hObject;

% Update handles structure
guidata(hObject, handles);

% UIWAIT makes selectPoints wait for user response (see UIRESUME)
% uiwait(handles.figure1);

global jsonConfigFilename;
global jsonData;
global curSequenceNumber;
global maxSequenceNumber;
global curFrameNumber;
global maxFrameNumber;
global image;
global dataPath;

jsonConfigFilename = get(handles.configFilename, 'String');
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
    if ~isfield(jsonData.sequences{i}, 'groundTruth')
        jsonData.sequences{i}.groundTruth = [];
    end
end

% pointer = nan * ones(16,16);
% pointer([1,end],1:2:end) = 1;
% pointer([1,end],2:2:end) = 2;
% pointer(1:2:end,[1,end]) = 1;
% pointer(2:2:end,[1,end]) = 2;
% pointer(5,5:11) = 1;
% pointer(5:11,5) = 1;
% pointer(11,5:11) = 1;
% pointer(5:11,11) = 1;
% 
% pointer(8,1:4) = 1;
% pointer(8,12:15) = 1;
% pointer(1:4,8) = 1;
% pointer(12:15,8) = 1;


% pointer(4,4:12) = 2;
% pointer(4:12,4) = 2;
% pointer(12,4:12) = 2;
% pointer(4:12,12) = 2;

% pointer = [
%     nan, nan, nan, nan, 2,   2,   2,   2,   2,   2,   2,   2,   nan, nan, nan, nan;
%     nan, nan, nan, 2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   nan, nan, nan;
%     nan, nan, 2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   nan, nan;
%     nan, 2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   nan;
%     2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2;
%     2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2;
%     2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2;
%     2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2;
%     2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2;
%     2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2;
%     2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2;    
%     2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2;
%     nan, 2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   nan;
%     nan, nan, 2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   nan, nan;
%     nan, nan, nan, 2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   nan, nan, nan;
%     nan, nan, nan, nan, 2,   2,   2,   2,   2,   2,   2,   2,   nan, nan, nan, nan;];
    

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

% set(gcf,'Pointer','circle')
%set(hObject,'Pointer','custom','PointerShapeCData',pointer,'PointerShapeHotSpot',(size(pointer)-1)/2)
set(hObject,'Pointer','custom','PointerShapeCData',pointer,'PointerShapeHotSpot',(size(pointer))/2)

sequenceChanged(handles)


function index = findFrameNumberIndex(jsonData, sequenceNumberIndex, frameNumberIndex)
index = -1;

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


% --- Outputs from this function are returned to the command line.
function varargout = selectPoints_OutputFcn(hObject, eventdata, handles) 
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
varargout{1} = handles.output;


% --- Executes on button press in previousSequence.
function previousSequence_Callback(hObject, eventdata, handles)
% hObject    handle to previousSequence (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global curSequenceNumber;
global maxSequenceNumber;
global curFrameNumber;
global maxFrameNumber;

if curSequenceNumber > 1
    curSequenceNumber = curSequenceNumber - 1;
end

sequenceChanged(handles);

% --- Executes on button press in nextSequence.
function nextSequence_Callback(hObject, eventdata, handles)
% hObject    handle to nextSequence (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global curSequenceNumber;
global maxSequenceNumber;
global curFrameNumber;
global maxFrameNumber;

if curSequenceNumber < maxSequenceNumber
    curSequenceNumber = curSequenceNumber + 1;
end

sequenceChanged(handles);


function curSequence_Callback(hObject, eventdata, handles)
% hObject    handle to curSequenceNumber (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of curSequenceNumber as text
%        str2double(get(hObject,'String')) returns contents of curSequenceNumber as a double

global curSequenceNumber;
global maxSequenceNumber;
global curFrameNumber;
global maxFrameNumber;

curSequenceNumberNew = str2double(get(hObject,'String'));

if curSequenceNumberNew <= maxSequenceNumber && curSequenceNumberNew >= 1
    curSequenceNumber = curSequenceNumberNew;
end

sequenceChanged(handles);

% --- Executes on button press in previousImage.
function previousImage_Callback(hObject, eventdata, handles)
% hObject    handle to previousImage (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global curSequenceNumber;
global maxSequenceNumber;
global curFrameNumber;
global maxFrameNumber;

if curFrameNumber > 1
    curFrameNumber = curFrameNumber - 1;
end

sequenceChanged(handles);

% --- Executes on button press in nextImage.
function nextImage_Callback(hObject, eventdata, handles)
% hObject    handle to nextImage (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global curSequenceNumber;
global maxSequenceNumber;
global curFrameNumber;
global maxFrameNumber;

if curFrameNumber < maxFrameNumber
    curFrameNumber = curFrameNumber + 1;
end

sequenceChanged(handles);


function curImage_Callback(hObject, eventdata, handles)
% hObject    handle to curFrameNumber (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of curFrameNumber as text
%        str2double(get(hObject,'String')) returns contents of curFrameNumber as a double
global curSequenceNumber;
global maxSequenceNumber;
global curFrameNumber;
global maxFrameNumber;

curFrameNumberNew = str2double(get(hObject,'String'));

if curFrameNumberNew <= maxSequenceNumber && curFrameNumberNew >= 1
    curFrameNumber = curFrameNumberNew;
end

sequenceChanged(handles);

% --- Executes during object creation, after setting all properties.
function curSequence_CreateFcn(hObject, eventdata, handles)
% hObject    handle to curSequenceNumber (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

global curSequenceNumber;
global maxSequenceNumber;
global curFrameNumber;
global maxFrameNumber;

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end

set(hObject,'String',num2str(curSequenceNumber));

% --- Executes during object creation, after setting all properties.
function curImage_CreateFcn(hObject, eventdata, handles)
% hObject    handle to curFrameNumber (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

global curSequenceNumber;
global maxSequenceNumber;
global curFrameNumber;
global maxFrameNumber;

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end

set(hObject,'String',num2str(curFrameNumber));


% --- Executes during object creation, after setting all properties.
function maxSequence_CreateFcn(hObject, eventdata, handles)
% hObject    handle to maxSequenceNumber (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

global curSequenceNumber;
global maxSequenceNumber;
global curFrameNumber;
global maxFrameNumber;

set(hObject,'String',num2str(maxSequenceNumber));

% --- Executes during object creation, after setting all properties.
function maxImage_CreateFcn(hObject, eventdata, handles)
% hObject    handle to maxFrameNumber (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

global curSequenceNumber;
global maxSequenceNumber;
global curFrameNumber;
global maxFrameNumber;

set(hObject,'String',num2str(maxFrameNumber));


% --- Executes during object creation, after setting all properties.
function axes1_CreateFcn(hObject, eventdata, handles)
% hObject    handle to axes1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

global image;
% imshow(image)
% impixelinfo;


function sequenceChanged(handles)
global jsonData;
global dataPath;
global curSequenceNumber;
global maxSequenceNumber;
global curFrameNumber;
global maxFrameNumber;
global image;
global allHandles;

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

% figure(2); imshow(axes1);

set(handles.curSequence, 'String', num2str(curSequenceNumber))
set(handles.maxSequence, 'String', num2str(maxSequenceNumber))
set(handles.curImage, 'String', num2str(curFrameNumber))
set(handles.maxImage, 'String', num2str(maxFrameNumber))

index = findFrameNumberIndex(jsonData, curSequenceNumber, curFrameNumber);

figure(get(handles.axes1,'Parent')) 
set(get(handles.axes1,'Parent'),'WindowButtonDownFcn',@axes1_ButtonDownFcn)
hold off;
imshow(image);
hold on;

if index ~= -1
    allCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners;
    for i = 1:length(allCorners)
        scatter(allCorners{i}.x+1, allCorners{i}.y+1, 'r+');
    end
end

function configFilename_Callback(hObject, eventdata, handles)
% hObject    handle to configFilename (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of configFilename as text
%        str2double(get(hObject,'String')) returns contents of configFilename as a double



% --- Executes during object creation, after setting all properties.
function configFilename_CreateFcn(hObject, eventdata, handles)
% hObject    handle to configFilename (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on mouse press over axes background.
function axes1_ButtonDownFcn(hObject, eventdata, handles)
% hObject    handle to axes1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% disp('toast');
% mousePosition = get(hObject, 'CurrentPoint');
% disp(mousePosition)

global jsonData;
global dataPath;
global curSequenceNumber;
global maxSequenceNumber;
global curFrameNumber;
global maxFrameNumber;
global image;
global allHandles;

% keyboard

axesPosition = get(allHandles.axes1, 'Position');
mousePosition = get(hObject, 'CurrentPoint');
% char = get(allHandles.axes1, 'CurrentCharacter')

imPosition = mousePosition - axesPosition(1:2);

if imPosition(1) < 0 || imPosition(2) < 0 || imPosition(1) >= axesPosition(3) || imPosition(2) >= axesPosition(4)
    return;
end

imPosition(1) = imPosition(1);
imPosition(2) = axesPosition(4) - imPosition(2);

imPosition(1) = size(image,2) * imPosition(1) / axesPosition(3);
imPosition(2) = size(image,1) * imPosition(2) / axesPosition(4);

% disp(axesPosition)
% disp(imPosition)

% disp(sprintf('dog: %d %d', curSequenceNumber, curFrameNumber));
index = findFrameNumberIndex(jsonData, curSequenceNumber, curFrameNumber);

if index == -1
    allFrameNumbers = jsonData.sequences{curSequenceNumber}.frameNumbers;
    jsonData.sequences{curSequenceNumber}.groundTruth{end+1}.frameNumber = allFrameNumbers(curFrameNumber);
    jsonData.sequences{curSequenceNumber}.groundTruth{end}.corners = [];
    index = length(jsonData.sequences{curSequenceNumber}.groundTruth);
end

% disp(index)
buttonType = get(hObject,'selectionType');
if strcmp(buttonType, 'normal') || strcmp(buttonType, 'open') % left click
    newPoint.x = imPosition(1);
    newPoint.y = imPosition(2);
    jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners{end+1} = newPoint;
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
    
    if minInd ~= -1 && minDist < 5
        newCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners([1:(minInd-1),(minInd+1):end]);
        jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners = newCorners;
    end
end

sequenceChanged(allHandles);

% --- Executes on mouse press over figure background.
function figure1_ButtonDownFcn(hObject, eventdata, handles)
% hObject    handle to figure1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
