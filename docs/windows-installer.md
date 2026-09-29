# Windows Installer

The Windows installer is built with NSIS (Nullsoft Scriptable Install System) using the script at `scripts/installer.nsi`.

## How It Works

- CI invokes `makensis -DAPP_VERSION="${VERSION}" scripts/installer.nsi`
- The `APP_VERSION` define is embedded in the installer window title, branding, and registry entries
- If `APP_VERSION` is not provided (local manual runs), it defaults to `"localbuild"`

## Installed Components

- **Core files (required)**: all files from `build/bin/` including the exe and runtime DLLs
- **Start Menu shortcuts**: optional
- **Desktop shortcut**: optional

## Registry

The installer writes to `HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\TCG Game Tracker` for Add/Remove Programs integration and to `HKLM\...\App Paths\tcg-game-tracker.exe` so the exe is findable via `PATH`.

## Uninstaller

The uninstaller removes the install directory, Start Menu entries, desktop shortcut, and registry keys.
