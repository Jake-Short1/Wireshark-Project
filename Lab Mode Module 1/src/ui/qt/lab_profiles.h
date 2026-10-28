/* lab_profiles.h - Lab Mode: the course labs and their preset display filters.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef LAB_PROFILES_H
#define LAB_PROFILES_H

enum LabId { LAB_OFF = 0, LAB_TCP, LAB_ARP, LAB_ICMP, LAB_DNS, LAB_TUNNEL, LAB_COUNT };

struct LabPreset  { const char *label; const char *filter; const char *tip; };
struct LabProfile {
    LabId        id;
    const char  *title;        // text shown in the combo box
    const char  *hint;         // "what to look for"; combo tooltip, later shown by Module 4
    const char  *base_filter;  // applied when the lab is chosen ("" for LAB_OFF)
    int          n_presets;
    LabPreset    presets[5];
};

// Indexed by LabId. Every filter was checked with dftest from Wireshark 3.2.3.
static const LabProfile lab_profiles[LAB_COUNT] = {
    { LAB_OFF, "Off (normal Wireshark)", "Lab Mode is off. Wireshark works as usual.", "", 0, {} },
    { LAB_TCP, "TCP (reset / hijack)", "Look for SYN, SYN-ACK and RST flags and the raw sequence numbers.", "tcp", 5, {
        { "SYN only", "tcp.flags.syn == 1 && tcp.flags.ack == 0", "First packet of each new connection." },
        { "SYN-ACK", "tcp.flags.syn == 1 && tcp.flags.ack == 1", "Server replies that accept a new connection." },
        { "RST packets", "tcp.flags.reset == 1", "Resets that abort a connection, including forged ones." },
        { "Telnet (port 23)", "tcp.port == 23", "Telnet sessions, which carry keystrokes in plain text." },
        { "Retransmissions", "tcp.analysis.retransmission", "Segments sent again, often after a hijack or reset." } } },
    { LAB_ARP, "ARP (cache poisoning / MITM)", "Watch for ARP replies that map a known IP address to a new MAC address.", "arp", 3, {
        { "Requests", "arp.opcode == 1", "Broadcast questions: who has this IP address?" },
        { "Replies", "arp.opcode == 2", "Answers that map an IP address to a MAC address." },
        { "Duplicate address", "arp.duplicate-address-detected", "Two MAC addresses claim the same IP address." } } },
    { LAB_ICMP, "ICMP (redirect)", "Look for Redirect messages (type 5) and check which gateway they announce.", "icmp", 4, {
        { "Echo request", "icmp.type == 8", "Ping requests." },
        { "Echo reply", "icmp.type == 0", "Ping replies." },
        { "Redirect", "icmp.type == 5", "Messages telling a host to use a different gateway." },
        { "Unreachable", "icmp.type == 3", "Errors saying a network, host or port cannot be reached." } } },
    { LAB_DNS, "DNS (local / remote)", "Match responses to queries by transaction ID and port, and watch for forged NS records.", "dns", 4, {
        { "Queries", "dns.flags.response == 0", "Questions sent to a DNS server." },
        { "Responses", "dns.flags.response == 1", "Answers from a DNS server, real or spoofed." },
        { "NS records", "dns.resp.type == 2", "Records naming the server responsible for a domain." },
        { "Errors", "dns.flags.rcode != 0", "Responses with an error code, such as NXDOMAIN." } } },
    { LAB_TUNNEL, "Tunnel / VPN", "Tunnelled traffic is encrypted, so compare the outer hosts and ports with the firewall rules.", "ssh || tls || udp", 4, {
        { "SSH", "ssh", "SSH sessions, including SSH tunnels." },
        { "TLS", "tls", "TLS-encrypted traffic, used by many VPNs." },
        { "ICMP", "icmp", "Pings, for example to test whether a tunnel works." },
        { "UDP", "udp", "UDP traffic, which carries most VPN tunnels." } } },
};

#endif // LAB_PROFILES_H
