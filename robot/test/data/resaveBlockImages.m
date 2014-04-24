
function resaveBlockImages()

im50 = imread('C:\Anki\blockImages\blockImages00050.png');
saveImageAsHeader(rgb2gray(im50), 'C:\Anki\products-cozmo\coretech\vision\blockImages\blockImage50.h', 'blockImage50');
saveImageAsHeader(imresize(rgb2gray(im50),[240,320]), 'C:\Anki\products-cozmo\coretech\vision\blockImages\blockImage50_320x240.h', 'blockImage50_320x240');

im189 = imread('C:\Anki\blockImages\blockImages00189.png');
saveImageAsHeader(imresize(rgb2gray(im189),[60,80]), 'C:\Anki\products-cozmo\coretech\vision\blockImages\blockImages00189_80x60.h', 'blockImages00189_80x60');

im190 = imread('C:\Anki\blockImages\blockImages00190.png');
saveImageAsHeader(imresize(rgb2gray(im190),[60,80]), 'C:\Anki\products-cozmo\coretech\vision\blockImages\blockImages00190_80x60.h', 'blockImages00190_80x60');