% function runParallelProcesses(numComputeThreads, workQueue, matlabCommandString)

% numComputeThreads is the number of Matlab processes to create

% workQueue is a 1D cell array of work, that is automatically split between
% the different Matlab processes. Each process will have a cell array
% "localWorkQueue" in its Matlab workspace, to be used to determine what
% pieces of work to compute.

% matlabCommandString is the command that is executed in each of the different Matlab processes

% This can be faster if temporaryDirectory is a RAM disk. Create a 1GB (1024mb*2048=2097152) disk in OSX by typing:
% diskutil erasevolume HFS+ 'RamDisk' `hdiutil attach -nomount ram://2097152`

function runParallelProcesses(numComputeThreads, workQueue, temporaryDirectory, matlabCommandString, splitFinely)
    if ~exist('splitFinely', 'var')
        splitFinely = true;
    end
    
    matlabCommandString = strrep(matlabCommandString, '%', '%%');
    
    if numComputeThreads == 1 || (length(workQueue) < 2)
        localWorkQueue = workQueue; %#ok<NASGU>
        iThread = 0;
        eval(matlabCommandString);
    else
        threadCompletionMutexFilenames = cell(numComputeThreads, 1);
        workerInputFilenames = cell(numComputeThreads, 1);
        scriptFilenames = cell(numComputeThreads, 1);
        
        if ~splitFinely
            iWorkQueue = 1;
            localWorkSize = ceil(length(workQueue) / numComputeThreads);
        end
        
        % launch threads
        for iThread = 0:(numComputeThreads-1)
            threadCompletionMutexFilenames{iThread+1} = [temporaryDirectory, sprintf('/runParallelProcesses_mutex%d.mat', iThread)];
            workerInputFilenames{iThread+1} = [temporaryDirectory, sprintf('/runParallelProcesses_input%d.mat', iThread)];
            
            scriptFilename_directory = [temporaryDirectory, '/'];
            scriptFilename_filename = sprintf('runParallelProcesses_input%d.m', iThread);
            scriptFilenames{iThread+1} = [scriptFilename_directory, scriptFilename_filename];
            
            if splitFinely
                localWorkQueue = workQueue((1+iThread):numComputeThreads:end); %#ok<NASGU>
            else                
                localWorkQueue = workQueue(iWorkQueue:min(iWorkQueue+localWorkSize-1, length(workQueue))); %#ok<NASGU>
                iWorkQueue = iWorkQueue + localWorkSize;
            end
                        
            savefast(workerInputFilenames{iThread+1}, 'localWorkQueue', 'iThread');
            
            try
                oldWarnings = warning();
                warning('off', 'MATLAB:DELETE:FileNotFound');
                delete(threadCompletionMutexFilenames{iThread+1});
                warning(oldWarnings);
            catch
            end
            
            % Save the commands as a script
            if ~ismac()
                hideWindowsCommand = [...
                    'frames = java.awt.Frame.getFrames; ',...
                    'for iFrame = 1:length(frames) ',...
                    '  try ',...
                    '    awtinvoke(frames(iFrame),''setState'', java.awt.Frame.ICONIFIED); ',...
                    '  catch',...
                    '  end;',...
                    'end;',...
                    'for iFrame = [1, 3:length(frames)] \n',...
                    '  try \n',...
                    '    awtinvoke(frames(iFrame),''setVisible'',0); \n',...
                    '  catch\n',...
                    '  end;\n',...
                    'end;\n\n'];
            else
                hideWindowsCommand = '';
            end
                        
            scriptText = hideWindowsCommand;
            scriptText = [scriptText, 'load(''', workerInputFilenames{iThread+1}, '''); \n']; %#ok<AGROW>
            scriptText = [scriptText, '\n%%User code starts...\n\n']; %#ok<AGROW>
            scriptText = [scriptText, matlabCommandString, ';\n']; %#ok<AGROW>
            scriptText = [scriptText, '\n%%User code ends\n\n']; %#ok<AGROW>
            scriptText = [scriptText, 'mut=1; \nsave(''', threadCompletionMutexFilenames{iThread+1}, ''',''mut'');\n']; %#ok<AGROW>
            scriptText = [scriptText, 'exit;\n']; %#ok<AGROW>
            
            fid = fopen(scriptFilenames{iThread+1}, 'w');
            fprintf(fid, scriptText);
            fclose(fid);
            
            if ispc()
                commandString = [...
                    'start /b matlab -noFigureWindows -nosplash -minimize -r ',...
                    '" cd ', scriptFilename_directory, '; ', scriptFilename_filename(1:(end-2)), '"'];
            elseif ismac()
                commandString = ['osascript ', ...
                    '-e ''tell application "Terminal" to do script "', matlabroot, '/bin/matlab -noFigureWindows -nosplash -nodesktop -nodisplay -r ',...
                    '\"cd ', scriptFilename_directory, ' ; ', scriptFilename_filename(1:(end-2)), '\" ; exit" '' '];
            else
                commandString = [...
                    'screen -A -m -d ', matlabroot, '/bin/matlab -noFigureWindows -nosplash -r ',...
                    '" cd ', scriptFilename_directory, '; ', scriptFilename_filename(1:(end-2)), '"']; % TODO: add exit?
            end
            
            commandString = strrep(commandString, '//', '/');
            
            system(commandString);
            
            pause(.01);
        end
        
        % Wait for the thread to complete
        for iThread = 1:length(threadCompletionMutexFilenames)
            while ~exist(threadCompletionMutexFilenames{iThread}, 'file')
                pause(.5);
            end
            delete(threadCompletionMutexFilenames{iThread});
            delete(workerInputFilenames{iThread});
        end
    end
end
