%imshows(images)
%imshows(im1,im2,im3); %for seperate figures
%imshows({im1,im2,im3},{im4,im5}); %for subplots

function imshows(varargin)

figNums=1:length(varargin);

for i=1:length(varargin)
    figure(figNums(i));
    curIms=varargin{i};
    if iscell(curIms)
        for j=1:length(curIms)
           subplot(1,length(curIms),j); imshow(curIms{j}); 
        end
    else
        imshow(curIms, 'Border', 'tight');
    end
    impixelinfo;
end