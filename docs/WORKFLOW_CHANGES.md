# Workflow Changes Summary

## Overview

The release workflow has been updated to use Pull Requests for firmware deployment instead of direct commits to the main branch.

## Why This Change?

### Problems with Direct Push Approach
1. ❌ Fails when main branch has moved ahead: `! [rejected] HEAD -> main (fetch first)`
2. ❌ Doesn't work with protected main branch
3. ❌ No review opportunity before deploying firmware
4. ❌ No audit trail of firmware deployments

### Benefits of PR Approach
1. ✅ **Works with protected branches** - PRs can be configured to bypass protection rules
2. ✅ **Review opportunity** - Team can review before firmware goes live
3. ✅ **Audit trail** - Each deployment creates a PR with discussion/review
4. ✅ **No conflicts** - Workflow fetches latest main before creating changes
5. ✅ **Can auto-merge** - Enable auto-merge for automated deployments
6. ✅ **Transparent** - Easy to see what changed in each firmware update

## What Changed

### Workflow File (.github/workflows/release.yml)

**Before:**
```yaml
- name: Commit and push firmware to repository
  run: |
    git config user.name "github-actions[bot]"
    git config user.email "github-actions[bot]@users.noreply.github.com"
    git add docs/firmware/
    git commit -m "Update firmware for release ${{ github.event.release.tag_name }}"
    git push origin HEAD:main
```

**After:**
```yaml
- name: Prepare firmware for GitHub Pages
  run: |
    # Fetch latest main to avoid conflicts
    git fetch origin main
    git checkout main
    
    # Clean up old firmware files
    find docs/firmware/ -name "*.bin" -type f -delete
    
    # Copy firmware with versioned name
    cp esp-smart-meter-${VERSION}.bin docs/firmware/esp-smart-meter-${VERSION}.bin
    
    # Create version.json pointing to versioned file
    # ... (creates JSON with download_url)

- name: Create Pull Request with firmware update
  uses: peter-evans/create-pull-request@v7
  with:
    commit-message: "Update firmware for release ${{ github.event.release.tag_name }}"
    title: "Update firmware for release ${{ github.event.release.tag_name }}"
    branch: firmware-update-${{ github.event.release.tag_name }}
    delete-branch: true
```

### Firmware Naming

**Before:**
- `docs/firmware/latest.bin` (loses version information)

**After:**
- `docs/firmware/esp-smart-meter-v1.0.0.bin` (versioned)
- Old versions automatically removed
- Clear history of what's deployed

### Code Changes

**UpdateManager.cpp/h:**
- Removed hardcoded `GITHUB_PAGES_FIRMWARE_URL` constant
- Now always reads `download_url` from version.json
- Works with any versioned filename

## How to Use

### Automatic Workflow (Recommended)

1. Create a GitHub release (e.g., v0.0.5)
2. Workflow automatically:
   - Builds firmware
   - Uploads to release
   - Creates PR with firmware update
3. Review and merge the PR (or enable auto-merge)
4. GitHub Pages serves the new firmware
5. ESP devices automatically detect and can install update

### Enable Auto-Merge (Optional)

To automatically merge firmware PRs:

```bash
# Enable auto-merge for a specific PR
gh pr merge firmware-update-v0.0.5 --auto --squash

# Or configure in repository settings:
# Settings → Branches → main → Require pull request reviews before merging
# Enable "Allow auto-merge"
```

### Protected Branch Configuration

If using branch protection on main:

1. Go to: Settings → Branches → main → Edit
2. Add exception for GitHub Actions bot:
   - Under "Restrict who can push", add: `github-actions[bot]`
   - Or enable "Allow specified actors to bypass required pull requests"

## Testing the Workflow

1. Create a test release (e.g., v0.0.5-test)
2. Watch the Actions tab for the workflow run
3. Verify a PR is created: `firmware-update-v0.0.5-test`
4. Check PR contents:
   - New firmware file added
   - version.json updated
   - Old firmware files removed
5. Merge the PR
6. Verify GitHub Pages serves the new firmware
7. Test OTA update on ESP device

## Troubleshooting

### PR Creation Fails

**Error:** `Resource not accessible by integration`

**Solution:** Ensure workflow has proper permissions:
```yaml
permissions:
  contents: write
  pull-requests: write
```

### PR Cannot Be Merged

**Error:** `Required status checks failed`

**Solution:** 
- Check if branch protection requires specific checks
- Add exception for bot-created PRs
- Or manually approve/merge the PR

### Old Firmware Files Not Deleted

**Issue:** Multiple .bin files in docs/firmware/

**Solution:** 
- Manually delete old files
- Ensure workflow has correct cleanup command:
  ```bash
  find docs/firmware/ -name "*.bin" -type f -delete
  ```

## Rollback

If you need to roll back to direct push approach:

1. Revert the workflow changes
2. Change back to direct push method
3. Note: Won't work with protected branches

But PR approach is recommended for production use.
