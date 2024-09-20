# victor

Welcome to `victor`. This is the home of the Anki Vector robot's source code. Original README: [README-orig.md](/README-orig.md)

## Branches

Branches where I am doing work:

- `master` (this)
  -  Little bug fixes and tests. No major enhancements.
- `actual-picovoice`
  -  Implementation of Picovoice Porcupine v1.3.0 as a wake word engine. The wake word in this one, by default, is "bumblebee". You can't implement a custom one. It is free and doesn't need an access key.
- `open-source`
  -  An attempt to replace every confidential component with something that wouldn't get me in trouble if publicized. W.I.P.
- `actual-picovoice-3.0`
  -  Picovoice Porcupine v3.0.0 (latest) as a wake word engine. The wake word is "Hey Vector" and you can implement any custom one, but it requires a Picovoice access key. More details are in that branch's README.
- `vosk-ww`
  - Vosk as a wake word engine. Uses a lot of RAM, but the wake word can be whatever word you want, and no access key is required as Vosk is open-source software.

## Building

 - Prereqs: Make sure you have `docker` and `git-lfs` installed. You need a Linux machine. macOS and WSL are untested.

1. Clone the repo and cd into it:

```
cd ~
git clone --recurse-submodules https://github.com/kercre123/victor
cd victor
git lfs pull
```

2. Make sure you can run Docker as a normal user. This will probably involve:

```
sudo groupadd docker
sudo gpasswd -a $USER docker
newgrp docker
```

3. Run the build script:
```
./wire/build-d.sh
```

3. It should just work! The output will be in `./_build/vicos/Release/`

- If you get a permission denied error, try these commands, then run the build script again:
```
sudo chown root:docker /var/run/docker.sock
sudo chmod 660 /var/run/docker.sock
```


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
