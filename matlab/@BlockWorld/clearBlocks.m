function clearBlocks(this)

for i = 1:this.MaxBlocks
    if ~isempty(this.blocks{i})
        delete(this.blocks{i}.handle);
        this.blocks{i} = [];
    end
end

% for i = 1:numel(this.markers)
%     if ~isempty(this.markers{i})
%         delete(this.markers{i}.handles);
%         this.markers{i} = [];
%     end
% end

end