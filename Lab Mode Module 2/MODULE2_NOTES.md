# Lab Mode, Module 2: Field Copy Panel (notes)

## README fragment

### Feature Title & Description

**Lab Mode: Field Copy Panel**

The Field Copy panel is a dock titled **Lab Fields (Copy)** on the right side of the main window. When the student selects a packet, the panel lists the values they most often type into the SEED lab attack scripts. These are the source and destination MAC and IP addresses, the IP ID and TTL, TCP and UDP ports, the **raw** TCP sequence and acknowledgment numbers, the TCP payload length, flags and window, the ICMP type, code and redirect gateway, the ARP opcode, sender and target, and the DNS transaction ID and query name. Only the fields present in the selected packet are shown. Each value can be selected with the mouse, and the **Copy** button next to it puts the value on the clipboard (the button briefly reads "Copied!"). When no packet or several packets are selected, or when the file is closed, the panel shows "Select one packet to see its key fields."

**How to trigger it:** open a capture (or start a live capture) and click any packet. If the panel has been closed, reopen it from **View → Lab Fields (Copy)**.

### Justification

The SEED attack scripts need exact values from captured packets: the next sequence number for a TCP reset or hijack, the gateway's MAC address for ARP poisoning, the transaction ID for DNS spoofing. Beginners find these values by expanding several levels of the packet-details tree, and they often copy the wrong one. The most common mistake is Wireshark's *relative* sequence number (1) instead of the raw one the script needs. Retyping a ten-digit number by hand adds typos. The panel gathers the values in one place, always shows raw TCP numbers, and copies each one with a single click.

*Example:* in the TCP session hijacking lab, the student selects the victim's last Telnet packet. The panel shows *TCP seq (raw)* and *TCP ack (raw)*, and two Copy clicks put the exact numbers into the scapy script, with no tree navigation and no relative-number mistakes.

The panel works with every lab and does not depend on the Lab selector, so it is useful even when Lab Mode is off.

### Modified Files List

| File | Change | Reason |
|---|---|---|
| `ui/qt/field_copy_panel.h` | New (27 lines) | Declares `FieldCopyPanel`, a `QDockWidget` with the `framesSelected(QList<int>)` slot. |
| `ui/qt/field_copy_panel.cpp` | New (113 lines) | The field table, the lookup of each field in the selected packet's dissection tree, the Copy buttons and the hint. |
| `ui/qt/main_window.cpp` | +9 lines | Includes the header, creates the dock on the right, connects it to `PacketList::framesSelected`, and adds its show/hide action to the View menu. Both hooks are wrapped in `Lab Mode: Module 2 begin/end` markers. |
| `ui/qt/CMakeLists.txt` | +2 lines (build script) | Adds the new files after `proto_tree.h` / `proto_tree.cpp` so AUTOMOC builds them. |

## How to explain this code (live demo)

- **The field table (`field_specs[]`)** pairs a readable label with a Wireshark field name, such as `"TCP seq (raw)"` with `tcp.seq_raw`. The panel walks this table in order, so adding a field means adding one entry.
- **Wiring (`main_window.cpp`):** the main window creates the dock with itself as the parent (Qt deletes it automatically) and docks it on the right. It connects the packet list's `framesSelected` signal to the panel, and adds the dock's toggle action to the View menu so a closed panel can be reopened.
- **On every selection, `framesSelected()`** checks for exactly one row *and* a dissected packet (`cf->edt`). A deselect also arrives as a single row, so the row count alone is not enough. It builds a fresh page: for each table entry, `fieldText()` looks the field up in the packet's tree. Empty values are skipped; otherwise it adds a label, a selectable monospace value and a Copy button. `QScrollArea::setWidget()` then replaces the old page and deletes it with all its widgets, so nothing leaks.
- **`fieldText()`** turns the field name into an ID (`proto_registrar_get_id_byname`). It finds the first, outermost occurrence in the tree with `proto_find_first_finfo`, and converts the value to text exactly as Wireshark displays it (`fvalue_to_string_repr`).
- **Data flow:** click a packet → `PacketList::selectionChanged` dissects it (`cf_select_packet`) → emits `framesSelected` → `FieldCopyPanel::framesSelected` reads `cf->edt` → rows appear. Click **Copy** → the lambda calls `QApplication::clipboard()->setText(value)` and shows "Copied!". A one-second `QTimer::singleShot` restores "Copy"; it holds a `QPointer`, so it does nothing if the row was rebuilt in the meantime. Closing the file emits `CaptureFile::captureEvent(Closed)`, which resets the panel to the hint.

## Line-count report

`git diff <module1-base-commit> --numstat -- '*.cpp' '*.h'`:

| File | Added | Deleted |
|---|---|---|
| `ui/qt/field_copy_panel.cpp` | 113 | 0 |
| `ui/qt/field_copy_panel.h` | 27 | 0 |
| `ui/qt/main_window.cpp` | 9 | 0 |
| **Total** | **149** | **0** |

That is under the 150 hard cap and 9 over the 110–140 target. `CMakeLists.txt` adds 2 lines, which are not counted. The optional "Copy all" button was left out because the module was already over 140.

## Verification checklist

Environment: Ubuntu 20.04.6, gcc 9.4.0, Qt 5.12.8, CMake 3.16.3. This was a WSL distro set up with the same packages as the SEED VM, not the SEED VM itself. Wireshark v3.2.3 with Module 1 applied and committed as the base. Test captures: `demo/lab_demo.pcap` (21 packets: TCP handshake, Telnet data, retransmission, forged RST, ARP request/replies, ICMP echo/redirect/unreachable, DNS query/response/NXDOMAIN, SSH, TLS, UDP) and `big_demo.pcap` (the same file concatenated 30 times with `mergecap -a`, 630 packets).

| # | Check | Result |
|---|---|---|
| 1 | Clean build, zero new warnings in our files | **PASS**: `make` exit 0, 0 compiler diagnostics (`-Wall -Wextra -std=c++11`) in `field_copy_panel.cpp`, `main_window.cpp` and the other recompiled files |
| 2 | TCP data packet: values present; Copy output matches; `tcp.seq_raw == <pasted>` selects it | **PASS**: frame 7 shows ports 40000 → 23, seq (raw) 1001, ack (raw) 5001, length 4, flags, window. The clipboard (read with `xclip`) gave `1001`, `5001` and `02:42:0a:09:00:06`, matching `tshark -T fields -e tcp.seq_raw -e tcp.ack_raw -e eth.dst`, which uses the same formatting as *Copy → Value*. `tcp.seq_raw == 1001` matches frames 6, 7 and 8 (they share that sequence number). The Copy button shows "Copied!" and reverts after one second. I compared against tshark, not by pasting into a text editor. |
| 3 | ARP, ICMP redirect/echo and DNS packets show only relevant rows | **PASS**: ARP shows MAC and ARP rows only. Echo shows IP plus ICMP type 8/code 0. Redirect shows the outer IP header plus type 5, code 1, gateway 10.9.0.105 (not the quoted inner packet). DNS shows UDP ports, ID `0x00001234` and query name. |
| 4 | Nothing selected, multi-select, no file, file closed: no crash, hint shown | **PASS**: multi-select and Ctrl+click deselect show the hint, and going back to one row restores the fields. With no file loaded (after File → Close) the hint is shown and the app keeps running. Closing the file needed the `captureEvent(Closed)` handler, because `PacketList` does not emit `framesSelected` once the file is gone. |
| 5 | Close the dock, reopen through the View menu | **PASS**: "Lab Fields (Copy)" appears at the bottom of the View menu and brings the dock back. |
| 6 | No leaks: valgrind, or no growth over 500 packets | **PASS**: stepping 500 packets with the Down key three times on the 630-packet capture gave RSS 221,664 → 222,248 → 222,252 → 222,252 KB. Only the first pass grows, from Wireshark's own dissection caches. **Valgrind** (`memcheck --leak-check=full`, 2 × 20 packet steps on `lab_demo.pcap`, File → Close, quit): 0 loss records and 0 invalid reads/writes involving `field_copy_panel` / `FieldCopyPanel`. The only "definitely lost" blocks (352 bytes) are in upstream code: `WirelessTimeline::tap_timeline_reset` on file close, and system `libfontconfig`. |
| 7 | Line count ≤ 150 | **PASS**: 149 |

Also verified: all 25 field names exist in v3.2.3 (`./run/dftest <field>`). `module2.diff` applies with `git apply --check` to a fresh v3.2.3 checkout plus `module1.diff`, and the result is byte-identical to the tested tree. `git diff --check` reports no whitespace problems.

## Differences from the spec (source wins)

- **`proto_get_finfo_ptr_array()` → `proto_find_first_finfo()`.** In v3.2.3, `proto_get_finfo_ptr_array()` only returns fields registered as "interesting", meaning used by an active display filter or tap (`epan/proto.c`, line 10013). The selected packet is dissected without priming our fields, so the spec's helper would return nothing for most rows. `proto_find_first_finfo()` searches the whole tree and returns the first (outermost) match. The caller frees the array with `g_ptr_array_free`.
- **Hook position:** the `framesSelected` connect is at line 477 of `main_window.cpp`, not ~469, because Module 1 added 8 lines above it.
- **Deselect:** `PacketList` reports "nothing selected" as a one-element list containing `-1`, so the panel also checks `cf->edt` rather than relying only on `rows.size() == 1`.
- **File close:** no `framesSelected` is emitted on close (`PacketList::selectionChanged` returns early when the file is gone), so the panel listens to `CaptureFile::captureEvent` for `Closed` (4 lines).
- **Dock width:** the dock opened so narrow that the Copy buttons were hidden behind a scrollbar. One line sets a minimum width of about 60 average character widths.
- **Value format:** values use Wireshark's own `FTREPR_DISPLAY` formatting, the same as tshark and *Copy → Value*. Hex-displayed integers (IP ID, DNS ID) therefore come out as `0x00001234`, a valid Python or C literal.
- The `#include "field_copy_panel.h"` line is also wrapped in Module 2 markers, so every hook in `main_window.cpp` is clearly attributed.
- "Copy all" (the stretch goal) is not included; see the line-count report.
