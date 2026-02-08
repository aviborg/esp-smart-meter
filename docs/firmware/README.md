# Firmware Directory

This directory hosts firmware binaries for OTA (Over-The-Air) updates via GitHub Pages.

## Files

- `latest.bin` - The most recent firmware binary
- `version.json` - Version metadata for the latest firmware

## How It Works

1. When a new release is created, the GitHub Actions workflow builds the firmware
2. The workflow copies the firmware binary as `latest.bin` to this directory
3. A `version.json` file is created with version information
4. The changes are committed and pushed to the repository
5. GitHub Pages serves these files at `https://aviborg.github.io/esp-smart-meter/firmware/`
6. ESP devices fetch firmware updates directly from GitHub Pages (no redirects!)

## OTA Update URL

ESP devices fetch firmware from:
```
https://aviborg.github.io/esp-smart-meter/firmware/latest.bin
```

Version information is available at:
```
https://aviborg.github.io/esp-smart-meter/firmware/version.json
```

## Benefits

✅ Direct HTTPS download (no 302 redirects)
✅ Short, stable URLs
✅ Works reliably with ESP8266HTTPUpdate
✅ No time-limited signed URLs
✅ Simple and maintainable
