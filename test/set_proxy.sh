#!/bin/bash -e

## clear previous iptable config
sudo iptables --flush

## Opening DNS port
sudo iptables -A INPUT -p udp --sport 53 -j ACCEPT
sudo iptables -A INPUT -p udp --dport 53 -j ACCEPT
sudo iptables -A OUTPUT -p udp --sport 53 -j ACCEPT
sudo iptables -A OUTPUT -p udp --dport 53 -j ACCEPT

## Opening ports for signals
declare -a ips1=("184.72.16.87" "35.168.106.53" "54.178.26.110" "52.221.23.86" "13.230.60.35")

for i in "${ips1[@]}"
do
   eval "sudo iptables -A INPUT -p udp -s '$i' --sport 8443 -j ACCEPT"
   eval "sudo iptables -A INPUT -p udp -s '$i' --dport 8443 -j ACCEPT"
   eval "sudo iptables -A INPUT -p udp -d '$i' --sport 8443 -j ACCEPT"
   eval "sudo iptables -A INPUT -p udp -d '$i' --dport 8443 -j ACCEPT"
   eval "sudo iptables -A OUTPUT -p udp -s '$i' --sport 8443 -j ACCEPT"
   eval "sudo iptables -A OUTPUT -p udp -s '$i' --dport 8443 -j ACCEPT"
   eval "sudo iptables -A OUTPUT -p udp -d '$i' --sport 8443 -j ACCEPT"
   eval "sudo iptables -A OUTPUT -p udp -d '$i' --dport 8443 -j ACCEPT"
done

## Opening ports for media
declare -a ips2=("128.14.195.192/28" "128.14.245.16/28" "128.14.72.80/28" "128.14.200.208/28" "148.153.53.105" "148.153.53.106" "129.227.113.112/28" "129.227.55.96/28" "122.10.153.74" "129.227.234.123" "122.10.153.86" "103.101.125.23" "164.52.24.42")

for i in "${ips2[@]}"
do
   eval "sudo iptables -A INPUT -p udp -s '$i' --sport 4590:4600 -j ACCEPT"
   eval "sudo iptables -A INPUT -p udp -s '$i' --sport 8001:8010 -j ACCEPT"
   eval "sudo iptables -A INPUT -p udp -s '$i' --dport 4590:4600 -j ACCEPT"
   eval "sudo iptables -A INPUT -p udp -s '$i' --dport 8001:8010 -j ACCEPT"
   eval "sudo iptables -A INPUT -p udp -d '$i' --sport 4590:4600 -j ACCEPT"
   eval "sudo iptables -A INPUT -p udp -d '$i' --sport 8001:8010 -j ACCEPT"
   eval "sudo iptables -A INPUT -p udp -d '$i' --dport 4590:4600 -j ACCEPT"
   eval "sudo iptables -A INPUT -p udp -d '$i' --dport 8001:8010 -j ACCEPT"

   eval "sudo iptables -A OUTPUT -p udp -s '$i' --sport 4590:4600 -j ACCEPT"
   eval "sudo iptables -A OUTPUT -p udp -s '$i' --sport 8001:8010 -j ACCEPT"
   eval "sudo iptables -A OUTPUT -p udp -s '$i' --dport 4590:4600 -j ACCEPT"
   eval "sudo iptables -A OUTPUT -p udp -s '$i' --dport 8001:8010 -j ACCEPT"
   eval "sudo iptables -A OUTPUT -p udp -d '$i' --sport 4590:4600 -j ACCEPT"
   eval "sudo iptables -A OUTPUT -p udp -d '$i' --sport 8001:8010 -j ACCEPT"
   eval "sudo iptables -A OUTPUT -p udp -d '$i' --dport 4590:4600 -j ACCEPT"
   eval "sudo iptables -A OUTPUT -p udp -d '$i' --dport 8001:8010 -j ACCEPT"
done

sudo iptables -A INPUT -p udp -j DROP
sudo iptables -A OUTPUT -p udp -j DROP

