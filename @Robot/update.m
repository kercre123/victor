function update(this, img, matImage)

if nargin < 2 || isempty(img)
    this.camera.grabFrame();
else
    this.camera.image = img;
end

this.observationWindow(2:end) = this.observationWindow(1:end-1);

if this.world.HasMat
    assert(~isempty(this.matCamera), ...
        'Robot in a BlockWorld with a Mat must have a matCamera defined.');
    
    if nargin < 3 || isempty(matImage)
        matImage = this.matCamera.grabFrame;
    end
else
    matImage = [];
end

this.observationWindow{1} = Observation(this.camera.image, this, matImage);
% this.frame = this.observationWindow{1}.frame;

end % FUNCTION Robot/update()