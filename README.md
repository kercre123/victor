# victor

Welcome to `victor`. This is the home of the Anki Vector robot's source code. Original README: [README-orig.md](/README-orig.md)

## Branch specifics

**This branch uses Picovoice Porcupine 3.0.0 rather than TrulyHandsfree for wake-word recognition. It's a LOT more accurate and better at handling motor noise, but requires an "access key" from [console.picovoice.ai](https://console.picovoice.ai/) (it's free forever!). The actual processing is done locally, Picovoice just has usage limits. For Porcupine, you can use it forever among up to 3 devices on one account.

So, before you deploy this onto your robot, SSH in and run `echo "picovoice_access_key" > /data/picoKey` (replacing `picovoice_access_key` with your actual key).**

For a version that doesn't need an API key, check the `actual-picovoice` branch instead. It uses an older version of Picovoice which doesn't need an access key, but the wakeword is "bumblebee" rather than "Hey Vector".

To implement a custom wake word, generate one at the Picovoice Porcupine console. Then copy the file to Vector's /anki/data/assets/cozmo_resources/assets/porcupineModels/Hey-Vector_v3.0.0.ppn and (on Vector) run `systemctl stop anki-robot.target`, wait a few seconds, then `systemctl start anki-robot.target`.

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
