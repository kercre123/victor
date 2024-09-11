# victor

Welcome to `victor`. This is the home of the Anki Vector robot's source code. Original README: [README-orig.md](/README-orig.md)

## Building

 - Prereqs: Make sure you have `docker` and `git-lfs` installed. You need a Linux machine. WSL should work. macOS may work???

1. Clone the repo and cd into it:

```
cd ~
git clone --recurse-submodules https://github.com/kercre123/victor
cd victor
git lfs pull
```

2. Make sure you can run Docker as a normal user. This will probably involve:

```
sudo gpasswd -a $USER docker
newgrp docker
```

3. Run the build script:
```
./wire/build-d.sh
```

3. It should just work! The output will be in `./_build/vicos/Release/`

## Deploying

1. Echo your robot's IP address to robot_ip.txt (in the root of the victor repo):

```
echo 192.168.1.150 > robot_ip.txt
```

2. Copy your bot's SSH key to a file called `robot_sshkey` in the root of this repo.

3. Run:

```
./wire/deploy-d.sh
```

## Why is this not public?

Okao, TrulyHandsFree, the WWise SDK, and probably a bunch of other stuff are licensed pieces of software which really should remain somewhat confidential. They are so old that we could probably get away with it, but I don't want to risk it.
