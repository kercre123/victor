
blocks = [3 17 78 90];
faces = [1 9 12];
ctr = 1;
for i = 1:length(blocks) 
    for j = 1:length(faces)
        h_axes = subplot(length(codes),length(blocks),ctr); 
        fiducialPlus2DCodeTest(blocks(i), faces(j), ...
            'circleType', 'docking', 'h_axes', h_axes)
        axis(h_axes, 'off')
        title(h_axes, sprintf('Block %d, Face %d', blocks(i), faces(j)))
        ctr = ctr + 1; 
    end 
end

fix_subplots(length(codes), length(blocks))