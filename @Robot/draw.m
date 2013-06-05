function h = draw(this, varargin)

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

body = rotateAndTranslate(bodyX, bodyY, bodyZ, R, t);
h_body = patch(body{:}, BodyColor, ...
    'FaceColor', BodyColor, 'EdgeColor', 0.5*BodyColor, 'Parent', AxesHandle);

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

tire = rotateAndTranslate(wheelX_L, wheelY_F, wheelZ, R, t);
h_tire(1) = surf(tire{:}, WheelColor, 'Parent', AxesHandle);
h_hub(1) = patch(tire{1}(2,:), tire{2}(2,:), tire{3}(2,:), WheelColor, 'Parent', AxesHandle);

tire = rotateAndTranslate(wheelX_R, wheelY_F, wheelZ, R, t);
h_tire(2) = surf(tire{:}, WheelColor, 'Parent', AxesHandle);
h_hub(2) = patch(tire{1}(1,:), tire{2}(1,:), tire{3}(1,:), WheelColor, 'Parent', AxesHandle);

tire = rotateAndTranslate(wheelX_L, wheelY_B, wheelZ, R, t);
h_tire(3) = surf(tire{:}, WheelColor, 'Parent', AxesHandle);
h_hub(3) = patch(tire{1}(2,:), tire{2}(2,:), tire{3}(2,:), WheelColor, 'Parent', AxesHandle);

tire = rotateAndTranslate(wheelX_R, wheelY_B, wheelZ, R, t);
h_tire(4) = surf(tire{:}, WheelColor, 'Parent', AxesHandle);
h_hub(4) = patch(tire{1}(1,:), tire{2}(1,:), tire{3}(1,:), WheelColor, 'Parent', AxesHandle);

set(h_tire, 'FaceColor', WheelColor, 'EdgeColor', .5*WheelColor);

eyeX = EyeRadius * cylX ;
eyeX_L = eyeX - EyeRadius;
eyeX_R = eyeX + EyeRadius;
eyeY = EyeLength * cylZ + BodyLength/2 - EyeLength/3;
eyeZ = EyeRadius * cylY + EyeRadius + BodyHeight + .5*WheelRadius;

eye = rotateAndTranslate(eyeX_L, eyeY, eyeZ, R, t);
h_eye(1) = surf(eye{:}, EyeColor, 'Parent', AxesHandle);

eye = rotateAndTranslate(eyeX_R, eyeY, eyeZ, R, t);
h_eye(2) = surf(eye{:}, EyeColor, 'Parent', AxesHandle);
    
set(h_eye, 'FaceColor', EyeColor, 'EdgeColor', 'none', 'Parent', AxesHandle);

eyeWhiteX_L = eyeX_L;
eyeWhiteX_R = eyeX_R;
eyeWhiteY = eyeY - .1*EyeLength;
eyeWhiteZ = eyeZ;

eyeWhite = rotateAndTranslate(eyeWhiteX_L, eyeWhiteY, eyeWhiteZ, R, t);
h_eyeWhite(1) = patch(eyeWhite{1}(2,:), eyeWhite{2}(2,:), eyeWhite{3}(2,:), 'w', 'Parent', AxesHandle);

eyeWhite = rotateAndTranslate(eyeWhiteX_R, eyeWhiteY, eyeWhiteZ, R, t);
h_eyeWhite(2) = patch(eyeWhite{1}(2,:), eyeWhite{2}(2,:), eyeWhite{3}(2,:), 'w', 'Parent', AxesHandle);

pupilX_L = .5*EyeRadius * cylX - EyeRadius ;
pupilX_R = .5*EyeRadius * cylX + EyeRadius ;
pupilY = eyeY - .09*EyeLength;
pupilZ = .5*EyeRadius * cylY + EyeRadius + BodyHeight + .5*WheelRadius;

pupil = rotateAndTranslate(pupilX_L(2,:), pupilY(2,:), pupilZ(2,:), R, t);
h_eyePupil(1) = patch(pupil{:}, 'k', 'Parent', AxesHandle);
pupil = rotateAndTranslate(pupilX_R(2,:), pupilY(2,:), pupilZ(2,:), R, t);
h_eyePupil(2) = patch(pupil{:}, 'k', 'Parent', AxesHandle);
    
axis(AxesHandle, 'equal')

if noHold
    hold(AxesHandle, 'off');
end

if nargout > 0
   h = [h_body; h_tire(:); h_hub(:); h_eye(:); h_eyeWhite(:); h_eyePupil(:)]; 
end

end


function out = rotateAndTranslate(Xin, Yin, Zin, R, t)

Xout = R(1,1)*Xin + R(1,2)*Yin + R(1,3)*Zin + t(1);
Yout = R(2,1)*Xin + R(2,2)*Yin + R(2,3)*Zin + t(2);
Zout = R(3,1)*Xin + R(3,2)*Yin + R(3,3)*Zin + t(3);

out = {Xout, Yout, Zout};

end

