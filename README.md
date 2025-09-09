# Esp32-web-server
SD Card web server dengan file manager sederhana
# PINOUT SERVER (REFERENSI PIN)

| **Label**    | **GPIO Pin** | **Deskripsi**|
|--------------|--------------|--------------|
| `PIN_CLK`    | GPIO4        | CLK SD Card  |
| `PIN_MISO`   | GPIO5        | MISO SD Card |
| `PIN_MOSI`   | GPIO6        | MOSI SD Card |
| `PIN_CS_CARD`| GPIO7        | CS SD Card   |
| `led`        | GPIO8        | LED indikator|
---
### PENTING GA PENTING
lampu hidup pas kartu sd ga kebaca
```cpp
// Konfigurasi pin
#define PIN_CLK 4
#define PIN_MISO 5
#define PIN_MOSI 6
#define PIN_CS_CARD 7
#define led 8

// rute server

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
    server.on("/image-list", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/image-list.html", "text/html");
    });
    server.on("/novel", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/novel.html", "text/html");
    });
    server.on("/manga", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/manga.html", "text/html");
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
