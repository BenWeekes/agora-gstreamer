sudo iptables -I OUTPUT -p udp   -d 140.210.77.68 -m udp --dport 8443 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 125.88.159.163 -m udp --dport 8443 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 128.1.87.146 -m udp --dport 8443 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 128.1.77.34 -m udp --dport 8443 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 128.1.78.146 -m udp --dport 8443 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 69.28.51.142 -m udp --dport 8443 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 107.155.14.132 -m udp --dport 8443 -j ACCEPT

sudo iptables -I OUTPUT -p udp   -d 140.210.77.68 -m udp --dport 1080 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 125.88.159.163 -m udp --dport 1080 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 128.1.87.146 -m udp --dport 1080 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 128.1.77.34 -m udp --dport 1080 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 128.1.78.146 -m udp --dport 1080 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 69.28.51.142 -m udp --dport 1080 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 107.155.14.132 -m udp --dport 1080 -j ACCEPT


sudo iptables -I OUTPUT -p udp   -d 140.210.77.68 -m udp --dport 8000 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 125.88.159.163 -m udp --dport 8000 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 128.1.87.146 -m udp --dport 8000 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 128.1.77.34 -m udp --dport 8000 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 128.1.78.146 -m udp --dport 8000 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 69.28.51.142 -m udp --dport 8000 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 107.155.14.132 -m udp --dport 8000 -j ACCEPT


sudo iptables -I OUTPUT -p udp   -d 140.210.77.68 -m udp --dport 25000 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 125.88.159.163 -m udp --dport 25000 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 128.1.87.146 -m udp --dport 25000 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 128.1.77.34 -m udp --dport 25000 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 128.1.78.146 -m udp --dport 25000 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 69.28.51.142 -m udp --dport 25000 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 107.155.14.132 -m udp --dport 25000 -j ACCEPT

sudo iptables -I OUTPUT -p udp   -d 106.3.140.194 -m udp --dport 8001:8005 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 106.3.140.195 -m udp --dport 8001:8005 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 164.52.53.77 -m udp --dport 8001:8005 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 164.52.53.78 -m udp --dport 8001:8005 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 128.1.78.94 -m udp --dport 8001:8005 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 148.153.53.105 -m udp --dport 8001:8005 -j ACCEPT
sudo iptables -I OUTPUT -p udp   -d 148.153.53.106 -m udp --dport 8001:8005 -j ACCEPT
#sudo iptables -P OUTPUT DROP
sudo iptables -I OUTPUT -p udp --dport 8443 -j DROP
sudo iptables -I OUTPUT -p udp --dport 4000:4050 -j DROP
sudo iptables -L -n --line-number
