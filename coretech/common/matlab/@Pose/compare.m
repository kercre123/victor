function d = compare(P1, P2)
% Compute the "distance" between two Poses.

% TODO: incorporate uncertainty.

% For now, just compare the translations:
if iscell(P2)
    N = numel(P2);
    d = zeros(size(P2));
    for i = 1:N
        d(i) = sqrt(sum( (P1.T - P2{i}.T).^2 ));
    end
else
    d = sqrt(sum( (P1.T - P2.T).^2 ));
end

end % FUNCTION compare()