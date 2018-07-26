sudo tc qdisc add dev eth0 root handle 1: tbf rate 2.3mbit burst 32kbit latency 400ms
#sudo tc qdisc change dev eth0 root handle 1: tbf rate 0.4mbit burst 32kbit latency 400ms
#sudo tc qdisc del dev eth0 root

