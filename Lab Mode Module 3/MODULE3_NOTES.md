# Lab Mode, Module 3: Lab Styling (notes)

## README fragment

### Feature Title & Description

**Lab Mode: Lab Styling**

Lab Styling makes the packet list adapt to the lab chosen in the **Lab:** toolbar. When the student selects a lab, Wireshark colors the packets that matter for that lab and adds up to three columns with the values the lab needs. The five labs are:

| Lab | Colored packets | Extra columns |
|---|---|---|
| TCP (reset / hijack) | RST (light red), Telnet data (yellow), SYN (green), SYN-ACK (cyan) | Seq (raw), Ack (raw), Flags |
| ARP (cache poisoning / MITM) | duplicate-address replies, i.e. poisoning (light red), replies (yellow), requests (blue) | ARP op, Sender MAC, Sender IP |
| ICMP (redirect) | redirects (light red), unreachable (yellow), echo request (green), echo reply (cyan) | ICMP type, ICMP code, Gateway |
| DNS (local / remote) | error responses (pink), responses (yellow), queries (blue) | DNS ID, Query, Rcode |
| Tunnel / VPN | SSH (light red), TLS (cyan), ICMP (yellow) | TCP stream |

Selecting another lab removes the previous lab's colors and columns before applying the new ones, and **Off** restores the normal look. The colors use Wireshark's temporary "Colorize Conversation" slots, so the user's coloring rules are untouched. The columns exist only in memory: they are never saved to the preferences file and are removed when Wireshark quits.

**How to trigger it:** open a capture (or start a live capture) and pick a lab from the **Lab:** drop-down menu.

### Justification

In a busy capture, the handful of packets that prove an attack worked is easy to miss among hundreds of ordinary ones. The values the student needs, such as the raw sequence number or the MAC address in an ARP reply, are hidden several levels deep in the packet-details tree. Lab Styling makes the important packets stand out at a glance and puts the important values directly in the packet list. Beginners spend their time understanding the attack instead of searching the GUI. Because the styling switches automatically with the lab and is never saved, students cannot accidentally break their normal Wireshark setup.

*Example:* in the ARP cache poisoning lab, the student selects **ARP**. The attacker's forged reply turns light red (Wireshark flags it as a duplicate address), and the *Sender MAC* column shows the attacker's MAC next to the gateway's IP in the *Sender IP* column. The poisoning is visible in one row, without opening a single packet.

### Modified Files List

| File | Change | Reason |
|---|---|---|
| `ui/qt/lab_style.h` | New (28 lines) | Declares `LabStyle`, a `QObject` with the `setLab(int)` slot. |
| `ui/qt/lab_style.cpp` | New (101 lines) | The per-lab table of color rules and columns, plus the code that removes the previous lab's styling and applies the new one. |
| `ui/qt/main_window.cpp` | +7 lines | Includes the header, creates `LabStyle` (owned by the main window) and connects it to `LabToolBar::labChanged`. Both hooks are wrapped in `Lab Mode: Module 3 begin/end` markers. |
| `ui/qt/CMakeLists.txt` | +2 lines (build script) | Adds the new files after `coloring_rules_dialog.h` / `.cpp`. |

## How to explain this code (live demo)

- **The table (`lab_styles[]`)** has one entry per `LabId`. Each entry lists up to four color rules (a temporary color slot plus a display filter) and up to three columns (a title plus a field name). Unused entries are left empty (`NULL`), and the loops stop at the first empty one.
- **Wiring (`main_window.cpp`):** the main window creates `LabStyle` with itself as the parent, so Qt deletes it, and connects the Lab toolbar's `labChanged(int)` signal to `LabStyle::setLab(int)`.
- **`setLab()` first undoes the previous lab.** `color_filters_reset_tmp()` clears all ten temporary colors. `removeLabColumns()` walks the column list and removes only columns whose title *and* field match the previous lab's columns, so the user's own columns survive.
- **Then it applies the new lab.** `color_filters_set_tmp()` sets each rule (when rules overlap, the lower slot wins) and `column_prefs_add_custom()` inserts each column before *Info*. Then `columnsChanged()` and `recolorPackets()` make the packet list mark every row as stale; each row re-dissects itself when it is drawn, so the new columns and colors appear. `prefs_main_write()` is deliberately not called, so nothing reaches the preferences file.
- **Data flow:** pick a lab → `LabToolBar` emits `labChanged(id)` → `LabStyle::setLab(id)` → temporary color filters and in-memory columns are replaced → the packet list redraws with the new colors and columns. When Wireshark quits, an `aboutToQuit` handler removes the lab columns again.

## Line-count report

`git diff <module1-base-commit> --numstat -- '*.cpp' '*.h'`:

| File | Added | Deleted |
|---|---|---|
| `ui/qt/lab_style.cpp` | 101 | 0 |
| `ui/qt/lab_style.h` | 28 | 0 |
| `ui/qt/main_window.cpp` | 7 | 0 |
| **Total** | **136** | **0** |

That is within the 110–140 target and under the 150 cap. `CMakeLists.txt` adds 2 lines, which are not counted.

## Verification checklist

**Environment:** Ubuntu 20.04.6, gcc 9.4.0, Qt 5.12.8, CMake 3.16.3. This was a WSL distro set up with the same packages as the SEED VM, not the SEED VM itself. Wireshark v3.2.3 with Module 1 applied and committed as the base.

**Test capture:** `demo/lab_demo.pcap` (21 packets: TCP handshake, Telnet data, retransmission, forged RST, ARP request plus real and forged replies, ICMP echo/redirect/unreachable, DNS query/response/NXDOMAIN, SSH, TLS, UDP).

**Test preferences:** the test user's preferences file had one user-created custom column, **My Seq** (`tcp.seq_raw`). It uses the same field as a lab column, which proves that only the lab's own columns are removed.

| # | Check | Result |
|---|---|---|
| 1 | Clean build, zero new warnings | **PASS**: `make` exit 0, 0 compiler diagnostics (`-Wall -Wextra -std=c++11`). Every filter and field in the table passes `./run/dftest`. |
| 2 | TCP: RST / SYN / SYN-ACK / Telnet data colored; three columns populated; raw seq/ack correct | **PASS**: RST light red, Telnet data yellow, SYN green, SYN-ACK cyan. Seq/Ack (raw) for frames 4–10 (1000/0, 5000/1001, 1001/5001 ×3, 5001/1005, 5011/0) match `tshark -T fields -e tcp.seq_raw -e tcp.ack_raw`. |
| 3 | Switching ARP → ICMP → DNS → Tunnel removes the previous styling completely; no column growth after 20 switches | **PASS**: each lab showed only its own columns and colors. After 20 more switches, the TCP lab shows exactly one set of its three columns, the header is identical to the first time, and RSS stayed flat (220,872 → 220,776 KB). The column count was compared through the visible header; `prefs.num_cols` was not read directly. |
| 4 | Off restores the original look; user columns and default coloring rules untouched | **PASS**: after Off, the header (including *My Seq*) and the default colors match the screenshot taken before any lab was selected. |
| 5 | Quit with a lab active and restart: no leftover lab columns; `preferences` file not modified | **PASS**: the `preferences` file has the same MD5 and the same timestamp after quitting with TCP active. After restarting and selecting TCP, exactly one set of TCP columns appears, so none survived. Note: Wireshark writes its `recent` file in its own quit handler, which runs *before* ours, so `recent` stores column widths for the lab columns. These width entries are harmless, are not columns, and are dropped the next time `recent` is written. |
| 6 | No file → select labs → open a file: no crash, columns present after load | **PASS**: started with no file, picked TCP, then opened the capture from the welcome page. The TCP columns and colors were present right after loading. |
| 7 | Line count ≤ 150 | **PASS**: 136 |

**Also verified:**
- `module3.diff` applies with `git apply --check` to a fresh v3.2.3 checkout plus `module1.diff`, and the result is byte-identical to the tested tree.
- It also applies cleanly on top of `module1.diff` + `module2.diff`.
- A full from-scratch build of v3.2.3 + Modules 1, 2 and 3 together passed: `make` exit 0, 0 compiler diagnostics in any Lab Mode file. In a smoke test of that build, the TCP lab filter (Module 1), the Field Copy dock (Module 2) and the lab columns and colors (Module 3) all worked at once, and the app quit cleanly (`screenshots/9_modules_1_2_3_together.png`).
- `git diff --check` reports no whitespace problems.

## Differences from the spec (source wins)

- **No `CaptureFile` parameter, no `cf_redissect_packets()`.** `PacketList::columnsChanged()` calls `PacketListModel::resetColumns()`, which invalidates every row (`PacketListRecord::invalidateAllRecords`). Each row then re-dissects itself the next time it is drawn (`PacketListRecord::columnString`). Tested: the new columns are filled immediately without a full redissection, which would re-read the whole capture on every lab switch. Since `CaptureFile` would then be unused, the constructor is `LabStyle(PacketList *pl, QObject *parent)` and the hook passes `(packet_list_, this)`.
- **Column removal uses `column_prefs_remove_link()` on the list node** instead of `column_prefs_remove_nth(index)`. In v3.2.3, `remove_nth` only looks up the node and calls `remove_link`, which unlinks the node but does not free it. Walking the list and removing the node directly avoids index bookkeeping, and `g_list_free_1()` frees the node so it doesn't leak.
- **Only the previously applied lab's columns are removed**, matched by title *and* field. A user column with exactly the same title and field as a lab column (for example "Seq (raw)" / `tcp.seq_raw`) would be removed while that lab is active. A user column with the same field but a different title ("My Seq") is kept, as tested.
- **Color slots differ from the spec's 1/2/3/4.** The temporary colors are pastels: 1 light red, 2 pink, 3 lilac, 4 blue, 5 grey-teal, 6 cyan, 7 green, 8 yellow, 9 khaki, 10 grey. 3, 5, 9 and 10 are hard to tell apart from normal rows. The rules are evaluated in slot order (`color_filters.c` appends slots 1–10 in order; the first match wins), so a more specific rule must use a lower slot. The DNS "error" rule therefore uses slot 2, below the responses rule at slot 8; otherwise NXDOMAIN responses would just look like responses.
- **Hook position:** `setTabOrder(df_combo_box_->lineEdit(), packet_list_)` is at line 517, not ~509. `lab_toolbar_` is created earlier, at line 404, so it is non-NULL at the hook.
- **Include order:** `epan/color_filters.h` does not include `glib.h` itself, so `packet_list.h`, which brings in `config.h` and glib, is included first.
- **Known limitations (not tested):**
  - Selecting a lab clears any temporary "Colorize Conversation" colors the user set by hand, because `color_filters_reset_tmp()` resets all ten slots, as the spec requires.
  - If the user opens *Edit → Preferences* and clicks OK while a lab is active, Wireshark saves its whole column list, which would include the lab columns.
  - Custom columns show every occurrence of a field, so an ICMP redirect's *ICMP type* reads `5,8` (the outer redirect plus the quoted echo request).
