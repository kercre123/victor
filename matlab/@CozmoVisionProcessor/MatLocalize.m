function MatLocalize(this, img)
%{
// [xMat, yMat, orient] = matLocalization(this.matImage, ...
    //   'pixPerMM', pixPerMM, 'camera', this.robot.matCamera, ...
    //   'matSize', world.matSize, 'zDirection', world.zDirection, ...
    //   'embeddedConversions', this.robot.embeddedConversions);

// % Set the pose based on the result of the matLocalization
// this.pose = Pose(orient*[0 0 -1], ...
    // [xMat yMat this.robot.appearance.WheelRadius]);
//    this.pose.name = 'ObservationPose';
%}

error('Not finished implementing MatLocalize in CozmoVisionProcessor yet!');

matMarker = matLocalization(img, ...
   'pixPerMM', this.pixPerMM, 'returnMarkerOnly', true); 
matOrient = matMarker.upAngle; 

xMatSquare = matMarker.X; 
yMatSquare = matMarker.Y; 
centroid = matMarker.centroid; 
xImgCen = centroid(1); 
yImgCen = centroid(2); 
matUpDir = matMarker.upDirection;

mxArray *mx_isValid = engGetVariable(matlabEngine_, "isMatMarkerValid");
const bool matMarkerIsValid = mxIsLogicalScalarTrue(mx_isValid);

if matMarker.isValid
    
        CozmoMsg_ObservedMatMarker msg;
        msg.size = sizeof(CozmoMsg_ObservedMatMarker);
        msg.msgID = MSG_V2B_CORE_MAT_MARKER_OBSERVED;
        
        mxArray *mx_xMatSquare = engGetVariable(matlabEngine_, "xMatSquare");
        mxArray *mx_yMatSquare = engGetVariable(matlabEngine_, "yMatSquare");
        
        msg.x_MatSquare = static_cast<u16>(mxGetScalar(mx_xMatSquare));
        msg.y_MatSquare = static_cast<u16>(mxGetScalar(mx_yMatSquare));
        
        mxArray *mx_xImgCen = engGetVariable(matlabEngine_, "xImgCen");
        mxArray *mx_yImgCen = engGetVariable(matlabEngine_, "yImgCen");
        
        msg.x_imgCenter = static_cast<f32>(mxGetScalar(mx_xImgCen));
        msg.y_imgCenter = static_cast<f32>(mxGetScalar(mx_yImgCen));
        
        mxArray *mx_upDir = engGetVariable(matlabEngine_, "matUpDir");
        msg.upDirection = static_cast<u8>(mxGetScalar(mx_upDir)) - 1; % Note the -1 for C vs. Matlab indexing
        
        mxArray *mx_matAngle = engGetVariable(matlabEngine_, "matOrient");
        msg.angle = static_cast<f32>(mxGetScalar(mx_matAngle));
        
        PRINT(['Sending ObservedMatMarker message: Square (%d,%d) at ' ...
            '(%.1f,%.1f) with orientation %.1f degrees and upDirection=%d\n'], ...
            msg.x_MatSquare, msg.y_MatSquare, ...
            msg.x_imgCenter, msg.y_imgCenter, ...
            msg.angle * (180/M_PI), msg.upDirection);
        
        matMarkerMailbox_.putMessage(msg);

end

end % FUNCTION MatLocalize()