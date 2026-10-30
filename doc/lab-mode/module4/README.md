# Wireshark Lab Mode: Module 4 (Explain This Packet)

This patch for **Wireshark v3.2.3** adds an **Explain This Packet** dock at the bottom of the main window. While a lab is selected in the **Lab:** toolbar (Module 1), clicking a packet shows a plain-English explanation, for example:

> **TCP from 10.9.0.6:23 to 10.9.0.5:40000**
> RST: one side is abruptly closing the connection. In the TCP reset lab, this is the forged packet that kills a session.
> *Look for SYN, SYN-ACK and RST flags and the raw sequence numbers.*

The first line names the protocol and endpoints, the second explains what the packet does (quoting real values such as raw sequence numbers, MAC addresses, the redirect gateway or the DNS transaction ID), and the last line is the selected lab's hint. The dock is hidden while Lab Mode is **Off**.

Module 4 builds on **Module 1**. It is independent of Modules 2 and 3, and all four can be applied together.

Tested on Ubuntu 20.04 (gcc 9.4, Qt 5.12.8, CMake 3.16), the same toolchain as the SEED Labs VM.

## What's in this folder

| Path | Contents |
|---|---|
| `module4.diff` | The Module 4 patch, made on top of Module 1 (`git diff <module1-base-commit>`) |
| `base/module1.diff` | Module 1's patch, which Module 4 requires (identical to the Module 1 deliverable) |
| `ui/qt/` | The integrated Wireshark source files live in the repository checkout; they are not duplicated in this artifact folder. |
| `demo/explain_demo.pcap` | Demo capture: `lab_demo.pcap` plus a normal TCP close (FIN, FIN, ACK), 24 packets |
| `demo/lab_demo.pcap` | The shared Lab Mode demo capture (21 packets) |
| `demo/make_explain_demo.py` | Script that builds `explain_demo.pcap` from `lab_demo.pcap` (needs `python3-scapy`) |
| `screenshots/` | The dock for each kind of packet, the placeholder, Off, and the View menu entry |
| `MODULE4_NOTES.md` | README fragment, "how to explain this code", line count, verification checklist |
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

**Option A: VirtualBox shared folder.** If you share the parent `NetSec` folder, it appears as `/media/sf_NetSec`. Quote the paths, because the folder names contain spaces. If `ls` says *Permission denied*, run `sudo usermod -aG vboxsf $USER` and reboot.

```bash
ls -la "/media/sf_NetSec/Lab Mode Module 4/"
cp "/media/sf_NetSec/Lab Mode Module 4/module4.diff" "/media/sf_NetSec/Lab Mode Module 4/base/module1.diff" ~/
cp "/media/sf_NetSec/Lab Mode Module 4/demo/explain_demo.pcap" ~/
sed -i 's/\r$//' ~/module1.diff ~/module4.diff     # remove Windows line endings, if any
```

**Option B: clone this repository inside the VM**

```bash
git clone <this-repo-url> ~/lab-mode
cp ~/lab-mode/doc/lab-mode/module4/module4.diff ~/lab-mode/doc/lab-mode/module4/base/module1.diff ~/lab-mode/doc/lab-mode/module4/demo/explain_demo.pcap ~/
```

## 4. Apply the patches

**Checkout that already has Module 1 (and possibly Modules 2 and 3) applied:** apply only Module 4.

```bash
cd ~/wireshark
git apply --check -v ~/module4.diff && git apply ~/module4.diff
```

**Fresh v3.2.3 checkout:** apply Module 1 first, then Module 4.

```bash
cd ~/wireshark
git apply --check -v ~/module1.diff && git apply ~/module1.diff
git apply --check -v ~/module4.diff && git apply ~/module4.diff
```

Check the result with `git status --short`. You should see `explain_panel.h` and `explain_panel.cpp` as new files, and `main_window.cpp` and `CMakeLists.txt` as modified.

## 5. Build

```bash
mkdir -p ~/ws-build && cd ~/ws-build
cmake ../wireshark          # rerun this (or "cmake .") because the CMake file list changed
make -j$(nproc)
```

The build is done when it ends with `[100%] Built target wireshark` and shows no `error:` lines.

## 6. Run

```bash
cd ~/ws-build
./run/wireshark ~/explain_demo.pcap
```

Run it as your normal user, not with `sudo`.

## 7. Check that it works (demo capture)

| Action | Expected result |
|---|---|
| Start Wireshark (Lab: *Off*) | No Explain dock |
| Pick **TCP (reset / hijack)** | The **Explain This Packet** dock appears at the bottom |
| Select packet 4 (SYN) | "SYN: the client asks to open a connection ... Initial sequence number: 1000." |
| Select packet 5 (SYN-ACK) | "SYN-ACK: the server accepts the connection (step 2). It acknowledges 1001." |
| Select packet 7 (Telnet data) | "Data segment carrying 4 bytes. The receiver next expects sequence number 1005 ..." |
| Select packet 10 (RST) | "RST: one side is abruptly closing the connection. In the TCP reset lab ..." |
| Select packet 22 (FIN) | "FIN: this side is closing the connection normally." |
| Pick **ARP**, select packet 3 | "ARP reply: 10.9.0.1 is at 02:42:0a:09:00:69. A reply nobody asked for can poison ..." |
| Pick **ICMP**, select packet 13 | "ICMP redirect: the router tells the host to send traffic to 10.9.0.105 instead ..." |
| Pick **DNS**, select packet 16 | "DNS response for www.example.com, transaction ID 0x00001234 ..." |
| Pick **Tunnel / VPN**, select packet 21 (UDP) | "No explanation is available for this kind of packet in this lab." |
| Ctrl+click a second packet | "Select one packet to see an explanation." |
| Close the dock (×), then **View → Explain This Packet** | The dock comes back |
| Pick **Off** | The dock hides |

## Troubleshooting

| Problem | Fix |
|---|---|
| `git apply` says `main_window.cpp: patch does not apply` | Module 1 isn't applied (or a different version is). Apply `base/module1.diff` first, on a clean v3.2.3 tree |
| `git apply` reports a *corrupt patch* | Run `sed -i 's/\r$//' ~/module4.diff` and try again |
| Build error about `explain_panel.h` or undefined `ExplainPanel` | Run `cmake .` in `~/ws-build` before `make` |
| The dock never appears | Pick a lab other than *Off*, or use **View → Explain This Packet** |

## Files changed by Module 4

| File | Change |
|---|---|
| `ui/qt/explain_panel.h` / `.cpp` | New: the `ExplainPanel` dock and its explanation rules |
| `ui/qt/main_window.cpp` | Create the dock, add it to the View menu, connect it to the packet list and the Lab toolbar (between `Lab Mode: Module 4 begin/end` markers) |
| `ui/qt/CMakeLists.txt` | Add the new files to the build |
