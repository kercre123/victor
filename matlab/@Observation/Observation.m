classdef Observation
    
    properties(GetAccess = 'public', SetAccess = 'public')
        image; % should only be used for initialization/visualization!
        matImage; 
        
        robot; % handle to parent robot
        
        pose;
        markers = {};
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        numMarkers;
    end
    
    methods(Access = 'public')
        
        function this = Observation(parentRobot)
            this.image = parentRobot.camera.image;
            
            this.robot = parentRobot;
            world = this.robot.world;
            this.pose = Pose();
                 
            this.robot.visibleBlocks = {};
            this.robot.visibleFaces = {};

            this.markers = simpleDetector(this.image, ...
                'embeddedConversions', world.embeddedConversions);
            
            if world.hasMat
                assert(~isempty(parentRobot.matCamera), ...
                    'Robot in a BlockWorld with a Mat must have a matCamera defined.');

                this.matImage = parentRobot.matCamera.image;
            end
            
            numSeenMarkers = this.numMarkers;
            used = false(1,numSeenMarkers);
            blockTypes = cellfun(@(marker)marker.blockType, this.markers);
                        
            if world.hasMat
                % If there's a mat in the world, use that to update the
                % robot's pose.  Then update any existing blocks and add
                % new ones, relative to that updated position.
                
                % Update robot's pose:
                % TODO: return uncertainty in matLocalization
                pixPerMM = 18.0;
                if strcmp(this.robot.matCamera.deviceType, 'webot')
                    % Compute the resolution according to the camera's pose
                    % and FOV.  
                    % TODO: this need not be recomputed at every step!
                    %pixPerMM = 22.0;
                    matCamHeightInPix = (this.robot.matCamera.nrows/2) / ...
                        tan(this.robot.matCamera.FOV_vertical/2);
                    
                    pixPerMM = matCamHeightInPix / ...
                        (this.robot.appearance.WheelRadius + ...
                        this.robot.matCamera.pose.T(3));
                end
                [xMat, yMat, orient] = matLocalization(this.matImage, ...
                    'pixPerMM', pixPerMM, 'camera', this.robot.matCamera, ...
                    'matSize', world.matSize, 'zDirection', world.zDirection, ...
                    'embeddedConversions', this.robot.embeddedConversions);
                             
                % Set the pose based on the result of the matLocalization
                this.pose = Pose(orient*[0 0 -1], ...
                    [xMat yMat this.robot.appearance.WheelRadius]);
                this.pose.name = 'ObservationPose';
                
                % Also update the parent robot's pose to match (*before* 
                % adding new blocks, whose position will depend on this):
                this.robot.pose = this.pose;
                                
                % Add new markers/blocks to world or merge existing ones, 
                % relative to robot's updated position
                for i_marker = 1:numSeenMarkers
                        
                    % Pass all unused markers with the same block type to
                    % addOrMergeBlock.  It will handle figuring out how
                    % many of that block type were actually observed and
                    % instantiate as many as needed.
                    if ~used(i_marker)
                        whichMarkers = blockTypes == blockTypes(i_marker);
                        world.addOrUpdateBlock(this.robot, this.markers(whichMarkers));
                        used(whichMarkers) = true;
                    end
                    
                end % FOR each marker
                       
                
                % TODO: For any existing blocks we did NOT see this
                % observation, check to see if we *should* have seen them,
                % given our current pose.  If so, and we didn't see them
                % and update their position above, then it may (?) mean
                % they have been moved and we need to delete them or mark
                % them as having much higher uncertainty b/c we don't know
                % where they are.
                % NOTE: "should have seen them" will also need to take
                % occlusion by other blocks we did see into account!
                
            else % we have no mat, must use previously-seen blocks to infer robot pose
                if world.numBlocks == 0
                    
                    % If we haven't instantiated any blocks in our world, AND
                    % there's no mat to use for robot localization, then let
                    % the world origin start at the Robot's current position
                    % and instantiate a block for each marker we detected.
                                        
                    % In case we see two sides of the same block at once, we
                    % want to pass in all markers of that block together to get
                    % the most accurate estimate of the block's position, using
                    % all observed markers.
                    for i_marker = 1:numSeenMarkers
                        if ~used(i_marker)
                            whichMarkers = blockTypes == blockTypes(i_marker);
                            world.addOrUpdateBlock(this.robot, this.markers(whichMarkers));
                            used(whichMarkers) = true;
                        end
                    end
                    
                else
                    % We've already got world coordinates set up around an
                    % earlier observation
                    
                    % Two cases:
                    % 1. We see at least one block again that we've already seen.  Update
                    %    the robot's position relative to that block's position (or the
                    %    average of where the set of re-detected blocks say the robot is)
                    % 2. We don't see any blocks we've seen before. We could instatiate the
                    %    new blocks based on the last known position of the robot, but this
                    %    seems pretty error-prone, since we presumably moved if we're
                    %    seeing a new block...
                    
                    % Find blocks we've seen before (we don't have to have seen
                    % the individual marker because we know where all markers
                    % of a block _should_ be in 3D once we've seen one of them)
                    numSeenMarkers = this.numMarkers;
                    matchedMarkers2D = cell(1,numSeenMarkers);
                    matchedMarkers3D = cell(1,numSeenMarkers);
                    used = false(1,numSeenMarkers);
                    
                    for i_marker = 1:numSeenMarkers
                        bType = blockTypes(i_marker);
                        
                        if bType > world.MaxBlockTypes
                            warning('Out-of-range block type detected! (%d > %d)', bType, this.MaxBlockTypes);
                            keyboard
                        end
                        
                        if ~isempty(world.blocks{bType})
                            % Create a temporary block of this type and compute
                            % what its position is according to this marker.
                            % If it is "close enough" to an existing block of
                            % the same type, *ASSUME* it is that block,
                            % *ASSUME* that block has not actually moved since
                            % we last observed it, and thus use that block to
                            % help estimate how the robot moved. There is an
                            % implicit assumption of small motion here: the
                            % robot should not have moved too much from the
                            % last time it saw this block AND that block must
                            % not have been moved.
                            B_temp = Block(bType, this.numMarkers);
                            blockPose = BlockWorld.computeBlockPose(...
                                this.robot, B_temp, this.markers(i_marker));
                            minDist = compare(blockPose, world.blocks{bType}{1}.pose);
                            whichBlock = 1;
                            for i_block = 2:length(world.blocks{bType})
                                crntDist = compare(blockPose, world.blocks{bType}{i_block}.pose);
                                if crntDist < minDist
                                    minDist = crntDist;
                                    whichBlock = i_block;
                                end
                            end % FOR each block of this type
                            
                            if minDist < B_temp.mindim
                                used(i_marker) = true;
                                matchedMarkers2D{i_marker} = ...
                                    this.markers{i_marker}.corners;
                                
                                matchedMarkers3D{i_marker} = getPosition( ...
                                    world.blocks{bType}{whichBlock}.getFaceMarker(this.markers{i_marker}.faceType), 'World');
                            end % IF a close-enough block was found
                            
                        end % IF there is already a block of this type
                                                         
                    end % FOR each marker we observed
                    
                    % Use previously-seen markers to update the robot/camera's position
                    if ~any(used)
                        warning(['No markers found matched one seen before. Not sure yet ' ...
                            'what to do in this case.']);
                        
                        % For now, just leave robot where it was, and pretend
                        % we didn't see anything
                        this.pose = this.robot.pose;
                        this.markers = {};
                    else
                        % Use the corresponding 2D and 3D markers for
                        % previously-seend blocks to compute the where the 
                        % robot must have moved to in order to re-see those
                        % blocks' markers in those locations
                        matchedMarkers2D = vertcat(matchedMarkers2D{:});
                        matchedMarkers3D = vertcat(matchedMarkers3D{:});
                        
                        % Compute the marker's pose w.r.t. the camera
                        markerPose_wrt_camera = this.robot.camera.computeExtrinsics( ...
                            matchedMarkers2D, matchedMarkers3D, ...
                            'initializeWithCurrentPose', true);
                        
                        % Now get the robot's pose w.r.t. that marker,
                        % using Pose tree chaining:
                        this.pose = this.robot.pose.getWithRespectTo(markerPose_wrt_camera);
                                   
                        % Also update the parent robot's pose to match (*before*
                        % adding new blocks, whose position will depend on this):
                        this.robot.pose = this.pose;
                        
                        % Add new markers/blocks to world, relative to robot's 
                        % updated position, using all unused markers of
                        % the same block type.
                        for i_marker = 1:numSeenMarkers
                            if ~used(i_marker)
                                whichMarkers = ~used & blockTypes == blockTypes(i_marker);
                                world.addOrUpdateBlock(this.robot, this.markers(whichMarkers));
                                used(whichMarkers) = true;
                            end
                        end % FOR each marker
                        
                    end
                    
                end % IF first observation or not
                
            end % IF world has a Mat for localization
            
            
            %% Update Block Observations
            % Loop over each block we saw and update the observations (e.g. in Webots).
            % Set the one closest to our center of view to be the "selected" block for
            % this robot, with a different color.
            % TODO: when there are multiple robots, we will need a "selected" color for each...
            if ~isempty(this.robot.visibleBlocks)
                
                % If the previously-selected block is not one of the currently 
                % visible blocks, make sure its color gets updated:
                if ~isempty(world.selectedBlock) && ...
                        ~any(cellfun(@(block)isequal(block, world.selectedBlock), ...
                        this.robot.visibleBlocks))
                    
                    fprintf('Unselecting selected block that is no longer visible.\n');
                    
                    world.updateObsBlockPose(world.selectedBlock.blockType, ...
                        world.selectedBlock.pose, world.UnselectedBlockColor);
                end
                
                % Find the visible block closest to center of our field of view:
                minDist = inf;
                selected = 0;
                for i_vis = 1:length(this.robot.visibleBlocks)
                    blockWRTcamera = this.robot.visibleBlocks{i_vis}.pose.getWithRespectTo(this.robot.camera.pose);
                    [u,v] = this.robot.camera.projectPoints(blockWRTcamera.T');
                    dist = (u-this.robot.camera.ncols/2)^2 + (v-this.robot.camera.nrows/2)^2;
                    if dist < minDist
                        selected = i_vis;
                        minDist = dist;
                    end
                end
                
                assert(selected > 0, ...
                    'No block found closest to image center for selection.');
                            
                % Make that block the selected block:
                world.selectedBlock = this.robot.visibleBlocks{selected};
                
                % Update the visible blocks' observed locations, making the
                % selected one have a different color:
                for i_vis = 1:length(this.robot.visibleBlocks)
                    if i_vis == selected
                        color = world.SelectedBlockColor; 
                    else
                        color = world.UnselectedBlockColor; 
                    end
                    
                    world.updateObsBlockPose(this.robot.visibleBlocks{i_vis}.blockType, ...
                        this.robot.visibleBlocks{i_vis}.pose, color);
                end
                
            end % IF any visible blocks
            
            world.displayMessage('observation', ...
                sprintf('Visible: %d Markers, %d Blocks', ...
                numSeenMarkers, length(this.robot.visibleBlocks)))
            
        end % CONSTRUCTOR Observation()
        
    end % METHODS public
    
    methods % get/set
        function N = get.numMarkers(this)
            N = length(this.markers);
        end
        
    end
    
end % CLASSDEF Observation