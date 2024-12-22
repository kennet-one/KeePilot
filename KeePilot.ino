#include "M5Cardputer.h"
#include "M5GFX.h"
#include "painlessMesh.h"
#include "mash_parameter.h"
#include <IRremote.hpp>

//------------------------ MESH НАЛАШТУВАННЯ ------------------------
Scheduler userScheduler; 
painlessMesh  mesh;

//void receivedCallback( uint32_t from, String &msg ) {
  // Обробка отриманих повідомлень, якщо потрібно
//}

//void newConnectionCallback(uint32_t nodeId) {}
//void changedConnectionCallback() {}
//void nodeTimeAdjustedCallback(int32_t offset) {}

//------------------------ IR НАЛАШТУВАННЯ --------------------------
#define IR_TX_PIN 44
bool projectorPilotMode = false; // Режим пілота
LGFX_Sprite mainScreenSprite(&M5Cardputer.Display);
LGFX_Sprite pilotModeSprite(&M5Cardputer.Display);
LGFX_Sprite indicatorSprite(&M5Cardputer.Display);

//------------------------ ВВОД ТЕКСТУ ------------------------------
M5Canvas canvas(&M5Cardputer.Display);
String inputData = "";
bool isInputMode = false;  // Режим введення тексту

void showMainScreen() {
  mainScreenSprite.fillScreen(BLACK);
  mainScreenSprite.pushSprite(0,0);
}

void showProjectorPilotScreen() {
  pilotModeSprite.fillScreen(BLACK);
  pilotModeSprite.setTextFont(&fonts::Font4); // Менший шрифт
  pilotModeSprite.setTextColor(GREEN);
  pilotModeSprite.setTextDatum(middle_center);
  pilotModeSprite.setTextSize(1);

  pilotModeSprite.drawString("ProjectorPilot Mode",
                             pilotModeSprite.width()/2,
                             pilotModeSprite.height()/2 - 40);

  pilotModeSprite.drawString("Press P to send IR",
                             pilotModeSprite.width()/2,
                             pilotModeSprite.height()/2);

  pilotModeSprite.drawString("Press ` to exit",
                             pilotModeSprite.width()/2,
                             pilotModeSprite.height()/2 + 40);

  pilotModeSprite.pushSprite(0,0);
}

void sendIRCommand() {
  IrSender.sendNECRaw(0xE21DFC03, 0);
  indicatorSprite.fillScreen(BLACK);
  indicatorSprite.fillCircle(8, 8, 7, GREEN);
  indicatorSprite.pushSprite(32, 105);
  delay(500);

  indicatorSprite.fillScreen(BLACK);
  indicatorSprite.fillCircle(8, 8, 7, YELLOW);
  indicatorSprite.pushSprite(32, 105);
  delay(500);

  showProjectorPilotScreen();
}

void startInputMode() {
  isInputMode = true;  // Увімкнути режим введення
  inputData = "> ";    // Додаємо символ вводу

  canvas.createSprite(240, 28);
  canvas.fillSprite(0x404040);  
  canvas.setTextFont(&fonts::Font4);
  canvas.setTextSize(1);
  canvas.setTextColor(WHITE);
  canvas.setCursor(0, 0);
  canvas.print(inputData);
  canvas.pushSprite(0, 135 - 28);
}

void handleInput() {
  if (M5Cardputer.Keyboard.isChange()) {
    if (M5Cardputer.Keyboard.isPressed()) {
      Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();
      std::string pressedKey(state.word.begin(), state.word.end());
      
      if (pressedKey == "`") {
        isInputMode = false;
        inputData = "";
        canvas.fillSprite(BLACK);
        canvas.pushSprite(0, 135 - 28);
        showMainScreen();
        return; // Припиняємо подальшу обробку
      }
      for (char c : state.word) {
        if (state.opt && c == 's') { // пропустити як нажате комбо
          continue;
        }
        inputData += c;
      }

      if (state.del && inputData.length() > 2) {
        inputData.remove(inputData.length() - 1);
      }

      if (state.enter) {
        isInputMode = false;  
        String savedText = inputData.substring(2);  
        inputData = "";  
        mesh.sendBroadcast(savedText);

        canvas.fillSprite(BLACK);
        canvas.pushSprite(0, 135 - 28);
      }

      if (!isInputMode) {
        canvas.fillSprite(BLACK);
      } else {
        canvas.fillSprite(0x404040);
      }
      canvas.setCursor(0, 0);
      canvas.print(inputData);
      canvas.pushSprite(0, 135 - 28);
    }
  }
}

void setup() {
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  //mesh.onReceive(&receivedCallback);
  //mesh.onNewConnection(&newConnectionCallback);
  //mesh.onChangedConnections(&changedConnectionCallback);
  //mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setTextDatum(middle_center);
  M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
  M5Cardputer.Display.setTextSize(1);

  IrSender.begin(DISABLE_LED_FEEDBACK);
  IrSender.setSendPin(IR_TX_PIN);

  int w = M5Cardputer.Display.width();
  int h = M5Cardputer.Display.height();
  mainScreenSprite.createSprite(w, h);
  pilotModeSprite.createSprite(w, h);
  indicatorSprite.createSprite(16, 16);

  showMainScreen();
}

void loop() {
  mesh.update();
  M5Cardputer.update();  

  // Якщо активний режим вводу, він має пріоритет
  if (isInputMode) {
    handleInput();
  } else {
    // Режим вводу не активний, перевіряємо пілотний режим
    if (M5Cardputer.Keyboard.isPressed() && !projectorPilotMode) {
      Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();
      std::string pressedKey(state.word.begin(), state.word.end());
      if (state.opt && pressedKey == "p") {
        projectorPilotMode = true;
        showProjectorPilotScreen();
      }
    }

    if (projectorPilotMode && M5Cardputer.Keyboard.isPressed()) { 
      Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();
      std::string pressedKey(state.word.begin(), state.word.end());
      if (pressedKey == "`") {
        projectorPilotMode = false;
        showMainScreen();
      } else if (pressedKey == "p") {
        if (!state.opt) {
          sendIRCommand();
        }
      }
    }

    // Перевірка ввімкнення режиму вводу (OPT+S) – має вищий пріоритет за пілота
    if (M5Cardputer.Keyboard.isPressed()) {
      Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();
      std::string pressedKey(state.word.begin(), state.word.end());
      if (state.opt && pressedKey == "s") {
        isInputMode = true;
        projectorPilotMode = false; 
        startInputMode();
      }
    }
  }
}
