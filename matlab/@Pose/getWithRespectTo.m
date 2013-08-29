function P_wrt_other = getWithRespectTo(this, other)
% Find this Pose with respect to a different parent.

from = this;
to   = other;

P_from = Pose(from.Rmat, from.T);
P_to   = Pose(to.Rmat, to.T);

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
    
    P_from = P_from * from.parent;
    from = from.parent;
end

while to.treeDepth > from.treeDepth
    if isempty(to.parent)
        error('The two poses must be part of the same tree.');
    end
    
    P_to = P_to * to.parent;
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
    
    P_from = P_from * from.parent;
    P_to = P_to * to.parent;
    
    to = to.parent;
    from = from.parent;
end

% Now compute the total transformation from this pose, up the "from" path
% in the tree, to the common ancestor, and back down the "to" side to the 
% final other pose.
P_wrt_other = P_from * P_to.inv; 

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

%%
end % FUCNTION test()
