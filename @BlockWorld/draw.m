function draw(this, varargin)

% AxesWorld = [];
% AxesReproject = [];
% 
% parseVarargin(varargin{:});

% if isempty(AxesWorld)
%     if isempty(AxesProject)
        
AxesWorld = subplot(1,1,1, 'Parent', namedFigure('BlockWorld 3D'));
AxesReproject = zeros(1,this.numRobots);
for i = 1:this.numRobots
    AxesReproject(i) = subplot(this.numRobots,1,i, ...
        'Parent', namedFigure('BlockWorld Reproject'));
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
        'AxesHandle', AxesReproject(i_robot));
    
    title(AxesReproject(i_robot), sprintf('Robot %d''s View', i_robot));
    
end % FOR each robot

drawnow

end % FUNCTION draw()