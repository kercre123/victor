function E = refineHelper(tform, mag, canonicalBoundary)


tform = maketform('projective', tform);

[xi, yi] = tforminv(tform, canonicalBoundary(:,1), canonicalBoundary(:,2));

E = -sum(interp2(mag,xi,yi,'bilinear', inf));

% namedFigure('refineHelper')
% plot(xi,yi, 'r');
% title(E)
% drawnow

end