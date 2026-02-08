# Firmware Directory

This directory hosts firmware binaries for OTA (Over-The-Air) updates via GitHub Pages.

## Files

- `esp-smart-meter-v*.bin` - Versioned firmware binaries (e.g., `esp-smart-meter-v1.0.0.bin`)
- `version.json` - Version metadata pointing to the current firmware

## How It Works

1. When a new release is created, the GitHub Actions workflow builds the firmware
2. The workflow creates a Pull Request that:
   - Removes old firmware binaries
   - Adds the new versioned firmware: `esp-smart-meter-v1.0.0.bin`
   - Updates `version.json` with version information and download URL
3. After the PR is merged, GitHub Pages serves the firmware
4. ESP devices fetch the download URL from `version.json` and update directly

## OTA Update Process

ESP devices:
1. Fetch version info from:
   ```
   https://aviborg.github.io/esp-smart-meter/firmware/version.json
   ```

2. Parse the JSON to get the download URL:
   ```json
   {
     "version": "1.0.0",
     "download_url": "https://aviborg.github.io/esp-smart-meter/firmware/esp-smart-meter-v1.0.0.bin",
     ...
   }
   ```

3. Download firmware from the versioned URL

## Benefits

✅ Direct HTTPS download (no 302 redirects)
✅ Versioned filenames for clear history
✅ Works reliably with ESP8266HTTPUpdate
✅ No time-limited signed URLs
✅ Automatic cleanup of old versions
✅ Pull Request workflow works with protected branches
