# MFC Serial Terminal

A modern, fast, native Windows serial port terminal built with MFC and C++17.  
** Just a clean, professional COM port tool.**

![Build](https://github.com/YOUR_USERNAME/MFCSerialTerminal/actions/workflows/build.yml/badge.svg)
![License](https://img.shields.io/badge/license-MIT-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)

---

## Features

| Feature | Details |
|--------|---------|
| **Multi-mode display** | ASCII, HEX, and Mixed (inline hex for non-printable bytes) |
| **Timestamps** | Per-line HH:MM:SS.mmm timestamps, toggleable |
| **Send bar** | Type and send text or raw hex bytes (`0D 0A 41`) |
| **CR/LF control** | Independent CR and LF append toggles |
| **File send** | Stream any binary or text file to the port |
| **Port enumeration** | Auto-detects all available COM ports via SetupAPI |
| **Full baud range** | 110 to 921600 baud |
| **Flow control** | None, Hardware (RTS/CTS), Software (XON/XOFF) |
| **Line signals** | Toggle DTR/RTS; live CTS/DSR status in status bar |
| **Dark UI** | Dark terminal with color-coded RX (green) / TX (blue) |
| **Log save** | Export full session log to `.log` or `.txt` |
| **Settings persistence** | Last port config saved to Windows registry |

---


## Building

### Requirements
- Visual Studio 2022 (Community or higher)
- Desktop development with C++ workload
- MFC component (included in the workload)

### Steps

```bash
git clone https://github.com/YOUR_USERNAME/MFCSerialTerminal.git
cd MFCSerialTerminal
```

Open `MFCSerialTerminal.vcxproj` in Visual Studio, select **Release | x64**, and build (`Ctrl+Shift+B`).

Or from the command line (with MSBuild in PATH):

```powershell
msbuild MFCSerialTerminal.vcxproj /p:Configuration=Release /p:Platform=x64 /m
```

The output EXE will be in `x64\Release\`.

---

## Usage

1. Launch `MFCSerialTerminal.exe`
2. Go to **Connection → Settings** and choose your COM port, baud rate, etc.
3. Click **Connection → Connect** (or press `F5`)
4. Type in the send bar and press **Enter** or click **Send**
5. Toggle **HEX** checkbox to send raw hex bytes (e.g. `FF 00 1A 0D`)

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `F5` | Connect |
| `F6` | Disconnect |
| `Ctrl+S` | Save log |
| `Ctrl+L` | Clear log |
| `Enter` (send bar) | Send text |

---

##️ Project Structure

```
MFCSerialTerminal/
├── src/
│   ├── stdafx.h / stdafx.cpp       # Precompiled headers
│   ├── SerialTerminal.h / .cpp     # CWinApp entry point
│   ├── MainFrame.h / .cpp          # Main window, menu, layout
│   ├── SerialPort.h / .cpp         # Win32 serial I/O engine (overlapped I/O)
│   ├── TerminalView.h / .cpp       # Owner-draw listbox terminal display
│   └── ConnectDialog.h / .cpp      # Port settings dialog
├── .github/workflows/build.yml     # CI: MSBuild on Windows
├── MFCSerialTerminal.vcxproj       # VS2022 project file
└── README.md
```

---

## Architecture

- **Serial I/O**: Full overlapped (async) I/O using `OVERLAPPED` structs — never blocks the UI thread
- **Thread safety**: Read thread posts `WM_SERIAL_DATA` messages to the UI thread; no shared state accessed without mutex
- **Display**: Custom owner-draw `CListBox` subclass renders each line with color, timestamp, and direction tag
- **Port discovery**: Uses Windows `SetupAPI` (`SetupDiGetClassDevs`) for accurate COM port enumeration

---

## Contributing

Pull requests welcome! Ideas for future features:

- [ ] Plugin/scripting system (Lua)
- [ ] Modbus RTU protocol decoder
- [ ] Multi-tab sessions
- [ ] Graph/plotter for numeric serial streams
- [ ] Auto-reconnect on disconnect

---

## License

MIT © cada69
