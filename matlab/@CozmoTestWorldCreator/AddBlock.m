function AddBlock(this, varargin)

BlockPose = Pose();
BlockProto = 'Block1x1';
BlockName  = '';
MarkerImg  = '';
% FrontFace  = '';
% BackFace   = '';
% LeftFace   = '';
% RightFace  = '';
% TopFace    = '';
% BottomFace = '';
parseVarargin(varargin{:});

fid = fopen(this.worldFile, 'at');
cleanup = onCleanup(@()fclose(fid));

% if isempty(BackFace)
%     BackFace = FrontFace;
% end
% if isempty(LeftFace)
%     LeftFace = FrontFace;
% end
% if isempty(RightFace)
%     RightFace = LeftFace;
% end
% if isempty(TopFace)
%     TopFace = FrontFace;
% end
% if isempty(BottomFace)
%     BottomFace = TopFace;
% end


T = BlockPose.T / 1000;

if ~isempty(BlockName)
    blockType = upper(BlockName);
    BlockName = 'Block';
else
    blockType = 0;
    BlockName = '';
end
    
fprintf(fid, [ ...
    '%s {\n' ...
    '  name "%s"\n' ...
    '  type "%s"\n' ... 
    '  translation %f %f %f\n' ...
    '  rotation    %f %f %f %f\n', ...
    '  markerImg "%s.png"\n', ...%     '  facemarkerFront  ["textures/%s.png"]\n', ...%     '  facemarkerBack   ["textures/%s.png"]\n', ...%     '  facemarkerLeft   ["textures/%s.png"]\n', ...%     '  facemarkerRight  ["textures/%s.png"]\n', ...%     '  facemarkerTop    ["textures/%s.png"]\n', ...%     '  facemarkerBottom ["textures/%s.png"]\n', ...
    '}\n\n'], ...
    BlockProto, BlockName, blockType, T(1), T(2), T(3), ...
    BlockPose.axis(1), BlockPose.axis(2), BlockPose.axis(3), ...
    BlockPose.angle, ...
    MarkerImg);
%     FrontFace, BackFace, LeftFace, RightFace, TopFace, BottomFace);

end