function makeWebotMat

fprintf('Creating mat... (this can take a minute or so) ...');
gridMat('matSize', 1000, 'lineWidth', 1.5, ...
    'mirrorX', true, 'mirrorY', true);

set(gca, 'Units', 'norm', 'Pos', [0 0 1 1]);
axis tight off
set(gcf, 'Units', 'pixels', 'Pos', [600 550 512 512], ...
    'PaperPositionMode', 'auto', 'Color', 'w');

tempImgName = fullfile(tempdir, 'gridMatInit.png');
print('-dpng', '-r1200', tempImgName);

mat = imread(tempImgName);
mat = imresize(mat, [4096 4096], 'lanczos3');

imwrite(mat, fullfile('worlds', 'mat.png'));

fprintf(' Done!\n');
end