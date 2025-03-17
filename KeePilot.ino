#include "M5Cardputer.h"
#include "M5GFX.h"
#include "painlessMesh.h"
#include "mash_parameter.h"
#include <IRremote.hpp>

//------------------------ MESH НАЛАШТУВАННЯ ------------------------
Scheduler userScheduler;
painlessMesh  mesh;

//------------------------ ЗМІННІ ВВОДУ ----------------------------
String inputData = "";
bool isInputMode = false;  // Режим введення тексту

//------------------------ IR-НАЛАШТУВАННЯ -------------------------
#define IR_TX_PIN 44
bool projectorPilotMode = false; // Режим пілота

//------------------------ СПРАЙТИ ---------------------------
LGFX_Sprite mainScreenSprite(&M5Cardputer.Display);
LGFX_Sprite pilotModeSprite(&M5Cardputer.Display);
LGFX_Sprite indicatorSprite(&M5Cardputer.Display);
LGFX_Sprite inputSprite(&M5Cardputer.Display);

//------------------------ СТАНИ / ЕКРАНИ ----------------------
// Наприклад, OS0 (головний екран), OS1 (екран введення тексту), OS2 (пілот проектору)
enum CORESCREEN {
  OS0,
  OS1,
  OS2,
  OS3,
  OS4
} screen = OS0;

//-------------------------------------------------------------
// ФУНКЦІЯ ВІДМАЛЬОВУВАННЯ ПОТОЧНОГО ЕКРАНУ
//-------------------------------------------------------------
void coreScreen() {
  switch (screen) {
    //--- ГОЛОВНИЙ ЕКРАН (OS0) ---
    case OS0: {
      mainScreenSprite.fillScreen(BLACK);

      // Правильне зчитування батареї (припустимо, що 407 -> 4.07В)
      int voltageMilliVolt = M5Cardputer.Power.getBatteryVoltage();
      float voltage        = voltageMilliVolt / 100.0;  // Замість 1000.0

      char voltageText[10];
      sprintf(voltageText, "%.2fV", voltage);

      mainScreenSprite.setFont(&fonts::Font4);
      mainScreenSprite.setTextSize(1);
      mainScreenSprite.setTextColor(WHITE);
      mainScreenSprite.setTextDatum(top_right);

      int screenWidth = mainScreenSprite.width();
      // Виводимо вольтаж у правому верхньому куті
      mainScreenSprite.drawString(voltageText, screenWidth - 4, 2);

      mainScreenSprite.pushSprite(0, 0);
      }
      break;

    //--- ЕКРАН ВВЕДЕННЯ ТЕКСТУ (OS1) ---
    case OS1: {
      // Режим введення вже встановлено у handleInput (isInputMode = true)
      // Просто відмалюємо спрайт:
      inputSprite.fillSprite(0x404040);
      inputSprite.setFont(&fonts::Font4);
      inputSprite.setTextSize(1);
      inputSprite.setTextColor(WHITE);
      inputSprite.setCursor(0, 0);

      // Наприклад, inputData = "> "
      inputSprite.print(inputData);
      inputSprite.pushSprite(0, 135 - 28);
      }
      break;

    //--- ЕКРАН "ПІЛОТ ПРОЕКТОРУ" (OS2) ---
    case OS2: {
      pilotModeSprite.fillScreen(BLACK);
      pilotModeSprite.setTextFont(&fonts::Font4);
      pilotModeSprite.setTextColor(GREEN);
      pilotModeSprite.setTextDatum(middle_center);
      pilotModeSprite.setTextSize(1);

      pilotModeSprite.drawString("ProjectorPilot Mode", 120, 12);
      pilotModeSprite.drawFastHLine(0, 20, pilotModeSprite.width(), RED);

      pilotModeSprite.drawString("P-power",       50, 50);
      pilotModeSprite.drawString("`-exit",       140, 50);
      pilotModeSprite.drawString("ok",           210, 50);
      pilotModeSprite.drawString("space-sourse", 160, 75);

      // Стрілки малюємо трикутниками
      // Вгору
      pilotModeSprite.drawTriangle(60, 70, 55, 80, 65, 80, GREEN);
      pilotModeSprite.fillTriangle(60, 70, 55, 80, 65, 80, GREEN);
      // Вниз
      pilotModeSprite.drawTriangle(60, 110, 55, 100, 65, 100, GREEN);
      pilotModeSprite.fillTriangle(60, 110, 55, 100, 65, 100, GREEN);
      // Вліво
      pilotModeSprite.drawTriangle(40, 90, 50, 85, 50, 95, GREEN);
      pilotModeSprite.fillTriangle(40, 90, 50, 85, 50, 95, GREEN);
      // Вправо
      pilotModeSprite.drawTriangle(80, 90, 70, 85, 70, 95, GREEN);
      pilotModeSprite.fillTriangle(80, 90, 70, 85, 70, 95, GREEN);

      pilotModeSprite.pushSprite(0, 0);
      }
      break;

    // ДЛЯ ОС3, ОС4 — за потреби
    default:
      break;
  }
}

//-------------------------------------------------------------
// Анімація індикатора передавання IR
//-------------------------------------------------------------
void sendIRCommand() {
  indicatorSprite.fillScreen(BLACK);
  indicatorSprite.fillCircle(8, 8, 7, GREEN);
  indicatorSprite.pushSprite(32, 105);
  delay(500);

  indicatorSprite.fillScreen(BLACK);
  indicatorSprite.fillCircle(8, 8, 7, YELLOW);
  indicatorSprite.pushSprite(32, 105);
  delay(500);
}

//-------------------------------------------------------------
// ОБРОБКА КЛАВІАТУРИ
//-------------------------------------------------------------
void handleInput() {
  if (M5Cardputer.Keyboard.isChange()) {
    if (M5Cardputer.Keyboard.isPressed()) {
      Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();
      std::string pressedKey(state.word.begin(), state.word.end());

      for (char x : state.word) {
        switch (x) {
          case '`': // бектік: вихід з усіх режимів, назад на OS0
            projectorPilotMode = false;
            isInputMode        = false;
            inputData          = "";
            inputSprite.fillSprite(BLACK);
            inputSprite.pushSprite(0, 135 - 28);

            screen = OS0;
            return;  // Щоб не обробляти інші символи

          case 's':
            if (state.opt) { 
              // 's' із OPT — ігноруємо
              break;
            }
            inputData += x;  // 's' без OPT
            break;

          case ';': // Вверх
            if (projectorPilotMode) {
              IrSender.sendNECRaw(0xF609FC03, 0);
              sendIRCommand();
              return;
            }
            break;

          case '.': // Вниз
            if (projectorPilotMode) { 
              IrSender.sendNECRaw(0xFE01FC03, 0);
              sendIRCommand();
              return;
            }
            break;

          case ',': // Вліво
            if (projectorPilotMode) { 
              IrSender.sendNECRaw(0xF708FC03, 0);
              sendIRCommand();
              return;
            }
            break;

          case '/': // Вправо
            if (projectorPilotMode) { 
              IrSender.sendNECRaw(0xF50AFC03, 0);
              sendIRCommand();
              return;
            }
            break;

          case ' ': // Вибір джерела
            if (projectorPilotMode) { 
              IrSender.sendNECRaw(0xE01FFC03, 0);
              sendIRCommand();
              return;
            }
            break;

          case 'p':
            if (state.opt) { 
              // 'p' із OPT — ігноруємо
              break;
            }
            if (projectorPilotMode) { 
              IrSender.sendNECRaw(0xE21DFC03, 0);
              sendIRCommand();
              return;
            }
            inputData += x;  
            break;

          default:
            // Усі інші символи
            inputData += x;
            break;
        }
      }

      // Якщо натиснули "del" і рядок довший за "> "
      if (state.del && inputData.length() > 2) {
        inputData.remove(inputData.length() - 1);
      }

      // Комбінація OPT + S => Ввімкнення режиму введення (OS1)
      if (state.opt && pressedKey == "s") {
        isInputMode        = true;
        projectorPilotMode = false;
        inputData          = "> "; // Завжди починаємо з "> "

        screen = OS1;
        return;  
      }

      // Комбінація OPT + P => Ввімкнення режиму «пілот проектору» (OS2)
      if (state.opt && pressedKey == "p") {
        isInputMode        = false;
        projectorPilotMode = true;

        screen = OS2;
        return;
      }

      // Натиснули Enter і перебуваємо у режимі введення
      if (state.enter && isInputMode) {
        isInputMode = false;
        String savedText = inputData.substring(2);  
        // Наприклад, розсилаємо по mesh:
        mesh.sendBroadcast(savedText);

        // Очищаємо інпут-спрайт
        inputSprite.fillSprite(BLACK);
        inputSprite.pushSprite(0, 135 - 28);
      }

      // Натиснули Enter у режимі projectorPilotMode => відправляємо IR (OK)
      if (state.enter && projectorPilotMode) {
        IrSender.sendNECRaw(0xF40BFC03, 0);
        sendIRCommand();
      }

      // Якщо ми не в режимі введення, скидаємо inputData (щоб порожнім було)
      if (!isInputMode) {
        inputData = "";
      }
    }
  }
}

//-------------------------------------------------------------
// SETUP
//-------------------------------------------------------------
void setup() {
  // Ініціалізація mesh
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);

  // Ініціалізація M5Cardputer
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setTextDatum(middle_center);
  M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
  M5Cardputer.Display.setTextSize(1);

  // Ініціалізація IR
  IrSender.begin(DISABLE_LED_FEEDBACK);
  IrSender.setSendPin(IR_TX_PIN);

  // Створення спрайтів (один раз)
  int w = M5Cardputer.Display.width();
  int h = M5Cardputer.Display.height();
  mainScreenSprite.createSprite(w, h);
  pilotModeSprite.createSprite(w, h);
  indicatorSprite.createSprite(16, 16);
  inputSprite.createSprite(240, 28);

  // Початковий екран (OS0)
  screen = OS0;
}

//-------------------------------------------------------------
// LOOP
//-------------------------------------------------------------
void loop() {
  // 1) Малюємо поточний екран
  coreScreen();

  // 2) Оновлюємо mesh
  mesh.update();

  // 3) Оновлюємо M5Cardputer
  M5Cardputer.update();

  // 4) Обробляємо натискання клавіш
  handleInput();
}
