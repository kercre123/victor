function update(this, img, matImage)

% Get head camera's current position:
this.updateHeadPose();

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

switch(this.operationMode)
    
    case 'DOCK'
        
        fprintf('Executing auto-docking control...\n');
        
        if isempty(this.dockingBlock)
            if isempty(this.visibleBlocks)
                warning('No blocks seen, cannot initiate auto-dock.');
            else
                if length(this.visibleBlocks) > 1
                    warning('Attempting to dock, but more than one block visible - using first.');
                end
                
                % Stop moving if we were
                this.drive(0,0);
                
                % Until we are done docking, keep using the same block/face:
                % TODO: smarter way of choosing which block/face to dock with
                this.dockingBlock = this.visibleBlocks{1};
                this.dockingFace  = this.dockingBlock.getFaceMarker(this.visibleFaces{1});
                this.virtualBlock = Block(this.dockingBlock.blockType, 0);
                this.virtualFace  = this.virtualBlock.getFaceMarker(this.dockingFace.faceType);
                
                % Lower the lift to get it out of the way of the camera,
                % if we're not carrying a block (in which case the lift
                % should be up)
                if this.isBlockLocked
                    this.liftPosition = 'UPUP';
                else
                    this.liftPosition = 'DOWN';
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
                
                % Put the lift angle in the docking position
                switch(this.liftPosition)
                    case 'DOWN'
                        this.liftPosition = 'DOCK';
                        this.operationMode = 'WAIT_FOR_LOCK';
                        
                    case 'UP'
                        % Lift already up, ready for docking with high
                        % block
                        assert(~this.isBlockLocked, ...
                            'Not expecting to be carrying a block right now.');
                        this.operationMode = 'WAIT_FOR_LOCK';
                        
                    case {'UPUP', 'UP_PLUS'}
                        if this.isBlockLocked
                            % Carrying block up high, next we wanna put
                            % this block down
                            this.liftPosition = 'UP';
                            this.operationMode = 'WAIT_FOR_UNLOCK';
                            this.releaseBlock();
                        end
                        
                    otherwise
                        error('Not sure where to put lift!');
                end
                               
            end
            
        end
        
    case 'WAIT_FOR_LOCK'
        % Wait for signal that we've attached to the block, then lift it
        if this.isBlockLocked
            this.operationMode = 'LIFT';
        end
        
    case 'WAIT_FOR_UNLOCK'
        if ~this.isBlockLocked
            % Once we've dropped the block, back up to admire our handywork
           this.operationMode = 'BACK_UP_TO_SEE_MARKER'; 
        end
            
    case 'LIFT'
        
        this.liftPosition = 'UP_PLUS';
        this.operationMode = '';
        
    case 'BACK_UP_TO_SEE_MARKER'
        % Back up until we can see a full marker again
        if isempty(this.visibleBlocks)
            this.drive(-5, -5);
        else
            this.operationMode = '';
        end
        
        
end % SWITCH(operationMode)

end % FUNCTION Robot/update()