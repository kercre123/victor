% function runParallelProcesses(numComputeThreads, workQueue, matlabCommandString)

% numComputeThreads is the number of Matlab processes to create

% workQueue is a 1D cell array of work, that is automatically split between
% the different Matlab processes. Each process will have a cell array
% "localWorkQueue" in its Matlab workspace, to be used to determine what
% pieces of work to compute.

% matlabCommandString is the command that is executed in each of the different Matlab processes

function runParallelProcesses(numComputeThreads, workQueue, temporaryDirectory, matlabCommandString)
    if numComputeThreads == 1 || isempty(workQueue)
        localWorkQueue = workQueue; %#ok<NASGU>
        eval(matlabCommandString);
    else
        threadCompletionMutexFilenames = cell(numComputeThreads, 1);
        workerInputFilenames = cell(numComputeThreads, 1);
        scriptFilenames = cell(numComputeThreads, 1);
        
        % launch threads
        for iThread = 0:(numComputeThreads-1)
            threadCompletionMutexFilenames{iThread+1} = [temporaryDirectory, sprintf('/runParallelProcesses_mutex%d.mat', iThread)];
            workerInputFilenames{iThread+1} = [temporaryDirectory, sprintf('/runParallelProcesses_input%d.mat', iThread)];
            
            scriptFilename_directory = [temporaryDirectory, '/'];
            scriptFilename_filename = sprintf('runParallelProcesses_input%d.m', iThread);
            scriptFilenames{iThread+1} = [scriptFilename_directory, scriptFilename_filename];
            
            localWorkQueue = workQueue((1+iThread):numComputeThreads:end); %#ok<NASGU>
            save(workerInputFilenames{iThread+1}, 'localWorkQueue');
            
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
                % %                 commandString = 'osascript -e ''tell application "Terminal" to do script "matlab -nodesktop -nosplash -r \"disp(5);\""'''
                %                 matlabScript2 = strrep(matlabScript, '''', '\"''');
                %                 commandString = ['osascript -e ''tell application "Terminal" to do script "', matlabroot, '/bin/matlab -noFigureWindows -nosplash -nodesktop -nodisplay -r ','\"', matlabScript2, '; exit;\"; exit; " '' '];
                %                 %                     '\"', hideWindowsCommand, matlabScript, '\"'];
                %             else

                %                     'osascript -e ''tell application "Terminal" to do script "', matlabroot, '/bin/matlab -noFigureWindows -nosplash -nodesktop -nodisplay -r ',...
%                 commandString = [...
%                     'commandString="\"cd ', scriptFilename_directory, '; ', scriptFilename_filename(1:(end-2)), '\"" ; ',...
%                     'osascript -e ''tell application "Terminal" to do script "', matlabroot, '/bin/matlab -noFigureWindows -nosplash -r ',...
%                     '$commandString "'''];

%                     '-e ''tell application "Terminal" to activate'' ',...

%                 commandString = [...
%                     'osascript ',...
%                     '-e ''tell application "System Events" to tell process "Terminal" to keystroke "t" using command down'' ',...
%                     '-e ''tell application "Terminal" to do script "', matlabroot, '/bin/matlab -noFigureWindows -nosplash -nodesktop -nodisplay -r ',...
%                     '\"cd ', scriptFilename_directory, ' ; ', scriptFilename_filename(1:(end-2)), '\" ; exit" '''];                
                commandString = [...
                    'osascript ',...
                    '-e ''tell application "Terminal" to activate'' ',...
                    '-e ''tell application "System Events" to tell process "Terminal" to keystroke "t" using command down'' ',...
                    '-e ''tell application "Terminal" to do script "', matlabroot, '/bin/matlab -noFigureWindows -nosplash -nodesktop -nodisplay -r ',...
                    '\"cd ', scriptFilename_directory, ' ; ', scriptFilename_filename(1:(end-2)), '\" ; exit" in front window '' '];                

            else
                commandString = [...
                    'screen -A -m -d ', matlabroot, '/bin/matlab -noFigureWindows -nosplash -r ',...
                    '" cd ', scriptFilename_directory, '; ', scriptFilename_filename(1:(end-2)), '"']; % TODO: add exit?
            end
            
            system(commandString);
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
