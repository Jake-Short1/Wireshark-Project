# Wireshark Lab Mode: Module 1 (Lab Selector + Preset Filter Toolbar)

This patch for **Wireshark v3.2.3** adds a **Lab:** toolbar below the display filter bar. Pick a course lab (TCP, ARP, ICMP, DNS, Tunnel / VPN) to apply its base display filter. Then click a preset button to apply one of that lab's most useful filters. Shift+click a preset to put the filter in the filter bar without applying it, so you can edit it first. Choosing **Off** clears the filter.

Tested on the SEED Labs VM: Ubuntu 20.04, gcc 9, Qt 5.12, CMake.

## What's in this repository

| Path | Contents |
|---|---|
| `module1.diff` | The patch (`git diff v3.2.3`) |
| `src/ui/qt/` | Full copies of the new and changed source files |
| `demo/lab_demo.pcap` | Small demo capture with traffic for every preset |
| `demo/make_lab_demo_pcap.py` | Script that regenerates the demo capture (needs `python3-scapy`) |
| `screenshots/` | Screenshots of the toolbar in action |
| `MODULE1_NOTES.md` | Design notes, the verification report and teammate integration notes |

## 1. Install the build dependencies

```bash
sudo apt-get update
sudo apt-get install -y cmake build-essential flex bison libglib2.0-dev libgcrypt20-dev libc-ares-dev libpcap-dev qtbase5-dev qttools5-dev qttools5-dev-tools qtmultimedia5-dev libqt5svg5-dev libpcre2-dev
```

## 2. Get Wireshark v3.2.3

```bash
git clone --depth 1 --branch v3.2.3 https://github.com/wireshark/wireshark.git ~/wireshark
cd ~/wireshark
git describe --tags --exact-match --match 'v*'    # must print v3.2.3
```

## 3. Copy the patch into the VM

**Option A: clone this repository inside the VM**

```bash
git clone <this-repo-url> ~/lab-mode
cp ~/lab-mode/module1.diff ~/lab-mode/demo/lab_demo.pcap ~/
```

**Option B: VirtualBox shared folder**

Share this folder with the VM (Settings → Shared Folders). It appears under `/media/sf_<folder_name>`, for example `/media/sf_Lab_Mode_Module_1`.

```bash
ls /media/                                   # find the share's name
ls -la /media/sf_Lab_Mode_Module_1/          # check that you can read it
```

If `ls` says *Permission denied*, add your user to the `vboxsf` group, then reboot (or log out and back in):

```bash
sudo usermod -aG vboxsf $USER
```

Then copy the files into your home folder. The `sed` step removes Windows line endings, which would otherwise break `git apply`:

```bash
cp /media/sf_Lab_Mode_Module_1/module1.diff ~/module1.diff
sed -i 's/\r$//' ~/module1.diff
cp /media/sf_Lab_Mode_Module_1/demo/lab_demo.pcap ~/
```

## 4. Apply the patch

```bash
cd ~/wireshark
git apply --check -v ~/module1.diff     # dry run; must report no errors
git apply ~/module1.diff
git status --short                      # 4 modified files (M) + 3 new lab_* files (??)
```

## 5. Build

```bash
mkdir -p ~/ws-build && cd ~/ws-build
cmake ../wireshark
make -j$(nproc)
```

The first build takes a while. It is done when the output ends with `[100%] Built target wireshark` and contains no `error:` lines. If CMake reports a missing library, install it with `apt` and run `cmake .` again.

## 6. Run

```bash
cd ~/ws-build
./run/wireshark ~/lab_demo.pcap
```

Run it as your normal user, not with `sudo`. For live capture without root, run this once:

```bash
sudo setcap cap_net_raw,cap_net_admin+eip ~/ws-build/run/dumpcap
```

## 7. Check that it works (demo capture)

| Action | Expected result |
|---|---|
| Start Wireshark | A **Lab:** row under the filter bar, set to *Off*; 21 of 21 packets shown |
| Pick **TCP (reset / hijack)** | Filter `tcp` (green), 9 packets, 5 preset buttons |
| Click **RST packets** | 1 packet: the forged reset |
| Shift+click **SYN only** | The filter text changes, but the list is not refiltered yet |
| Pick **ARP**, click **Duplicate address** | 1 packet: the poisoning reply |
| Pick **ICMP**, click **Redirect** | 1 packet |
| Pick **DNS**, click **NS records** | 1 packet |
| Pick **Tunnel / VPN** | Filter `ssh \|\| tls \|\| udp`, 8 packets |
| Pick **Off** | Filter cleared, 21 packets, preset buttons gone |
| *View → Filter Toolbar* (untick it) | The filter bar and the Lab bar hide together |

Optionally, check any filter string with the build's `dftest` tool:

```bash
./run/dftest 'tcp.flags.syn == 1 && tcp.flags.ack == 0'
```

## Troubleshooting

| Problem | Fix |
|---|---|
| `find` / `ls` shows nothing under `/media/sf_...` | Add your user to `vboxsf` (step 3) and reboot |
| The file is missing from the shared folder | OneDrive may not have downloaded it yet: in Windows, right-click the folder → *Always keep on this device* |
| `git apply` reports a *corrupt patch* | Run `sed -i 's/\r$//' ~/module1.diff` and try again |
| `git apply` reports *already exists* or *patch failed* | The tree has an earlier change. Reset it with `git checkout -- . && git clean -fd ui/qt` (this discards local edits), then apply again |
| `git describe` prints `wireshark-3.2.3` | That's the same commit (it has both tags); `--match 'v*'` prints `v3.2.3` |

## Files changed by the patch

| File | Change |
|---|---|
| `ui/qt/lab_profiles.h` | New: shared table of labs, hints and preset filters |
| `ui/qt/lab_toolbar.h` / `lab_toolbar.cpp` | New: the `LabToolBar` widget |
| `ui/qt/main_window.h` / `main_window.cpp` | Create and connect the toolbar |
| `ui/qt/main_window_slots.cpp` | Apply or clear the requested filter; follow the filter toolbar's visibility |
| `ui/qt/CMakeLists.txt` | Add the new files to the build |
