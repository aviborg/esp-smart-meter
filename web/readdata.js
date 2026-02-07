async function getText(file, id) {
    try {
        let x = await fetch(file);
        let y = await x.text();
        insertDataInHtml(id, y);
    } catch (e) {
        console.log(e);
    }
};

function insertDataFromJSON(filename, id) {
    getText(filename, id);
    setInterval(getText, 10000, filename, id);
};

function insertDataInHtml(id, data) {
    console.log(id, data);
    data = JSON.parse(data);
    let txt = parseJSONData(data)
    document.getElementById(id).innerHTML = txt;
}

function parseJSONData(meterObject) {
    let text = "";
    for (x in meterObject) {
        if (typeof(meterObject[x]) == "object") {
            text += "<div class='flex-container'>" + x + parseJSONData(meterObject[x]) + "</div>";
        } else {
            text += "<div class='data-field'>" + x + ": " + meterObject[x] + "</div>";
        }
    }
    return text;
}

async function checkVersion() {
    try {
        let response = await fetch('/version.json');
        let versionData = await response.json();
        displayVersionInfo(versionData);
    } catch (e) {
        console.log('Error fetching version:', e);
    }
    // Check again every 5 minutes
    setInterval(checkVersion, 300000);
}

function displayVersionInfo(data) {
    let html = '<h3>Firmware Information</h3>';
    html += '<div class="version-field">Current Version: ' + data.current_version + '</div>';
    html += '<div class="version-field">Status: ' + data.status + '</div>';
    
    if (data.update_available) {
        html += '<div class="version-field">New Version Available: ' + data.latest_version + '</div>';
        html += '<button onclick="triggerUpdate()" class="update-button">Update Now</button>';
    }
    
    html += '<button onclick="manualCheckUpdate()" class="check-button">Check for Updates</button>';
    
    document.getElementById('version-info').innerHTML = html;
}

async function manualCheckUpdate() {
    try {
        await fetch('/update/check');
        // Wait a moment for the check to complete, then refresh version info
        setTimeout(async () => {
            let response = await fetch('/version.json');
            let versionData = await response.json();
            displayVersionInfo(versionData);
        }, 2000);
    } catch (e) {
        console.log('Error checking for updates:', e);
        alert('Failed to check for updates');
    }
}

async function triggerUpdate() {
    if (!confirm('Are you sure you want to update the firmware? The device will reboot.')) {
        return;
    }
    
    try {
        let response = await fetch('/update/trigger', { method: 'POST' });
        let message = await response.text();
        alert(message);
    } catch (e) {
        console.log('Error triggering update:', e);
        alert('Failed to trigger update');
    }
}