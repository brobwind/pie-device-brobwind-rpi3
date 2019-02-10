#!/vendor/bin/sh

# set up the eth0 interface
# if required
my_ip=`getprop net.shared_net_ip`
case "$my_ip" in
    "")
    ;;
    *) ifconfig eth0 "$my_ip" netmask 255.255.255.0 up
    ;;
esac

