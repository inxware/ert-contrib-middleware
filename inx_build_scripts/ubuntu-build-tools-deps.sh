echo "Run this script on UBUNTU - tested on 10.10,  but may work on erlier versions also
echo "You will need to run from a sudo account and use your password"
echo "Any Key to continue "
read -n 1
echo "Need to do some packages and system patches as root:"
sudo -i
# of course we could do this in one line..

sudo apt-get install libcloog-ppl0
sudo apt-get install intltool
sudo apt-get install libmpfr-dev
pushd .
cd /usr/lib
ln -s libmpfr.so.4 libmpfr.so.1
popd
sudo apt-get install gettext
sudo apt-get install libglib2.0-dev
sudo apt-get install bison
sudo apt-get install flex
exit
