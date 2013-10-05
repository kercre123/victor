% function outputString = convertInitializationArrayToElementwiseAssigment(pattern, numbers)

% outputString = convertInitializationArrayToElementwiseAssigment('b[%d] = %d; ', [1,2,3,4,5]);


function outputString = convertInitializationArrayToElementwiseAssigment(pattern, numbers)

outputString = '';
for i = 1:length(numbers)
    outputString = [outputString, sprintf(pattern, i-1, numbers(i))];
end

