function rectWorld(filename)

if nargin == 0
   filename = "tmp.env"
end

world = [];

if exist(filename, 'file')
   world = reshape(textread(filename), 5, []);
end

close all; %TEMP
figure;
axis equal;
hold on;
for i=1:size(world,2)
    plotRect(world(:,i))
end

end
