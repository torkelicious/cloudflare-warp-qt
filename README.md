# Cloudflare Warp Qt

A lightweight C++ (Qt6) GUI based on the Cloudflare WARP CLI (`warp-cli`) on Linux.

The application provides a system tray icon and a compact control panel for toggling the connection status, managing configuration options, and handling registration.

## Features

* **Toggle Connection** – Connect or disconnect directly from the UI or the system tray.
* **System Tray** – Persistent tray icon with status indication (Connected / Disconnected) and a context menu.
* **Settings Menu**

  * **Auto-Connect** – Automatically connect to WARP when the application starts.
  * **Auto-Start** – Add the application to system startup (`~/.config/autostart`).
  * **Operation Modes** – Switch between `warp`, `doh`, `dot`, etc.
  * **Registration** – Register a new client via the GUI.
  * **Service Fixer** – Built-in utility to enable the `warp-svc` daemon and disable the conflicting official `warp-taskbar`.

## Requirements

* **Qt6** (`qt6-base`)
* **Cloudflare WARP** (e.g. `cloudflare-warp-bin` or another implementation providing `warp-cli`)
* **CMake** and **GCC/Clang** for building

## Build & Install

You can use the provided `build_install.sh` script to compile and install the application.

```bash
./build_install.sh
```

## Uninstall

To uninstall the application, run the included uninstaller script:

```bash
./uninstall.sh
```

I will probably either replace or compliment the current installation scripts with a PKGBUILD soon.

## Command-Line Arguments

* `--show` – Launch the application with the window visible (instead of starting minimized to the system tray).

## Troubleshooting

### “The application is already running”

The app uses a lock file at `/tmp/warp-qt.lock` to prevent multiple instances. Check your system tray—the icon may be hidden.

### Service issues, or duplicate tray icons

If the official `warp-taskbar` is running, it may create an additional system tray icon and consume unnecessary memory.
Go to **Settings → Troubleshooting** and click **“Enable Warp Daemon & Kill Official Tray”** to resolve this automatically.
