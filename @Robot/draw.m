function draw(this, varargin)

AxesHandle = [];
BodyColor = [.7 .7 0];
BodyHeight = 30;
BodyWidth = 60;
BodyLength = 75;

WheelWidth = 15;
WheelRadius = 15;
WheelColor = 0.25*ones(1,3);

EyeLength = .65*BodyLength;
EyeRadius = .47*BodyWidth/2;
EyeColor = [.5 0 0];

Position = this.position;
Rotation = this.orientation;

parseVarargin(varargin{:});

if isempty(AxesHandle)
    AxesHandle = gca;
end

% Canonical block coordinates
% Note that Z coords are negative because they go backward from marker on
% the face.
cubeX = [0 1 1 0; 0 1 1 0; 0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0]' - .5;
cubeY = [0 0 1 1; 0 0 1 1; 0 0 1 1; 0 0 1 1; 1 1 1 1; 0 0 0 0]' - .5;
cubeZ = [0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0; 0 0 1 1; 0 0 1 1]' - .5;

bodyX = BodyWidth * cubeX;
bodyY = BodyLength * cubeY;
bodyZ = BodyHeight * cubeZ +.5*WheelRadius+.5*BodyHeight;
 
% this.position = [200 300 0];
% this.orientation = [0 0 pi/30];
 
R = Rotation;
if isvector(R)
    R = rodrigues(R);
end
t = Position;

initHandles = isempty(this.handles) || ...
    get(this.handles.body, 'Parent') ~= AxesHandle;

body = rotateAndTranslate(bodyX, bodyY, bodyZ, R, t);
if initHandles
    this.handles.body = patch(body{:}, BodyColor, ...
        'FaceColor', BodyColor, 'EdgeColor', 0.5*BodyColor, 'Parent', AxesHandle);
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

wheelX = WheelWidth * cylZ;
wheelY = WheelRadius * cylX;
wheelZ = WheelRadius * cylY;

wheelX_L = wheelX + .5*BodyWidth + .5*WheelWidth;
wheelY_F = wheelY + .5*BodyLength - WheelRadius;
wheelX_R = wheelX - .5*BodyWidth - .5*WheelWidth;
wheelY_B = wheelY - .5*BodyLength + WheelRadius;
wheelZ   = wheelZ + WheelRadius;

tire{1} = rotateAndTranslate(wheelX_L, wheelY_F, wheelZ, R, t);
tire{2} = rotateAndTranslate(wheelX_R, wheelY_F, wheelZ, R, t);
tire{3} = rotateAndTranslate(wheelX_L, wheelY_B, wheelZ, R, t);
tire{4} = rotateAndTranslate(wheelX_R, wheelY_B, wheelZ, R, t);
if initHandles
    for i = 1:4
        this.handles.tire(i) = surf(tire{i}{:}, WheelColor, 'Parent', AxesHandle);
        this.handles.hub(i) = patch(tire{i}{1}(2,:), tire{i}{2}(2,:), tire{i}{3}(2,:), WheelColor, 'Parent', AxesHandle);
    end
    set(this.handles.tire, 'FaceColor', WheelColor, 'EdgeColor', .5*WheelColor);
else
    for i = 1:4
        updateHelper(this.handles.tire(i), tire{i});
        updateHelper(this.handles.hub(i), tire{i}, 2);
    end
end

eyeX = EyeRadius * cylX ;
eyeX_L = eyeX - EyeRadius;
eyeX_R = eyeX + EyeRadius;
eyeY = EyeLength * cylZ + BodyLength/2 - EyeLength/3;
eyeZ = EyeRadius * cylY + EyeRadius + BodyHeight + .5*WheelRadius;

eyeL = rotateAndTranslate(eyeX_L, eyeY, eyeZ, R, t);
eyeR = rotateAndTranslate(eyeX_R, eyeY, eyeZ, R, t);
if initHandles
    this.handles.eye(1) = surf(eyeL{:}, EyeColor, 'Parent', AxesHandle);
    this.handles.eye(2) = surf(eyeR{:}, EyeColor, 'Parent', AxesHandle);
    set(this.handles.eye, 'FaceColor', EyeColor, 'EdgeColor', 'none', 'Parent', AxesHandle);
else
    updateHelper(this.handles.eye(1), eyeL);
    updateHelper(this.handles.eye(2), eyeR);
end


eyeWhiteX_L = eyeX_L;
eyeWhiteX_R = eyeX_R;
eyeWhiteY = eyeY - .1*EyeLength;
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

pupilX_L = .5*EyeRadius * cylX - EyeRadius ;
pupilX_R = .5*EyeRadius * cylX + EyeRadius ;
pupilY = eyeY - .09*EyeLength;
pupilZ = .5*EyeRadius * cylY + EyeRadius + BodyHeight + .5*WheelRadius;

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

