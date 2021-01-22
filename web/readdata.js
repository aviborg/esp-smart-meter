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
            text += "<div class='flex-container'>" + x + ": " + parseJSONData(meterObject[x]) + "</div>";
        } else {
            text += "<div class='data-field'>" + x + ": " + meterObject[x] + "</div>";
        }
    }
    return text;
}