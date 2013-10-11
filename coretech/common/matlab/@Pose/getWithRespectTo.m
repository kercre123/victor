function P_wrt_other = getWithRespectTo(this, other)
% Find this Pose with respect to a different parent.

% if ~isa(other, 'Pose') && isobject(other) && isprop(other, 'pose')
%     other = other.pose;
% end

from = this;

if Pose.isRootPose(other)  
    
    P_wrt_other = Pose(from.Rmat, from.T);
    
    while ~Pose.isRootPose(from.parent)
        if isempty(from.parent)
            error('The two poses must be part of the same tree.');
        end
        
        P_wrt_other = from.parent * P_wrt_other;
        from = from.parent;
    end
    
else
    assert(isa(other, 'Pose'), ...
        'Expecting another Pose object to get new pose with respect to.');
    
    P_from = Pose(from.Rmat, from.T);
    
    to   = other;
    P_to = Pose(to.Rmat, to.T);
    
    % First make sure we are pointing at two nodes of the same tree depth,
    % which is the only way they could possibly share the same parent.
    % Until that's true, walk the deeper node up until it is at the same depth
    % as the shallower node, keeping track of the total transformation along
    % the way.
    % (Only one of the following two loops should run, depending on which node
    % is deeper in the tree)
    while from.treeDepth > to.treeDepth
        if isempty(from.parent)
            error('The two poses must be part of the same tree.');
        end
        
        P_from = from.parent * P_from;
        from = from.parent;
    end
    
    while to.treeDepth > from.treeDepth
        if isempty(to.parent)
            error('The two poses must be part of the same tree.');
        end
        
        P_to = to.parent * P_to;
        to = to.parent;
    end
    
    assert(to.treeDepth == from.treeDepth, ...
        'Expecting matching treeDepths at this point.');
    
    % Now that we are pointing to the nodes of the same depth, keep moving up
    % until those nodes have the same parent, totalling up the transformations
    % along the way
    while to.parent ~= from.parent
        if isempty(from.parent) || isempty(to.parent)
            error('The two poses must be part of the same tree.');
        end
        
        P_from = from.parent * P_from;
        P_to = to.parent * P_to;
        
        to = to.parent;
        from = from.parent;
    end
    
    % Now compute the total transformation from this pose, up the "from" path
    % in the tree, to the common ancestor, and back down the "to" side to the
    % final other pose.
    P_wrt_other = P_to.inv * P_from;
    
end % IF/ELSE other pose is RootPose

% The Pose we are about to return is w.r.t. the "other" pose provided (that
% was the whole point of the exercise!), so set its parent accordingly:
P_wrt_other.parent = other;

end % FUNCTION getTransformTo()

function test %#ok<DEFNU>
%% Test 
% (Click in this cell and press Command+Enter to run)
clf 
A = Block(75, 1); 
B = Block(75, 1); B.pose = Pose(.73*[1 1 0]/sqrt(2), [10 20 30]); B.pose.parent = A.pose;
C = Block(75, 1); C.pose = Pose(-.23*[0 0 1], [-50 -70 -80]); C.pose.parent = A.pose;
D = Block(75, 1); D.pose = Pose(-.5*[0 1 0], [25 10 -15]); D.pose.parent = B.pose;
E = Block(75, 1); E.pose = Pose(1.4*[0 1 1]/sqrt(2), [5 100 -20]); E.pose.parent = C.pose;

subplot 131
draw(D, 'FaceColor', 'g');
D2 = Block(75, 1); 
D2.pose = D.pose.getWithRespectTo(B.pose);
draw(D2, 'FaceColor', 'b', 'FaceAlpha', .25);
axis equal

subplot 132
draw(C, 'FaceColor', 'g');
C2 = Block(75, 1);
C2.pose = C.pose.getWithRespectTo(D.pose);
draw(C2, 'FaceColor', 'b', 'FaceAlpha', .25);
axis equal

subplot 133
draw(E, 'FaceColor', 'g');
E2 = Block(75, 1);
E2.pose = E.pose.getWithRespectTo(D.pose);
draw(E2, 'FaceColor', 'b', 'FaceAlpha', .25);
axis equal

%% Test 2
A = Block(75, 1);  
B = Block(75, 1); B.pose = Pose([0 0 0], [0 0 100]); B.pose.parent = A.pose;
C = Block(75, 1); C.pose = Pose([0 0 pi/2], [0 0 100]); C.pose.parent = B.pose;

clf
draw(A, 'FaceColor', 'r');
draw(B, 'FaceColor', 'g', 'FaceAlpha', .35);
draw(C, 'FaceColor', 'b', 'FaceAlpha', .35);

%%
C2 = Block(75, 1); C2.pose = C.pose.getWithRespectTo(A.pose);
draw(C2, 'FaceColor', 'g', 'FaceAlpha', .25);

B2 = Block(75, 1); B2.pose = B.pose.getWithRespectTo(C.pose);
draw(B2, 'FaceColor', 'b', 'FaceAlpha', .25);
axis equal
%%
end % FUCNTION test()
