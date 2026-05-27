sudo ifconfig if_e1 172.16.10.254/24
sudo ifconfig if_e2 172.16.11.253/24
sudo sysctl net.ipv4.ip_forward=1
sudo sysctl net.ipv4.icmp_echo_ignore_broadcasts=0
sudo iptables -P FORWARD ACCEPT
iptables -F FORWARD
sudo route add -net 172.16.1.0/24 gw 172.16.11.254
