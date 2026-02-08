# Testing the Auto-Update Feature

## Prerequisites

Before you can test the auto-update feature, you need to:

1. **Build the firmware**
   ```bash
   platformio run -e esp8266
   ```
   The firmware binary will be at `.pio/build/esp8266/firmware.bin`

2. **Upload initial firmware to the device**
   - Connect via USB and upload with: `platformio run -e esp8266 --target upload`
   - Or use OTA if already configured: `platformio run -e esp8266 --target upload --upload-port <device-ip>`

3. **Create a GitHub Release**
   - Tag version: `v1.0.0` (or match the version in `include/version.h`)
   - Upload the `firmware.bin` file as a release asset

## Test Plan

### 1. Verify Current Version Display

1. Open the device web interface at `http://emeter/` (or your device's IP)
2. Scroll down to the "Firmware Information" section
3. Verify it shows:
   - Current Version: 1.0.0
   - Status: "Not checked" (initially)

### 2. Test Manual Update Check (No Update Available)

1. Click the "Check for Updates" button
2. Wait ~2 seconds for the check to complete
3. Verify the status updates to "Up to date" or "Checking..."
4. If latest release is v1.0.0, status should show "Up to date"

### 3. Test Update Detection

1. **Increment version in code**:
   - Edit `include/version.h`: Change `FIRMWARE_VERSION` to `"1.0.1"`
   - Rebuild: `platformio run -e esp8266`
   - Upload to device: `platformio run -e esp8266 --target upload`

2. **Create new GitHub release**:
   - Tag: `v1.1.0`
   - Upload the new `firmware.bin`

3. **Test detection**:
   - Click "Check for Updates"
   - Status should show: "Update available: 1.1.0"
   - "Update Now" button should appear

### 4. Test Manual Update Trigger

⚠️ **Warning**: This will reboot the device

1. Click the "Update Now" button
2. Confirm the dialog
3. Device should:
   - Show "Update started" message
   - Download firmware
   - LED should blink during update
   - Automatically reboot
   - After reboot, version should show 1.1.0

### 5. Test Automatic Update Check

1. Monitor serial console output
2. Wait for the device to run through its schedule states
3. Every hour (3600000 ms), you should see:
   ```
   Checking for firmware updates...
   ```
4. If an update is available, it will be logged but NOT automatically applied
   - User must manually trigger the update via web UI

### 6. Test API Endpoints

#### Version Info
```bash
curl http://emeter/version.json
```
Expected response:
```json
{
  "current_version": "1.0.0",
  "latest_version": "1.1.0",
  "update_available": true,
  "status": "Update available: 1.1.0"
}
```

#### Manual Check
```bash
curl http://emeter/update/check
```
Expected: "Update check initiated"

#### Trigger Update
```bash
curl -X POST http://emeter/update/trigger
```
Expected: "Update started. Device will reboot after update."

## Monitoring and Debugging

### Serial Console

Monitor the device's serial output at 115200 baud:
```bash
platformio device monitor -e esp8266
```

Look for messages like:
```
UpdateManager initialized
Current firmware version: 1.0.0
Checking for firmware updates...
New version available: 1.1.0
Starting firmware update...
Downloading from: https://github.com/...
```

### Common Issues

**"Check failed" status**
- Device may not have internet access
- GitHub API may be unreachable
- Check serial console for detailed error messages

**Update download fails**
- Verify the release has a .bin file attached
- Check device has sufficient free space
- Verify stable network connection

**Device doesn't reboot after update**
- Check serial console for ESP8266httpUpdate error messages
- Binary may be corrupted or incompatible
- Try manual power cycle

## Network Testing

Test different network scenarios:

1. **Behind firewall**: Verify GitHub API (api.github.com) is accessible
2. **Proxy network**: May need additional configuration
3. **Slow connection**: Update may timeout; monitor serial output

## Security Testing

⚠️ **Do not perform in production environment**

1. **Test without HTTPS** (if supported):
   - Verify the device properly handles protocol errors
   
2. **Test with invalid URL**:
   - Modify code to point to invalid URL
   - Verify proper error handling

3. **Test with corrupted firmware**:
   - Upload a non-firmware file as .bin
   - Verify device doesn't brick (should fail gracefully)

## Rollback Testing

If an update fails:
1. Device should remain on previous firmware version
2. Can manually upload working firmware via USB
3. OTA should still be available if update didn't complete

## Performance Testing

1. **Memory usage**: Monitor free heap during update check
2. **Update time**: Measure time from trigger to reboot
3. **Network bandwidth**: Monitor download speed
4. **Concurrent operations**: Verify meter reading continues during checks

## Success Criteria

- ✅ Current version displays correctly in web UI
- ✅ Manual update check works and detects new versions
- ✅ Update download and installation succeeds
- ✅ Device reboots with new firmware version
- ✅ Automatic periodic checks work without blocking
- ✅ All API endpoints respond correctly
- ✅ No memory leaks during update checks
- ✅ Serial output provides useful debugging information
- ✅ Device remains operational if update fails
