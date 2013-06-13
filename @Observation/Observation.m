classdef Observation
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        image; % should only be used for initialization/visualization!
        robot; % handle to parent robot
        
        frame;
        markers = {};
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        numMarkers;
    end
    
    methods(Access = 'public')
        
        function this = Observation(img, parentRobot)
            this.image = img;
            this.markers = simpleDetector(this.image);
            this.robot = parentRobot;
            
            numSeenMarkers = this.numMarkers;
            
            world = this.robot.world;
            if world.numBlocks == 0
                % If we haven't instantiated any blocks in our world, let the world
                % origin start at the Robot's current position and instantiate a block
                % for each marker we detected.
                this.frame = Frame();
                
                for i_marker = 1:numSeenMarkers
                    
                    world.addBlock(this.robot, this.markers{i_marker});
                    
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
                matchedMarkers = false(1,numSeenMarkers);
                p_marker = cell(1,numSeenMarkers);
                P_marker = cell(1,numSeenMarkers);
                for i_marker = 1:numSeenMarkers
                    bType = this.markers{i_marker}.blockType;
                    if bType > world.MaxBlocks
                        warning('Out-of-range block detected! (%d > %d)', bType, this.MaxBlocks);
                        keyboard
                    end
                    
                    if ~isempty(world.blocks{bType})
                        matchedMarkers(i_marker) = true;
                        p_marker{i_marker} = this.markers{i_marker}.corners;
                        
                        B = world.blocks{bType};
                        M = B.getFaceMarker(this.markers{i_marker}.faceType);
                        P_marker{i_marker} = M.P;
                    end
                    
                end
                
                % Use previously-seen markers to update the robot/camera's position
                if ~any(matchedMarkers)
                    warning(['No markers found matched one seen before. Not sure yet ' ...
                        'what to do in this case.']);
                else
                    p_marker = vertcat(p_marker{:});
                    P_marker = vertcat(P_marker{:});
                    invRobotFrame = this.robot.camera.computeExtrinsics( ...
                        p_marker, P_marker);
                    this.frame = inv(invRobotFrame);
                    this.robot.frame = this.frame;
                    
                    % Add new markers/blocks to world, relative to robot's updated position
                    for i_marker = 1:numSeenMarkers
                        if ~matchedMarkers(i_marker)
                            world.addBlock(this.robot, this.markers{i_marker});
                        end
                    end % FOR each marker
                    
                end
                
            end % IF first observation or not
            
        end % CONSTRUCTOR Observation()
        
    end % METHODS public
    
    methods % get/set
        function N = get.numMarkers(this)
            N = length(this.markers);
        end
        
    end
    
end % CLASSDEF Observation