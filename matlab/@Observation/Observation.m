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
            
            this.markers = simpleDetector(this.image, 'embeddedConversions', parentRobot.embeddedConversions);
            this.robot = parentRobot;
            this.pose = Pose();
            
            world = this.robot.world;
            
            if world.HasMat
                assert(~isempty(parentRobot.matCamera), ...
                    'Robot in a BlockWorld with a Mat must have a matCamera defined.');

                this.matImage = parentRobot.matCamera.image;
            end
            
            % Find blocks we've seen before (we don't have to have seen
            % the individual marker because we know where all markers
            % of a block _should_ be in 3D once we've seen one of them)
            numSeenMarkers = this.numMarkers;
            matchedMarkers = false(1,numSeenMarkers);
            p_marker = cell(1,numSeenMarkers);
            P_marker = cell(1,numSeenMarkers);
            blockTypes = cellfun(@(marker)marker.blockType, this.markers);
            used = false(1,numSeenMarkers);
            
            for i_marker = 1:numSeenMarkers
                bType = blockTypes(i_marker);
                if bType > world.MaxBlocks
                    warning('Out-of-range block detected! (%d > %d)', bType, this.MaxBlocks);
                    keyboard
                end
                
                B = world.getBlock(bType);
                if ~isempty(B)
                    matchedMarkers(i_marker) = true;
                    p_marker{i_marker} = this.markers{i_marker}.corners;
                    
                    P_marker{i_marker} = getPosition( ...
                        B.getFaceMarker(this.markers{i_marker}.faceType));
                end
                
            end
            
            if world.HasMat
                % If there's a mat in the world, use that to update the
                % robot's pose.  Then update any existing blocks and add
                % new ones, relative to that updated position.
                
                % Update robot's pose:
                % TODO: return uncertainty in matLocalization
                [xMat, yMat, orient] = matLocalization(this.matImage, ...
                    'pixPerMM', 18.0, 'camera', this.robot.matCamera, 'embeddedConversions', this.robot.embeddedConversions);
                this.pose = Pose(orient*[0 0 -1], [xMat yMat 0]);
                
                % Also update the parent robot's pose to match (*before* 
                % adding new blocks, whose position will depend on this):
                this.robot.pose = this.pose;
                
                % Add new markers/blocks to world, relative to robot's updated position
                for i_marker = 1:numSeenMarkers
                    
                    % Update existing blocks:
                    if matchedMarkers(i_marker) && ~used(i_marker)
                        whichMarkers = blockTypes == blockTypes(i_marker);
                        world.updateBlock(this.robot, this.markers(whichMarkers));
                        used(whichMarkers) = true;
                    end
                    
                    % Add new blocks:
                    if ~matchedMarkers(i_marker) && ~used(i_marker)
                        whichMarkers = blockTypes == blockTypes(i_marker);
                        world.addBlock(this.robot, this.markers(whichMarkers));
                        used(whichMarkers) = true;
                    end
                end % FOR each marker
                        
                
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
                            world.addBlock(this.robot, this.markers(whichMarkers));
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
                    
                    % Use previously-seen markers to update the robot/camera's position
                    if ~any(matchedMarkers)
                        warning(['No markers found matched one seen before. Not sure yet ' ...
                            'what to do in this case.']);
                        
                        % For now, just leave robot where it was, and pretend
                        % we didn't see anything
                        this.pose = this.robot.pose;
                        this.markers = {};
                    else
                        p_marker = vertcat(p_marker{:});
                        P_marker = vertcat(P_marker{:});
                        invRobotPose = this.robot.camera.computeExtrinsics( ...
                            p_marker, P_marker, 'initializeWithCurrentPose', true);
                        this.pose = inv(invRobotPose);
                                   
                        % Also update the parent robot's pose to match (*before*
                        % adding new blocks, whose position will depend on this):
                        this.robot.pose = this.pose;
                        
                        % Add new markers/blocks to world, relative to robot's updated position
                        for i_marker = 1:numSeenMarkers
                            if ~matchedMarkers(i_marker) && ~used(i_marker)
                                whichMarkers = blockTypes == blockTypes(i_marker);
                                world.addBlock(this.robot, this.markers(whichMarkers));
                                used(whichMarkers) = true;
                            end
                        end % FOR each marker
                        
                    end
                    
                end % IF first observation or not
                
            end % IF world has a Mat for localization
            
        end % CONSTRUCTOR Observation()
        
    end % METHODS public
    
    methods % get/set
        function N = get.numMarkers(this)
            N = length(this.markers);
        end
        
    end
    
end % CLASSDEF Observation