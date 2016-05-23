# change dir to the project dir, no matter where script is executed from
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
VAGRANT=`which vagrant`

VAGRANT_RUNNING=$(vagrant status | grep running | grep virtualbox | wc -l)
VAGRANT_EXISTS=$(vagrant status | grep poweroff | grep virtualbox | wc -l)

if [ "$VAGRANT_RUNNING" -gt 0 ];then
    echo ">>> Vagrant VM already running, using existing VM"
    cd $DIR ; $VAGRANT provision

elif [ "$VAGRANT_EXISTS" -gt 0 ];then
    echo ">>> Vagrant VM exists but is not running"
    cd $DIR ; $VAGRANT up ; $VAGRANT provision

else
    echo ">>> No Vagrant VM exists, starting a new one"
    cd $DIR ; $VAGRANT up
fi

EXIT_STATUS=$?

echo ">>> Stopping Vagrant instance"
$VAGRANT halt

# exit
echo "EXIT_STATUS ${EXIT_STATUS}"
exit $EXIT_STATUS