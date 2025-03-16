#include "M5Cardputer.h"
#include "M5GFX.h"
#include "painlessMesh.h"
#include "mash_parameter.h"
#include <IRremote.hpp>

//------------------------ MESH НАЛАШТУВАННЯ ------------------------
Scheduler userScheduler;
painlessMesh  mesh;
//------------------------до введення тексту---------------------------
String inputData = "";
bool isInputMode = false;  // Режим введення тексту
//------------------------спрайти---------------------------
LGFX_Sprite mainScreenSprite(&M5Cardputer.Display);
LGFX_Sprite pilotModeSprite(&M5Cardputer.Display);
LGFX_Sprite indicatorSprite(&M5Cardputer.Display);
LGFX_Sprite inputSprite(&M5Cardputer.Display);

void showMainScreen() {
  inputData = "";
  mainScreenSprite.fillScreen(BLACK);

  int voltageMilliVolt = M5Cardputer.Power.getBatteryVoltage();
  float voltage = voltageMilliVolt / 1000.0;  

  char voltageText[10];
  sprintf(voltageText, "%.2fV", voltage);

  mainScreenSprite.setFont(&fonts::Font4);
  mainScreenSprite.setTextSize(1);
  mainScreenSprite.setTextColor(WHITE);
  mainScreenSprite.setTextDatum(top_right);

  int screenWidth = mainScreenSprite.width();

  // Виводимо текст вольтажу в правий верхній кут
  mainScreenSprite.drawString(voltageText, screenWidth - 4, 2);

  mainScreenSprite.pushSprite(0, 0);
}

void showProjectorPilotScreen() {
  pilotModeSprite.fillScreen(BLACK);
  pilotModeSprite.setTextFont(&fonts::Font4); // Менший шрифт
  pilotModeSprite.setTextColor(GREEN);
  pilotModeSprite.setTextDatum(middle_center);
  pilotModeSprite.setTextSize(1);
                    // ;-up// .-d// ,-L// /-R// ок
  pilotModeSprite.drawString("ProjectorPilot Mode",120,12);
  pilotModeSprite.drawFastHLine(0, 20, pilotModeSprite.width(), RED);

  pilotModeSprite.drawString("P-power" ,50, 50);
  pilotModeSprite.drawString("`-exit", 140, 50);
  pilotModeSprite.drawString("ok", 210, 50);
  pilotModeSprite.drawString("space-sourse", 160, 75);
  
  pilotModeSprite.drawTriangle(60, 70, 55, 80, 65, 80, GREEN); // в верх
  pilotModeSprite.fillTriangle(60, 70, 55, 80, 65, 80, GREEN);

  pilotModeSprite.drawTriangle(60, 110, 55, 100, 65, 100, GREEN);  // ↓ (вниз)
  pilotModeSprite.fillTriangle(60, 110, 55, 100, 65, 100, GREEN);

  pilotModeSprite.drawTriangle(40, 90, 50, 85, 50, 95, GREEN);  // ← (вліво)
  pilotModeSprite.fillTriangle(40, 90, 50, 85, 50, 95, GREEN);

  pilotModeSprite.drawTriangle(80, 90, 70, 85, 70, 95, GREEN);  // → (вправо)
  pilotModeSprite.fillTriangle(80, 90, 70, 85, 70, 95, GREEN);

  pilotModeSprite.pushSprite(0,0);
}

void sendIRCommand() {  // анімація значка передачі
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

  inputSprite.createSprite(240, 28);
  inputSprite.fillSprite(0x404040);  
  inputSprite.setTextFont(&fonts::Font4);
  inputSprite.setTextSize(1);
  inputSprite.setTextColor(WHITE);
  inputSprite.setCursor(0, 0);
  inputSprite.print(inputData);
  inputSprite.pushSprite(0, 135 - 28);
}
//------------------------ IR НАЛАШТУВАННЯ --------------------------
#define IR_TX_PIN 44
bool projectorPilotMode = false; // Режим пілота

//------------------------ ВВОД ТЕКСТУ ------------------------------

void handleInput() {
  if (M5Cardputer.Keyboard.isChange()) {
    if (M5Cardputer.Keyboard.isPressed()) {
      Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();
      std::string pressedKey(state.word.begin(), state.word.end());

      for (char x : state.word) {
        switch (x) {
          case '`'://бектік, виходимо з режиму вводу
            projectorPilotMode = false;
            isInputMode = false;
            inputData = "";
            inputSprite.fillSprite(BLACK);
            inputSprite.pushSprite(0, 135 - 28);
            showMainScreen();
            return; 

          case 's':
            if (state.opt) { //'s' із OPT в пустоту
              //Нічого в inputData
              break;
            }
            inputData += x;  //'s' без OPT +
            break;

          case ';': // в верх
            if (projectorPilotMode) { 
              IrSender.sendNECRaw(0xF609FC03, 0);
              sendIRCommand();
              break;
            }
            break;
            
          case '.': // вниз
            if (projectorPilotMode) { 
              IrSender.sendNECRaw(0xFE01FC03, 0);
              sendIRCommand();
              break;
            }
            break;

          case ',': // вліво
            if (projectorPilotMode) { 
              IrSender.sendNECRaw(0xF708FC03, 0);
              sendIRCommand();
              break;
            }
            break;

          case '/': // вправо
            if (projectorPilotMode) { 
              IrSender.sendNECRaw(0xF50AFC03, 0);
              sendIRCommand();
              break;
            }
            break;

          case ' ': // вибор источніка
            if (projectorPilotMode) { 
              IrSender.sendNECRaw(0xE01FFC03, 0);
              sendIRCommand();
              break;
            }
            break;

          case 'p':
            if (state.opt) { //'p' із OPT в пустоту
              //Нічого в inputData
              break;
            }
            if (projectorPilotMode) { 
              IrSender.sendNECRaw(0xE21DFC03, 0);
              sendIRCommand();
              break;
            }
            inputData += x;  //'s' без OPT +
            break;

          default:
            // Усі інші до рядка
            inputData += x;
            break;
        }
      }


      if (state.del && inputData.length() > 2) {
        inputData.remove(inputData.length() - 1);
      }

      if (state.opt && pressedKey == "s") {
        showMainScreen();
        isInputMode = true;
        projectorPilotMode = false; 
        startInputMode();
      }

      if (state.opt && pressedKey == "p") {
        showMainScreen();
        isInputMode = false;
        projectorPilotMode = true; 
        showProjectorPilotScreen();
      }

      if (state.enter && isInputMode) {
        isInputMode = false;  
        String savedText = inputData.substring(2);    
        mesh.sendBroadcast(savedText);

        inputSprite.fillSprite(BLACK);
        inputSprite.pushSprite(0, 135 - 28);
      }

      if (state.enter && projectorPilotMode) {
        if (projectorPilotMode) { 
          IrSender.sendNECRaw(0xF40BFC03, 0);
          sendIRCommand();
        }
      }

      if (isInputMode) {
        inputSprite.fillSprite(0x404040);
        inputSprite.setCursor(0, 0);
        inputSprite.setTextColor(WHITE);
        inputSprite.print(inputData);
        inputSprite.pushSprite(0, 135 - 28);
      } else {// удаляем ненужний спрайт вводу
        inputSprite.deleteSprite();
      }
    }
  }
}

void setup() {
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);

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

  handleInput();
}