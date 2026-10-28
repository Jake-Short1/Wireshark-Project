# Lab Mode, Module 1: Lab Selector + Preset Filter Toolbar

## What is in this folder

| Path | What it is |
|---|---|
| `module1.diff` | `git diff v3.2.3` output. Apply with `git apply module1.diff` on a clean v3.2.3 checkout. |
| `src/ui/qt/` | Full copies of the 3 new files and the 4 changed files (the same content as the diff). |
| `demo/make_lab_demo_pcap.py` | Scapy script that writes `demo/lab_demo.pcap` (21 packets, covering every preset). |
| `demo/lab_demo.pcap` | The generated demo capture. |
| `screenshots/` | Screenshots from the real build: startup, TCP lab, RST preset, Shift+click, ARP lab, Off. |

## Verification report

All checks below were run on **Ubuntu 20.04.6 with gcc 9.4.0, Qt 5.12.8 and CMake 3.16.3**. That environment was a dedicated Ubuntu 20.04 WSL distro using the same package versions as the SEED VM, not the SEED VM itself. Rebuild once on the VM before the demo.

1. **Build.** A full baseline build of pristine v3.2.3 passed. After the patch, `cmake . && make -j16` passed with **0 errors and 0 compiler warnings**, both in the touched files and in the 17 files that were recompiled because `main_window.h` changed. The flags were `-Wall -Wextra -Wextra-semi ... -std=c++11`. Note: v3.2.3 does not add `-Werror` to the `qtui` target. The baseline build already had 127 warnings in unrelated upstream files; none were added.
2. **Live GUI test** (Xvfb, with `demo/lab_demo.pcap`):
   - The "Lab:" toolbar appears on its own row under the filter bar, set to "Off". Startup does not change the filter (21/21 shown).
   - Choosing each lab applies its base filter: `tcp` 9, `arp` 3, `icmp` 4, `dns` 4, `ssh || tls || udp` 8 packets. The preset buttons are replaced each time.
   - Presets tested in the GUI: RST packets (1), Duplicate address (1), Redirect (1), NS records (1), SSH (1). Each time the filter bar shows the text, it turns green, and the packet list updates.
   - Shift+click (SYN only, Retransmissions) fills the filter bar without applying it: the list stays unchanged and the apply arrow is highlighted.
   - The preset tooltip shows the tip plus "Shift+click to edit before applying."
   - Choosing **Off** clears the filter (21/21) and removes the buttons.
   - *View → Filter Toolbar* hides and shows both bars. The hidden state is saved in `recent` and restored at the next startup.
   - 20 rapid lab switches: no crash, resident memory unchanged (233,988 KB before and after), clean exit, empty stderr.
3. **dftest:** all 25 filters (20 presets and 5 base filters) were read directly from the compiled `lab_profiles.h` table, and every one passed `./run/dftest`. The match counts on the demo pcap were TCP 9/1/1/1/7/1, ARP 3/1/2/1, ICMP 4/2/1/1/1, DNS 4/2/2/1/1, Tunnel 8/1/1/4/6. "Echo request" matches 2 packets because the redirect quotes the original ping. That is normal Wireshark behaviour.
4. **No capture loaded:** the app starts, lab selection and presets update the filter bar, Shift+click works, Off clears the filter, and there is no crash.
5. **Line count** (`git diff v3.2.3 --numstat -- '*.cpp' '*.h'`): **156 added, 0 deleted.** That is under the 160 hard cap but 6 over the 150 target. Breakdown: `lab_profiles.h` 50, `lab_toolbar.cpp` 49, `lab_toolbar.h` 33, `main_window_slots.cpp` 13, `main_window.cpp` 8, `main_window.h` 3. CMake adds 2 lines, which are not counted.
6. **Patch:** `git diff v3.2.3 --check` is clean. `git apply --check module1.diff` succeeds on a fresh v3.2.3 clone, and the applied tree is identical to the tested tree.

### Differences from the spec (source wins)

- `git describe --tags --exact-match` prints **`wireshark-3.2.3`**, because the commit (f39b508) carries both tags. `git describe --tags --exact-match --match 'v*'` prints `v3.2.3`.
- `git diff v3.2.3` **ignores new untracked files**. Run `git add -N ui/qt/lab_profiles.h ui/qt/lab_toolbar.h ui/qt/lab_toolbar.cpp` first, or the diff is missing them.
- **Not done:** disabling the toolbar during startup. `setFeaturesEnabled(false)` runs at `main_window.cpp:351`, before any filter widget (or `lab_toolbar_`) exists. A one-liner there would use an uninitialised pointer, so I skipped it as the spec allows. Startup runs before the event loop, so there is no practical risk.
- **Done:** show/hide with the filter toolbar (2 lines in `layoutToolbars()` and `showHideMainWidgets()`).
- `main_window.h` **includes** `lab_toolbar.h` instead of forward-declaring it. This saves a line and gives teammates `LabId` and `lab_profiles` wherever `main_window.h` is included.
- Old buttons are removed with `qDeleteAll(preset_actions_)` instead of a `removeAction()` + `delete` loop. Deleting a `QAction` already removes it from every widget, so the effect is the same with 3 fewer lines.
- `lab_profiles.h` is not listed in CMake, because neighbouring plain headers (`simple_dialog.h`, `capture_event.h`) are not listed either.
- Every line number in the spec matched v3.2.3.

## Teammate integration notes

```cpp
// main_window.cpp, under the "Lab Mode integration point" comment (main_window.h already includes lab_toolbar.h):
connect(lab_toolbar_, &LabToolBar::labChanged, my_widget_, &MyWidget::onLabChanged);

void MyWidget::onLabChanged(int lab_id)          // fires on every pick, incl. re-picks and LAB_OFF
{
    const LabProfile &lab = lab_profiles[lab_id];  // lab.title, lab.hint, lab.base_filter,
                                                   // lab.presets[0 .. lab.n_presets - 1]
    if (lab_id == LAB_OFF) { /* restore normal Wireshark behaviour */ }
}
```
- Call `lab_toolbar_->currentLab()` at any time to get a `LabId`. Startup is `LAB_OFF`, and `labChanged` is **not** emitted at startup.
- **Do not change the shapes in `lab_profiles.h`.** The enum order, struct fields and table layout stay as they are, because the table is indexed by `LabId`. Only edit text (titles, hints, tips), and run any new filter through `./run/dftest` first.
- Module 1 is at 156 of its 160 lines, so put your code in your own files.

## README fragment

### Feature Title & Description

**Lab Mode: One-Click Display Filters for the Course Labs**

Lab Mode adds a **Lab:** toolbar directly below Wireshark's display filter bar. Its drop-down menu lists the course labs: *TCP (reset / hijack)*, *ARP (cache poisoning / MITM)*, *ICMP (redirect)*, *DNS (local / remote)* and *Tunnel / VPN*. It also offers *Off (normal Wireshark)*, which is the default at startup.

Choosing a lab immediately applies that lab's base display filter, for example `tcp` or `arp`. It also shows a row of preset buttons for the packets that matter in that lab, such as *SYN only*, *RST packets* and *Retransmissions* in the TCP lab. Clicking a preset applies its filter. The filter text appears in the filter bar, which turns green, and the packet list updates at once. **Shift+click** places the filter in the filter bar without applying it, so the student can edit it first. Wireshark's own filter buttons use the same convention. Hovering over a lab shows a one-sentence hint about what to look for, and hovering over a preset explains what it isolates. Choosing **Off** clears the display filter and removes the preset buttons. The Lab toolbar is shown and hidden together with the filter toolbar (*View → Filter Toolbar*).

**How to trigger it:** start Wireshark, open a capture or start a live capture, and pick a lab from the **Lab:** drop-down menu.

### Justification

Display filters are Wireshark's most powerful feature, but beginners rarely know the field names or the exact syntax. Typing `ssl` instead of `tls`, or `tcp.flags.rst` instead of `tcp.flags.reset`, is a typical mistake. During a lab, the student's attention should be on the attack, not on looking up filter syntax. Lab Mode packages the filters each course lab needs into one click, and every filter has been validated against Wireshark 3.2.3 with `dftest`. Because the filter text is written into the normal filter bar, students also learn the syntax by seeing it, and they can Shift+click a preset to refine it.

*Example:* in the TCP reset lab, the student selects *TCP (reset / hijack)* and clicks *RST packets*. The forged reset is then the only packet on screen, ready to compare with the sequence number the victim expects next. In the ARP cache poisoning lab, *Duplicate address* isolates the attacker's forged reply, which Wireshark flags because it claims the gateway's IP address with a different MAC address.

Lab Mode is off by default and never changes the user's filter at startup, so experienced users see normal Wireshark. It is also the foundation for the other Lab Mode modules, which react to the selected lab through the `labChanged` signal and the shared `lab_profiles` table.

### Modified Files List

| File | Change | Reason |
|---|---|---|
| `ui/qt/lab_profiles.h` | New (50 lines) | Shared, data-only table of the labs: title, hint, base filter and up to five preset filters with tooltips, indexed by `LabId`. Used by all Lab Mode modules. |
| `ui/qt/lab_toolbar.h` | New (33 lines) | Declares `LabToolBar`, a `QToolBar` with the `labChanged(int)` and `filterRequested(QString, bool)` signals. |
| `ui/qt/lab_toolbar.cpp` | New (49 lines) | Builds the "Lab:" label and the lab drop-down. On each selection it rebuilds the preset buttons and emits the signals. |
| `ui/qt/main_window.h` | +3 lines | Includes `lab_toolbar.h` and adds the `lab_toolbar_` member and the `onLabFilterRequested()` slot. |
| `ui/qt/main_window.cpp` | +8 lines | Creates the toolbar on its own row below the filter bar, connects it, and marks the integration point for the other modules. |
| `ui/qt/main_window_slots.cpp` | +13 lines | `onLabFilterRequested()` applies the filter through the existing `onFilterSelected()`, and handles `""` (clear), which `onFilterSelected()` ignores. The Lab toolbar's visibility follows the filter toolbar. |
| `ui/qt/CMakeLists.txt` | +2 lines (build script) | Adds `lab_toolbar.h` and `lab_toolbar.cpp` to the Qt UI target, so AUTOMOC generates the meta-object code. |

## How to explain this code (live demo)

- **`lab_profiles.h` is just data.** There is one `LabProfile` per course lab, stored in an array indexed by `LabId`, so `lab_profiles[LAB_TCP]` is the TCP lab. Each profile has a title, a hint, a base filter and up to five `LabPreset`s (label, filter, tip). Adding text never needs code changes.
- **The `LabToolBar` constructor** adds a `QLabel("Lab:")` and a `QComboBox` with one entry per lab. The combo index equals the `LabId`, and each entry's tooltip is the lab's hint. It connects `activated(int)` with `SIGNAL()`/`SLOT()`, which picks the `int` overload without C++14's `QOverload`. `activated` fires only when the user picks a lab, so startup never touches the filter.
- **`labActivated(index)`** deletes the old preset buttons (`qDeleteAll`; a deleted `QAction` also leaves the toolbar). It then creates one `QAction` per preset with its tooltip, and connects each one to a small lambda that holds a copy of its preset. Finally it emits `filterRequested(base_filter, false)` and `labChanged(id)`. For Off, the base filter is `""`.
- **`MainWindow`** creates the toolbar with `addToolBar()`, and `insertToolBarBreak()` puts it on its own row. It connects `filterRequested` to `onLabFilterRequested`. That slot reuses Wireshark's `onFilterSelected()` and only adds the "empty means clear" case. The toolbar's visibility follows *View → Filter Toolbar*.
- **Signal flow:** preset click → `QAction::triggered` → lambda → `emit filterRequested(filter, shiftHeld)` → `MainWindow::onLabFilterRequested()` → `onFilterSelected()` → `DisplayFilterCombo::setDisplayFilter()`, which updates the bar text and turns it green if valid. If Shift is not held, `applyDisplayFilter()` follows, then `DisplayFilterEdit` emits `filterPackets`, then `MainWindow::filterPackets()` → `cf_filter_packets()`, which refreshes the packet list.
