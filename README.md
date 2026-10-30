# Lab Mode for Wireshark 3.2.3

**Lab Mode** is a set of four features added to the Wireshark 3.2.3 Qt interface. Together they make Wireshark easier to use for beginners working through the SEED network-security labs: the TCP reset attack, TCP session hijacking, ARP cache poisoning and man-in-the-middle, the ICMP redirect attack, local and remote DNS attacks, and bypassing firewalls with SSH tunnels or VPNs.

| Module | Feature | Where it appears |
|---|---|---|
| 1 | Lab Selector and Preset Filter Toolbar | A **Lab:** toolbar below the display filter bar |
| 2 | Field Copy Panel | A **Lab Fields (Copy)** dock on the right |
| 3 | Lab Styling | Lab-specific packet colors and columns in the packet list |
| 4 | Explain This Packet | An **Explain This Packet** dock at the bottom |

Modules 2, 3 and 4 each build on Module 1, and all four work together.

## The patch

`patch.diff` is a unified git patch against the official Wireshark tag **v3.2.3**, generated with:

```bash
git diff v3.2.3 > patch.diff
```

It changes **13 files** (9 new, 4 modified) and adds **590 lines of C/C++**, with no deletions. A further 8 lines go into the build script `ui/qt/CMakeLists.txt`.

To apply and build it:

```bash
git clone --depth 1 --branch v3.2.3 https://github.com/wireshark/wireshark.git
cd wireshark
git apply --check patch.diff && git apply patch.diff
mkdir -p ../ws-build && cd ../ws-build
cmake ../wireshark && make -j$(nproc)
./run/wireshark
```

The patch was built and tested on Ubuntu 20.04 (gcc 9.4, Qt 5.12.8, CMake 3.16), the toolchain of the SEED Labs VM. The build is C++11, as required by Wireshark 3.2.3, and it produces no compiler warnings in any of the new or changed files. `LAB_MODE_SETUP.md` is a step-by-step setup guide with troubleshooting, and each `Lab Mode Module N` folder holds that module's own patch, design notes, test results and screenshots.

## Lines of C/C++ added per module

| Module | Files | Lines |
|---|---|---|
| 1 – Lab Selector and Preset Filter Toolbar | `lab_profiles.h` 50, `lab_toolbar.h` 33, `lab_toolbar.cpp` 49, `main_window.h` 3, `main_window.cpp` 8, `main_window_slots.cpp` 13 | 156 |
| 2 – Field Copy Panel | `field_copy_panel.h` 27, `field_copy_panel.cpp` 113, `main_window.cpp` 9 | 149 |
| 3 – Lab Styling | `lab_style.h` 28, `lab_style.cpp` 101, `main_window.cpp` 7 | 136 |
| 4 – Explain This Packet | `explain_panel.h` 27, `explain_panel.cpp` 112, `main_window.cpp` 10 | 149 |
| **Total** | | **590** |

All files are in `ui/qt/`. In `main_window.cpp`, each module's lines sit between `// ---- Lab Mode: Module N begin ----` and `// ---- Lab Mode: Module N end ----` markers.

---

## Module 1 – Lab Selector and Preset Filter Toolbar

### Feature Title & Description

**Lab Mode: Lab Selector and Preset Filter Toolbar**

This module adds a **Lab:** toolbar directly below Wireshark's display filter bar. Its drop-down menu lists the course labs:

- TCP (reset / hijack)
- ARP (cache poisoning / MITM)
- ICMP (redirect)
- DNS (local / remote)
- Tunnel / VPN
- **Off (normal Wireshark)**, the default at startup

Choosing a lab immediately applies that lab's base display filter (for example `tcp` or `arp`) and shows a row of preset buttons for the packets that matter in that lab. For example, the TCP lab offers *SYN only*, *SYN-ACK*, *RST packets*, *Telnet (port 23)* and *Retransmissions*.

- Clicking a preset applies its filter: the filter text appears in the filter bar, the bar turns green, and the packet list updates.
- **Shift+click** only places the filter in the filter bar, so it can be edited before applying it. Wireshark's own filter buttons use the same convention.
- Hovering over a lab shows a one-sentence hint about what to look for; hovering over a preset explains what it isolates.
- Choosing **Off** clears the display filter and removes the preset buttons.
- The toolbar is shown and hidden together with the filter toolbar (*View → Filter Toolbar*).

**How to trigger it:** open a capture or start a live capture, then pick a lab from the **Lab:** drop-down menu.

### Justification

Display filters are Wireshark's most powerful tool, but beginners rarely know the field names or the exact syntax. Typing `ssl` instead of `tls`, or `tcp.flags.rst` instead of `tcp.flags.reset`, is a common mistake that costs lab time. Lab Mode turns the filters each lab needs into single clicks, and every filter was validated against Wireshark 3.2.3 with `dftest`. Because the filter text is written into the normal filter bar, students learn the syntax by seeing it, and they can Shift+click a preset to refine it.

*Example:* in the TCP reset lab, the student picks *TCP (reset / hijack)* and clicks *RST packets*. The forged reset is then the only packet on screen, ready to compare with the sequence number the victim expects next.

Lab Mode is off by default and never changes the user's filter at startup, so experienced users see normal Wireshark. The module also provides what the other three modules build on: the `labChanged` signal and the shared table of labs in `lab_profiles.h`.

### Modified Files List

| File | Change | Reason |
|---|---|---|
| `ui/qt/lab_profiles.h` | New (50 lines) | A data-only table of the labs: title, hint, base filter and up to five preset filters with tooltips, indexed by `LabId`. Modules 3 and 4 also read it. |
| `ui/qt/lab_toolbar.h` | New (33 lines) | Declares `LabToolBar`, a `QToolBar` with the signals `labChanged(int)` and `filterRequested(QString, bool)`. |
| `ui/qt/lab_toolbar.cpp` | New (49 lines) | Builds the "Lab:" label and the lab drop-down menu. On each selection it replaces the preset buttons and emits the signals; each button emits its filter, and Shift+click only prepares it. |
| `ui/qt/main_window.h` | +3 lines | Includes `lab_toolbar.h` and adds the `lab_toolbar_` member and the `onLabFilterRequested()` slot. |
| `ui/qt/main_window.cpp` | +8 lines | Creates the toolbar on its own row below the filter bar, connects it, and marks the integration point used by the other modules. |
| `ui/qt/main_window_slots.cpp` | +13 lines | `onLabFilterRequested()` applies a filter through Wireshark's existing `onFilterSelected()`. It also handles the empty filter (clear), which `onFilterSelected()` ignores. The toolbar's visibility follows the filter toolbar. |

---

## Module 2 – Field Copy Panel

### Feature Title & Description

**Lab Mode: Field Copy Panel**

This module adds a dock titled **Lab Fields (Copy)** on the right of the main window. When a packet is selected, it lists the values students most often type into the lab attack scripts:

- source and destination MAC and IP addresses, IP ID and TTL
- TCP and UDP ports
- the **raw** TCP sequence and acknowledgment numbers, TCP payload length, flags and window
- ICMP type, code and redirect gateway
- ARP opcode, sender and target
- DNS transaction ID and query name

Only the fields present in the selected packet are shown. Each value can be selected with the mouse, and the **Copy** button next to it puts the value on the clipboard; the button briefly reads "Copied!". With no packet or several packets selected, or after the file is closed, the panel shows "Select one packet to see its key fields."

**How to trigger it:** open a capture and click any packet. If the panel was closed, reopen it with **View → Lab Fields (Copy)**.

### Justification

The SEED attack scripts need exact values from captured packets: the next sequence number for a TCP reset or hijack, the gateway's MAC address for ARP poisoning, the transaction ID for DNS spoofing. Beginners find these values by expanding several levels of the packet-details tree, and they often copy the wrong one. The most common mistake is Wireshark's default *relative* sequence number (for example 1) instead of the raw number the attack needs. Retyping a ten-digit number by hand adds typos. The panel puts the values in one place, always shows raw TCP numbers, and copies each one with a single click.

*Example:* in the TCP session hijacking lab, the student selects the victim's last Telnet packet. The panel shows *TCP seq (raw)* and *TCP ack (raw)*, and two clicks on Copy put the exact numbers into the scapy script.

The panel does not depend on the lab selector, so it is useful in every lab and even when Lab Mode is off.

### Modified Files List

| File | Change | Reason |
|---|---|---|
| `ui/qt/field_copy_panel.h` | New (27 lines) | Declares `FieldCopyPanel`, a `QDockWidget` with the `framesSelected(QList<int>)` slot. |
| `ui/qt/field_copy_panel.cpp` | New (113 lines) | The table of fields to show. A helper finds each field in the selected packet's dissection tree (`proto_find_first_finfo`) and formats it as Wireshark displays it. The file also builds the rows with their Copy buttons, shows the hint, and resets the panel when the capture file is closed. |
| `ui/qt/main_window.cpp` | +9 lines | Includes the header, creates the dock on the right, connects it to `PacketList::framesSelected`, and adds its show/hide action to the View menu. |

---

## Module 3 – Lab Styling

### Feature Title & Description

**Lab Mode: Lab Styling**

This module makes the packet list adapt to the lab chosen in the **Lab:** toolbar. When a lab is selected, Wireshark colors the packets that matter for that lab and adds up to three columns with the values the lab needs:

| Lab | Colored packets | Extra columns |
|---|---|---|
| TCP (reset / hijack) | RST (light red), Telnet data (yellow), SYN (green), SYN-ACK (cyan) | Seq (raw), Ack (raw), Flags |
| ARP (cache poisoning / MITM) | Replies flagged as a duplicate address, i.e. poisoning (light red); replies (yellow); requests (blue) | ARP op, Sender MAC, Sender IP |
| ICMP (redirect) | Redirects (light red), unreachable (yellow), echo request (green), echo reply (cyan) | ICMP type, ICMP code, Gateway |
| DNS (local / remote) | Error responses (pink), responses (yellow), queries (blue) | DNS ID, Query, Rcode |
| Tunnel / VPN | SSH (light red), TLS (cyan), ICMP (yellow) | TCP stream |

Selecting another lab removes the previous lab's colors and columns before applying the new ones, and **Off** restores the normal look.
- The colors use Wireshark's temporary *Colorize Conversation* colors, so the user's own coloring rules are never changed.
- The columns exist only in memory: they are never written to the preferences file, and they are removed when Wireshark quits.
- Columns the user created themselves are never touched.

**How to trigger it:** open a capture or start a live capture, then pick a lab from the **Lab:** drop-down menu.

### Justification

In a busy capture, the few packets that prove an attack worked are easy to miss among hundreds of ordinary ones. The values that matter, such as a raw sequence number or the MAC address inside an ARP reply, are hidden several levels deep in the packet-details tree. Lab Styling makes the important packets stand out at a glance and puts the important values straight into the packet list. Beginners can then spend their time understanding the attack instead of searching the interface. Because the styling follows the selected lab and is never saved, students cannot accidentally break their normal Wireshark setup.

*Example:* in the ARP cache poisoning lab, the student selects **ARP**. The attacker's forged reply turns light red, because Wireshark detects that it claims an IP address already seen with another MAC. Its *Sender MAC* column shows the attacker's MAC address next to the gateway's IP in *Sender IP*, so the poisoning is visible in a single row without opening any packet.

### Modified Files List

| File | Change | Reason |
|---|---|---|
| `ui/qt/lab_style.h` | New (28 lines) | Declares `LabStyle`, a `QObject` with the `setLab(int)` slot. |
| `ui/qt/lab_style.cpp` | New (101 lines) | A per-lab table of color rules and columns. `setLab()` first removes the previous lab's temporary colors (`color_filters_reset_tmp`) and its columns, matching both title and field so user columns are kept. It then sets the new lab's rules (`color_filters_set_tmp`), adds its columns before *Info* (`column_prefs_add_custom`), and asks the packet list to refresh. The lab columns are removed again when Wireshark quits. |
| `ui/qt/main_window.cpp` | +7 lines | Includes the header, creates `LabStyle` (owned by the main window) and connects it to `LabToolBar::labChanged`. |

---

## Module 4 – Explain This Packet

### Feature Title & Description

**Lab Mode: Explain This Packet**

This module adds a dock at the bottom of the main window that appears whenever a lab is selected. When a packet is clicked, the dock shows three lines:
- **A bold heading** with the protocol and endpoints, for example *TCP from 10.9.0.6:23 to 10.9.0.5:40000*.
- **A plain-English explanation** of what the packet does, quoting its real values.
- **The selected lab's hint** about what to look for, in italics.

| Packet | Explanation (shortened) |
|---|---|
| TCP RST | One side is abruptly closing the connection; in the TCP reset lab, this is the forged packet. |
| TCP SYN / SYN-ACK | Step 1 / step 2 of the three-way handshake, with the raw initial sequence number or the acknowledged number. |
| TCP data | The number of bytes and the next sequence number the receiver expects (sequence + length), which a session hijacker must forge. |
| TCP FIN / ACK | A normal close / an acknowledgment up to the raw acknowledgment number. |
| ARP request / reply | Which host asks for which IP address / "10.9.0.1 is at 02:42:0a:09:00:69", with a warning that unsolicited replies can poison ARP caches. |
| ICMP | Echo request and reply, redirect (with the announced gateway), destination unreachable. |
| DNS | Query or response, with the query name and transaction ID, and why a spoofer must match that ID. |

Other packets get "No explanation is available for this kind of packet in this lab." The dock is hidden while Lab Mode is **Off**. It can be closed with its × button and reopened through **View → Explain This Packet**.

**How to trigger it:** select a lab in the **Lab:** drop-down menu, then click any packet.

### Justification

Beginners can see every byte of a packet in Wireshark and still not know what the packet *means*. The packet-details tree lists flags and numbers, but it does not say "this is the forged reset" or "this is the sequence number your attack script has to use". The Explain panel turns the packet list into a guided walkthrough: each click answers, in one or two sentences with the real values filled in, what the packet does and why it matters for the lab. Along the way it teaches the protocol mechanics the labs depend on: the steps of the TCP handshake, sequence numbers plus payload length, ARP request and reply semantics, and DNS transaction IDs.

*Example:* in the TCP session hijacking lab, the student clicks the victim's last Telnet data packet. The panel reads *"Data segment carrying 4 bytes. The receiver next expects sequence number 1005 (seq + length); a session-hijacking attacker must forge exactly this number."* That is the value the student's attack script needs, already calculated.

### Modified Files List

| File | Change | Reason |
|---|---|---|
| `ui/qt/explain_panel.h` | New (27 lines) | Declares `ExplainPanel`, a `QDockWidget` with the `framesSelected(QList<int>)` and `setLab(int)` slots. |
| `ui/qt/explain_panel.cpp` | New (112 lines) | Reads fields from the selected packet's dissection tree and builds the bold heading from the available addresses and ports. It applies the explanation rules in order (ARP, ICMP, TCP, DNS, then the fallback; the first match wins) and shows the result with the lab hint from `lab_profiles.h`. It shows the dock only while a lab is active and returns to the placeholder text when no single packet is selected or the file is closed. |
| `ui/qt/main_window.cpp` | +10 lines | Includes the header, creates the dock at the bottom, adds it to the View menu, and connects it to `PacketList::framesSelected` and `LabToolBar::labChanged`. |

---

## Build-script change (not counted as C/C++)

| File | Change | Reason |
|---|---|---|
| `ui/qt/CMakeLists.txt` | +8 lines | Adds the new headers to `WIRESHARK_QT_HEADERS` and the new sources to `WIRESHARK_QT_SRC`: `lab_toolbar`, `field_copy_panel`, `lab_style` and `explain_panel`, one header and one source each. `lab_profiles.h` is header-only data and is not listed, like other plain headers in that directory. With CMake's automatic `moc` step, the new `Q_OBJECT` classes are compiled without further changes. |
