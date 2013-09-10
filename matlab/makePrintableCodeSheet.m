% Procedure:
% 1. Run this
% 2. Matlab seems to do weird blurring to the image codes, so carefully
%    screen grab the extents of the white axes
% 3. Paste into Preview as a new image
% 4. Print at 100% scaling

markerSize = 3.84; % in cm

% All dims in cm
marginSpacing = 1.25; 
codeSpacing = 1.1; 
pageHeight = 8.5 * 2.54; 
pageWidth  = 11 * 2.54;

namedFigure('Code Sheet', 'Units', 'centimeters')
clf

blocks = [35 40 45 50];
faces = [1 5 8 10];

rowPos = marginSpacing;
colPos = marginSpacing;

h_axes = zeros(1 + length(blocks)*length(faces));
h_axes(1) = axes('Units', 'centimeters', 'Pos', [0 0 pageWidth pageHeight], ...
    'Color', 'w', 'XTick', [], 'YTick', [], ...
    'XGrid', 'on', 'YGrid', 'on', 'Box', 'on');

ctr = 2;
for i = 1:length(blocks) 
    for j = 1:length(faces)
        %h_axes = subplot(length(faces),length(blocks),ctr); 
        h_axes(ctr) = axes('Units', 'centimeters', ...
            'Pos', [colPos rowPos markerSize markerSize]);
        
        generateMarkerImage(blocks(i), faces(j), ...
            'targetSize', markerSize, ...
            'fiducialType', 'square', 'h_axes', h_axes(ctr), ...
            'fiducialSize', .4, 'borderSpacing', .3, ...
            'centerTargetType', 'dots')
        axis(h_axes(ctr), 'off')
        title(h_axes(ctr), sprintf('Block %d, Face %d', blocks(i), faces(j)))
        
        if j < length(faces) || i < length(blocks)
            ctr = ctr+1;
            colPos = colPos + markerSize + codeSpacing;
            if (colPos+markerSize) > (pageWidth-marginSpacing)
                colPos = marginSpacing;
                rowPos = rowPos + markerSize + codeSpacing;
                if (rowPos+markerSize) > (pageHeight-marginSpacing)
                    error('Overflowing page: adjust spacing/margins.');
                end
            end
        end
        
    end 
end

for i = 1:length(h_axes)
    pos = get(h_axes(i), 'Pos');
    set(h_axes(i), 'Pos', [pos(1:2)+1 pos(3:4)]);
end


%fix_subplots(length(faces), length(blocks))

%figure_margin([.1 .1 .8 .8])