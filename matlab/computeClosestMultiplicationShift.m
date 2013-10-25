
function computeClosestMultiplicationShift(divisor, maxRightShiftBits, totalBitDepth)

displayAll = false;

original = 1.0 / divisor;

multipliers = 1:(2^totalBitDepth);

% absoluteDifferences = inf * ones(length(multipliers), maxRightShiftBits+1);
% multipleDifferences = inf * ones(length(multipliers), maxRightShiftBits+1);

bestOverallVal = Inf;

for bit = 0:maxRightShiftBits
    divide = 1 / (2^bit);
    
    approximations = (1.0 * multipliers) * divide;

    absoluteDifferences = abs(original - approximations);
    multipleDifferences = max(abs(approximations)/abs(original), abs(approximations)/abs(approximations));

    [bestVal, bestInd] = min(absoluteDifferences);
    
    if bestVal < bestOverallVal
        bestOverallVal = bestVal;
        bestOverall.ind = bestInd;
        bestOverall.bit = bit;
        bestOverall.multiplier = multipliers(bestInd);
        bestOverall.approximation = approximations(bestInd);
    end
    
    if displayAll
        disp(sprintf('Bit %d: bestVal: %f bestMultiplier: %d (%f vs %f)', bit, bestVal, multipliers(bestInd), original, approximations(bestInd)));
    end
end

disp(sprintf('bestShift %d: bestVal: %f bestMultiplier: %d (%f vs %f)', bestOverall.bit, bestOverallVal, bestOverall.multiplier, original, bestOverall.approximation));

% keyboard