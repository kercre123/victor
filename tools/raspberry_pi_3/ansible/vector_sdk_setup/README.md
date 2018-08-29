# Ansible Playbooks
Here are the following playbooks, and what you can expect out of them.

### Pre-Requisites
Before you can run these playbooks, you need to do the following:

1. Install Ansible and Git on your local machine `brew install ansible`. Assumes you're on a Mac 
2. Figure out your Pi's IP address. [Here is the Pi's documentation on finding it.](https://www.raspberrypi.org/documentation/remote-access/ip-address.md)
3. Configure IP address of your RPi3 in "hosts" file. If you have more than one
Raspberry Pi, add more lines and enter your details. This includes password. 

## Playbook breakdowns

### vector\_sdk\_setup
This playbook gets the Vector SDK setup on the Raspberry Pi 3 (RPi3). It currently does the following:

1. (Currently implemented) Checks which version of python3 is on the raspbian distribution. We need 3.6 or above. 
2. (Currently implemented) If we're below, we will install python 3.6.0. 
3. (Currently implemented) Check that we have the Victor repo installed on our RPi3. If not, grab it.
4. (Currently implemented) Install the Vector SDK dependencies
5. (TBD) Update the Vector repo periodically.

To run the playbook (expect this to take at least 30 minutes), enter this command in your terminal (assumes you're in the root directory `vector_sdk_setup`):

> ./playbook.yml -vv --ask-vault-pass -e "vector\_ip=192.168.45.206 vector_name=Vector-N8P4"

Where the vault password can be found by asking Awot Ghirmai at awot@anki.com (TODO: add password in 1password)

The vector\_ip and vector_name can be found on the robot after it has been connected to wifi (press its backpack button twice for the name, and then lift its lift for the IP).

**Important thing to note with this playbook:** the Vector SDK is currently going through alot of changes, and as of 8/15, this playbook will get a Vector up and running. Its **very** likely though that this playbook will need to be reworked as the SDK changes. 


