%% Build binomial kernels
N = 12;
numPlotRows = round(sqrt(N));
numPlotCols = ceil(N/numPlotRows);

binomialKernel = cell(1, N);

binomialKernel{1} = [1 1];
for i = 2:N
    binomialKernel{i} = conv(binomialKernel{i-1}, binomialKernel{1});
end

%% Compare to Gaussians
sigma = zeros(1,N);
g = cell(1, N);
for i = 1:N
    width = length(binomialKernel{i});
    x = linspace(-width/2, width/2, width);
    
    sigma(i) = sqrt((i+1)/4);
    g{i} = exp(-x.^2 / (2*sigma(i)^2));
    g{i} = g{i}/sum(g{i});
    
    subplot(numPlotRows, numPlotCols, i), hold off
    
    b = binomialKernel{i}/sum(binomialKernel{i});
    stem(x, b); 
    hold on
    plot(x, g{i}, 'r')
    title({sprintf('Sigma = %.3f', sigma(i)), ...
        sprintf('RMS Error = %.3f', sqrt(mean( (b-g{i}).^2 )))});
end

%%
numLevels = 8;

p0 = cell(1, numLevels);
p1 = cell(1, numLevels);
p2 = cell(1, numLevels);

DoG = cell(1, numLevels);

kernel = [1 4 6 4 1];
kernel = kernel / sum(kernel);
p0{1} = img;
for k = 1:numLevels
   p1{k} = separable_filter(p0{k}, kernel);
   p2{k} = separable_filter(separable_filter(p1{k}, kernel), kernel);
   DoG{k} = p1{k} - p2{k};
   p0{k+1} = p2{k}(1:2:end,1:2:end);   
end

for k = 1:numLevels
    DoG{k} = imresize(DoG{k}, size(img), 'bilinear');
end
DoG = cat(3, DoG{:});
[~,whichScale] = max(abs(DoG),[],3);

subplot 121
imshow(img)
subplot 122
imagesc(whichScale), axis image
