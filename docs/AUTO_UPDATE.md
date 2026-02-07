# Auto-Update Feature

## Overview

The ESP Smart Meter now includes an automatic firmware update feature that checks for new releases from the GitHub repository and can update itself over-the-air (OTA).

## How It Works

### Automatic Background Checks

The device automatically checks for new firmware releases every hour by:
1. Querying the GitHub API for the latest release
2. Comparing the version tag with the current firmware version
3. If a newer version is available, it marks an update as available

### Version Information

The current firmware version is defined in `include/version.h`:
```cpp
#define FIRMWARE_VERSION "1.0.0"
```

### Manual Update Check

Users can manually trigger an update check via:
- Web UI: Click the "Check for Updates" button on the main page
- API: Send a GET request to `/update/check`

### Manual Update Trigger

When an update is available, users can trigger the update via:
- Web UI: Click the "Update Now" button (will prompt for confirmation)
- API: Send a POST request to `/update/trigger`

## Web Interface

The main web interface at `http://emeter/` (or your configured hostname) displays:
- Current firmware version
- Update status (checking, up-to-date, update available, etc.)
- Latest available version (if an update exists)
- "Check for Updates" button
- "Update Now" button (only shown when update is available)

## API Endpoints

### GET /version.json

Returns version information in JSON format:
```json
{
  "current_version": "1.0.0",
  "latest_version": "1.1.0",
  "update_available": true,
  "status": "Update available: 1.1.0"
}
```

### GET /update/check

Manually triggers a check for updates. Returns:
- 200: "Update check initiated"
- 500: Error message if UpdateManager is not initialized

### POST /update/trigger

Triggers the firmware update process. Returns:
- 200: "Update started. Device will reboot after update."
- 400: "No update available" (if no update is available)
- 500: Error message if UpdateManager is not initialized

**Note:** After triggering an update, the device will download the new firmware and automatically reboot when complete.

## Release Requirements

For the auto-update feature to work, GitHub releases must:
1. Include a version tag (e.g., `v1.0.0` or `1.0.0`)
2. Include a compiled firmware binary (`.bin` file) as a release asset
3. The firmware binary should be compiled for the ESP8266 platform

## Version Numbering

The project uses semantic versioning (MAJOR.MINOR.PATCH):
- MAJOR: Incompatible API changes
- MINOR: Added functionality in a backwards compatible manner
- PATCH: Backwards compatible bug fixes

## Security Considerations

⚠️ **CRITICAL SECURITY WARNINGS:**

### Certificate Validation

**The current implementation disables TLS certificate validation** for both GitHub API requests and firmware downloads. This creates potential security vulnerabilities:

1. **Man-in-the-Middle (MITM) Attacks**: Without certificate validation, an attacker on the same network could intercept the connection and:
   - Serve malicious firmware updates
   - Inject false version information
   - Compromise the device

2. **Current Implementation Rationale**: 
   - Certificate validation is disabled for simplicity and to reduce memory usage on the ESP8266
   - The design assumes the device operates on a **trusted, protected local network**
   - Full certificate validation on ESP8266 can be challenging due to memory constraints

3. **Recommended Improvements for Production**:
   ```cpp
   // Option 1: Certificate Fingerprint (lightweight)
   client.setFingerprint("GitHub certificate SHA1 fingerprint");
   
   // Option 2: Full Certificate Chain (more secure but uses more memory)
   client.setCACert(github_root_ca);
   
   // Option 3: Certificate Pinning for GitHub API
   ```

### Network Security

1. **No Authentication**: The update endpoints (`/update/check`, `/update/trigger`) have no authentication
   - Anyone on the local network can trigger updates
   - Consider adding HTTP Basic Auth or API tokens for production

2. **Network Isolation**: 
   - Deploy the device on a separate, trusted network segment
   - Use firewall rules to restrict access to the device
   - Consider VLANs or IoT-specific network segments

### Firmware Integrity

The current implementation does NOT verify:
- Firmware signatures
- Checksums or hashes
- Code signing

**Recommended for Production**:
- Implement firmware signature verification
- Add checksum validation before applying updates
- Use signed releases with cryptographic verification

### Additional Security Measures

For production deployments, consider:

1. **Update Authorization**: Add authentication to update endpoints
2. **Rate Limiting**: Prevent abuse of update check endpoints
3. **Update Windows**: Only allow updates during maintenance windows
4. **Rollback Capability**: Implement dual-partition updates with rollback
5. **Audit Logging**: Log all update attempts and their outcomes
6. **Manual Approval**: Require explicit user confirmation before updates

## Troubleshooting

### Update Check Fails

If update checks fail:
1. Verify the device has internet connectivity
2. Check that GitHub API is accessible from your network
3. Review serial console output for error messages

### Update Download Fails

If firmware download fails:
1. Ensure the release includes a `.bin` file
2. Verify sufficient free space on the device
3. Check network stability

### Device Doesn't Reboot After Update

If the device doesn't reboot after update:
1. Manually power cycle the device
2. Check serial console for error messages
3. The update may have failed; check logs

## Development

### Building Releases

When creating a new release:
1. Update the version in `include/version.h`
2. Build the firmware: `platformio run -e esp8266`
3. The firmware binary will be at `.pio/build/esp8266/firmware.bin`
4. Create a GitHub release with the version tag
5. Upload the `firmware.bin` file as a release asset

### Testing Updates

To test the update mechanism:
1. Build and upload firmware with version X
2. Create a GitHub release with version X+1 and a firmware binary
3. Access the web interface and check for updates
4. Trigger the update and verify it completes successfully
