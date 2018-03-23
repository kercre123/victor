#!/usr/bin/env bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

if [ ! -d "node_modules" ]; then
  ./install.sh
fi

# Check for existence of ssh key pair on the client,
# add generate it there if necessary:
: ${KEY_FILE_PRIVATE:="id_rsa_vic_dev"}
: ${KEY_FILE_PUBLIC:="$KEY_FILE_PRIVATE.pub"}

if [ ! -f ~/.ssh/$KEY_FILE_PRIVATE ]; then
    echo "Generating ssh key pair on this client"
    ssh-keygen -t rsa -b 4096 -f $KEY_FILE_PRIVATE -C "Victor" -N ""
    chmod 0600 $KEY_FILE_PRIVATE
    chmod 0644 $KEY_FILE_PUBLIC
    mv $KEY_FILE_PRIVATE ~/.ssh/$KEY_FILE_PRIVATE
    mv $KEY_FILE_PUBLIC ~/.ssh/$KEY_FILE_PUBLIC
fi

# Now to get the public key onto the specified robot:
GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
ROBOT_IP_SCRIPT=${GIT_PROJ_ROOT}/project/victor/scripts/victor_set_robot_ip.sh
expect -f vic_set_robot_key.expect $1 $USER $ROBOT_IP_SCRIPT

# Add some lines to client's ssh config file to prevent
# certain warnings (now that we have the IP address)
: ${CONFIG_FILE:="/Users/$USER/.ssh/config"}
read ROBOT_IP < robot_ip.txt
HOST_LINE="Host $ROBOT_IP"
count=$(grep -o "$HOST_LINE" $CONFIG_FILE | wc -l)
if [ $count == 0 ]; then
    echo "Adding lines to "$CONFIG_FILE
    echo $HOST_LINE >> $CONFIG_FILE
    echo "   IdentityFile ~/.ssh/id_rsa_vic_dev" >> $CONFIG_FILE
    echo "   StrictHostKeyChecking no" >> $CONFIG_FILE
    echo "   UserKnownHostsFile /dev/null" >> $CONFIG_FILE
    echo "   LogLevel ERROR" >> $CONFIG_FILE
fi
