% Script for generating atan and asin lookup tables 
% for steeringController.

ASIN_INPUT_RANGE_MAX = 1;
ASIN_INPUT_RANGE_STEP_SIZE = 0.02; % ASIN_INPUT_RANGE_MAX should be an integer multiple of this value.

ATAN_INPUT_RANGE_MAX = 20; %15
ATAN_INPUT_RANGE_STEP_SIZE = 0.2; %0.25; % ATAN_INPUT_RANGE_MAX should be an integer multiple of this value.

asin_input_range = 0:ASIN_INPUT_RANGE_STEP_SIZE:ASIN_INPUT_RANGE_MAX;
atan_input_range = 0:ATAN_INPUT_RANGE_STEP_SIZE:ATAN_INPUT_RANGE_MAX;

asin_output = asin(asin_input_range);
atan_output = atan(atan_input_range);


figure(1);
plot(asin_input_range, asin_output, 'o');
title('asin');
figure(2);
plot(atan_input_range, atan_output, 'o');
title('atan');


% Multiply floats by scalar so we can store in int
floatMultiplier = 100;
asin_output_int = round(asin_output * floatMultiplier);
atan_output_int = round(atan_output * floatMultiplier);


% Write to file
fid = fopen('trig_lut.txt', 'w');


fprintf(fid,'#define ATAN_LUT_SIZE %d\n', length(atan_output));
fprintf(fid,'#define ATAN_LUT_INPUT_MULTIPLIER %d\n', 1/ATAN_INPUT_RANGE_STEP_SIZE);
fprintf(fid,'#define ATAN_LUT_OUTPUT_MULTIPLIER %d\n', floatMultiplier);

fprintf(fid,'#define ASIN_LUT_SIZE %d\n', length(asin_output));
fprintf(fid,'#define ASIN_LUT_INPUT_MULTIPLIER %d\n', 1/ASIN_INPUT_RANGE_STEP_SIZE);
fprintf(fid,'#define ASIN_LUT_OUTPUT_MULTIPLIER %d\n', floatMultiplier);


numEntriesInLine = 10;
n=0;
fprintf(fid, 'const u8 asin_lut[] = \n{\n  ');
for i=1:length(asin_output)
    fprintf(fid,'%d,', asin_output_int(i));
    n = n+1;
    if n==numEntriesInLine
        fprintf(fid,'\n  ');
        n = 0;
    end
end
fprintf(fid, '\n};\n\n');

n = 0;
fprintf(fid, 'const u8 atan_lut[] = \n{\n  ');
for i=1:length(atan_output)
    fprintf(fid,'%d,', atan_output_int(i));
    n = n+1;
    if n==numEntriesInLine
        fprintf(fid,'\n  ');
        n = 0;
    end
end    
fprintf(fid, '\n};\n\n');

fclose(fid);

disp('SUCCESS: Generated trig_lut.txt');