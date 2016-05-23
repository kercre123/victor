# -*- mode: ruby -*-
# vi: set ft=ruby :

# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure(2) do |config|
  # The most common configuration options are documented and commented below.
  # For a complete reference, please see the online documentation at
  # https://docs.vagrantup.com.

  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://atlas.hashicorp.com/search.
  #config.vm.box = "base"
  #config.vm.box = "hashicorp/precise32" #<-- 12.04 LTS
  #config.vm.box = "ubuntu/vivid64"      #<--15.04
  config.vm.box = "ubuntu/trusty64"      #<--14.04 LTS
  config.vm.provider "virtualbox" do |v|
    v.memory = 2048
    v.cpus = 2
  end

  ####
  # Local Scripts
  # Any local scripts you may want to run post-provisioning.
  # Add these to the same directory as the Vagrantfile.
  ##########
  config.vm.provision "shell", path: "./tools/vagrant/setup-valgrind.sh", privileged: false
  config.vm.provision "shell", path: "./tools/vagrant/run-linux-valgrind.sh", privileged: false
end
