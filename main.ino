//{ ==============================================================================
// Library & header file
//( =========================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
//) =========================================================================
// SPI Pins & Constants
//( =========================================================================
#define PIN_CLK 22
#define PIN_MISO 35
#define PIN_MOSI 18
#define PIN_CS_CARD 5
#define led 8
#define MAX_FILES_LIST 1000 // Batas maksimal file yang ditampilkan untuk menghindari kehabisan memori

//) =========================================================================
// Global Variables & Objects
//( =========================================================================
AsyncWebServer server(80);

//) =========================================================================
// HTML Pages (PROGMEM)
//( =========================================================================

// HTML for the WiFi Manager page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP SD Card Files Manager</title>
    <meta charset="UTF-8">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 5px;
            background-color: #f4f4f9;
            color: #333;
        }
        h1, h3 {
            color: #555;
        }
        input {
            background-color: transparent;
        }
        input::placeholder,input:disabled::placeholder {
            color: purple;
            opacity: 1;
        }
        .none,.none:after {
            outline: none;
            font-size: 18;
            border: none;
        }
    </style>
</head>
<body>
<div>
    <div>
        <br />
        <input class='none' type="text" id="item-name" disabled="true" placeholder="Files Manager v0.1.0" required>
        <span style="display:none" id="confirm">
            <button type="button" onclick="closed();">Cancel
            </button>
            <button type="submit" id="confirm-button">
                <strong id="confirm-content">
                    Confirm
                </strong>
            </button>
        </span>
    </div>
    <p>
        <button onclick="action('Create.Folder');return false;">Create Folder</button>
    </p>
    <p id="message" style="color:red">
        &nbsp;
    </p>
    <p>
        <h3>Upload File </h3>
        <div>
            <form id="uploadForm" enctype="multipart/form-data">
                <input type="file" name="fileInput" id="fileInput" multiple><button type="submit">Unggah</button>
            </form>
            <progress id="progressBar" value="0" max="100" style="width: 200px;"></progress><span id="progress-label">0%</span>
        </div>
        <div>
            <h3>Lokasi: <span id="pathBar"></span></h3>
        </div>
        <table>
            <thead>
                <tr>
                    <th style="width: 30%;">Name</th>
                    <th style="width: 25%;">Size</th>
                    <th></th>
                    <th style="width: ">Actions</th>
                </tr>
            </thead>
            <tbody id="file-list-body">
            </tbody>
        </table>
    </p>
</div>
    <script>
        // const elements (
        const pathBar = _("pathBar");
        const message = _("message");
        const fileInput = _('fileInput');
        const progressBar = _('progressBar');
        const progressLabel = _('progress-label');
        const fileListBody = _("file-list-body");
        const confirmButton = _("confirm-button");
        const confirm = _("confirm");
        const confirmContent = _("confirm-content");
        const itemName = _("item-name");
        let currentPath = "/";
        //) Utility function (
        function _(el) {
            return document.getElementById(el);
        }
        function action(name, item = '') {
            confirm.style.display = '';
            confirmContent.textContent = name;
            itemName.disabled = false;
            itemName.value = '';
            if (name) {
                if (name == 'Delete') {
                    itemName.value = item;
                    itemName.disabled = true;
                    confirmButton.setAttribute("onclick", `deleteFileOrFolder("${item}");`);
                } else if (name == "Rename") {
                    itemName.value = item;
                    itemName.disabled = false;
                    itemName.focus();
                    confirmButton.setAttribute("onclick", `renameFileOrFolder("${item}");`);
                } else {
                    itemName.disabled = false;
                    itemName.focus();
                    confirmButton.setAttribute("onclick", `createFolder();`);
                }
            }
        }
        function closed(){
            confirm.style.display = 'none';
            confirmContent.textContent = '';
            itemName.disabled = true;
            itemName.value = '';
        }
        function resolvePath(basePath, relativePath) {
            return `${basePath}/${relativePath}`.replace(/\/\/+/g, '/').replace(/\/$/, '') || '/';
        }
        function showMessage(msg) {
            message.innerHTML = msg;
            setTimeout(() => {
                message.innerHTML = "&nbsp;";
            }, 5000);
        }
        function handleResponse(response) {
            if (response.ok) {
                const contentType = response.headers.get("Content-Type");
                if (contentType && contentType.includes("application/json")) {
                    return response.json();
                } else {
                    return response.text();
                }
            } else {
                return response.text().then(text => {
                    throw new Error(text);
                });
            }
        }
        function formatBytes(bytes, decimals = 2) {
            if (bytes === 0) return '0 Bytes';
            const k = 1024;
            const dm = decimals < 0 ? 0: decimals;
            const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
        }
        //) File Manager Function (
        function updatePathBar() {
            pathBar.innerHTML = '';
            const parts = currentPath.split('/').filter(p => p !== '');
            let fullPath = '';
            const rootLink = document.createElement('a');
            rootLink.href = '#';
            rootLink.innerText = 'Home';
            rootLink.setAttribute("onclick", "listFiles('/');");
            pathBar.appendChild(rootLink);
            pathBar.innerHTML += ' / ';

            parts.forEach(part => {
                fullPath = resolvePath(fullPath, part);
                const link = document.createElement('a');
                link.href = '#';
                link.innerText = part;
                link.setAttribute("onclick", `listFiles("${fullPath}");`);
                pathBar.appendChild(link);
                pathBar.innerHTML += ' / ';
            });
        }
        function listFiles(path) {
            const url = `/list?path=${encodeURIComponent(path)}`;
            fetch(url)
            .then(handleResponse)
            .then(data => {
                currentPath = data.path;
                updatePathBar();
                renderFileList(data.files);
                showMessage("Daftar file dimuat.");
            })
            .catch(error => {
                showMessage(`Gagal memuat daftar file: ${error.message}`);
            });
        }
        function renderFileList(files) {
            fileListBody.innerHTML = '';
            const sortedFiles = files.sort((a, b) => {
                if (a.isDir !== b.isDir) return a.isDir ? -1: 1;
                return a.name.localeCompare(b.name);
            });
            if (currentPath !== "/") {
                const parentPath = currentPath.substring(0, currentPath.lastIndexOf('/')) || '/';
                const row = fileListBody.insertRow();
                row.innerHTML = `
                <tr align='left'><td><a href="#" onclick="listFiles('${parentPath === '' ? '/': parentPath}')"><span>.../</span></a></td></tr>
                `;
            }

            sortedFiles.forEach(item => {
                const row = fileListBody.insertRow();
                const icon = item.isDir ? '&#x1F4C1;': '&#x1F4C4;';
                const fileSize = item.isDir ? 'Folder': formatBytes(item.size);
                const itemPath = item.name;


                row.innerHTML = `
                <tr align='left'>
                <td>
                <a ${item.isDir ? `href="#" onclick="listFiles('${itemPath}')"`: `href="${itemPath}" target="_blank")`}>
                <span>${icon}</span>
                <span>${itemPath}</span>
                </a>
                </td>
                <td>${fileSize}</td>
                <td>
                <button onclick="action('Delete','${itemPath}');">Delete</button>
                </td>
                <td><button onclick="action('Rename','${itemPath}')">Rename</button>
                </td>
                <td>${item.isDir ? '': `<button onclick="downloadFile('${itemPath}')">Download</button>`}
                </td>
                </tr>
                `;
            });
        }
        function createFolder() {
            const folderName = itemName.value.trim();
            if (!folderName) {
                showMessage("Nama folder tidak boleh kosong."); return;
            }
            const path = resolvePath(currentPath, folderName);
            fetch(`/createFolder?path=${encodeURIComponent(path)}`, {
                method: 'POST'
            })
            .then(handleResponse)
            .then(response => {
                showMessage(response); listFiles(currentPath);
            })
            .catch(error => showMessage(`Gagal membuat folder: ${error.message}`));
            closed();
        }
        function renameFileOrFolder(oldName) {
            const newName = itemName.value.trim();
            if (newName === oldName || newName === null || newName === '') {
                showMessage("Nama baru tidak falid.");
            } else {
                fetch(`/rename?from=${encodeURIComponent(oldName)}&to=${encodeURIComponent(newName)}`, {
                    method: 'POST'
                })
                .then(handleResponse)
                .then(response => {
                    showMessage(response); listFiles(currentPath);
                })
                .catch(error => showMessage(`Gagal mengubah nama: ${error.message}`));
            }
            closed();
        }
        function deleteFileOrFolder(filePath) {
            fetch(`/delete?path=${encodeURIComponent(filePath)}`, {
                method: 'POST'
            })
            .then(handleResponse)
            .then(response => {
                showMessage(response); listFiles(currentPath);
            })
            .catch(error => showMessage(`Gagal menghapus: ${error.message}`));
            closed();
        }
        function downloadFile(filePath) {
            const fileName = filePath.split('/').pop();
            let tempLink = document.createElement('a');
            tempLink.style.display = 'none';
            tempLink.href = filePath;
            tempLink.download = fileName;
            document.body.appendChild(tempLink);
            tempLink.click();
            document.body.removeChild(tempLink);
        }
        _('uploadForm').addEventListener('submit', function(e) {
            e.preventDefault();
            const files = fileInput.files;
            if (files.length === 0) {
                showMessage('Silakan pilih file terlebih dahulu.');
                return;
            }
            const totalFiles = files.length;
            let uploadedCount = 0;
            const uploadNextFile = () => {
                if (uploadedCount >= totalFiles) {
                    showMessage('Unggahan berhasil!');
                    listFiles(currentPath);
                    progressBar.value = 0;
                    progressLabel.innerHTML = '0%';
                    fileInput.value = '';
                    return;
                }
                const file = files[uploadedCount];
                const formData = new FormData();
                formData.append('file', file, file.name);
                const xhr = new XMLHttpRequest();
                xhr.upload.addEventListener('progress', function(e) {
                    if (e.lengthComputable) {
                        let percent = Math.round((e.loaded / e.total) * 100);
                        progressBar.value = percent;
                        progressLabel.innerHTML = `${uploadedCount + 1}/${totalFiles} - ${percent}%`;
                    }
                });
                xhr.onreadystatechange = function() {
                    if (this.readyState === 4) {
                        if (this.status === 200) {
                            uploadedCount++;
                            uploadNextFile();
                        } else {
                            showMessage(`Unggahan gagal untuk ${file.name}. Status: ${this.status}. Pesan: ${this.responseText}`);
                        }
                    }
                };
                xhr.open('POST', `/upload?path=${encodeURIComponent(currentPath)}&filename=${encodeURIComponent(file.name)}`, true);
                xhr.send(formData);
            };
            uploadNextFile();
        });
        //)
        window.onload = function() {
            listFiles("/");
        };
    </script>
</body>
</html>

)rawliteral";

//) =========================================================================
// Utility Functions
//( =========================================================================
/**
* @brief mengecek apakah kartu terpasang
* @return True jika ya, False jika tidak
*/
bool sdCardMounted() {
    return SD.cardType() != CARD_NONE;
}

/**
* @brief membersihkan double slash
* @param path yg perlu dibersihkan.
* @return String path yg sudah bersih.
*/
String cleanPath(String path) {
    // Mengembalikan slash jika path kosong atau sama
    if (path.isEmpty() || path == "/") {
        return "/";
    }
    // menghapus duplikat slash dan slash di akhir
    path.replace("//", "/");
    if (path.length() > 1 && path.endsWith("/")) {
        path.remove(path.length() - 1);
    }
    // memastikan path selalu dimulai dengan '/'
    if (!path.startsWith("/")) {
        path = "/" + path;
    }
    return path;
}

/**
* @brief membaca baris pertama file dari kartu SD
* @param path file.
* @return String dari baris petama atau kosong jika gagal.
*/
String readFile(const char *path) {
    File file = SD.open(path);
    if (!file || file.isDirectory()) {
        Serial.println("- failed to open file for reading");
        return String();
    }
    String fileContent;
    while (file.available()) {
        fileContent = file.readStringUntil('\n');
        break;
    }
    file.close();
    return fileContent;
}

/**
* @brief menulis atau menimpa file ke sd card.
* @param path -> lokasi file.
* @param message -> yg akan ditilulis.
*/
void writeFile(const char *path, const char *message) {
    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

/**
* @brief mengahpus file atau directory secara rekursif.
* @param path -> jalur file atau directory.
* @return True jika berhasil, False jika gagal.
*/
bool deleteRecursive(String path) {
    File item = SD.open(path);
    if (!item) {
        return SD.remove(path);
    }
    if (item.isDirectory()) {
        File entry;
        while ((entry = item.openNextFile())) {
            String entryPath = path + "/" + entry.name();
            deleteRecursive(entryPath);
            entry.close();
        }
        item.close();
        return SD.rmdir(path);
    } else {
        item.close();
        return SD.remove(path);
    }
}

// asu!!!!!!!
/**
* @brief Sets up a WiFi access point.
* @param ssid The SSID for the AP.
* @param pass The password for the AP.
*/
void wifiConnect(String ssid, String pass) {
    WiFi.mode(WIFI_AP);
    if (ssid.isEmpty()) {
        ssid = "ESP_Server";
    }
    
    WiFi.softAP(ssid.c_str(), pass.c_str());
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
}

//) =========================================================================
// Web Server Handlers
//( =========================================================================

// Fungsi untuk menangani unggahan file
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!sdCardMounted()) {
         request->send(500, "text/plain", "SD card tidak terpasang.");
         return;
    }
    String path = cleanPath(request->arg("path"));
    if (path.isEmpty() || !index && !SD.exists(path)) {
         request->send(404, "text/plain", "Jalur tidak ditemukan.");
         return;
    }

    if (!index) {
        String fullPath = path + "/" + filename;
        Serial.printf("Mengunggah file: %s ke %s\n", filename.c_str(), fullPath.c_str());
        request->_tempFile = SD.open(fullPath, FILE_WRITE);
        if (!request->_tempFile) {
            request->send(500, "text/plain", "Gagal membuka file di SD card.");
            return;
        }
    }

    if (request->_tempFile) {
        if (request->_tempFile.write(data, len) != len) {
            Serial.println("Gagal menulis ke file.");
        }
    }
    
    if (final) {
        if (request->_tempFile) {
            request->_tempFile.close();
            if (SD.exists(cleanPath(path + "/" + filename))) {
                 Serial.println("Unggahan berhasil.");
                 request->send(200, "text/plain", "Unggahan berhasil!");
            } else {
                 request->send(500, "text/plain", "Unggahan gagal.");
            }
        } else {
            request->send(500, "text/plain", "Unggahan gagal. Objek file tidak valid.");
        }
    }
}

// Fungsi untuk menangani daftar file
void handleListFiles(AsyncWebServerRequest *request) {
    if (!sdCardMounted()) {
        request->send(500, "text/plain", "SD card tidak terpasang.");
        return;
    }

    String path = cleanPath(request->arg("path"));

    if (!SD.exists(path)) {
        request->send(404, "text/plain", "Jalur tidak ditemukan.");
        return;
    }

    File root = SD.open(path);
    if (!root || !root.isDirectory()) {
        if(root) root.close();
        request->send(400, "text/plain", "Jalur tidak valid atau bukan direktori.");
        return;
    }
/// start
    const size_t JSON_BUFFER_SIZE = 12 * 1024; // fuck
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    JsonArray fileList = doc["files"].to < JsonArray > ();

    File file;
    int fileCount = 0;
    while ((file = root.openNextFile()) && fileCount < MAX_FILES_LIST) {
        JsonObject item = fileList.createNestedObject();
        item["name"] = file.name();
        item["size"] = file.size();
        item["isDir"] = file.isDirectory();
        fileCount++;
        file.close();
    }
/// end
    root.close();

    JsonObject storage = doc.createNestedObject("storage");
    storage["total"] = (uint64_t)SD.totalBytes();
    storage["used"] = (uint64_t)SD.usedBytes();
    doc["path"] = path;

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

//
// Fungsi untuk membuat folder baru
void handleCreateFolder(AsyncWebServerRequest *request) {
    if (!sdCardMounted()) {
        request->send(500, "text/plain", "SD card tidak terpasang.");
        return;
    }
    String path = cleanPath(request->arg("path"));
    if (path.isEmpty() || SD.exists(path)) {
        request->send(400, "text/plain", "Nama folder tidak valid atau sudah ada.");
        return;
    }
         if (SD.mkdir(path)) {
    //if (createRecursiveDir(path)) {
        request->send(200, "text/plain", "Folder berhasil dibuat!");
    } else {
        request->send(500, "text/plain", "Gagal membuat folder.");
    }
}

// Fungsi untuk merubah nama
void handleRename(AsyncWebServerRequest *request) {
    if (!sdCardMounted()) {
        request->send(500, "text/plain", "SD card tidak terpasang.");
        return;
    }
    String oldPath = cleanPath(request->arg("from"));
    String newPath = cleanPath(request->arg("to"));
    if (oldPath.isEmpty() || newPath.isEmpty() || !SD.exists(oldPath) || SD.exists(newPath)) {
        request->send(400, "text/plain", "Jalur tidak valid.");
        return;
    }
    if (SD.rename(oldPath, newPath)) {
        request->send(200, "text/plain", "Berhasil merubah nama!");
    } else {
        request->send(500, "text/plain", "Gagal merubah nama.");
    }
}

// Fungsi untuk menghapus file atau folder
void handleDelete(AsyncWebServerRequest *request) {
    if (!sdCardMounted()) {
        request->send(500, "text/plain", "SD card tidak terpasang.");
        return;
    }
    String path = cleanPath(request->arg("path"));
    if (path.isEmpty() || !SD.exists(path)) {
        request->send(400, "text/plain", "Jalur tidak valid atau tidak ada.");
        return;
    }
    if (deleteRecursive(path)) {
        request->send(200, "text/plain", "Item berhasil dihapus!");
    } else {
        request->send(500, "text/plain", "Gagal menghapus Item.");
    }
}

// nyimpen wifi
void handleSaveWifi(AsyncWebServerRequest *request) {
    if (!sdCardMounted()) {
        request->send(500, "text/plain", "SD card tidak terpasang.");
        return;
    }
    String ssid = request->arg("ssid");
    String pass = request->arg("pass");

    String existingSsid = readFile("/ssid.txt");
    String existingPass = readFile("/pass.txt");

    if (ssid != existingSsid) {
        writeFile("/ssid.txt", ssid.c_str());
    }
    if (pass != existingPass) {
        writeFile("/pass.txt", pass.c_str());
    }
    request->send(200, "text/plain", "berhasil disimpan.");
    delay(3000);
    ESP.restart();
}

//) =========================================================================
// Setup and Loop
//( =========================================================================

/**
* @brief Initializes the SD card and sets up the web server.
*/
void setup() {
    Serial.begin(115200);
    pinMode(led, OUTPUT);
    digitalWrite(led, LOW);
    // Inisialisasi SPI untuk SD Card
    //SPI.begin(PIN_CLK, PIN_MISO, PIN_MOSI, PIN_CS_CARD);
    // Inisialisasi SD Card
    //if (!SD.begin(PIN_CS_CARD)) {
    if (!SD.begin()) {
        digitalWrite(led, HIGH);
        wifiConnect("ESP_Server","");
    } else {
        digitalWrite(led, LOW);
        // Load values saved in SD
        String ssid = readFile("/ssid.txt");
        String pass = readFile("/pass.txt");
        wifiConnect(ssid, pass);
    }

    // root static file
    server.serveStatic("/", SD, "/");

    // Mengatur rute server
    // Set root URL ("/") to display the web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (sdCardMounted() && SD.exists("/index.html")) {
            request->send(SD, "/index.html", "text/html");
        } else {
            request->send_P(200, "text/html", index_html);
        }
    });
server.on("/bookmark", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/bookmark.html", "text/html");
    });
    server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/history.html", "text/html");
    });
    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/settings.html", "text/html");
    });
    server.on("/chapter-list", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/chapter-list.html", "text/html");
    });
    server.on("/text-list", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/text-list.html", "text/html");
    });
    server.on("/img-list", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/image-list.html", "text/html");
    });
    server.on("/novel", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/novel.html", "text/html");
    });
    server.on("/komik", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/komik.html", "text/html");
    });
    server.on("/a", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/a.html", "text/html");
    });
    server.on("/b", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/b.html", "text/html");
    });
    server.on("/c", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/c.html", "text/html");
    });
    server.on("/d", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/d.html", "text/html");
    });
    server.on("/e", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/e.html", "text/html");
    });
    server.on("/f", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/f.html", "text/html");
    });
    server.on("/g", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/g.html", "text/html");
    });
    
    
    
    server.on("/wifi-manager", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/wifi-manager.html", "text/html");
    });
    // Rute untuk manajer file
    server.on("/file-manager", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/file-manager.html", "text/html");
    });
        server.on("/upload", HTTP_POST, 
        [](AsyncWebServerRequest *request){
            request->send(200); // Kirim respons awal
        }, handleUpload);

    server.on("/list", HTTP_GET, handleListFiles);
    server.on("/createFolder", HTTP_POST, handleCreateFolder);
    server.on("/rename", HTTP_POST, handleRename);
    server.on("/delete", HTTP_POST, handleDelete);
    server.on("/save_wifi", HTTP_POST, handleSaveWifi);
    // Menangani rute yang tidak ditemukan

    server.onNotFound([](AsyncWebServerRequest *request) {
        if (sdCardMounted() && SD.exists("/404.html")) {
            request->send(SD, "/404.html", "text/html");
        } else {
        request->send(404, "text/plain", "Halaman tidak ditemukan.");
        }
    });
    server.begin();
    Serial.println("Server HTTP dimulai.");
}

void loop() {}

//) =========================================================================}
