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
        this.matCamera.grabFrame();
    else
        this.matCamera.image = matImage;
    end
end

this.observationWindow{1} = Observation(this);

end % FUNCTION Robot/update()