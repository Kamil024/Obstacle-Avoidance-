#include <WiFi.h>
#include <WebServer.h>

// ========== WiFi AP Settings ==========
const char* apSsid     = "Selecta Ice Cream";
const char* apPassword = "noconnect024";
const int MAX_CLIENTS  = 1;

// Built-in LED for connection status
#define LED_PIN 2

// ========== Web server ==========
WebServer server(80);

// ========== Motor Pins (L298N) ==========
#define IN1 14   
#define IN2 27
#define IN3 26   
#define IN4 25
#define ENA_PIN 33
#define ENB_PIN 32

// PWM settings
const int PWM_FREQ = 20000;
const int PWM_RES  = 8;
const int ENA_CH   = 0; // Fallback channel for Core v2.x
const int ENB_CH   = 1; // Fallback channel for Core v2.x

// Speed values
uint8_t manualSpeed = 230;    
uint8_t autoSpeed   = 200;    

// Ultrasonic Sensor
#define TRIG_PIN 4
#define ECHO_PIN 5

// ========== Flags & States ==========
bool forwardCmd = false, backCmd = false, leftCmd = false, rightCmd = false;
bool obstacleMode = false;
enum AutoState { AUTO_IDLE, AUTO_FORWARD, AUTO_BACKWARD, AUTO_TURN };
AutoState autoState = AUTO_IDLE;
unsigned long autoStateStart = 0;
bool turnLeftNext = true;
const unsigned long BACK_TIME = 600;
const unsigned long TURN_TIME = 700;
const float OBSTACLE_DIST_CM = 20.0;

// Variables for non-blocking double blink loop
unsigned long previousBlinkTime = 0;
int blinkState = 0;

// ========== NEW XBOX PRO HTML PROGRAM ==========
const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <title>ESP32 ROVER - XBOX PRO</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      text-align: center;
      background: radial-gradient(circle at center, #2a2a35 0%, #101014 100%);
      color: #eee;
      margin: 0;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      overflow: hidden;
    }
    .container {
      animation: fadeUp 0.8s ease-out;
      width: 100%;
      max-width: 400px;
      position: relative;
    }
    @keyframes fadeUp {
      from { opacity: 0; transform: translateY(30px); }
      to { opacity: 1; transform: translateY(0); }
    }
    h1 { margin-bottom: 5px; text-shadow: 0 0 15px rgba(0, 150, 255, 0.6); letter-spacing: 3px; color: #fff; }
    .subtitle { color: #888; font-size: 13px; margin-bottom: 10px; text-transform: uppercase; }
    #help-btn {
      position: absolute;
      top: -40px;
      right: 10px;
      width: 35px;
      height: 35px;
      border-radius: 50%;
      background: rgba(255,255,255,0.1);
      border: 1px solid #444;
      color: #fff;
      cursor: pointer;
      font-weight: bold;
      display: flex;
      align-items: center;
      justify-content: center;
      transition: 0.3s;
    }
    #help-btn:hover { background: #0096ff; }
    .modal-overlay {
      display: none;
      position: fixed;
      z-index: 100;
      left: 0; top: 0; width: 100%; height: 100%;
      background-color: rgba(0,0,0,0.85);
      backdrop-filter: blur(5px);
      align-items: center;
      justify-content: center;
    }
    .modal-content {
      background: #1a1a1f;
      padding: 30px;
      border-radius: 25px;
      border: 1px solid #00ffcc;
      max-width: 320px;
      text-align: left;
      box-shadow: 0 0 30px rgba(0,255,204,0.2);
    }
    .modal-content h3 { color: #00ffcc; margin-top: 0; border-bottom: 1px solid #333; padding-bottom: 10px; }
    .modal-content ul { list-style: none; padding: 0; line-height: 2; }
    .modal-content span { color: #fff; font-weight: bold; background: #333; padding: 2px 8px; border-radius: 5px; font-size: 12px; margin-right: 5px; }
    .close-modal { float: right; cursor: pointer; color: #888; font-size: 20px; }
    #credits-trigger {
      position: fixed;
      bottom: 10px;
      left: 10px;
      font-size: 10px;
      color: #222;
      cursor: pointer;
      padding: 10px;
      transition: color 0.5s;
      z-index: 10;
    }
    #credits-trigger:hover { color: #555; }
    .credits-modal-content { border-color: #ff00ff; box-shadow: 0 0 30px rgba(255,0,255,0.2); text-align: center; }
    .credits-modal-content h3 { color: #ff00ff; }
    .team-member { margin: 10px 0; font-weight: bold; color: #fff; letter-spacing: 1px; }
    #controller-status {
      font-size: 11px;
      font-weight: bold;
      letter-spacing: 1px;
      padding: 8px;
      margin-bottom: 20px;
      color: #555;
      border: 1px solid #333;
      border-radius: 10px;
      display: inline-block;
    }
    .connected { color: #00ffcc !important; border-color: #00ffcc !important; }
    .grid {
      margin-bottom: 40px;
      padding: 30px 15px;
      background: rgba(255, 255, 255, 0.02);
      border-radius: 30px;
      box-shadow: 0 10px 40px 0 rgba(0, 0, 0, 0.5);
      border: 1px solid rgba(255, 255, 255, 0.05);
    }
    .row { margin: 12px 0; display: flex; justify-content: center; gap: 10px; }
    .btn {
      display: inline-block;
      padding: 20px;
      font-size: 14px;
      font-weight: bold;
      text-transform: uppercase;
      border-radius: 16px;
      border: 1px solid #333;
      cursor: pointer;
      min-width: 100px;
      background: linear-gradient(145deg, #222, #181818);
      color: #888;
      box-shadow: 6px 6px 12px #0b0b0d, -6px -6px 12px #25252f;
      transition: all 0.2s ease;
      user-select: none;
      outline: none;
      -webkit-tap-highlight-color: transparent;
    }
    .btn.active, .btn:active { transform: scale(0.95); border-color: #00ffcc; color: #00ffcc; box-shadow: inset 4px 4px 8px #0b0b0d; }
    .btn.locked { opacity: 0.2; cursor: not-allowed; }
    .stop-btn.active, .stop-btn:active { border-color: #ffaa00; color: #ffaa00; }
    #mode-toggle { min-width: 180px; padding: 22px; font-size: 16px; }
    .mode-off { border-color: #441111 !important; color: #aa4444 !important; background: linear-gradient(145deg, #251010, #150505) !important; }
    .mode-on { border-color: #00ffcc !important; color: #fff !important; background: linear-gradient(145deg, #0a2e1a, #05150a) !important; animation: pulseGreen 2s infinite; }
    @keyframes pulseGreen {
      0% { box-shadow: 0 0 0 0 rgba(0, 255, 204, 0.4); }
      70% { box-shadow: 0 0 0 15px rgba(0, 255, 204, 0); }
      100% { box-shadow: 0 0 0 0 rgba(0, 255, 204, 0); }
    }
  </style>
</head>
<body>
  <div class="container">
    <div id="help-btn" onclick="toggleModal('help-modal', true)">?</div>
    <h1>ESP32 CONTROL</h1>
    <p class="subtitle">Yaerz👽</p>
    <div id="controller-status">🎮 DISCONNECTED (PRESS A BUTTON)</div>
    <div class="grid">
      <div class="row">
        <button id="btn-f" class="btn manual-ctrl" onmousedown="sendCmd('f',1)" onmouseup="sendCmd('f',0)" ontouchstart="sendCmd('f',1)" ontouchend="sendCmd('f',0)">Forward</button>
      </div>
      <div class="row">
        <button id="btn-l" class="btn manual-ctrl" onmousedown="sendCmd('l',1)" onmouseup="sendCmd('l',0)" ontouchstart="sendCmd('l',1)" ontouchend="sendCmd('l',0)">Left</button>
        <button id="btn-s" class="btn stop-btn manual-ctrl" onmousedown="sendCmd('s',1)" onmouseup="sendCmd('s',0)" ontouchstart="sendCmd('s',1)" ontouchend="sendCmd('s',0)">Stop</button>
        <button id="btn-r" class="btn manual-ctrl" onmousedown="sendCmd('r',1)" onmouseup="sendCmd('r',0)" ontouchstart="sendCmd('r',1)" ontouchend="sendCmd('r',0)">Right</button>
      </div>
      <div class="row">
        <button id="btn-b" class="btn manual-ctrl" onmousedown="sendCmd('b',1)" onmouseup="sendCmd('b',0)" ontouchstart="sendCmd('b',1)" ontouchend="sendCmd('b',0)">Backward</button>
      </div>
    </div>
    <div class="row">
      <button id="mode-toggle" class="btn mode-off" onclick="handleToggle()">AUTO: OFF</button>
    </div>
  </div>

  <div id="help-modal" class="modal-overlay">
    <div class="modal-content">
      <span class="close-modal" onclick="toggleModal('help-modal', false)">&times;</span>
      <h3>ROVER GUIDE</h3>
      <ul>
        <li><span>L-STICK</span> / <span>D-PAD</span> : Drive</li>
        <li><span>L1 / LB</span> : Emergency Stop</li>
        <li><span>R1 / RB</span> : Toggle AUTO Mode</li>
        <li><span>SELECT</span> : Open/Close Help</li>
      </ul>
    </div>
  </div>

  <div id="credits-trigger" onclick="toggleModal('credits-modal', true)">© CREDITS</div>
  <div id="credits-modal" class="modal-overlay">
    <div class="modal-content credits-modal-content">
      <span class="close-modal" onclick="toggleModal('credits-modal', false)">&times;</span>
      <h3>DEVELOPED BY</h3>
      <div class="team-member">Rey Vincent William Sazon</div>
      <div class="team-member">Jayson Libaton</div>
      <div class="team-member">Jujin Ferrer</div>
    </div>
  </div>

  <script>
    let lastState = { f: 0, b: 0, l: 0, r: 0, s: 0 };
    let lastAutoBtnState = false; 
    let lastHelpBtnState = false;
    let autoModeActive = false;
    let gamepadIndex = null;

    function toggleModal(id, show) {
      document.getElementById(id).style.display = show ? 'flex' : 'none';
    }

    function sendCmd(dir, state) {
      if (autoModeActive) return; 
      const el = document.getElementById(`btn-${dir}`);
      if (el) state === 1 ? el.classList.add('active') : el.classList.remove('active');
      fetch(`/cmd?dir=${dir}&state=${state}`).catch(e => {});
    }

    function handleToggle() {
      const btn = document.getElementById('mode-toggle');
      autoModeActive = !autoModeActive;
      btn.innerText = autoModeActive ? "AUTO: ON" : "AUTO: OFF";
      btn.className = autoModeActive ? "btn mode-on" : "btn mode-off";
      document.querySelectorAll('.manual-ctrl').forEach(b => {
        autoModeActive ? b.classList.add('locked') : b.classList.remove('locked');
      });
      fetch(`/mode?auto=${autoModeActive ? 1 : 0}`).catch(e => {});
    }

    window.addEventListener("gamepadconnected", (e) => {
      gamepadIndex = e.gamepad.index;
      document.getElementById('controller-status').innerText = "🎮 " + e.gamepad.id.split('(')[0].toUpperCase() + " ACTIVE";
      document.getElementById('controller-status').classList.add('connected');
      pollGamepad();
    });

    function pollGamepad() {
      if (gamepadIndex === null) return;
      const gp = navigator.getGamepads()[gamepadIndex];
      if (!gp) return;

      let autoBtn = gp.buttons[5].pressed;
      if (autoBtn && !lastAutoBtnState) handleToggle();
      lastAutoBtnState = autoBtn;

      let helpBtn = gp.buttons[8].pressed;
      if (helpBtn && !lastHelpBtnState) {
        let isVisible = document.getElementById('help-modal').style.display === 'flex';
        toggleModal('help-modal', !isVisible);
      }
      lastHelpBtnState = helpBtn;

      if (!autoModeActive) {
          const deadzone = 0.5;
          const current = {
            f: (gp.buttons[12].pressed || gp.axes[1] < -deadzone) ? 1 : 0,
            b: (gp.buttons[13].pressed || gp.axes[1] > deadzone) ? 1 : 0,
            l: (gp.buttons[14].pressed || gp.axes[0] < -deadzone) ? 1 : 0,
            r: (gp.buttons[15].pressed || gp.axes[0] > deadzone) ? 1 : 0,
            s: (gp.buttons[4].pressed) ? 1 : 0 
          };
          for (let key in current) {
            if (current[key] !== lastState[key]) {
              sendCmd(key, current[key]);
              lastState[key] = current[key];
            }
          }
      }
      requestAnimationFrame(pollGamepad);
    }
  </script>
</body>
</html>
)=====";

// ========== Motor Functions ==========
void setMotorSpeed(uint8_t left, uint8_t right) {
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  ledcWrite(ENA_PIN, left);
  ledcWrite(ENB_PIN, right);
#else
  ledcWrite(ENA_CH, left);
  ledcWrite(ENB_CH, right);
#endif
}

void driveStop() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  setMotorSpeed(0, 0);
}

void driveForward(uint8_t speed) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  setMotorSpeed(speed, speed);
}

void driveBackward(uint8_t speed) {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  setMotorSpeed(speed, speed);
}

void driveLeft(uint8_t speed) {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  setMotorSpeed(speed, speed);
}

void driveRight(uint8_t speed) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  setMotorSpeed(speed, speed);
}

float getDistanceCm() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  return (duration == 0) ? -1.0 : (duration * 0.0343) / 2.0;
}

void handleManualMovement() {
  if (forwardCmd) driveForward(manualSpeed);
  else if (backCmd) driveBackward(manualSpeed);
  else if (leftCmd) driveLeft(manualSpeed);
  else if (rightCmd) driveRight(manualSpeed);
  else driveStop();
}

void handleAutoObstacle() {
  unsigned long now = millis();
  switch (autoState) {
    case AUTO_IDLE: autoState = AUTO_FORWARD; break;
    case AUTO_FORWARD: {
      float d = getDistanceCm();
      if (d > 0 && d < OBSTACLE_DIST_CM) { driveStop(); autoState = AUTO_BACKWARD; autoStateStart = now; }
      else driveForward(autoSpeed);
      break;
    }
    case AUTO_BACKWARD:
      if (now - autoStateStart >= BACK_TIME) { driveStop(); autoState = AUTO_TURN; autoStateStart = now; }
      else driveBackward(autoSpeed);
      break;
    case AUTO_TURN:
      if (turnLeftNext) driveLeft(autoSpeed); else driveRight(autoSpeed);
      if (now - autoStateStart >= TURN_TIME) { driveStop(); turnLeftNext = !turnLeftNext; autoState = AUTO_FORWARD; }
      break;
  }
}

// double blink comment
void handleDoubleBlink() {
  if (WiFi.softAPgetStationNum() > 0) {
    unsigned long currentMillis = millis();
    
    if (blinkState == 0) {
      digitalWrite(LED_PIN, HIGH);
      if (currentMillis - previousBlinkTime >= 150) { blinkState = 1; previousBlinkTime = currentMillis; }
    } else if (blinkState == 1) {
      digitalWrite(LED_PIN, LOW);
      if (currentMillis - previousBlinkTime >= 150) { blinkState = 2; previousBlinkTime = currentMillis; }
    } else if (blinkState == 2) {
      digitalWrite(LED_PIN, HIGH);
      if (currentMillis - previousBlinkTime >= 150) { blinkState = 3; previousBlinkTime = currentMillis; }
    } else if (blinkState == 3) {
      digitalWrite(LED_PIN, LOW);
      if (currentMillis - previousBlinkTime >= 1000) { blinkState = 0; previousBlinkTime = currentMillis; }
    }
  } else {
    digitalWrite(LED_PIN, LOW);
    blinkState = 0;
  }
}

void handleRoot() { server.send_P(200, "text/html", index_html); }
void handleCmd() {
  String dir = server.arg("dir");
  int state = server.arg("state").toInt();
  if (!obstacleMode) {
    bool pressed = (state != 0);
    if (dir == "f") forwardCmd = pressed;
    else if (dir == "b") backCmd = pressed;
    else if (dir == "l") leftCmd = pressed;
    else if (dir == "r") rightCmd = pressed;
    else if (dir == "s") { forwardCmd = backCmd = leftCmd = rightCmd = false; driveStop(); }
  }
  server.send(200, "text/plain", "OK");
}

void handleMode() {
  obstacleMode = (server.arg("auto").toInt() == 1);
  if (obstacleMode) { forwardCmd = backCmd = leftCmd = rightCmd = false; autoState = AUTO_FORWARD; }
  else driveStop();
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_PIN, OUTPUT);
  
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  ledcAttach(ENA_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(ENB_PIN, PWM_FREQ, PWM_RES);
#else
  ledcSetup(ENA_CH, PWM_FREQ, PWM_RES);
  ledcAttachPin(ENA_PIN, ENA_CH);
  ledcSetup(ENB_CH, PWM_FREQ, PWM_RES);
  ledcAttachPin(ENB_PIN, ENB_CH);
#endif

  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);
  
  // Apply MAX_CLIENTS restriction
  WiFi.softAP(apSsid, apPassword, 1, 0, MAX_CLIENTS);
  
  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);
  server.on("/mode", handleMode);
  server.begin();
}

void loop() {
  server.handleClient();
  
  handleDoubleBlink(); 

  if (obstacleMode) handleAutoObstacle();
  else handleManualMovement();
  
  delay(5);
}