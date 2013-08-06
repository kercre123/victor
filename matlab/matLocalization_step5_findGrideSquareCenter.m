function [xcenIndex, ycenIndex] = matLocalization_step5_findGrideSquareCenter(imgRot, squareWidth, pixPerMM, lineWidth, embeddedConversions, DEBUG_DISPLAY)


% Now using stencil that looks for edges of the stripes by using
% derivatives instead.  The goal is to be less sensitive to lighting
x = linspace(-squareWidth,squareWidth,pixPerMM*2*squareWidth);
gridStencil = max(exp(-(x+squareWidth/2).^2/(2*(lineWidth/3)^2)), ...
    exp(-(x-squareWidth/2).^2/(2*(lineWidth/3)^2)));
gridStencil = image_right(gridStencil) - gridStencil; % derivative

% Collect average derivatives along the rows and columns, so we can just do
% simple 1D filtering with the stencil below
colDeriv = mean(image_right(imgRot)-imgRot,1);
rowDeriv = mean(image_down(imgRot)-imgRot,2);

% Find where the derivatives agree best with the stencil
[~,xcenIndex] = max(conv2(colDeriv, gridStencil, 'same'));
[~,ycenIndex] = max(conv2(rowDeriv', gridStencil, 'same'));