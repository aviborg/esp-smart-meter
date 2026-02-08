# Creating Releases with Auto-Update Support

This guide explains how to create GitHub releases that work with the ESP Smart Meter's auto-update feature.

## Overview

The auto-update system uses **GitHub Pages** to host firmware binaries. When you create a release, the automated workflow:
1. Builds the firmware with the correct version
2. Uploads the binary to the GitHub release (for manual downloads)
3. Copies the firmware to `docs/firmware/latest.bin`
4. Updates `docs/firmware/version.json` with version metadata
5. GitHub Pages serves these files to ESP devices for OTA updates

## Release Checklist

Before creating a release, ensure:

- [ ] GitHub Pages is enabled (Settings → Pages → Source: main → /docs)
- [ ] Code changes committed and pushed to main
- [ ] All tests pass (if applicable)
- [ ] Changelog/release notes prepared

**Note**: You do NOT need to manually update `include/version.h` - the workflow does this automatically!

## Creating a GitHub Release (Automated Build)

### Via GitHub Web Interface (Recommended)

1. **Navigate to Releases**
   - Go to: `https://github.com/aviborg/esp-smart-meter/releases`
   - Click "Draft a new release"

2. **Choose a Tag**
   - Tag version: `v1.1.0` (must start with 'v', e.g., v0.0.4, v1.0.0)
   - Target: `main` branch
   - Click "Create new tag"

3. **Fill Release Information**
   - Release title: `v1.1.0` or descriptive name
   - Description: List changes, fixes, and new features

4. **Publish Release**
   - Choose "Set as the latest release"
   - Click "Publish release"

5. **Automated Workflow**
   The GitHub Actions workflow will automatically:
   - Build the firmware with the version from the tag
   - Upload `esp-smart-meter-v1.1.0.bin` to the release
   - Copy firmware to `docs/firmware/latest.bin`
   - Update `docs/firmware/version.json`
   - Commit and push to GitHub Pages

### Via GitHub CLI

```bash
# Create a release (workflow will build automatically)
gh release create v1.1.0 \
  --title "Release v1.1.0" \
  --notes "Release notes here"
```

## Manual Build (Advanced)

## Release Naming Conventions

### Tag Names

The auto-update feature supports:
- `v1.0.0` (recommended)
- `1.0.0`
- `V1.0.0`

The 'v' or 'V' prefix will be automatically stripped for version comparison.

### Firmware File Names

Any `.bin` file in the release assets will be detected. Recommended names:
- `firmware.bin` (simple)
- `esp-smart-meter-v1.0.0.bin` (descriptive)
- `esp8266-firmware-v1.0.0.bin` (platform-specific)

## Semantic Versioning

Follow semantic versioning (MAJOR.MINOR.PATCH):

- **MAJOR** (1.0.0 → 2.0.0): Breaking changes, incompatible API changes
- **MINOR** (1.0.0 → 1.1.0): New features, backwards compatible
- **PATCH** (1.0.0 → 1.0.1): Bug fixes, backwards compatible

Examples:
```
1.0.0 → 1.0.1  Bug fix release
1.0.1 → 1.1.0  New auto-update feature added
1.1.0 → 2.0.0  Major API refactor
```

## Release Types

### Stable Releases

- Use standard version tags: `v1.0.0`
- Marked as "latest release" on GitHub
- Automatically detected by all devices

### Pre-releases (Beta/RC)

- Tag with suffix: `v1.1.0-beta.1`, `v1.1.0-rc.1`
- Mark as "pre-release" on GitHub
- NOT automatically detected (requires code modification)

### Development Builds

- Not recommended for auto-update
- Use for manual testing only

## Multi-Platform Releases (Future)

If supporting multiple platforms (ESP8266, ESP32):

1. **Build all variants**:
   ```bash
   platformio run -e esp8266
   platformio run -e esp32
   ```

2. **Name binaries clearly**:
   - `firmware-esp8266.bin`
   - `firmware-esp32.bin`

3. **Update code to detect correct binary**:
   Modify `UpdateManager::fetchLatestRelease()` to filter by platform

## Release Notes Template

```markdown
## What's New in v1.1.0

### New Features
- Auto-update functionality for OTA firmware updates
- Web UI for version management

### Improvements
- Enhanced error handling in serial parser
- Optimized memory usage

### Bug Fixes
- Fixed issue #123: Memory leak in JSON parser
- Fixed issue #124: WiFi reconnection bug

### Breaking Changes
- None

### Security
- Added security documentation for OTA updates
- Note: Certificate validation disabled (see docs/AUTO_UPDATE.md)

## Installation
1. Download `firmware.bin` from assets below
2. Flash via OTA or USB according to installation guide
3. Or use the auto-update feature if already running v1.0.0+

## Upgrade Notes
- Safe to upgrade from v1.0.0
- No configuration changes required
- Device will reboot after update

## Known Issues
- Update check requires internet connectivity
- See docs/AUTO_UPDATE.md for security considerations
```

## Verification After Release

1. **Check release is published**:
   - Visit releases page
   - Verify "latest" badge is shown
   - Verify .bin file is attached

2. **Test API access**:
   ```bash
   curl https://api.github.com/repos/aviborg/esp-smart-meter/releases/latest
   ```
   Should return JSON with your new release

3. **Test on device**:
   - Navigate to web UI
   - Click "Check for Updates"
   - Should detect new version
   - (Optional) Trigger update to verify

## Troubleshooting

### Release not detected by devices

- Verify release is marked as "latest"
- Check version tag format (should be `vX.Y.Z` or `X.Y.Z`)
- Ensure .bin file is attached
- Wait a few minutes for GitHub CDN to propagate

### Binary file too large

- GitHub release assets have size limits (2GB per file, 2GB per release)
- ESP8266 firmware should be <1MB (typically ~300-500KB)
- If too large, review build flags and strip debug symbols

### Wrong binary downloaded

- Verify only one .bin file per platform in release
- Check `UpdateManager::fetchLatestRelease()` logic
- Add platform-specific filtering if needed

## Automation (Future Enhancement)

Consider setting up GitHub Actions to:
- Automatically build firmware on tag push
- Run tests before release
- Upload binaries to release
- Generate changelog from commits

Example workflow structure:
```yaml
name: Release
on:
  push:
    tags:
      - 'v*'
jobs:
  build-and-release:
    - Build firmware
    - Run tests
    - Create release
    - Upload binaries
```

## Rollback Procedure

If a release has issues:

1. **Create hotfix release**:
   - Fix the issue
   - Increment patch version (e.g., 1.1.0 → 1.1.1)
   - Release normally

2. **Un-latest a release** (not recommended):
   - Edit the problematic release
   - Uncheck "Set as the latest release"
   - Mark a previous release as latest

3. **Delete release** (last resort):
   - Only if not yet widely deployed
   - Devices already updated will remain on that version
   - Create new release with higher version number
