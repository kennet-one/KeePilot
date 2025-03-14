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

  // Виводимо текст вольтажу в правий верхній кут з відступом
  mainScreenSprite.drawString(voltageText, screenWidth - 4, 2);

  // Розміри і позиція індикатора батареї
  const int battWidth = 26;   // Загальна ширина іконки батареї
  const int battHeight = 12;  // Висота іконки батареї
  const int battX = screenWidth - battWidth - 100; // Посунули значно лівіше від тексту
  const int battY = 3;

  // Малюємо контур батареї
  mainScreenSprite.drawRect(battX, battY, battWidth, battHeight, WHITE);
  mainScreenSprite.fillRect(battX + battWidth, battY + 3, 2, 6, WHITE); // Носик батареї

  // Обчислюємо рівень заряду в %
  int batteryPercent = map(voltageMilliVolt, 330, 420, 0, 100);
  batteryPercent = constrain(batteryPercent, 0, 100);

  // Ширина заповнення (22 - з невеликими полями)
  int fillWidth = map(batteryPercent, 0, 70, 0, battWidth - 4);

  // Заповнення батареї відповідно до рівня заряду
  uint16_t fillColor = batteryPercent > 20 ? GREEN : RED;
  mainScreenSprite.fillRect(battX + 2, battY + 2, fillWidth, battHeight - 4, fillColor);

  // Показуємо індикатор зарядки (маленька блискавка зліва)
  //if (M5Cardputer.Power.isCharging()) {
    // Блискавка з ліній 
  //  int cx = battX - 12;
  //  int cy = battY + 2;
  //  mainScreenSprite.drawLine(cx + 3, cy, cx, cy + 5, YELLOW);
  //  mainScreenSprite.drawLine(cx, cy + 5, cx + 4, cy + 5, YELLOW);
  //  mainScreenSprite.drawLine(cx + 4, cy + 5, cx, cy + 10, YELLOW);
  //}
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
  pilotModeSprite.drawString("L-ok", 210, 50);
  pilotModeSprite.drawString("space-sourse", 160, 75);

  //pilotModeSprite.drawString("↑", 60, 75);
  //pilotModeSprite.drawString("← ↓ →", 60, 100);
  
  pilotModeSprite.drawTriangle(60, 70, 55, 80, 65, 80, GREEN);
  pilotModeSprite.fillTriangle(60, 70, 55, 80, 65, 80, GREEN);

  // ↓ (вниз)
  pilotModeSprite.drawTriangle(60, 110, 55, 100, 65, 100, GREEN);
  pilotModeSprite.fillTriangle(60, 110, 55, 100, 65, 100, GREEN);

  // ← (вліво)
  pilotModeSprite.drawTriangle(40, 90, 50, 85, 50, 95, GREEN);
  pilotModeSprite.fillTriangle(40, 90, 50, 85, 50, 95, GREEN);

  // → (вправо)
  pilotModeSprite.drawTriangle(80, 90, 70, 85, 70, 95, GREEN);
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

          case 'l': // ok
            if (projectorPilotMode) { 
              IrSender.sendNECRaw(0xF40BFC03, 0);
              sendIRCommand();
              break;
            }
            inputData += x;
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

      if (state.enter) {
        isInputMode = false;  
        String savedText = inputData.substring(2);  
        inputData = "";  
        mesh.sendBroadcast(savedText);

        inputSprite.fillSprite(BLACK);
        inputSprite.pushSprite(0, 135 - 28);
      }

      if (!isInputMode ) {  // удаляем ненужний спрайт вводу
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
