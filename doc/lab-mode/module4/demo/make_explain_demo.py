#!/usr/bin/env python3
"""Build explain_demo.pcap: lab_demo.pcap (21 packets) plus a normal TCP close
(FIN, FIN, ACK), so every rule of the Explain panel can be tried.

    sudo apt install python3-scapy                    # once
    python3 make_explain_demo.py lab_demo.pcap        # writes explain_demo.pcap next to this script
"""
import os
import sys

from scapy.all import IP, TCP, Ether, rdpcap, wrpcap

packets = list(rdpcap(sys.argv[1] if len(sys.argv) > 1 else "lab_demo.pcap"))
clock = float(packets[-1].time)

client = Ether(src="02:42:0a:09:00:05", dst="02:42:0a:09:00:06") / IP(src="10.9.0.5", dst="10.9.0.6")
server = Ether(src="02:42:0a:09:00:06", dst="02:42:0a:09:00:05") / IP(src="10.9.0.6", dst="10.9.0.5")
close = [
    client / TCP(sport=40001, dport=23, flags="FA", seq=2000, ack=6000),   # client closes
    server / TCP(sport=23, dport=40001, flags="FA", seq=6000, ack=2001),   # server closes too
    client / TCP(sport=40001, dport=23, flags="A", seq=2001, ack=6001),    # final ACK
]
for pkt in close:
    clock += 0.05
    pkt.time = clock
    packets.append(pkt)

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "explain_demo.pcap")
wrpcap(out, packets)
print("wrote %d packets to %s" % (len(packets), out))
