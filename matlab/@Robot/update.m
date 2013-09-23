function update(this, img, matImage)

% Get head camera and lift's current positions:
% TODO: do i need to do this every update? or can it be triggered?
this.updateHeadPose();
this.updateLiftPose(); 

if nargin < 2 || isempty(img)
    this.camera.grabFrame();
else
    this.camera.image = img;
end

this.observationWindow(2:end) = this.observationWindow(1:end-1);

if ~isempty(this.world) && this.world.hasMat
    assert(~isempty(this.matCamera), ...
        'Robot in a BlockWorld with a Mat must have a matCamera defined.');
    
    if nargin < 3 || isempty(matImage)
        this.matCamera.grabFrame();
    else
        this.matCamera.image = matImage;
    end
end

this.observationWindow{1} = Observation(this);

this.world.displayMessage('Robot', sprintf('Mode: %s', this.operationMode));

switch(this.operationMode)
    
    case 'DRIVE_TO_POSITION'
                
        pathComplete = this.followPath();
        if pathComplete
            drive(0,0);
            this.operationMode = '';
        end
        
    case 'DOCK'
               
        if isempty(this.dockingBlock)
            if isempty(this.visibleBlocks)
                warning('No blocks seen, cannot initiate auto-dock.');
            elseif isempty(this.world.selectedBlock)
                warning('No block selected, cannot initate auto-dock.');
            else
                % Stop moving if we were
                this.drive(0,0);
                
                % Until we are done docking, keep using the same
                % block/face, i.e. the one marked as "selected" at the time
                % we got issued the DOCK command.
                this.dockingBlock = this.world.selectedBlock;
                
                % Choose the face closest to our current position as the
                % one we will dock with:
                minDist = inf;
                selected = 0;
                for i_face = 1:this.dockingBlock.numMarkers
                    
                    faceWRTcamera = this.dockingBlock.markers{i_face}.pose.getWithRespectTo(this.camera.pose);
                    
                    % In camera coordinates, Z is the distance from us
                    dist = faceWRTcamera.T(3);
                                        
                    if dist < minDist
                        selected = i_face;
                        minDist = dist;
                    end
                end
                assert(selected > 0, 'No closest face selected!');
                this.dockingFace = this.dockingBlock.markers{selected};
                
                this.virtualBlock = Block(this.dockingBlock.blockType, 0);
                this.virtualFace  = this.virtualBlock.getFaceMarker(this.dockingFace.faceType);
                
                % Lower the lift to get it out of the way of the camera,
                % if we're not carrying a block (in which case the lift
                % should be up)
                if this.isBlockLocked && this.liftAngle < this.LiftAngle_High
                    this.operationMode = 'MOVE_LIFT';
                    this.liftAngle = this.LiftAngle_High;
                    this.nextOperationMode = 'DOCK';
                                        
                elseif ~this.isBlockLocked && this.liftAngle > this.LiftAngle_Low
                    this.operationMode = 'MOVE_LIFT';
                    this.liftAngle = this.LiftAngle_Low;
                    this.nextOperationMode = 'DOCK';
                    
                end
            end
        else
            
            try
                inPosition = this.dockWithBlockStep();
            catch E
                if any(strcmp(E.identifier, ...
                        {'FindDockingTarget:TooFewLocalMaxima', ...
                        'FindDockingTarget:BadSpacing'}))
                    
                    warning('FindDockingTarget() failed: %s', E.message);
                    
                    % Lost the marker, back up until we can see a full marker so we
                    % can try again
                    this.operationMode = 'BACK_UP_TO_SEE_MARKER';
                    
                    return;
                else
                    rethrow(E)
                end
            end
            
            if inPosition
                % Finished positioning the robot for docking!
                
                % Stop moving
                this.drive(0,0);
                
                if this.isBlockLocked
                    assert(this.liftAngle > this.LiftAngle_Place, ...
                        'Expecting to be carrying block up high.');
                    
                    this.liftAngle = this.LiftAngle_High;
                    this.operationMode = 'MOVE_LIFT';
                    this.nextOperationMode = 'PLACE_BLOCK';
                else
                    if this.dockingBlock.pose.T(3) > this.dockingBlock.mindim
                        % We are attempting to dock with a high ("placed")
                        % block
                        this.liftAngle = this.LiftAngle_Place;
                    else
                        % We are attempting to dock with a low block
                        this.liftAngle = this.LiftAngle_Dock;
                    end
                    
                    this.operationMode = 'MOVE_LIFT';
                    this.nextOperationMode = 'WAIT_FOR_LOCK';
                end
                                               
            end
            
        end
        
    case 'MOVE_LIFT'
        liftInPosition = this.liftControlStep();
        if liftInPosition
            this.operationMode = this.nextOperationMode;
        end
        
    case 'UPDATE_BLOCK_POSE'
        
        this.world.updateObsBlockPose(this.dockingBlock.blockType, ...
            this.dockingBlock.pose, this.world.UnselectedBlockColor);
        
        % Reset for next dock maneuver:
        this.dockingBlock = [];
        this.dockingFace  = [];
        this.virtualBlock = [];
        this.virtualFace  = [];
        
        this.operationMode = '';
        
    case 'PLACE_BLOCK'
        this.liftAngle = this.LiftAngle_Place;
        this.operationMode = 'MOVE_LIFT';
        this.nextOperationMode = 'WAIT_FOR_UNLOCK';
        
    case 'WAIT_FOR_LOCK'
        % Wait for signal that we've attached to the block, then set its
        % pose relative to the lifter, and lift it.
        if this.isBlockLocked
            %desktop
            %keyboard
            
            this.dockingBlock.pose = this.dockingBlock.pose.getWithRespectTo(this.liftPose);
            this.dockingBlock.pose.parent = this.liftPose;
            
            % Raise up the block to carry it, and update it's position once
            % that is done.
            this.liftAngle = this.LiftAngle_High;
            this.operationMode = 'MOVE_LIFT';
            this.nextOperationMode = 'UPDATE_BLOCK_POSE';
            this.world.selectedBlock = [];
        end
        
    case 'WAIT_FOR_UNLOCK'
        this.releaseBlock();
        
        if ~this.isBlockLocked
            
            % Reset for next dock maneuver:
            this.dockingBlock = [];
            this.dockingFace  = [];
            this.virtualBlock = [];
            this.virtualFace  = [];
                
            % Once we've dropped the block, back up to admire our handywork
            this.operationMode = 'BACK_UP_TO_SEE_MARKER';
        end
            
    case 'BACK_UP_TO_SEE_MARKER'
        % Back up until we can see a full marker again
        if isempty(this.visibleBlocks)
            this.drive(-5, -5);
        else
            this.operationMode = '';
        end
        
        
end % SWITCH(operationMode)

end % FUNCTION Robot/update()