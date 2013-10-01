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
        
    case 'INITIATE_DOCK'
        
        if isempty(this.visibleBlocks)
            warning('No blocks seen, cannot initiate auto-dock.');
            this.operationMode = '';
            
        elseif isempty(this.world.selectedBlock)
            warning('No block selected, cannot initate auto-dock.');
            this.operationMode = '';
            
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
            
            % Now that we've assigned the virtual docking block, we
            % need to figure out where we want to see it
            this.computeVirtualBlockPose();
            
            % Lower the lift to get it out of the way of the camera,
            % if we're not carrying a block (in which case the lift
            % should be up)
            if this.isBlockLocked && this.liftAngle < this.LiftAngle_High
                this.operationMode = 'MOVE_LIFT';
                this.liftAngle = this.LiftAngle_High;
                this.nextOperationMode = 'PRE-DOCK';
                
            elseif ~this.isBlockLocked && this.liftAngle > this.LiftAngle_Low
                this.operationMode = 'MOVE_LIFT';
                this.liftAngle = this.LiftAngle_Low;
                this.nextOperationMode = 'PRE-DOCK';
            else
                this.operationMode = 'PRE-DOCK';
            end
            
        end % IF any blocks visible
        
    case 'PRE-DOCK'
        
        %         % Pre-compute the pre-dock position/orientation using current
        %         % estimate of block's pose.
        %
        %         % Project the docking position of the face of the block we are
        %         % docking with onto the ground:
        %         preDockPose = this.dockingFace.preDockPose.getWithRespectTo('World');
        %         this.xPreDock = preDockPose.T(1);
        %         this.yPreDock = preDockPose.T(2);
        %
        %         blockPose = this.dockingBlock.pose.getWithRespectTo('World');
        %         this.orientPreDock = atan2(blockPose.T(2) - preDockPose.T(2), ...
        %             blockPose.T(1) - preDockPose.T(1));
        
        this.operationMode = 'PRE-DOCK_POSITION';
            
    case 'PRE-DOCK_POSITION'
        % Move to the pre-dock _position_ first.
        
        % Update the position based on most current block pose estimate:
        preDockPose = this.dockingFace.preDockPose.getWithRespectTo('World');
        this.xPreDock = preDockPose.T(1);
        this.yPreDock = preDockPose.T(2);
        
        distanceThreshold = 3; % mm
               
        inPosition = this.goToPosition(this.xPreDock, this.yPreDock, ...
            distanceThreshold);
        
        if inPosition
            this.operationMode = 'PRE-DOCK_ORIENTATION';            
        end
        
    case 'PRE-DOCK_ORIENTATION'
        % Face the block once we are in pre-dock position.
        
        % Update the orientation 
        preDockPose = this.dockingFace.preDockPose.getWithRespectTo('World');
                
        blockPose = this.dockingBlock.pose.getWithRespectTo('World');
        this.orientPreDock = atan2(blockPose.T(2) - preDockPose.T(2), ...
            blockPose.T(1) - preDockPose.T(1));
        
        % Once we are in position, re-orient ourselves to face the block.
        K_turn = 2;
        headingThreshold = 1; % degrees
        
        [leftMotorVelocity, rightMotorVelocity, headingError] = computeTurnVelocity(...
            this, this.orientPreDock, this.headingAngle, K_turn);
        
        if abs(headingError) < headingThreshold*pi/180
            this.operationMode = 'DOCK';
        else
            this.drive(leftMotorVelocity, rightMotorVelocity);
        end
            
    case 'DOCK'
        
        assert(~isempty(this.dockingBlock) && ~isempty(this.dockingFace) && ...
            ~isempty(this.virtualBlock) && ~isempty(this.virtualFace), ...
            'Docking and Virtual block/face should be set by now.');
        
        try
            inPosition = this.dockWithBlockStep();
        catch E
            if any(strcmp(E.identifier, ...
                    {'FindDockingTarget:TooFewLocalMaxima', ...
                    'FindDockingTarget:BadSpacing', ...
                    'FindDockingTarget:DuplicateMaximaSelected'}))
                
                warning('FindDockingTarget() failed: %s', E.message);
                
                % Lost the marker, back up until we can see a full marker so we
                % can try again
                this.operationMode = 'BACK_UP_TO_SEE_MARKER';
                
                return;
            else
                rethrow(E)
            end
            
        end % TRY/CATCH dockWithBlockStep()
        
        if inPosition
            % Finished positioning the robot for docking!
            
            % Stop moving
            this.drive(0,0);
            
            % Move lift up or down, depending on whether we are docking
            % with a high/low block or placing a block we are carrying.
            if this.isBlockLocked
                assert(this.liftAngle > this.LiftAngle_Place, ...
                    'Expecting to be carrying block up high.');
                
                this.operationMode = 'PLACE_BLOCK';
                
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
            
        end % IF in docking position

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