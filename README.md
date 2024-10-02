# victor

Welcome to `victor`. This is the home of the Anki Vector robot's source code. Original README: [README-orig.md](/README-orig.md)

## Branch Info

This branch uses the Snowboy wake-word engine. It's reasonably fast and accurate.

The wake-word is still "hey vector".

## Building

 - Prereqs: Make sure you have `docker` and `git-lfs` installed. You need a Linux machine. WSL should work. macOS may work???

0. It would probably be smart to store your GitHub credentials. This makes it so next time you enter them, you never have to enter them again:

```
git config --global credential.helper store
```

1. Clone the repo and cd into it:

```
cd ~
git clone --recurse-submodules https://github.com/kercre123/victor -b snowboy
cd victor
git lfs pull
```

2. Make sure you can run Docker as a normal user. This will probably involve:

```
sudo groupadd docker
sudo gpasswd -a $USER docker
newgrp docker
sudo chown root:docker /var/run/docker.sock
sudo chmod 660 /var/run/docker.sock
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
