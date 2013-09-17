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
                    this.setLiftPosition('UP');
                else
                    this.setLiftPosition('DOWN');
                end
            end
        else
            
            inPosition = this.dockWithBlockStep();
            
            if inPosition
                % Finished positioning the robot for docking!
                
                % Stop moving
                this.drive(0,0);
                
                % Put the lift angle in the docking position
                this.setLiftPosition('DOCK');
                                
                % Reset the virtual/docking block stuff to turn off docking mode
                this.operationMode = 'WAIT_FOR_LOCK';
               
            end
            
        end
        
    case 'WAIT_FOR_LOCK'
        % Wait for signal that we've attached to the block, then lift it up
        if this.isBlockLocked
            this.operationMode = 'LIFT';
        end
        
    case 'LIFT'
               
        this.setLiftPosition('UP');
        this.operationMode = '';
        
        
end % SWITCH(operationMode)

end % FUNCTION Robot/update()