thresholds = [500, 550, 575, 600, 750];
colors = 'rmgbc';

numThresholds = length(thresholds);
assert(numThresholds <= length(colors), 'Need one color for each threshold');

sumTpr = zeros(1,numThresholds);
sumFpr = zeros(1,numThresholds);
sumTpr2 = zeros(1,numThresholds);
sumFpr2 = zeros(1,numThresholds);

numRuns = 50;
for i = 0:numRuns-1
  results = csvread(['identification_results_' num2str(i) '.txt']);
  
  if i==0
    hold off
  end
  
  [tpr, fpr, thresh] = ROC(round(1000*results(:,1)), results(:,2), 'linespec', 'k');
  hold on
  
  for iThresh = 1:numThresholds
    index= find(thresh>=thresholds(iThresh), 1);
    plot(fpr(index), tpr(index), '.', 'MarkerSize', 10, 'Color', colors(iThresh));
    sumTpr(iThresh) = sumTpr(iThresh) + tpr(index);
    sumFpr(iThresh) = sumFpr(iThresh) + fpr(index);
    sumTpr2(iThresh) = sumTpr2(iThresh) + tpr(index)^2;
    sumFpr2(iThresh) = sumFpr2(iThresh) + fpr(index)^2;
  end
end

avgFpr = sumFpr/numRuns;
avgTpr = sumTpr/numRuns;
covFpr = max(0.01, sumFpr2/numRuns - avgFpr.^2);
covTpr = max(0.01, sumTpr2/numRuns - avgTpr.^2);
h = zeros(1,numThresholds);
legendStrs = cell(1, numThresholds);
for iThresh = 1:numThresholds
  h(iThresh) = plot(avgFpr(iThresh), avgTpr(iThresh), 'x', 'Color', colors(iThresh), 'MarkerSize', 10, 'LineWidth', 3);
  C = [covFpr(iThresh) 0; 0 covTpr(iThresh)];
  draw_ellipse([avgFpr(iThresh) avgTpr(iThresh)], C, colors(iThresh), 3, 2, false);
  legendStrs{iThresh} = ['Thresh=' num2str(thresholds(iThresh))];
end

set(gca, 'XScale', 'linear')
set(gca, 'XLim', [0 5], 'YLim', [0 100])
grid on
legend(gca, h, legendStrs);
