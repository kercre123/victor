% For analyzing pickup verification snapshots taken from the robot

clear;

beforeName = 'robot32_img100.pgm';
afterName = 'robot32_img101.pgm';

before = imread(beforeName);
after = imread(afterName);

maxbefore = max(before(:));
maxafter = max(after(:));

beforeNorm = before * (255.0/double(maxbefore));
afterNorm = after * (255.0/double(maxafter));

% Show normalized before and after images
figure(1); imshow(beforeNorm);
figure(2); imshow(afterNorm);

beforeNorm = double(beforeNorm);
afterNorm = double(afterNorm);

diff = beforeNorm - afterNorm;
diffSq = diff.*diff;

% This is the final difference value we're comparing to the threshold
diffSqSum = sum(diffSq(:))

% Diff image
absdiff = abs(diff);
figure(3); imshow(uint8(absdiff));