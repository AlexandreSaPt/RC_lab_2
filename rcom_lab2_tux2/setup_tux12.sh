sudo ifconfig if_e1 172.16.11.1/24
sudo sysctl net.ipv4.icmp_echo_ignore_broadcasts=0
sudo route add -net 172.16.10.0/24 gw 172.16.11.253
sudo route add -net 172.16.1.0/24 gw 172.16.11.254
