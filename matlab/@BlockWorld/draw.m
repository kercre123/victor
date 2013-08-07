function draw(this, varargin)

% AxesWorld = [];
% AxesReproject = [];
% 
% parseVarargin(varargin{:});

% if isempty(AxesWorld)
%     if isempty(AxesProject)
        
AxesWorld = subplot(1,1,1, 'Parent', namedFigure('BlockWorld 3D'));
numCols = 1 + double(this.hasMat);
AxesReproject = zeros(this.numRobots, numCols);
h_fig = namedFigure('BlockWorld Reproject');
for i = 1:this.numRobots
    AxesReproject(i,1) = subplot(this.numRobots,numCols,i, ...
        'Parent', h_fig);
    if this.hasMat
        AxesReproject(i,2) = subplot(this.numRobots, numCols, 2*i, ...
            'Parent', h_fig);
    end
end    
%     end
% end

%% Draw the 3D World
for i_robot = 1:this.numRobots
    draw(this.robots{i_robot}, 'AxesHandle', AxesWorld); 
end

for i_block = 1:this.numBlocks
    draw(this.blocks{i_block}, 'AxesHandle', AxesWorld);
end

axis(AxesWorld, 'equal');
grid(AxesWorld, 'on');

%% Draw the reprojection error

for i_robot = 1:this.numRobots
    draw(this.robots{i_robot}.currentObservation, ...
        'AxesHandle', AxesReproject(i_robot,:));
    
    title(AxesReproject(i_robot,1), sprintf('Robot %d''s View', i_robot));
    
    if this.hasMat
        title(AxesReproject(i_robot,2), sprintf('Robot %d''s Mat View', i_robot));
    end
    
end % FOR each robot

drawnow

end % FUNCTION draw()