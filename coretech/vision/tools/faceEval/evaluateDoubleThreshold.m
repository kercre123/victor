% find cases where having a margin and low threshold would improve things
% call those true positives in the new ground truth

%iRun = 0;
%results = csvread(sprintf('_%d/identification_results_%d.txt', iRun, iRun));
%results = csvread('identification_results.txt');

score1 = results(:,1);
score2 = results(:,2);
recogTruth = results(:,3);

% Get num pos/neg for entire problem so we can show improvement as a
% fraction of the total, not just of the "candidate improvements"
num_pos = sum(recogTruth(:));
num_neg = sum(1-recogTruth(:));

clf, hold on
ROC(score1, recogTruth, 'linespec', 'k--', 'log_scale', false);
colors = 'rgbmcy';
for highThresh = 0.575 % [0.525 0.575 0.6 0.7 0.75 0.8]
  fpr_orig = 100*sum(score1(:) > highThresh & ~recogTruth)/num_neg;
  tpr_orig = 100*sum(score1(:) > highThresh & recogTruth(:))/num_pos;
  plot(fpr_orig, tpr_orig, 'kx', 'MarkerSize', 12, 'LineWidth', 3);
  text(fpr_orig, tpr_orig, sprintf('High=%.3f', highThresh));
  hold on
  
  for lowThresh = [0.5 0.55 0.6]
    if lowThresh >= highThresh
      continue
    end
    for margin = [0.1 0.15]
      aboveHigh = score1(:) > highThresh;
      aboveLowWithMargin = score1(:) > lowThresh & (score1(:)-score2(:)) > margin;
      fpr = 100*sum((aboveHigh | aboveLowWithMargin) & ~recogTruth)/num_neg;
      tpr = 100*sum((aboveHigh | aboveLowWithMargin) & recogTruth(:))/num_pos;
      plot(fpr, tpr, 'bo', 'MarkerSize', 12, 'LineWidth', 3);
      text(fpr, tpr, sprintf('High=%.3f/Low=%.3f/Margin=%.2f', highThresh, lowThresh, margin));
    end
  end
  
  % subplot(211)
  %
  % lowThresh = 0.45;
  % candidatesForImprovement = (score1 < highThresh) & (score1 > lowThresh);
  %
  % scoreDiff = score1-score2;
  % ROC(scoreDiff(candidatesForImprovement), recogTruth(candidatesForImprovement))
  %
  % title('Fixed Low Thresh')
  % set(gca, 'XScale', 'linear')
  
  margins = [0.1 0.15]; % linspace(0.05, 0.2, 4);
  h = zeros(1, length(margins));
  legendStrs = cell(1,length(margins));
  for i = 1:length(margins)
    candidatesForImprovement = (score1 < highThresh) & (score1-score2)>margins(i);
    if all(candidatesForImprovement==0)
      fprintf('No candidates for highThresh=%.3f margin=%.2f', highThresh, margins(i));
      continue;
    end
    h(i) = ROC(score1(candidatesForImprovement), recogTruth(candidatesForImprovement), ...
      'linespec', colors(i), 'num_pos', num_pos, 'num_neg', num_neg, ...
      'plot_offset', [fpr_orig tpr_orig]);
    hold on
    legendStrs{i} = ['Margin = ' sprintf('%.2f', margins(i))];
  end
  
end

legend(h, legendStrs)
set(gca, 'XScale', 'linear', 'XLim', [0 2.5])
grid on