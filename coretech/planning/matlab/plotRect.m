% rect is a single column containing x then y of one edge of a
% rectangle, then the length of the other dimension

function P = plotRect(rect)

% first two points
P = [rect(1:2)'; rect(3:4)'];

% perpendicular (unit) vector to the first line
perp = [rect(2), rect(3)] - [rect(4), rect(1)];
perp = perp / norm(perp);

P(3,:) = P(2,:) + rect(5) * perp;
P(4,:) = P(1,:) + rect(5) * perp;

plotP = P;
plotP(5,:) = P(1,:);

plot(plotP(:,1), plotP(:,2), 'b-');

end
