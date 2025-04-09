# victor

Welcome to `victor`. This is the home of the Anki Vector robot's source code. Original README: [README-orig.md](/README-orig.md)

Check the [wiki](https://github.com/kercre123/victor/wiki) for more information about the leak, what we can do with this, and general Vector info.

> [!WARNING]
> **You CANNOT currently deploy this to a regular, non-unlocked bot.**

## Branch Notes

- `kirkstone`
	-	Modified to compile with a new compiler (Clang 18)
	-	Designed to work with https://github.com/kercre123/vic-yocto-2
		-	A project intended to upgrade Vector's underlying OS
	-	Updated update-engine which should work with Python 3.12
	-	Actually meant to work with scarthgap but I don't feel like changing the branch name

## Changes

- The wiki includes a list of changes I made: [Changes I Made](https://github.com/kercre123/victor/wiki/Changes-I-Made)

## Building (Linux)

 - Prereqs: Make sure you have `docker` and `git-lfs` installed.

1. Clone the repo and cd into it:

```
cd ~
git clone --recurse-submodules https://github.com/kercre123/victor -b snowboy
cd victor
git lfs install
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
cd ~/victor
./wire/build-d.sh
```

3. It should just work! The output will be in `./_build/vicos/Release/`

## Building (Intel or ARM64 macOS)

 - Prereqs: Make sure you have [brew](https://brew.sh/) installed.
   -  Then: `brew install pyenv git-lfs ccache wget`

1. Clone the repo and cd into it:

```
cd ~
git clone --recurse-submodules https://github.com/kercre123/victor -b snowboy
cd victor
git lfs install
git lfs pull
```

2. Set up Python 2:

```
pyenv install 2.7.18
pyenv init
```

- Add the following to both ~/.zshrc and ~/.zprofile. After doing so, run the commands in your terminal session:
```
export PYENV_ROOT="$HOME/.pyenv"
[[ -d $PYENV_ROOT/bin ]] && export PATH="$PYENV_ROOT/bin:$PATH"
eval "$(pyenv init -)"
pyenv shell 2.7.18
```

3. Disable security:

```
sudo spctl --master-disable
sudo spctl --global-disable
```
- You will have to head to `System Settings -> Security & Privacy -> Allow applications from` and select "Anywhere".


4. Run the build script:
```
cd ~/victor
./wire/build.sh
```

5. It should just work! The output will be in `./_build/vicos/Release/`

## Deploying

1. Echo your robot's IP address to robot_ip.txt (in the root of the victor repo):

```
echo 192.168.1.150 > robot_ip.txt
```

2. Copy your bot's SSH key to a file called `robot_sshkey` in the root of this repo.

3. Run:

```
# Linux
./wire/deploy-d.sh

# macOS
./wire/deploy.sh
```
