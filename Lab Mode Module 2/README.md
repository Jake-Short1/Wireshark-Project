# Wireshark Lab Mode: Module 2 (Field Copy Panel)

This patch for **Wireshark v3.2.3** adds a **Lab Fields (Copy)** dock on the right of the main window. When you select a packet, the dock lists the values students need for the SEED lab scripts, each with a **Copy** button: MAC and IP addresses, ports, the **raw** TCP sequence and acknowledgment numbers, TCP flags, ICMP type, code and redirect gateway, ARP sender and target, and the DNS transaction ID and query name. Only the fields the packet actually contains are shown.

Module 2 builds on **Module 1** (the Lab selector toolbar). Apply `base/module1.diff` first, then `module2.diff`.

Tested on Ubuntu 20.04 (gcc 9.4, Qt 5.12.8, CMake 3.16), the same toolchain as the SEED Labs VM.

## What's in this folder

| Path | Contents |
|---|---|
| `module2.diff` | The Module 2 patch, made on top of Module 1 (`git diff <module1-base-commit>`) |
| `base/module1.diff` | Module 1's patch, which Module 2 requires (identical to the Module 1 deliverable) |
| `src/ui/qt/` | Full copies of the new and changed files: `field_copy_panel.h`, `field_copy_panel.cpp`, `main_window.cpp`, `CMakeLists.txt` |
| `demo/lab_demo.pcap` | Small demo capture with TCP, ARP, ICMP and DNS packets |
| `screenshots/` | The panel on TCP, ARP, ICMP and DNS packets; the Copy button; the hint; the View menu entry |
| `MODULE2_NOTES.md` | README fragment, "how to explain this code", line count and verification checklist |
| `COMMIT_MESSAGE.txt` | Suggested commit message for this folder |

## 1. Install the build dependencies (once)

```bash
sudo apt-get update
sudo apt-get install -y cmake build-essential flex bison libglib2.0-dev libgcrypt20-dev libc-ares-dev libpcap-dev qtbase5-dev qttools5-dev qttools5-dev-tools qtmultimedia5-dev libqt5svg5-dev libpcre2-dev
```

## 2. Get Wireshark v3.2.3 (skip if you already have the Module 1 checkout)

```bash
git clone --depth 1 --branch v3.2.3 https://github.com/wireshark/wireshark.git ~/wireshark
cd ~/wireshark
git describe --tags --exact-match --match 'v*'    # must print v3.2.3
```

## 3. Copy the patches into the VM

**Option A: clone this repository inside the VM**

```bash
git clone <this-repo-url> ~/lab-mode
cp ~/lab-mode/module2.diff ~/lab-mode/base/module1.diff ~/lab-mode/demo/lab_demo.pcap ~/
```

**Option B: VirtualBox shared folder**

Share this folder with the VM (Settings → Shared Folders). It appears under `/media/sf_<folder_name>`, for example `/media/sf_Lab_Mode_Module_2`. If `ls` on it says *Permission denied*, run `sudo usermod -aG vboxsf $USER` and reboot.

```bash
ls -la /media/sf_Lab_Mode_Module_2/
cp /media/sf_Lab_Mode_Module_2/module2.diff /media/sf_Lab_Mode_Module_2/base/module1.diff ~/
cp /media/sf_Lab_Mode_Module_2/demo/lab_demo.pcap ~/
sed -i 's/\r$//' ~/module1.diff ~/module2.diff     # remove Windows line endings, if any
```

## 4. Apply the patches

**Fresh v3.2.3 checkout:** apply Module 1, then Module 2.

```bash
cd ~/wireshark
git apply --check -v ~/module1.diff && git apply ~/module1.diff
git apply --check -v ~/module2.diff && git apply ~/module2.diff
```

**Checkout that already has Module 1 applied** (for example the one you built for Module 1): apply only Module 2.

```bash
cd ~/wireshark
git apply --check -v ~/module2.diff && git apply ~/module2.diff
```

Check the result with `git status --short`. You should see `field_copy_panel.h` and `field_copy_panel.cpp` as new files, and `main_window.cpp` and `CMakeLists.txt` as modified. If Module 1 was applied in the same step, its files show up too.

## 5. Build

```bash
mkdir -p ~/ws-build && cd ~/ws-build
cmake ../wireshark          # rerun this (or "cmake .") because the CMake file list changed
make -j$(nproc)
```

The first build from scratch takes a while; rebuilding an existing Module 1 build only recompiles a few files. The build is done when it ends with `[100%] Built target wireshark` and shows no `error:` lines.

## 6. Run

```bash
cd ~/ws-build
./run/wireshark ~/lab_demo.pcap
```

Run it as your normal user, not with `sudo`.

## 7. Check that it works (demo capture)

| Action | Expected result |
|---|---|
| Start Wireshark | A **Lab Fields (Copy)** dock on the right showing packet 1 (an ARP request) |
| Select packet 7 (Telnet data) | MACs, IPs, IP ID/TTL, ports 40000 → 23, **TCP seq (raw) 1001**, **TCP ack (raw) 5001**, length 4, flags, window |
| Click **Copy** next to *TCP seq (raw)* | The button says **Copied!** for a second; pasting gives `1001` |
| Filter `tcp.seq_raw == 1001` | Selects packets 6, 7 and 8 (they share that sequence number) |
| Select packet 3 (ARP reply) | Only MAC and ARP rows (opcode 2, sender 10.9.0.1 at 02:42:0a:09:00:69) |
| Select packet 13 (ICMP redirect) | IP rows plus ICMP type 5, code 1, gateway 10.9.0.105 |
| Select packet 16 (DNS response) | UDP ports 53 → 33333, DNS transaction ID 0x00001234, query name www.example.com |
| Ctrl+click a second packet | "Select one packet to see its key fields." |
| File → Close | The hint again; no crash |
| Close the dock (its × button), then **View → Lab Fields (Copy)** | The dock comes back |

Hex fields (IP ID, DNS ID) use Wireshark's own formatting (`0x00001234`), which is also a valid Python or C number.

## Troubleshooting

| Problem | Fix |
|---|---|
| `git apply` says `main_window.cpp: patch does not apply` | Module 1 isn't applied (or a different version is). Apply `base/module1.diff` first, on a clean v3.2.3 tree |
| `git apply` reports a *corrupt patch* | Run `sed -i 's/\r$//' ~/module2.diff` and try again |
| Build error: `field_copy_panel.h: No such file` or undefined `FieldCopyPanel` | Run `cmake .` in `~/ws-build` before `make` |
| The dock is missing | Use **View → Lab Fields (Copy)**, or **View → Reset Layout** |

## Files changed by Module 2

| File | Change |
|---|---|
| `ui/qt/field_copy_panel.h` / `.cpp` | New: the `FieldCopyPanel` dock widget |
| `ui/qt/main_window.cpp` | Create the dock, connect it to the packet list's selection, add it to the View menu (between `Lab Mode: Module 2 begin/end` markers) |
| `ui/qt/CMakeLists.txt` | Add the new files to the build |
