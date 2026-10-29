# Wireshark Lab Mode: Module 3 (Lab Styling)

This patch for **Wireshark v3.2.3** makes the packet list adapt to the lab chosen in the **Lab:** toolbar from Module 1. When a lab is selected, Wireshark:

- **colors the packets that matter for that lab**, for example forged TCP resets, poisoned ARP replies, ICMP redirects and DNS responses, and
- **adds a few extra columns** with the values the lab needs, for example raw TCP sequence/ack numbers, the ARP sender MAC/IP, or the DNS transaction ID.

Choosing another lab replaces the styling; choosing **Off** restores the normal look. Your own columns and coloring rules are never changed, and nothing is written to your preferences file.

Module 3 builds on **Module 1**. It is independent of Module 2, and both can be applied together.

Tested on Ubuntu 20.04 (gcc 9.4, Qt 5.12.8, CMake 3.16), the same toolchain as the SEED Labs VM.

## What's in this folder

| Path | Contents |
|---|---|
| `module3.diff` | The Module 3 patch, made on top of Module 1 (`git diff <module1-base-commit>`) |
| `base/module1.diff` | Module 1's patch, which Module 3 requires (identical to the Module 1 deliverable) |
| `src/ui/qt/` | Full copies of the new and changed files: `lab_style.h`, `lab_style.cpp`, `main_window.cpp`, `CMakeLists.txt` |
| `demo/lab_demo.pcap` | Small demo capture with TCP, ARP, ICMP, DNS, SSH, TLS and UDP packets |
| `screenshots/` | Each lab's colors and columns, Off before and after switching, and the temporary-color menu |
| `MODULE3_NOTES.md` | README fragment, "how to explain this code", line count, verification checklist |
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
ls -la "/media/sf_NetSec/Lab Mode Module 3/"
cp "/media/sf_NetSec/Lab Mode Module 3/module3.diff" "/media/sf_NetSec/Lab Mode Module 3/base/module1.diff" ~/
sed -i 's/\r$//' ~/module1.diff ~/module3.diff     # remove Windows line endings, if any
```

**Option B: clone this repository inside the VM**

```bash
git clone <this-repo-url> ~/lab-mode
cp ~/lab-mode/module3.diff ~/lab-mode/base/module1.diff ~/lab-mode/demo/lab_demo.pcap ~/
```

## 4. Apply the patches

**Checkout that already has Module 1 (and possibly Module 2) applied:** apply only Module 3.

```bash
cd ~/wireshark
git apply --check -v ~/module3.diff && git apply ~/module3.diff
```

**Fresh v3.2.3 checkout:** apply Module 1 first, then Module 3.

```bash
cd ~/wireshark
git apply --check -v ~/module1.diff && git apply ~/module1.diff
git apply --check -v ~/module3.diff && git apply ~/module3.diff
```

Check the result with `git status --short`. You should see `lab_style.h` and `lab_style.cpp` as new files, and `main_window.cpp` and `CMakeLists.txt` as modified.

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
./run/wireshark ~/lab_demo.pcap
```

Run it as your normal user, not with `sudo`.

## 7. Check that it works (demo capture)

| Action | Expected result |
|---|---|
| Start Wireshark | The usual columns and colors (Lab: *Off*) |
| Pick **TCP (reset / hijack)** | New columns **Seq (raw)**, **Ack (raw)**, **Flags** before *Info*. The forged RST (packet 10) is **light red**, Telnet data **yellow**, SYN **green**, SYN-ACK **cyan**. Packet 5 shows Seq 5000 / Ack 1001 |
| Pick **ARP** | The TCP columns disappear; **ARP op**, **Sender MAC**, **Sender IP** appear. The attacker's reply (packet 3) is **light red**, the real reply **yellow**, the request **blue** |
| Pick **ICMP** | **ICMP type**, **ICMP code**, **Gateway** columns. Redirect **light red**, unreachable **yellow**, echo request **green**, echo reply **cyan** |
| Pick **DNS** | **DNS ID**, **Query**, **Rcode** columns. Responses **yellow**, queries **blue**, error responses (NXDOMAIN) **pink** |
| Pick **Tunnel / VPN** | **TCP stream** column. SSH **light red**, TLS **cyan**, ICMP **yellow** |
| Switch labs many times | Each lab's columns appear only once; the column count never grows |
| Pick **Off** | Back to the original columns and colors; any columns you created yourself are still there |
| Quit with a lab selected, then restart | No lab columns are left over |

The colors are Wireshark's ten temporary "Colorize Conversation" colors (*View → Colorize Conversation*). Selecting a lab replaces any colors you set there by hand.

## Troubleshooting

| Problem | Fix |
|---|---|
| `git apply` says `main_window.cpp: patch does not apply` | Module 1 isn't applied (or a different version is). Apply `base/module1.diff` first, on a clean v3.2.3 tree |
| `git apply` reports a *corrupt patch* | Run `sed -i 's/\r$//' ~/module3.diff` and try again |
| Build error about `lab_style.h` or undefined `LabStyle` | Run `cmake .` in `~/ws-build` before `make` |
| No colors appear | Make sure *View → Colorize Packet List* is ticked |

## Files changed by Module 3

| File | Change |
|---|---|
| `ui/qt/lab_style.h` / `.cpp` | New: the `LabStyle` object with the per-lab color rules and columns |
| `ui/qt/main_window.cpp` | Create `LabStyle` and connect it to the Lab toolbar's `labChanged` signal (between `Lab Mode: Module 3 begin/end` markers) |
| `ui/qt/CMakeLists.txt` | Add the new files to the build |
