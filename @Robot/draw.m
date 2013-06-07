function draw(this, varargin)

AxesHandle = [];
Position = this.frame.T;
Rotation = this.frame.Rmat;

parseVarargin(varargin{:});

if isempty(AxesHandle)
    AxesHandle = gca;
end

app = this.appearance;

% Canonical block coordinates
% Note that Z coords are negative because they go backward from marker on
% the face.
cubeX = [0 1 1 0; 0 1 1 0; 0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0]' - .5;
cubeY = [0 0 1 1; 0 0 1 1; 0 0 1 1; 0 0 1 1; 1 1 1 1; 0 0 0 0]' - .5;
cubeZ = [0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0; 0 0 1 1; 0 0 1 1]' - .5;

bodyX = app.BodyWidth * cubeX;
bodyY = app.BodyLength * cubeY;
bodyZ = app.BodyHeight * cubeZ +.5*app.WheelRadius+.5*app.BodyHeight;
 
% this.position = [200 300 0];
% this.orientation = [0 0 pi/30];
 
R = Rotation;
if isvector(R)
    R = rodrigues(R);
end
t = Position;

initHandles = isempty(this.handles) || ~ishandle(this.handles.body) || ...
    get(this.handles.body, 'Parent') ~= AxesHandle;

body = rotateAndTranslate(bodyX, bodyY, bodyZ, R, t);
if initHandles
    this.handles.body = patch(body{:}, app.BodyColor, ...
        'FaceColor', app.BodyColor, 'EdgeColor', 0.5*app.BodyColor, ...
        'Parent', AxesHandle);
else
    updateHelper(this.handles.body, body);
end

noHold = ~ishold(AxesHandle);
if noHold
    hold(AxesHandle, 'on');
end

% Canonical cylinder:
[cylX, cylY, cylZ] = cylinder([1 1], 50);
cylZ = cylZ - .5;

wheelX = app.WheelWidth * cylZ;
wheelY = app.WheelRadius * cylX;
wheelZ = app.WheelRadius * cylY;

wheelX_L = wheelX + .5*app.BodyWidth + .5*app.WheelWidth;
wheelY_F = wheelY + .5*app.BodyLength - app.WheelRadius;
wheelX_R = wheelX - .5*app.BodyWidth - .5*app.WheelWidth;
wheelY_B = wheelY - .5*app.BodyLength + app.WheelRadius;
wheelZ   = wheelZ + app.WheelRadius;

tire{1} = rotateAndTranslate(wheelX_L, wheelY_F, wheelZ, R, t);
tire{2} = rotateAndTranslate(wheelX_R, wheelY_F, wheelZ, R, t);
tire{3} = rotateAndTranslate(wheelX_L, wheelY_B, wheelZ, R, t);
tire{4} = rotateAndTranslate(wheelX_R, wheelY_B, wheelZ, R, t);
row = [2 1 2 1];
if initHandles
    for i = 1:4
        this.handles.tire(i) = surf(tire{i}{:}, app.WheelColor, 'Parent', AxesHandle);
        this.handles.hub(i) = patch(tire{i}{1}(row(i),:), tire{i}{2}(row(i),:), tire{i}{3}(row(i),:), app.WheelColor, 'Parent', AxesHandle);
    end
    set(this.handles.tire, 'FaceColor', app.WheelColor, 'EdgeColor', .5*app.WheelColor);
else
    for i = 1:4
        updateHelper(this.handles.tire(i), tire{i});
        updateHelper(this.handles.hub(i), tire{i}, row(i));
    end
end

eyeX = app.EyeRadius * cylX ;
eyeX_L = eyeX - app.EyeRadius;
eyeX_R = eyeX + app.EyeRadius;
eyeY = app.EyeLength * cylZ + app.BodyLength/2 - app.EyeLength/3;
eyeZ = app.EyeRadius * cylY + app.EyeRadius + app.BodyHeight + .5*app.WheelRadius;

eyeL = rotateAndTranslate(eyeX_L, eyeY, eyeZ, R, t);
eyeR = rotateAndTranslate(eyeX_R, eyeY, eyeZ, R, t);
if initHandles
    this.handles.eye(1) = surf(eyeL{:}, app.EyeColor, 'Parent', AxesHandle);
    this.handles.eye(2) = surf(eyeR{:}, app.EyeColor, 'Parent', AxesHandle);
    set(this.handles.eye, 'FaceColor', app.EyeColor, 'EdgeColor', 'none', 'Parent', AxesHandle);
else
    updateHelper(this.handles.eye(1), eyeL);
    updateHelper(this.handles.eye(2), eyeR);
end


eyeWhiteX_L = eyeX_L;
eyeWhiteX_R = eyeX_R;
eyeWhiteY = eyeY - .1*app.EyeLength;
eyeWhiteZ = eyeZ;

eyeWhite1 = rotateAndTranslate(eyeWhiteX_L, eyeWhiteY, eyeWhiteZ, R, t);
eyeWhite2 = rotateAndTranslate(eyeWhiteX_R, eyeWhiteY, eyeWhiteZ, R, t);
if initHandles
    this.handles.eyeWhite(1) = patch(eyeWhite1{1}(2,:), eyeWhite1{2}(2,:), ...
        eyeWhite1{3}(2,:), 'w', 'Parent', AxesHandle);
    this.handles.eyeWhite(2) = patch(eyeWhite2{1}(2,:), eyeWhite2{2}(2,:), ...
        eyeWhite2{3}(2,:), 'w', 'Parent', AxesHandle);
else
    updateHelper(this.handles.eyeWhite(1), eyeWhite1, 2);
    updateHelper(this.handles.eyeWhite(2), eyeWhite2, 2);
end

pupilX_L = .5*app.EyeRadius * cylX - app.EyeRadius ;
pupilX_R = .5*app.EyeRadius * cylX + app.EyeRadius ;
pupilY = eyeY - .09*app.EyeLength;
pupilZ = .5*app.EyeRadius * cylY + app.EyeRadius + app.BodyHeight + .5*app.WheelRadius;

pupil1 = rotateAndTranslate(pupilX_L(2,:), pupilY(2,:), pupilZ(2,:), R, t);
pupil2 = rotateAndTranslate(pupilX_R(2,:), pupilY(2,:), pupilZ(2,:), R, t);
if initHandles
    this.handles.eyePupil(1) = patch(pupil1{:}, 'k', 'Parent', AxesHandle);
    this.handles.eyePupil(2) = patch(pupil2{:}, 'k', 'Parent', AxesHandle);
else
    updateHelper(this.handles.eyePupil(1), pupil1);
    updateHelper(this.handles.eyePupil(2), pupil2);
end
      
%axis(AxesHandle, 'equal')

if noHold
    hold(AxesHandle, 'off');
end

end


function out = rotateAndTranslate(Xin, Yin, Zin, R, t)

Xout = R(1,1)*Xin + R(1,2)*Yin + R(1,3)*Zin + t(1);
Yout = R(2,1)*Xin + R(2,2)*Yin + R(2,3)*Zin + t(2);
Zout = R(3,1)*Xin + R(3,2)*Yin + R(3,3)*Zin + t(3);

out = {Xout, Yout, Zout};

end

function updateHelper(h, data, row)
if nargin < 3
    set(h, 'XData', data{1}, 'YData', data{2}, 'ZData', data{3});
else
    set(h, 'XData', data{1}(row,:), 'YData', data{2}(row,:), 'ZData', data{3}(row,:));
end
end

