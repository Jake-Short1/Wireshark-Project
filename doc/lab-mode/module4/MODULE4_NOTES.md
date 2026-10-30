# Lab Mode, Module 4: Explain This Packet (notes)

## README fragment

### Feature Title & Description

**Lab Mode: Explain This Packet**

Explain This Packet is a dock at the bottom of the main window that appears whenever a lab is selected in the **Lab:** toolbar. When the student clicks a packet, the dock shows three things:
- a **bold first line** naming the protocol and endpoints, for example *TCP from 10.9.0.6:23 to 10.9.0.5:40000*,
- **one or two plain-English sentences** explaining what the packet does, quoting its real values,
- the selected lab's **"what to look for" hint** in italics.

| Packet | Explanation (shortened) |
|---|---|
| TCP RST | One side is abruptly closing the connection; in the TCP reset lab, this is the forged packet. |
| TCP SYN / SYN-ACK | Step 1 / step 2 of the three-way handshake, with the raw initial sequence number / acknowledged number. |
| TCP data | Number of bytes and the **next sequence number the receiver expects** (seq + length), which a hijacker must forge. |
| TCP FIN / ACK | Normal close / acknowledgment up to the raw ack number. |
| ARP request / reply | Who asks for which IP; *"10.9.0.1 is at 02:42:0a:09:00:69"*, with a warning about unsolicited replies. |
| ICMP | Echo request/reply, redirect (with the announced gateway), destination unreachable. |
| DNS | Query or response, with the query name and transaction ID, and why a spoofer must match the ID. |

Other packets get "No explanation is available for this kind of packet in this lab." The dock is hidden while Lab Mode is **Off**. It can be closed and reopened through **View → Explain This Packet**.

**How to trigger it:** select a lab in the **Lab:** drop-down menu, then click any packet.

### Justification

Beginners can see every byte of a packet in Wireshark and still not know what the packet *means*. The packet-details tree lists flags and numbers, but it does not say "this is the forged reset" or "this is the sequence number your attack script has to use". The Explain panel turns the packet list into a guided walkthrough. Each click answers "what is this packet doing, and why does it matter for this lab?" in one or two sentences with the real values filled in. It also teaches the protocol mechanics the labs depend on: the handshake steps, sequence plus length, ARP request/reply semantics, and DNS transaction IDs.

*Example:* in the TCP session hijacking lab, the student clicks the victim's last Telnet data packet. The panel reads *"Data segment carrying 4 bytes. The receiver next expects sequence number 1005 (seq + length); a session-hijacking attacker must forge exactly this number."* That is exactly the value the student's scapy script needs, already computed.

### Modified Files List

| File | Change | Reason |
|---|---|---|
| `ui/qt/explain_panel.h` | New (27 lines) | Declares `ExplainPanel`, a `QDockWidget` with the `framesSelected(QList<int>)` and `setLab(int)` slots. |
| `ui/qt/explain_panel.cpp` | New (112 lines) | Reads fields from the selected packet, applies the explanation rules, and shows the text with the lab hint from `lab_profiles.h`. |
| `ui/qt/main_window.cpp` | +10 lines | Includes the header, creates the dock at the bottom, adds it to the View menu, and connects it to `PacketList::framesSelected` and `LabToolBar::labChanged`. Both hooks are wrapped in `Lab Mode: Module 4 begin/end` markers. |
| `ui/qt/CMakeLists.txt` | +2 lines (build script) | Adds the new files after `welcome_page.h` / `.cpp`. |

## How to explain this code (live demo)

- **Wiring (`main_window.cpp`):** the main window creates the dock with itself as the parent (Qt deletes it) and docks it at the bottom. It adds its show/hide action to the View menu, connects the packet list's `framesSelected` to the panel, and connects the Lab toolbar's `labChanged` to `setLab`.
- **`setLab(id)`** remembers the lab, shows the dock for any lab, hides it for *Off*, and redraws the text so the hint line matches the new lab.
- **`framesSelected(rows)`** rebuilds the text. It explains the packet only if the packet list sent at most one row and Wireshark has a dissection of the selected packet (`cf->edt`). Otherwise it shows "Select one packet to see an explanation." Packet values are HTML-escaped before they go into the rich-text label.
- **`explain()`** reads fields with the helper `f()`, which finds the first (outermost) occurrence of a field by name and converts it to display text. It then walks the rules in order: ARP, ICMP, TCP (RST → SYN → SYN-ACK → FIN → data → ACK), DNS, then the fallback. The first match wins. ICMP is checked before TCP because ICMP errors carry a quoted copy of a TCP/UDP header. `title()` builds the bold line and leaves out addresses or ports the packet does not have, so no empty placeholders appear.
- **Data flow:** pick a lab → `labChanged(id)` → `setLab()` shows the dock. Click a packet → Wireshark dissects it (`cf_select_packet`) → `framesSelected` → `explain()` reads the fields → the label shows the bold heading, the explanation and the italic lab hint. Closing a file fires a capture-file event, which redraws the placeholder.

## Line-count report

`git diff <module1-base-commit> --numstat -- '*.cpp' '*.h'`:

| File | Added | Deleted |
|---|---|---|
| `ui/qt/explain_panel.cpp` | 112 | 0 |
| `ui/qt/explain_panel.h` | 27 | 0 |
| `ui/qt/main_window.cpp` | 10 | 0 |
| **Total** | **149** | **0** |

That is under the 150 hard cap and 9 over the 110–140 target. Thirteen explanation rules plus the field helper did not fit in 140 without dropping rules. To stay under the cap:
- each rule is a single line (several lines are longer than 120 characters),
- the explanation strings are plain strings, not `tr()`.

`CMakeLists.txt` adds 2 lines, which are not counted.

**All four modules together** (v3.2.3 + `module1.diff` + `module2.diff` + `module3.diff` + `module4.diff`): **590 added C/C++ lines**, within the assignment's 400–600.

## Verification checklist

**Environment:** Ubuntu 20.04.6, gcc 9.4.0, Qt 5.12.8, CMake 3.16.3. This was a WSL distro set up with the same packages as the SEED VM, not the SEED VM itself. Wireshark v3.2.3 with Module 1 applied and committed as the base.

**Test capture:** `demo/explain_demo.pcap`, which is `lab_demo.pcap` (TCP handshake, Telnet data, retransmission, forged RST, ARP request plus real and forged replies, ICMP echo/redirect/unreachable, DNS query/response/NXDOMAIN, SSH, TLS, UDP) plus a normal close (FIN, FIN, ACK), 24 packets. Values were compared with `tshark -T fields`.

| # | Check | Result |
|---|---|---|
| 1 | Clean build, zero new warnings | **PASS**: `make` exit 0, 0 compiler diagnostics (`-Wall -Wextra -std=c++11`). All 21 fields used exist (`./run/dftest`). |
| 2 | Off hides the dock; TCP shows it with the hint; SYN, SYN-ACK, data, RST, FIN explained correctly with the raw numbers | **PASS**: Off: no dock. TCP: dock with hint. Results (all match tshark): SYN "Initial sequence number: 1000"; SYN-ACK "acknowledges 1001"; ACK "up to 5001"; data "4 bytes ... next expects 1005"; RST sentence; FIN (frame 22) "closing the connection normally"; final ACK (frame 24) "up to 6001". |
| 3 | ARP, ICMP (redirect, echo), DNS (query, response): correct sentences and values | **PASS**: ARP request "10.9.0.5 asks who owns 10.9.0.1". Forged reply "10.9.0.1 is at 02:42:0a:09:00:69". Echo request/reply. Redirect "send traffic to 10.9.0.105". Unreachable. DNS query "www.example.com (transaction ID 0x00001234)". Responses for `www.example.com` / `nope.example.com` (0x00001234 / 0x00001235). |
| 4 | Uncovered packet type: fallback, no crash; no selection / multi-select / no file: placeholder | **PASS, with one difference**: the UDP packet (frame 21) gets "Packet from 10.9.0.5 to 10.9.0.6" plus the fallback sentence. **TLS is not a fallback case**: it travels in TCP segments, so the TCP data rule explains it (as the spec's own first-match rules require). Multi-select, a file closed with File → Close, and a start with no file all show the placeholder; no crash. A plain Ctrl+click deselect was not tested separately (it clears `edt` the same way). |
| 5 | Close with ×, reopen from the View menu; switching labs behaves | **PASS**: "Explain This Packet" appears at the bottom of the View menu and restores the dock. Off hides it, and picking ARP shows it again with the ARP explanation. |
| 6 | No empty placeholders ("<>", "from : to :") | **PASS**: none in any of the 18 packets checked. `title()` drops addresses or ports that are missing, and rules whose value is missing are skipped (ACK) or use a neutral word ("a name" for a DNS packet without a question). |
| 7 | Line count ≤ 150 | **PASS**: 149 |

**Also verified:**
- `module4.diff` applies with `git apply --check` to a fresh v3.2.3 + `module1.diff`, and the result is byte-identical to the tested tree.
- Applied after Modules 2 and 3, all four apply cleanly with plain `git apply`.
- Other orders also apply cleanly after `module1.diff`: 4-3-2, 3-4-2, 2-4-3, 4-2 and 3-4.
- A full from-scratch build of v3.2.3 + Modules 1–4 passed: `make` exit 0, 0 compiler diagnostics in any Lab Mode file. In a smoke test of that build, all four features worked at once on the forged RST: the TCP lab filter (1), the Field Copy dock (2), the lab columns and colors (3), and the Explain dock (4). The app quit cleanly (`screenshots/8_all_four_modules_together.png`).

## Differences from the spec (source wins)

- **`proto_get_finfo_ptr_array()` → `proto_find_first_finfo()`.** In v3.2.3, `proto_get_finfo_ptr_array()` only returns fields that an active display filter or tap asked for (`epan/proto.c`), so it misses most fields here. `proto_find_first_finfo()` searches the whole tree; the caller frees the array.
- **Booleans display as "1"/"0"** in v3.2.3, not "True" (checked with tshark), so flags are compared with `"1"`. A missing field compares as not set. No separate `isSet()` helper was needed.
- **No separate "last selection" member.** Wireshark keeps a dissection (`cf->edt`) of the selected packet only while exactly one packet is selected, so `setLab()` and file events call `framesSelected()` with an empty list and rely on `edt`. One catch, found in testing: on a multi-select the packet list emits `framesSelected` *before* it drops the old `edt`, so more than one row is also treated as "no single packet".
- **File close:** closing a file emits no `framesSelected`, so the panel also refreshes on every `CaptureFile::captureEvent`. This is safe because `cf_close()` clears `edt` (`cf_unselect_packet`) before it reports the file as closed.
- **Hook positions:**
  - The `mainStack` connect is at line 545, not ~537.
  - `lab_toolbar_` (line 404) and `packet_list_` (line 473) already exist there.
  - The `#include` is placed after `wireshark_application.h`, not in alphabetical position. Next to `export_*.h` its hunk overlapped Module 2's include hunk, and `module2.diff` + `module4.diff` would not apply together.
- **Rule order:** ARP and ICMP are checked before TCP, because ICMP error messages contain a quoted TCP/UDP header that would otherwise be explained as TCP. The TCP data rule also computes the next expected sequence number (seq + length, with 32-bit wrap-around).
- **Wording** follows the spec's table; the lab handouts were not attached.
- **Cosmetic:** the text has no left margin. Adding one would have taken the module over the 150-line cap.
