#!/usr/bin/env python3
"""Build lab_demo.pcap: a small capture with traffic for every Lab Mode preset.

    sudo apt install python3-scapy     # once, on the course VM
    python3 make_lab_demo_pcap.py      # writes lab_demo.pcap next to this script
    ./run/wireshark lab_demo.pcap      # from the Wireshark build directory

Each section below is labelled with the Lab Mode lab and preset it exercises.
"""
import os

from scapy.all import (ARP, DNS, DNSQR, DNSRR, ICMP, IP, TCP, UDP, Ether, Raw,
                       wrpcap)

GW_IP, GW_MAC = "10.9.0.1", "02:42:0a:09:00:01"
VICTIM_IP, VICTIM_MAC = "10.9.0.5", "02:42:0a:09:00:05"
SERVER_IP, SERVER_MAC = "10.9.0.6", "02:42:0a:09:00:06"
ATTACKER_IP, ATTACKER_MAC = "10.9.0.105", "02:42:0a:09:00:69"
DNS_IP = "10.9.0.53"
BCAST = "ff:ff:ff:ff:ff:ff"

packets = []
clock = 1730000000.0


def add(pkt, gap=0.05):
    """Append pkt, timestamped gap seconds after the previous packet."""
    global clock
    clock += gap
    pkt.time = clock
    packets.append(pkt)


def eth(src, dst):
    return Ether(src=src, dst=dst)


# --- ARP lab: requests, replies, then a poisoning reply (duplicate address) ---
add(eth(VICTIM_MAC, BCAST) / ARP(op=1, hwsrc=VICTIM_MAC, psrc=VICTIM_IP, pdst=GW_IP))
add(eth(GW_MAC, VICTIM_MAC) / ARP(op=2, hwsrc=GW_MAC, psrc=GW_IP, hwdst=VICTIM_MAC, pdst=VICTIM_IP))
add(eth(ATTACKER_MAC, VICTIM_MAC) / ARP(op=2, hwsrc=ATTACKER_MAC, psrc=GW_IP, hwdst=VICTIM_MAC, pdst=VICTIM_IP))

# --- TCP lab: a telnet session (SYN, SYN-ACK, data, retransmission, forged RST) ---
v2s = eth(VICTIM_MAC, SERVER_MAC) / IP(src=VICTIM_IP, dst=SERVER_IP)
s2v = eth(SERVER_MAC, VICTIM_MAC) / IP(src=SERVER_IP, dst=VICTIM_IP)
cseq, sseq, cport = 1000, 5000, 40000
add(v2s / TCP(sport=cport, dport=23, flags="S", seq=cseq))
add(s2v / TCP(sport=23, dport=cport, flags="SA", seq=sseq, ack=cseq + 1))
add(v2s / TCP(sport=cport, dport=23, flags="A", seq=cseq + 1, ack=sseq + 1))
add(v2s / TCP(sport=cport, dport=23, flags="PA", seq=cseq + 1, ack=sseq + 1) / Raw(b"ls\r\n"))
add(v2s / TCP(sport=cport, dport=23, flags="PA", seq=cseq + 1, ack=sseq + 1) / Raw(b"ls\r\n"), gap=0.3)
add(s2v / TCP(sport=23, dport=cport, flags="PA", seq=sseq + 1, ack=cseq + 5) / Raw(b"file.txt\r\n"))
# The attacker forges a reset "from" the server with the next expected sequence number.
add(eth(ATTACKER_MAC, VICTIM_MAC) / IP(src=SERVER_IP, dst=VICTIM_IP) /
    TCP(sport=23, dport=cport, flags="R", seq=sseq + 11))

# --- ICMP lab: ping, a redirect from the attacker, and an unreachable error ---
v2g = eth(VICTIM_MAC, GW_MAC) / IP(src=VICTIM_IP, dst="8.8.8.8")
add(v2g / ICMP(type=8, id=1, seq=1) / Raw(b"abcdefgh"))
add(eth(GW_MAC, VICTIM_MAC) / IP(src="8.8.8.8", dst=VICTIM_IP) / ICMP(type=0, id=1, seq=1) / Raw(b"abcdefgh"))
add(eth(ATTACKER_MAC, VICTIM_MAC) / IP(src=GW_IP, dst=VICTIM_IP) /
    ICMP(type=5, code=1, gw=ATTACKER_IP) / IP(src=VICTIM_IP, dst="8.8.8.8") / ICMP(type=8, id=1, seq=2))
add(eth(GW_MAC, VICTIM_MAC) / IP(src=GW_IP, dst=VICTIM_IP) /
    ICMP(type=3, code=3) / IP(src=VICTIM_IP, dst=SERVER_IP) / UDP(sport=50000, dport=9999))

# --- DNS lab: a query, a spoofed answer with a forged NS record, and an NXDOMAIN ---
v2d = eth(VICTIM_MAC, GW_MAC) / IP(src=VICTIM_IP, dst=DNS_IP)
d2v = eth(GW_MAC, VICTIM_MAC) / IP(src=DNS_IP, dst=VICTIM_IP)
add(v2d / UDP(sport=33333, dport=53) / DNS(id=0x1234, rd=1, qd=DNSQR(qname="www.example.com")))
add(d2v / UDP(sport=53, dport=33333) / DNS(
    id=0x1234, qr=1, aa=1, rd=1, ra=1, qd=DNSQR(qname="www.example.com"),
    an=DNSRR(rrname="www.example.com", type="A", rdata="1.2.3.5", ttl=259200),
    ns=DNSRR(rrname="example.com", type="NS", rdata="ns.attacker32.com", ttl=259200)))
add(v2d / UDP(sport=33334, dport=53) / DNS(id=0x1235, rd=1, qd=DNSQR(qname="nope.example.com")))
add(d2v / UDP(sport=53, dport=33334) / DNS(id=0x1235, qr=1, rd=1, ra=1, rcode=3,
                                           qd=DNSQR(qname="nope.example.com")))

# --- Tunnel / VPN lab: SSH banner, a TLS record and a UDP (VPN-like) datagram ---
add(v2s / TCP(sport=40022, dport=22, flags="PA", seq=1, ack=1) / Raw(b"SSH-2.0-OpenSSH_8.2p1 Ubuntu-4\r\n"))
add(v2s / TCP(sport=40443, dport=443, flags="PA", seq=1, ack=1) /
    Raw(b"\x17\x03\x03\x00\x08" + b"\x00" * 8))
add(v2s / UDP(sport=50001, dport=55555) / Raw(b"vpn-payload"))

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "lab_demo.pcap")
wrpcap(out, packets)
print("wrote %d packets to %s" % (len(packets), out))
