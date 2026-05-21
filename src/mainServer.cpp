#include "mainServer.h"
#include "task_coreIOT.h"

static WebServer webServer(80);
static bool g_apMode = false;
static bool g_routesRegistered = false;
static unsigned long g_staConnectedSinceMs = 0;

static const char *AP_SSID = "ESP32-Config";
static const char *AP_PASSWORD = "12345678";
static void triggerSTAReconnect() {
    if (wifi_ssid.length() == 0) {
        return;
    }

    WiFi.mode(WIFI_AP_STA);
    Serial.println("Attempting STA reconnect...");
    Serial.print("Target SSID: ");
    Serial.println(wifi_ssid);
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

    // Ensure config AP is available while reconnecting so user can still access web page.
    if (g_apMode) {
        WiFi.softAP(AP_SSID, AP_PASSWORD);
    }
}

static bool connectToSTA(uint8_t maxAttempts = 20) {
    if (wifi_ssid.length() == 0) {
        return false;
    }

    triggerSTAReconnect();

    Serial.println("\n\nStarting WiFi connection...");
    Serial.print("Connecting to: ");
    Serial.println(wifi_ssid);

    uint8_t attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected successfully!");
        Serial.print("STA IP: ");
        Serial.println(WiFi.localIP());
        xSemaphoreGive(xBinarySemaphoreInternet);
        return true;
    }

    Serial.println("Failed to connect to WiFi!");
    Serial.print("WiFi Status: ");
    Serial.println(WiFi.status());
    return false;
}

static void startAPModeIfNeeded() {
    if (g_apMode) {
        return;
    }

    WiFi.mode(WIFI_AP);
    bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);

    if (apStarted) {
        g_apMode = true;
        Serial.println("Config AP started");
        Serial.print("AP SSID: ");
        Serial.println(AP_SSID);
        Serial.print("AP IP: ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("Failed to start AP mode");
    }
}

static void stopAPModeIfNeeded() {
    if (!g_apMode) {
        return;
    }

    WiFi.softAPdisconnect(true);
    g_apMode = false;
    Serial.println("Config AP stopped");
}

static String renderHomePage() {
    String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>ESP32 Smart Home Dashboard</title>";
    html += "<style>";
    html += ":root{--bg:#07111f;--panel:rgba(10,18,33,.82);--panel-strong:rgba(16,26,48,.92);--text:#e5eefc;--muted:#9fb2cc;--line:rgba(148,163,184,.18);--primary:#4f8cff;--primary-2:#7c5cff;--accent:#22c55e;--accent-2:#f59e0b;--danger:#ef4444;--shadow:0 24px 80px rgba(3,7,18,.45);}";
    html += "*{box-sizing:border-box;}";
    html += "body{font-family:'Segoe UI',Tahoma,sans-serif;margin:0;min-height:100vh;padding:18px;color:var(--text);background:radial-gradient(circle at top left,#1a2b5b 0,transparent 32%),radial-gradient(circle at right 15%,rgba(124,92,255,.18) 0,transparent 24%),linear-gradient(160deg,#050914 0%,#0a1324 45%,#08101d 100%);position:relative;overflow-x:hidden;}";
    html += "body:before,body:after{content:'';position:fixed;inset:auto;border-radius:999px;filter:blur(18px);opacity:.55;pointer-events:none;animation:float 12s ease-in-out infinite;}";
    html += "body:before{width:240px;height:240px;top:-70px;left:-60px;background:rgba(79,140,255,.22);}";
    html += "body:after{width:180px;height:180px;right:-40px;bottom:120px;background:rgba(34,197,94,.16);animation-delay:-4s;}";
    html += ".wrap{max-width:980px;margin:0 auto;position:relative;z-index:1;}";
    html += ".hero{display:flex;align-items:flex-end;justify-content:space-between;gap:16px;margin-bottom:16px;padding:18px 20px;background:linear-gradient(135deg,rgba(79,140,255,.18),rgba(124,92,255,.12));border:1px solid rgba(148,163,184,.18);border-radius:22px;box-shadow:var(--shadow);backdrop-filter:blur(14px);animation:fadeUp .7s ease both;}";
    html += ".hero h1{font-size:28px;line-height:1.05;margin:0 0 8px;letter-spacing:-.03em;}";
    html += ".hero p{margin:0;color:var(--muted);max-width:56ch;}";
    html += ".hero-badge{display:inline-flex;align-items:center;gap:8px;padding:8px 12px;border-radius:999px;background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.08);font-size:12px;font-weight:700;letter-spacing:.08em;text-transform:uppercase;color:#d7e4fb;}";
    html += ".hero-badge i{width:9px;height:9px;border-radius:50%;background:linear-gradient(135deg,#22c55e,#4ade80);box-shadow:0 0 0 0 rgba(34,197,94,.5);animation:pulse 2s infinite;}";
    html += ".grid{display:grid;grid-template-columns:1.15fr .85fr;gap:16px;}";
    html += ".side-panel{display:flex;flex-direction:column;gap:16px;}";
    html += ".quick-panel{background:linear-gradient(160deg,rgba(124,92,255,.18),rgba(34,197,94,.10));border:1px solid rgba(148,163,184,.16);border-radius:20px;padding:18px;box-shadow:var(--shadow);backdrop-filter:blur(16px);animation:fadeUp .9s ease both;}";
    html += ".quick-panel h3{margin:0 0 12px;font-size:14px;letter-spacing:.08em;text-transform:uppercase;color:#dbeafe;}";
    html += ".quick-item{display:flex;align-items:center;justify-content:space-between;gap:10px;padding:10px 0;border-top:1px solid rgba(148,163,184,.12);}";
    html += ".quick-item:first-of-type{border-top:none;padding-top:0;}";
    html += ".quick-item span{color:var(--muted);font-size:13px;}";
    html += ".quick-item strong{color:#f8fbff;font-size:14px;}";
    html += ".card{background:var(--panel);border:1px solid var(--line);border-radius:20px;padding:18px 18px 16px;box-shadow:var(--shadow);backdrop-filter:blur(16px);animation:fadeUp .8s ease both;}";
    html += ".card:hover{transform:translateY(-1px);border-color:rgba(79,140,255,.28);}";
    html += "h2{font-size:15px;margin:0 0 12px;letter-spacing:.02em;text-transform:uppercase;color:#dbeafe;}";
    html += "p{margin:8px 0;color:var(--muted);}";
    html += ".status{display:inline-block;font-size:12px;font-weight:600;padding:3px 8px;border-radius:999px;color:#fff;}";
    html += ".ok{background:linear-gradient(135deg,#16a34a,#22c55e);}";
    html += ".warn{background:linear-gradient(135deg,#dc2626,#f43f5e);}";
    html += ".section{padding-top:12px;margin-top:14px;border-top:1px solid var(--line);}";
    html += "label{display:block;font-size:12px;font-weight:700;color:#cbd5e1;margin:8px 0 6px;letter-spacing:.04em;text-transform:uppercase;}";
    html += "input,select,button{width:100%;padding:12px 12px;border-radius:12px;border:1px solid rgba(148,163,184,.18);font-size:14px;transition:all .2s ease;}";
    html += "input,select{background:rgba(255,255,255,.06);color:var(--text);outline:none;}";
    html += "input::placeholder{color:#7e8fa8;}";
    html += "input:focus,select:focus{border-color:rgba(79,140,255,.55);box-shadow:0 0 0 4px rgba(79,140,255,.14);}";
    html += "button{background:linear-gradient(135deg,var(--primary),var(--primary-2));color:#fff;border:none;font-weight:700;cursor:pointer;margin-top:10px;box-shadow:0 14px 30px rgba(79,140,255,.22);}";
    html += "button:hover{transform:translateY(-1px);filter:saturate(1.08);}";
    html += "button.ghost{background:rgba(255,255,255,.04);color:var(--text);border:1px solid rgba(148,163,184,.18);box-shadow:none;}";
    html += ".row{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:8px;}";
    html += ".fan-box,.door-box,.chart-box,.stat{background:var(--panel-strong);border:1px solid rgba(148,163,184,.16);border-radius:16px;padding:14px;}";
    html += ".fan-value{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:10px;font-size:13px;color:var(--muted);}";
    html += "input[type='range']{padding:0;height:42px;accent-color:#7c5cff;background:transparent;}";
    html += ".door-row{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:10px;}";
    html += ".door-state{font-size:12px;font-weight:600;padding:4px 10px;border-radius:999px;color:#fff;}";
    html += ".door-open{background:linear-gradient(135deg,#16a34a,#22c55e);}";
    html += ".door-closed{background:linear-gradient(135deg,#dc2626,#ef4444);}";
    html += ".stats{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px;margin-bottom:10px;}";
    html += ".stat small{display:block;color:var(--muted);font-size:12px;margin-bottom:4px;}";
    html += ".stat strong{font-size:22px;color:#f8fbff;letter-spacing:-.02em;}";
    html += ".stat:nth-child(1){background:linear-gradient(135deg,rgba(239,68,68,.16),rgba(16,24,40,.88));}";
    html += ".stat:nth-child(2){background:linear-gradient(135deg,rgba(13,110,253,.16),rgba(16,24,40,.88));}";
    html += ".chart-box{padding:12px;}";
    html += "canvas{display:block;width:100%;height:220px;}";
    html += ".hint{font-size:12px;color:var(--muted);margin-top:10px;line-height:1.5;}";
    html += "@keyframes float{0%,100%{transform:translate3d(0,0,0) scale(1);}50%{transform:translate3d(0,-12px,0) scale(1.04);}}";
    html += "@keyframes fadeUp{from{opacity:0;transform:translateY(12px);}to{opacity:1;transform:translateY(0);}}";
    html += "@keyframes pulse{0%{box-shadow:0 0 0 0 rgba(34,197,94,.45);}70%{box-shadow:0 0 0 12px rgba(34,197,94,0);}100%{box-shadow:0 0 0 0 rgba(34,197,94,0);}}";
    html += "@media(max-width:860px){.grid{grid-template-columns:1fr;}.hero{flex-direction:column;align-items:flex-start;}}";
    html += "@media(max-width:560px){.row{grid-template-columns:1fr;}}";
    html += "</style></head><body><div class='wrap'><div class='card'>";
    html += "<div class='hero'><div><div class='hero-badge'><i></i> Live Dashboard</div><h1>ESP32 Smart Home Dashboard</h1><p>Monitor WiFi, fan, door, temperature, and humidity from one polished control panel.</p></div><div class='hero-badge'>Ready for control</div></div>";
    html += "<script>";
    html += "async function syncFanState(){";
    html += "try{";
    html += "const response=await fetch('/fan-state',{cache:'no-store'});";
    html += "if(!response.ok)return;";
    html += "const data=await response.json();";
    html += "const slider=document.getElementById('fanRange');";
    html += "const value=document.getElementById('fanValue');";
    html += "if(!slider||!value)return;";
    html += "if(document.activeElement!==slider&&slider.value!==String(data.speed)){slider.value=data.speed;}";
    html += "value.textContent=data.speed;";
    html += "}catch(e){}";
    html += "}";
    html += "async function syncDoorState(){";
    html += "try{";
    html += "const response=await fetch('/door-state',{cache:'no-store'});";
    html += "if(!response.ok)return;";
    html += "const data=await response.json();";
    html += "const chip=document.getElementById('doorState');";
    html += "const label=document.getElementById('doorActionLabel');";
    html += "if(!chip||!label)return;";
    html += "if(data.open){chip.textContent='Open';chip.className='door-state door-open';label.textContent='Close Door';}";
    html += "else{chip.textContent='Closed';chip.className='door-state door-closed';label.textContent='Open Door';}";
    html += "}catch(e){}";
    html += "}";
    html += "const tempPoints=[];";
    html += "const humidPoints=[];";
    html += "const maxPoints=30;";
    html += "function pushPoint(list,val){list.push(val);if(list.length>maxPoints)list.shift();}";
    html += "function drawLine(ctx,arr,minV,maxV,color,w,h,p){";
    html += "if(arr.length<2)return;";
    html += "ctx.beginPath();ctx.strokeStyle=color;ctx.lineWidth=2;";
    html += "for(let i=0;i<arr.length;i++){";
    html += "const x=p+(i*(w-2*p))/Math.max(1,maxPoints-1);";
    html += "const y=h-p-((arr[i]-minV)*Math.max(1,(h-2*p)))/Math.max(1,maxV-minV);";
    html += "if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}";
    html += "ctx.stroke();";
    html += "}";
    html += "function drawChart(){";
    html += "const canvas=document.getElementById('sensorChart');";
    html += "if(!canvas)return;";
    html += "const ratio=window.devicePixelRatio||1;";
    html += "const cssW=canvas.clientWidth||560;const cssH=200;";
    html += "if(canvas.width!==cssW*ratio||canvas.height!==cssH*ratio){canvas.width=cssW*ratio;canvas.height=cssH*ratio;}";
    html += "const ctx=canvas.getContext('2d');ctx.setTransform(ratio,0,0,ratio,0,0);";
    html += "ctx.clearRect(0,0,cssW,cssH);";
    html += "const p=28;";
    html += "ctx.strokeStyle='#e2e8f0';ctx.lineWidth=1;";
    html += "for(let i=0;i<=4;i++){const y=p+i*(cssH-2*p)/4;ctx.beginPath();ctx.moveTo(p,y);ctx.lineTo(cssW-p,y);ctx.stroke();}";
    html += "const all=tempPoints.concat(humidPoints);";
    html += "if(all.length===0)return;";
    html += "let minV=Math.min.apply(null,all);let maxV=Math.max.apply(null,all);";
    html += "minV=Math.floor((minV-2)/5)*5;maxV=Math.ceil((maxV+2)/5)*5;";
    html += "if(maxV-minV<10){maxV=minV+10;}";
    html += "ctx.fillStyle='#64748b';ctx.font='12px Segoe UI';";
    html += "ctx.fillText(String(maxV),6,p+4);ctx.fillText(String(minV),6,cssH-p+4);";
    html += "drawLine(ctx,tempPoints,minV,maxV,'#ef4444',cssW,cssH,p);";
    html += "drawLine(ctx,humidPoints,minV,maxV,'#0d6efd',cssW,cssH,p);";
    html += "}";
    html += "async function syncSensorState(){";
    html += "try{";
    html += "const response=await fetch('/sensor-state',{cache:'no-store'});";
    html += "if(!response.ok)return;";
    html += "const data=await response.json();";
    html += "const t=document.getElementById('tempValue');const h=document.getElementById('humidValue');";
    html += "if(t)t.textContent=Number(data.temperature).toFixed(1);";
    html += "if(h)h.textContent=Number(data.humidity).toFixed(1);";
    html += "if(Number.isFinite(data.temperature)&&Number.isFinite(data.humidity)){";
    html += "pushPoint(tempPoints,Number(data.temperature));pushPoint(humidPoints,Number(data.humidity));drawChart();}";
    html += "}catch(e){}";
    html += "}";
    html += "window.addEventListener('resize',drawChart);";
    html += "window.addEventListener('DOMContentLoaded',()=>{syncFanState();syncSensorState();syncDoorState();setInterval(syncFanState,1000);setInterval(syncSensorState,2000);setInterval(syncDoorState,1000);});";
    html += "</script>";

    html += "<div class='grid'><div>";
    html += "<p><b>WiFi Status:</b> ";
    if (WiFi.status() == WL_CONNECTED) {
        html += "<span class='status ok'>Connected</span> ";
        html += WiFi.SSID() + " (" + WiFi.localIP().toString() + ")";
    } else {
        html += "<span class='status warn'>Disconnected</span>";
    }
    html += "</p>";

    html += "<div class='section'><h2>WiFi Setup</h2>";
    html += "<form method='POST' action='/wifi'>";
    html += "<label>Network Name (SSID)</label>";
    html += "<input name='ssid' placeholder='Enter WiFi SSID' value='" + wifi_ssid + "' required>";
    html += "<label>Password</label>";
    html += "<input name='password' type='password' placeholder='Enter WiFi password'>";
    html += "<button type='submit'>Save & Connect</button></form>";
    html += "<form method='GET' action='/retry'><button class='ghost' type='submit'>Retry Connection</button></form></div>";

    html += "<div class='section'><h2>Fan Control</h2>";
    html += "<form method='GET' action='/fan'>";
    html += "<div class='fan-box'>";
    html += "<div class='fan-value'><span>Fan Speed</span><strong><span id='fanValue'>" + String(fan_speed) + "</span>%</strong></div>";
    html += "<input id='fanRange' name='speed' type='range' min='0' max='100' value='" + String(fan_speed) + "' oninput=\"document.getElementById('fanValue').textContent=this.value\">";
    html += "</div>";
    html += "<button type='submit'>Update Fan Speed</button></form></div>";

    html += "<div class='section'><h2>Door Control</h2>";
    html += "<form method='GET' action='/door'>";
    html += "<div class='door-box'>";
    html += "<div class='door-row'><span>Door Status</span><span id='doorState' class='door-state ";
    html += String(pos_servo > 0 ? "door-open'>Open" : "door-closed'>Closed");
    html += "</span></div>";
    html += "<input type='hidden' name='action' value='toggle'>";
    html += "</div>";
    html += "<button type='submit'><span id='doorActionLabel'>" + String(pos_servo > 0 ? "Close Door" : "Open Door") + "</span></button></form></div>";

    html += "<div class='section'><h2>Temperature & Humidity</h2>";
    html += "<div class='stats'>";
    html += "<div class='stat'><small>Current Temperature (C)</small><strong><span id='tempValue'>" + String(glob_temperature, 1) + "</span></strong></div>";
    html += "<div class='stat'><small>Current Humidity (%)</small><strong><span id='humidValue'>" + String(glob_humidity, 1) + "</span></strong></div>";
    html += "</div>";
    html += "<div class='chart-box'><canvas id='sensorChart'></canvas></div>";
    html += "</div>";

    html += "<p class='hint'>If the connection is lost, the ESP will automatically start a configuration access point again.</p>";
    html += "</div><div class='side-panel'><div class='quick-panel'><h3>System Snapshot</h3>";
    html += "<div class='quick-item'><span>WiFi Mode</span><strong>AP + STA</strong></div>";
    html += "<div class='quick-item'><span>Fan Control</span><strong>Live Slider</strong></div>";
    html += "<div class='quick-item'><span>Door Control</span><strong>Toggle Action</strong></div>";
    html += "<div class='quick-item'><span>Sensor Updates</span><strong>Auto Refresh</strong></div>";
    html += "</div><div class='quick-panel'><h3>Visual Theme</h3>";
    html += "<div class='quick-item'><span>Primary Accent</span><strong>Blue / Violet</strong></div>";
    html += "<div class='quick-item'><span>Status Colors</span><strong>Green / Red</strong></div>";
    html += "<div class='quick-item'><span>Layout Style</span><strong>Glass Dashboard</strong></div>";
    html += "</div></div></div></body></html>";
    return html;
}

static void handleRoot() {
    webServer.send(200, "text/html", renderHomePage());
}

static void handleWiFiConfig() {
    if (!webServer.hasArg("ssid")) {
        webServer.send(400, "text/plain", "Missing ssid");
        return;
    }

    wifi_ssid = webServer.arg("ssid");
    wifi_password = webServer.arg("password");

    g_staConnectedSinceMs = 0;
    triggerSTAReconnect();

    webServer.send(200, "text/html", renderHomePage());
}

static void handleRetryWiFi() {
    g_staConnectedSinceMs = 0;
    triggerSTAReconnect();
    webServer.sendHeader("Location", "/");
    webServer.send(303);
}

static void handleFanControl() {
    if (webServer.hasArg("speed")) {
        fan_speed = constrain(webServer.arg("speed").toInt(), 0, 100);
        int fan_val = map(fan_speed, 0, 100, 0, 255);
        analogWrite(FAN_PIN, fan_val);
        Serial.println("Web fan speed set to " + String(fan_speed) + "%");

        if (uxSemaphoreGetCount(xBinarySemaphoreFanUpdate) == 0) {
            xSemaphoreGive(xBinarySemaphoreFanUpdate);
        }
    }

    webServer.sendHeader("Location", "/");
    webServer.send(303);
}

static void handleFanState() {
    String json = "{\"speed\":" + String(fan_speed) + "}";
    webServer.send(200, "application/json", json);
}

static void handleDoorControl() {
    String action = webServer.arg("action");
    int targetPos = pos_servo;

    if (action == "open") {
        targetPos = 90;
    } else if (action == "close") {
        targetPos = 0;
    } else {
        targetPos = (pos_servo > 0) ? 0 : 90;
    }

    if (!myServo.attached()) {
        myServo.attach(DOOR_SERVO_PIN);
    }

    pos_servo = targetPos;
    myServo.write(pos_servo);
    Serial.println(String("Web door set to ") + (pos_servo > 0 ? "OPEN" : "CLOSED"));

    webServer.sendHeader("Location", "/");
    webServer.send(303);
}

static void handleDoorState() {
    String json = "{\"open\":" + String(pos_servo > 0 ? "true" : "false") + ",\"angle\":" + String(pos_servo) + "}";
    webServer.send(200, "application/json", json);
}

static void handleSensorState() {
    String json = "{\"temperature\":" + String(glob_temperature, 2) + ",\"humidity\":" + String(glob_humidity, 2) + "}";
    webServer.send(200, "application/json", json);
}

static void registerRoutesIfNeeded() {
    if (g_routesRegistered) {
        return;
    }

    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/wifi", HTTP_POST, handleWiFiConfig);
    webServer.on("/retry", HTTP_GET, handleRetryWiFi);
    webServer.on("/fan", HTTP_GET, handleFanControl);
    webServer.on("/fan-state", HTTP_GET, handleFanState);
    webServer.on("/door", HTTP_GET, handleDoorControl);
    webServer.on("/door-state", HTTP_GET, handleDoorState);
    webServer.on("/sensor-state", HTTP_GET, handleSensorState);
    webServer.onNotFound(handleRoot);
    webServer.begin();
    g_routesRegistered = true;

    Serial.println("Web server started on port 80");
}

void setupSTAMode() {
    WiFi.setSleep(false);
    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect(false);
    connectToSTA(20);
}

void TaskWebServer(void *pvParameters) {
    registerRoutesIfNeeded();

    if (WiFi.status() != WL_CONNECTED) {
        startAPModeIfNeeded();
    }

    unsigned long lastCheckMs = 0;

    while (1) {
        webServer.handleClient();

        unsigned long now = millis();
        if (now - lastCheckMs >= 1000) {
            lastCheckMs = now;

            if (WiFi.status() == WL_CONNECTED) {
                if (g_staConnectedSinceMs == 0) {
                    g_staConnectedSinceMs = now;
                    Serial.println("STA connected. Keeping AP available for portal access...");
                }

                if (uxSemaphoreGetCount(xBinarySemaphoreInternet) == 0) {
                    xSemaphoreGive(xBinarySemaphoreInternet);
                }
            } else {
                g_staConnectedSinceMs = 0;
                while (xSemaphoreTake(xBinarySemaphoreInternet, 0) == pdTRUE) {
                    // Drain semaphore so MQTT task blocks until internet is back.
                }
                startAPModeIfNeeded();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}