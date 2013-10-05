% function outputString = convertInitializationArrayToElementwiseAssigment(pattern, numbers)

% outputString = convertInitializationArrayToElementwiseAssigment('b[%d] = %d; ', [1,2,3,4,5]);

% outputString = convertInitializationArrayToElementwiseAssigment('b[%d][%d] = %d; ', [1,2,3;4,5,6]);


function outputString = convertInitializationArrayToElementwiseAssigment(pattern, numbers)

outputString = '';
if isvector(numbers)
    for i = 1:length(numbers)
        outputString = [outputString, sprintf(pattern, i-1, numbers(i))];
    end
else
    for y = 1:size(numbers,1)
        for x = 1:size(numbers,2)
            outputString = [outputString, sprintf(pattern, y-1, x-1, numbers(y, x))];
        end
    end
end

