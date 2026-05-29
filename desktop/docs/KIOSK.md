# Running ASCII Aquarium as a kiosk

This covers launching the aquarium full-screen on boot and keeping it the only
thing on screen — for the small x86 boxes with tiny displays this fork targets.

First build and install the binary so it lives on `PATH`:

```bash
cmake -S desktop -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build        # installs to /usr/local/bin/ascii-aquarium
```

Recommended kiosk flags:

```
ascii-aquarium --fullscreen --hide-cursor
```

`--fullscreen` uses borderless desktop-fullscreen scaled to fit; `--hide-cursor`
hides the pointer. Add `--capture-dir /path` if you want screenshots saved
somewhere specific (default is the per-user app data dir).

---

## Linux

Pick the approach that matches how the box already boots.

### A. Already boots to a desktop session (lightest touch)

Use an XDG autostart entry — works on GNOME, KDE, XFCE, LXDE, etc. Create
`~/.config/autostart/ascii-aquarium.desktop`:

```ini
[Desktop Entry]
Type=Application
Name=ASCII Aquarium
Exec=ascii-aquarium --fullscreen --hide-cursor
X-GNOME-Autostart-enabled=true
```

To stop the screen from blanking, also disable the screensaver / DPMS in the
desktop's power settings, or run once at session start:

```bash
xset s off; xset -dpms; xset s noblank
```

### B. systemd user service (boots to a session, want auto-restart)

A sample unit lives at [`packaging/ascii-aquarium.service`](../packaging/ascii-aquarium.service).
Install and enable it for your user:

```bash
mkdir -p ~/.config/systemd/user
cp desktop/packaging/ascii-aquarium.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now ascii-aquarium.service
```

If the box should start the app without anyone logging in, enable lingering so
the user service runs at boot:

```bash
sudo loginctl enable-linger "$USER"
```

The unit restarts the app if it ever exits — good for an unattended display.

### C. True single-app kiosk (no desktop at all)

For the leanest setup, run the app directly under a minimal kiosk compositor
instead of a full desktop. [`cage`](https://github.com/cage-kiosk/cage) is a
one-app Wayland kiosk:

```bash
sudo apt install cage            # Debian/Ubuntu/Mint/Pop
cage -- ascii-aquarium --fullscreen --hide-cursor
```

Run that from a getty autologin, or as a systemd *system* service bound to a
TTY. `cage` blanks everything else and has no window chrome, so the aquarium is
all that's visible. (`weston --shell=kiosk` is an alternative.)

X11 equivalent: an `~/.xinitrc` containing only
`exec ascii-aquarium --fullscreen --hide-cursor`, started via `startx` from an
autologin TTY.

---

## Windows

### A. Startup folder (simplest)

1. Press `Win+R`, type `shell:startup`, Enter — this opens the per-user Startup
   folder.
2. Right-click → New → Shortcut, and point it at the binary with kiosk flags:
   ```
   "C:\Path\To\ascii-aquarium.exe" --fullscreen --hide-cursor
   ```
3. The aquarium now launches at every logon.

### B. Task Scheduler (runs at logon, can auto-restart)

Useful when you want it to start even before Explorer is ready, or to restart
on crash:

```powershell
$action  = New-ScheduledTaskAction -Execute "C:\Path\To\ascii-aquarium.exe" `
             -Argument "--fullscreen --hide-cursor"
$trigger = New-ScheduledTaskTrigger -AtLogOn
$settings = New-ScheduledTaskSettingsSet -RestartCount 3 -RestartInterval (New-TimeSpan -Minutes 1)
Register-ScheduledTask -TaskName "ASCII Aquarium" -Action $action -Trigger $trigger -Settings $settings
```

### Notes

- To suppress sleep/screen-off on an unattended display, set the power plan's
  display + sleep timeouts to "Never" (Settings → System → Power).
- For a locked-down single-purpose machine, consider Windows **Assigned Access**
  (kiosk mode) pointing at the executable.

---

## Config & captures locations

- **Settings** persist to `config.ini` in the per-user config dir
  (`%APPDATA%\ascii-aquarium\` on Windows, `$XDG_CONFIG_HOME` or
  `~/.config/ascii-aquarium/` on Linux).
- **Screenshots / sequences** land in the app data dir by default
  (`%APPDATA%\ascii-aquarium\captures\` / `~/.local/share/ascii-aquarium/captures/`),
  or wherever `--capture-dir` points.

Delete `config.ini` to reset to defaults.
