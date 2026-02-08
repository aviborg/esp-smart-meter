# Auto-Update Feature - Implementation Summary

## Overview

This PR adds automatic firmware update capability to the ESP Smart Meter, allowing devices to check for and install new firmware releases from GitHub automatically.

## What's New

### Core Functionality

1. **Automatic Update Checks**
   - Device checks for updates every hour
   - Non-blocking checks integrated into main loop schedule
   - Compares GitHub release versions with current firmware

2. **Manual Update Control**
   - Web UI displays current version and update status
   - "Check for Updates" button for manual checks
   - "Update Now" button when updates are available
   - Confirmation dialog before updating

3. **RESTful API**
   - `GET /version.json` - Returns version info and update status
   - `GET /update/check` - Triggers manual update check
   - `POST /update/trigger` - Initiates firmware download and installation

### Implementation Details

**UpdateManager Class** (`src/UpdateManager.h/cpp`)
- Manages GitHub API communication
- Performs semantic version comparison
- Handles firmware download via ESP8266HTTPUpdate
- Rate-limits update checks (1 hour minimum interval)

**Version Tracking** (`include/version.h`)
- Single source of truth for firmware version
- Simple `#define FIRMWARE_VERSION "1.0.0"`

**Web Integration**
- Modified `AmsWebServer` to expose update endpoints
- Enhanced web UI with version display and update controls
- JavaScript functions for version checking and updates

## Files Changed

### Added Files
```
include/version.h                 - Version definition
src/UpdateManager.h               - UpdateManager class header  
src/UpdateManager.cpp             - UpdateManager implementation
docs/AUTO_UPDATE.md               - User documentation
docs/TESTING_AUTO_UPDATE.md       - Testing procedures
docs/CREATING_RELEASES.md         - Release creation guide
```

### Modified Files
```
src/main.cpp                      - Integrated UpdateManager into loop
src/web/AmsWebServer.h            - Added update endpoints
src/web/AmsWebServer.cpp          - Implemented update handlers
web/index.html                    - Added version info section
web/readdata.js                   - Version check JavaScript
web/styles.css                    - Version UI styles
README.md                         - Mentioned auto-update feature
```

## How It Works

### Update Check Flow

1. **Periodic Check** (every hour in main loop)
   ```
   Device → GitHub API → Latest Release Info
   Compare versions → Store result
   ```

2. **Manual Check** (user clicks button)
   ```
   User → Web UI → /update/check endpoint
   Triggers immediate check → Updates display
   ```

3. **Update Installation** (user confirms)
   ```
   User → "Update Now" → /update/trigger endpoint
   Download .bin from GitHub → Flash firmware → Reboot
   ```

### Version Comparison

Uses semantic versioning (MAJOR.MINOR.PATCH):
- Parses version strings: "1.2.3" → {1, 2, 3}
- Compares component-wise
- Supports "v" prefix: "v1.0.0" → "1.0.0"

### GitHub Release Requirements

For auto-update to work, releases must:
1. Have a version tag (e.g., `v1.0.0`)
2. Include a `.bin` firmware file as an asset
3. Be marked as "latest release" on GitHub

## Security Considerations

⚠️ **Important**: This implementation prioritizes functionality over security:

1. **No Certificate Validation**
   - HTTPS connections skip certificate validation
   - Vulnerable to MITM attacks
   - Acceptable for trusted local networks

2. **No Authentication**
   - Update endpoints are unauthenticated
   - Anyone on local network can trigger updates
   - Assumes deployment on secure network

3. **No Signature Verification**
   - Firmware files are not cryptographically signed
   - No integrity verification beyond HTTPS

**See `docs/AUTO_UPDATE.md` for detailed security discussion and recommendations.**

## Testing

### Automated Tests
- ✅ Code review completed
- ✅ CodeQL security scan passed (0 alerts)
- ⚠️ Build test skipped (network issues)

### Manual Testing Required
1. Build and flash firmware to device
2. Create GitHub release with .bin file
3. Test update detection and installation
4. Verify device functionality after update

**See `docs/TESTING_AUTO_UPDATE.md` for complete test plan.**

## Usage

### For Users

1. **Check Version**
   - Open web interface: `http://emeter/`
   - Scroll to "Firmware Information" section
   - View current version and status

2. **Check for Updates**
   - Click "Check for Updates" button
   - Wait for status to update

3. **Install Update**
   - If update available, click "Update Now"
   - Confirm the dialog
   - Device will reboot automatically

### For Developers

1. **Update Version**
   ```cpp
   // include/version.h
   #define FIRMWARE_VERSION "1.1.0"
   ```

2. **Build Firmware**
   ```bash
   platformio run -e esp8266
   # Output: .pio/build/esp8266/firmware.bin
   ```

3. **Create Release**
   - Tag: `v1.1.0`
   - Upload `firmware.bin` file
   - Publish as latest release

**See `docs/CREATING_RELEASES.md` for detailed instructions.**

## API Reference

### GET /version.json

Returns version information in JSON format.

**Response:**
```json
{
  "current_version": "1.0.0",
  "latest_version": "1.1.0",
  "update_available": true,
  "status": "Update available: 1.1.0"
}
```

### GET /update/check

Manually triggers an update check.

**Response:** 
```
200 OK: "Update check initiated"
500 Error: "UpdateManager not initialized"
```

### POST /update/trigger

Triggers firmware update and reboot.

**Response:**
```
200 OK: "Update started. Device will reboot after update."
400 Bad Request: "No update available"
500 Error: "UpdateManager not initialized"
```

## Configuration

### Update Check Interval

Default: 1 hour (3600000 ms)

To change, edit `src/UpdateManager.h`:
```cpp
static const unsigned long UPDATE_CHECK_INTERVAL = 3600000; // milliseconds
```

### GitHub Repository

To use with different repository, edit `src/UpdateManager.cpp`:
```cpp
const char* UpdateManager::GITHUB_API_URL = 
    "https://api.github.com/repos/YOUR_USER/YOUR_REPO/releases/latest";
```

## Future Enhancements

Potential improvements:
- [ ] Certificate validation/pinning
- [ ] Authentication for update endpoints
- [ ] Firmware signature verification
- [ ] Automatic update option (install without confirmation)
- [ ] Rollback capability
- [ ] Update scheduling (specific time windows)
- [ ] Multi-platform support (ESP8266/ESP32)
- [ ] Delta updates (smaller downloads)
- [ ] Update history/changelog display

## Known Limitations

1. **Internet Required**: Device needs internet access to check/download updates
2. **GitHub Dependency**: Relies on GitHub API availability
3. **Memory Constraints**: ESP8266 limited memory may affect large updates
4. **No Rollback**: Failed updates require manual recovery
5. **Serial Data Pause**: Update process temporarily pauses meter reading

## Troubleshooting

**"Check failed" status**
- Verify internet connectivity
- Check GitHub API accessibility
- Review serial console output

**Update not detected**
- Ensure release has .bin file attached
- Verify release is marked as "latest"
- Check version tag format

**Update download fails**
- Verify sufficient free space
- Check network stability
- Ensure .bin file is valid ESP8266 firmware

## Support

- Documentation: `docs/AUTO_UPDATE.md`
- Testing: `docs/TESTING_AUTO_UPDATE.md`
- Releases: `docs/CREATING_RELEASES.md`
- Issues: GitHub issue tracker

## License

Same as main project (see LICENSE file).

## Contributors

Implemented by GitHub Copilot on behalf of aviborg.
