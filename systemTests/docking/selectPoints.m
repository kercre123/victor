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
%      jsonData = loadjson('C:/Anki/systemTestImages/test_1.json');
%      selectPoints(jsonData)
%
% See also: GUIDE, GUIDATA, GUIHANDLES

% Edit the above text to modify the response to help selectPoints

% Last Modified by GUIDE v2.5 27-Jan-2014 16:12:52

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

global jsonData;
global curSequence;
global maxSequence;
global curImage;
global maxImage;
global image;

jsonData = varargin{1};

curSequence = 1;
maxSequence = length(jsonData.sequences);
curImage = 10;
maxImage = 10;
image = rand([480,640]);

sequenceChanged(handles)


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
global curSequence;
global maxSequence;
global curImage;
global maxImage;

if curSequence > 1
    curSequence = curSequence - 1;
end

sequenceChanged(handles);

% --- Executes on button press in nextSequence.
function nextSequence_Callback(hObject, eventdata, handles)
% hObject    handle to nextSequence (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global curSequence;
global maxSequence;
global curImage;
global maxImage;

if curSequence < maxSequence
    curSequence = curSequence + 1;
end

sequenceChanged(handles);


function curSequence_Callback(hObject, eventdata, handles)
% hObject    handle to curSequence (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of curSequence as text
%        str2double(get(hObject,'String')) returns contents of curSequence as a double

% --- Executes on button press in previousImage.
function previousImage_Callback(hObject, eventdata, handles)
% hObject    handle to previousImage (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global curSequence;
global maxSequence;
global curImage;
global maxImage;

if curImage > 1
    curImage = curImage - 1;
end

sequenceChanged(handles);

% --- Executes on button press in nextImage.
function nextImage_Callback(hObject, eventdata, handles)
% hObject    handle to nextImage (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global curSequence;
global maxSequence;
global curImage;
global maxImage;

if curImage < maxImage
    curImage = curImage + 1;
end

sequenceChanged(handles);


function curImage_Callback(hObject, eventdata, handles)
% hObject    handle to curImage (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of curImage as text
%        str2double(get(hObject,'String')) returns contents of curImage as a double


% --- Executes during object creation, after setting all properties.
function curSequence_CreateFcn(hObject, eventdata, handles)
% hObject    handle to curSequence (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

global curSequence;
global maxSequence;
global curImage;
global maxImage;

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end

set(hObject,'String',num2str(curSequence));

% --- Executes during object creation, after setting all properties.
function curImage_CreateFcn(hObject, eventdata, handles)
% hObject    handle to curImage (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

global curSequence;
global maxSequence;
global curImage;
global maxImage;

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end

set(hObject,'String',num2str(curImage));


% --- Executes during object creation, after setting all properties.
function maxSequence_CreateFcn(hObject, eventdata, handles)
% hObject    handle to maxSequence (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

global curSequence;
global maxSequence;
global curImage;
global maxImage;

set(hObject,'String',num2str(maxSequence));

% --- Executes during object creation, after setting all properties.
function maxImage_CreateFcn(hObject, eventdata, handles)
% hObject    handle to maxImage (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

global curSequence;
global maxSequence;
global curImage;
global maxImage;

set(hObject,'String',num2str(maxImage));


% --- Executes during object creation, after setting all properties.
function axes1_CreateFcn(hObject, eventdata, handles)
% hObject    handle to axes1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

global image;
imshow(image)
impixelinfo;


function sequenceChanged(handles)
global curSequence;
global maxSequence;
global curImage;
global maxImage;

set(handles.curSequence, 'String', num2str(curSequence))
set(handles.maxSequence, 'String', num2str(maxSequence))
set(handles.curImage, 'String', num2str(curImage))
set(handles.maxImage, 'String', num2str(maxImage))
