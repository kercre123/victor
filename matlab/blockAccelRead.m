% Matlab's Serial object seems to hang if I use it to connect to the block
% and stream data using fopen(). So open streaming with "screen" and dump
% the data to a logfile using -L and then read that file with Matlab.

%% Open Screen and lofgile
cmd = 'cd ~; screen -d -L -m /dev/tty.usbserial-A103FFFR 57600';
system(sprintf('osascript -e ''tell application "Terminal" to do script "%s" in window 1''', cmd));

fid = fopen('~/screenlog.0', 'rt');

% seek to end of file
fseek(fid, 0, 'eof');


historyLength = 500;
updatePeriod = 50;

data = zeros(3,historyLength);
p = cell(1,updatePeriod);

% Prepare empty plot
clf
colors = 'rgbk';
titles = {'X axis', 'Y axis', 'Z axis', 'Magnitude'};
for i = 1:4
  subplot(4,1,i)
  h(i) = plot(1:historyLength, nan(1,historyLength), colors(i)); hold on
  title(titles{i})
end
set(findobj(gcf, 'Type', 'axes'), 'YLim', [-25 25]);

index = 1;

[~, dg, dg2] = gaussian_kernel(0.5);

% Keep reading from file until ESC is pressed in the open figure
set(gcf, 'CurrentCharacter', ' ', 'Name', 'Running');
while get(gcf, 'CurrentCharacter') ~= 27
  
  % read a line of data
  line = fgets(fid);
  
  % if line read fails, go forward a byte and try again
  if ~ischar(line)
    fseek(fid, 1, 'cof');
    continue
  end
  
  % Try to get 3 numbers from the line. If that fails continue
  p{index} = sscanf(line, '%d', [3 1]);
  if length(p{index}) ~= 3
    disp('Skipping bad line')
    continue
  end
  
  % Got three numbers successfully. If it's time to append to the data
  % we're queuing according to update period, do so.
  index = index+1;
  if index == updatePeriod
    index = 1;
    
    data = [data(:,updatePeriod:end) p{:}];
    
    % Filter the data. This is horribly inefficient, since it als
    % RE-filters all prior data too...
    filteredData = imfilter(data, dg, 'replicate');
    %filteredData = data;
    
    % Display results
    for i=1:3
      set(h(i), 'YData', filteredData(i,:));
    end
    set(h(4), 'YData', sqrt(sum(filteredData.^2,1)));
    
    drawnow
  end
end

set(gcf, 'Name', 'Stopped');
fclose(fid);
    
%% Close down the screen
cmd = 'screen -X quit';
system(sprintf('osascript -e ''tell application "Terminal" to do script "%s" in window 1''', cmd));

%% Windows
% Serial seems to work in Windows

%% Open the connection
s = serial('COM3', 'BaudRate', 57600);
fopen(s);

%% Read/Plot Data Continuously:
set(gcf, 'CurrentCharacter', ' ');
historyLength = 100;
for j=1:3
    subplot(3,1,j)
    h(j) = plot(1:historyLength, nan(1,historyLength));
    set(gca, 'XLim', [1 historyLength], 'YLim', [0 255]);
end

data = zeros(3,historyLength);
while(get(gcf, 'CurrentCharacter')~= 27) 
    line = fgets(s);
    temp = sscanf(line, '%d', [3 1]); 
    
    if length(temp) ~= 3
        continue;
    end
    
    data(:,1:end-1) = data(:,2:end);
    data(:,end) = temp;
    for j=1:3
        set(h(j), 'YData', data(j,:));
    end
    drawnow
end

%% Close the connection
fclose(s)
