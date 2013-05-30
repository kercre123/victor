namedFigure('Code Sheet', 'Color', 'w')

blocks = [3 17 78 90];
faces = [1 9 12];
ctr = 1;
for i = 1:length(blocks) 
    for j = 1:length(faces)
        h_axes = subplot(length(faces),length(blocks),ctr); 
        fiducialPlus2DCodeTest(blocks(i), faces(j), ...
            'fiducialType', 'square', 'h_axes', h_axes, ...
            'fiducialSize', 3, 'borderSpacing', 3, ...
            'centerTargetType', 'circle')
        axis(h_axes, 'off')
        title(h_axes, sprintf('Block %d, Face %d', blocks(i), faces(j)))
        ctr = ctr + 1; 
    end 
end

fix_subplots(length(faces), length(blocks))