% function registers = checkRegisterUsage(filename)

% parse the text file filename, and look for ARM m4 int and float registers

function [intRegisters, floatRegisters] = checkRegisterUsage(filename)

fileId = fopen(filename, 'r');
text = fread(fileId,'*char')';
fclose(fileId);

intRegisterNames = cell(15,1);
floatRegisterNames = cell(32,1);

intRegisterNames{1} = 'lr';
intRegisterNames{2} = 'sp';

for r = 0:12
    intRegisterNames{r+3} = sprintf('r%d', r);
end

for s = 0:32
    floatRegisterNames{s+1} = sprintf('s%d', s);
end

foundIntRegisters = findRegisters(text, intRegisterNames);
foundFloatRegisters = findRegisters(text, floatRegisterNames);

printUsed(foundIntRegisters, intRegisterNames)
printUsed(foundFloatRegisters, floatRegisterNames)

numIntUsed = (length(foundIntRegisters) - sum(ismember(foundIntRegisters, 'sp')));
numFloatUsed = length(foundFloatRegisters);

percentIntUsed = 100 * numIntUsed / (length(intRegisterNames)-1);
percentFloatUsed = 100 * numFloatUsed / length(floatRegisterNames);

disp(sprintf('Using:\nint: %d/%d = %d%%\nflt: %d/%d = %d%%\n',...
    numIntUsed, (length(intRegisterNames)-1), round(percentIntUsed),...
    numFloatUsed, length(floatRegisterNames), round(percentFloatUsed)));

% keyboard

function printUsed(foundNames, registerNames)
    topString = '';
    bottomString = '';
    
    for i = 1:length(registerNames)
        registerNameLength = length(registerNames{i});
        
        topString = [topString, registerNames{i}, ' '];
        
        if isempty(find(strcmp(registerNames{i}, foundNames),1))
            bottomString = [bottomString, ' ', repmat(' ', [1,registerNameLength])];
        else
            bottomString = [bottomString, '*', repmat(' ', [1,registerNameLength])];
        end
    end
    
    disp(sprintf('%s\n%s\n',topString,bottomString));

function foundNames = findRegisters(text, registerNames)

    prefixCharacters = {' ', ',', '[', '{', '-'};
    suffixCharacters = {' ', ',', ']', '}', '-', '!', char(10), char(13)};

    foundNames = {};
    
    for iRegister = 1:length(registerNames)
        startIndexCandidates = strfind(text, registerNames{iRegister});

        registerNameLength = length(registerNames{iRegister});
        
        found = false;
        for i = startIndexCandidates
            if ~isempty(find(strcmp(text(i-1), prefixCharacters),1)) &&...
               ~isempty(find(strcmp(text(i+registerNameLength), suffixCharacters),1))
           
                found = true;
                break;
            end
        end

        if found
            foundNames{end+1} = registerNames{iRegister}; %#ok<AGROW>
        end
    end
