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

//------------------------ ЕКРАНИ (ENUM) ----------------------
enum CORESCREEN {
  OS0,   // Головний екран (вольтаж)
  OS1,   // Екран введення
  OS2,   // Пілот проектора
  OS3,   // Меню (графічне)
  OS4
} screen = OS0;

//=============================================================
//                МЕНЮ: СТРУКТУРА ТА ЛОГІКА
//=============================================================
enum ActionID {
  ACTION_NONE = 0,
  ACTION_BEDSIDE,
  ACTION_LAMPK,
};

// Тепер кожен MenuItem має ще й ідентифікатор дії
struct MenuItem {
  const char* title;    // Назва, як раніше
  bool isAction;
  MenuItem* submenu;
  int submenuCount;
  ActionID actionID;    // поле з enum
};

void performAction(ActionID id) {
  switch (id) {
    case ACTION_BEDSIDE:
      mesh.sendSingle(635035530,"bedside");
      break;

    case ACTION_LAMPK:
      mesh.sendSingle(434115122,"lam");
      break;

    default:
      // Для будь-яких інших дій
      break;
  }
}

void onMenuItemSelected(MenuItem &item) {
  // Якщо це дія — викликаємо performAction()
  if (item.isAction) {
    performAction(item.actionID);
  } else {
    // Йдемо в підменю (як раніше)
  }
}

MenuItem bedsideSubmenu[] = {
  {"ON/OFF",      true, nullptr, 0, ACTION_BEDSIDE},
};
// Підменю — обігрівач
MenuItem lampkSubmenu[] = {
  {"ON/OFF",      true, nullptr, 0},
};
// Підменю — камера
MenuItem cameraSubmenu[] = {
  {"Take Photo",   true, nullptr, 0},
  {"Stream Video", true, nullptr, 0},
};

// Головне меню
MenuItem mainMenuItems[] = {
  {"bedside",   false, bedsideSubmenu,   1},
  {"lampk", false, lampkSubmenu, 1},
  {"Camera", false, cameraSubmenu, 2},
};
int mainMenuCount = 3;

// «Поточне» меню, по якому ми ходимо
MenuItem* currentMenu     = mainMenuItems;
int       currentMenuSize = mainMenuCount;
int       selectedIndex   = 0;

// Стек, щоб вертатися назад
struct MenuState {
  MenuItem* menu;
  int menuSize;
  int selected;
};
std::vector<MenuState> menuStack;

//-------------------------------------------------------------
// Малюємо меню (стан OS3) на mainScreenSprite
//-------------------------------------------------------------
void drawMenu() {
  mainScreenSprite.fillScreen(BLACK);

  mainScreenSprite.setFont(&fonts::Font4);
  mainScreenSprite.setTextSize(1);
  mainScreenSprite.setTextColor(WHITE);
  mainScreenSprite.setTextDatum(textdatum_t::top_left);

  // Заголовок
  mainScreenSprite.drawString("Menu", 10, 5);

  int startY = 30;
  for (int i = 0; i < currentMenuSize; i++) {
    int y = startY + i * 20;
    if (i == selectedIndex) {
      // Виділяємо пункт
      mainScreenSprite.fillRect(0, y, mainScreenSprite.width(), 20, BLUE);
      mainScreenSprite.setTextColor(BLACK);
    } else {
      mainScreenSprite.setTextColor(WHITE);
    }
    mainScreenSprite.drawString(currentMenu[i].title, 10, y);
  }
  mainScreenSprite.pushSprite(0, 0);
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
// ГОЛОВНА ФУНКЦІЯ МАЛЮВАННЯ ЕКРАНІВ (CORE SCREEN)
//-------------------------------------------------------------
void coreScreen() {
  switch (screen) {
    //--- ГОЛОВНИЙ ЕКРАН (OS0) ---
    case OS0: {
      mainScreenSprite.fillScreen(BLACK);

      int voltageMilliVolt = M5Cardputer.Power.getBatteryVoltage();
      float voltage        = voltageMilliVolt / 100.0;  // 407 -> 4.07

      char voltageText[10];
      sprintf(voltageText, "%.2fV", voltage);

      mainScreenSprite.setFont(&fonts::Font4);
      mainScreenSprite.setTextSize(1);
      mainScreenSprite.setTextColor(WHITE);
      mainScreenSprite.setTextDatum(top_right);

      int screenWidth = mainScreenSprite.width();
      mainScreenSprite.drawString(voltageText, screenWidth - 4, 2);

      mainScreenSprite.pushSprite(0, 0);
      }
      break;

    //--- ЕКРАН ВВЕДЕННЯ ТЕКСТУ (OS1) ---
    case OS1: {
      inputSprite.fillSprite(0x404040);
      inputSprite.setFont(&fonts::Font4);
      inputSprite.setTextSize(1);
      inputSprite.setTextColor(WHITE);
      inputSprite.setCursor(0, 0);

      // Надрукуємо поточне inputData
      inputSprite.print(inputData);
      inputSprite.pushSprite(0, 135 - 28);
      }
      break;

    //--- ЕКРАН "ПІЛОТ ПРОЕКТОРА" (OS2) ---
    case OS2: {
      pilotModeSprite.fillScreen(BLACK);
      pilotModeSprite.setFont(&fonts::Font4);
      pilotModeSprite.setTextColor(GREEN);
      pilotModeSprite.setTextDatum(middle_center);
      pilotModeSprite.setTextSize(1);

      pilotModeSprite.drawString("ProjectorPilot Mode", 120, 12);
      pilotModeSprite.drawFastHLine(0, 20, pilotModeSprite.width(), RED);

      pilotModeSprite.drawString("P-power",       50, 50);
      pilotModeSprite.drawString("`-exit",       140, 50);
      pilotModeSprite.drawString("ok",           210, 50);
      pilotModeSprite.drawString("space-sourse", 160, 75);

      // стрілки
      pilotModeSprite.drawTriangle(60, 70, 55, 80, 65, 80, GREEN);
      pilotModeSprite.fillTriangle(60, 70, 55, 80, 65, 80, GREEN);
      pilotModeSprite.drawTriangle(60, 110, 55, 100, 65, 100, GREEN);
      pilotModeSprite.fillTriangle(60, 110, 55, 100, 65, 100, GREEN);
      pilotModeSprite.drawTriangle(40, 90, 50, 85, 50, 95, GREEN);
      pilotModeSprite.fillTriangle(40, 90, 50, 85, 50, 95, GREEN);
      pilotModeSprite.drawTriangle(80, 90, 70, 85, 70, 95, GREEN);
      pilotModeSprite.fillTriangle(80, 90, 70, 85, 70, 95, GREEN);

      pilotModeSprite.pushSprite(0, 0);
      }
      break;

    //--- МЕНЮ (OS3) ---
    case OS3: {
      // Замість окремої змінної isMenuActive — просто виклик drawMenu().
      drawMenu();
      }
      break;

    default:
      break;
  }
}

//-------------------------------------------------------------
// ОБРОБКА КЛАВІАТУРИ (ЗАЛЕЖНО ВІД screen)
//-------------------------------------------------------------

// Логіка дій у меню (OS3)
void handleMenuInput() {
  if (!M5Cardputer.Keyboard.isChange()) return;
  if (!M5Cardputer.Keyboard.isPressed()) return;

  Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();
  std::string pressedKey(state.word.begin(), state.word.end());

  for (char x : state.word) {
    // Вихід у OS0 — бектік
    if (x == '`') {
      // Повністю виходимо з меню
      menuStack.clear();
      screen = OS0;
      return;
    }
    // Повернутись назад — ','
    if (x == ',') {
      if (!menuStack.empty()) {
        MenuState st = menuStack.back();
        menuStack.pop_back();

        currentMenu     = st.menu;
        currentMenuSize = st.menuSize;
        selectedIndex   = st.selected;
      } else {
        // Якщо нема куди вертатись — виходимо на OS0
        screen = OS0;
      }
      return;
    }
    // Вгору (;)
    if (x == ';') {
      selectedIndex--;
      if (selectedIndex < 0) selectedIndex = currentMenuSize - 1;
      drawMenu(); 
      return;
    }
    // Вниз (.)
    if (x == '.') {
      selectedIndex++;
      if (selectedIndex >= currentMenuSize) selectedIndex = 0;
      drawMenu();
      return;
    }
    // Вибір пункту (вліво)
    if (x == '/') {
      MenuItem& item = currentMenu[selectedIndex];
      if (item.isAction) {
        // Виконуємо дію
        performAction(item.actionID);
        // Лишаємося в цьому ж меню
        drawMenu();
      } else {
        // Перехід у підменю
        menuStack.push_back({currentMenu, currentMenuSize, selectedIndex});
        currentMenu     = item.submenu;
        currentMenuSize = item.submenuCount;
        selectedIndex   = 0;

        drawMenu();
      }
      return;
    }
  }
}

// Звичайна логіка (OS0, OS1, OS2) — handleNormalInput
void handleNormalInput() {
  if (!M5Cardputer.Keyboard.isChange()) return;
  if (!M5Cardputer.Keyboard.isPressed()) return;

  Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();
  std::string pressedKey(state.word.begin(), state.word.end());

  for (char x : state.word) {
    switch (x) {
      case '`': // вихід на OS0
        projectorPilotMode = false;
        isInputMode        = false;
        inputData          = "";
        inputSprite.fillSprite(BLACK);
        inputSprite.pushSprite(0, 135 - 28);

        screen = OS0;
        return;

      case 's':
        if (state.opt) {
          // OPT+s => Введення (OS1)
          isInputMode        = true;
          projectorPilotMode = false;
          inputData          = "> ";

          screen = OS1;
          return;
        }
        inputData += x;
        break;

      case 'p':
        if (state.opt) {
          // OPT+p => Пілот (OS2)
          isInputMode        = false;
          projectorPilotMode = true;

          screen = OS2;
          return;
        }
        inputData += x;
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

      case ',':
        if (projectorPilotMode) {
          IrSender.sendNECRaw(0xF708FC03, 0);
          sendIRCommand();
          return;
        }
        break;

      case '/':
        if (projectorPilotMode) {
          IrSender.sendNECRaw(0xF50AFC03, 0);
          sendIRCommand();
          return;
        }
        break;

      case ' ':
        if (projectorPilotMode) {
          IrSender.sendNECRaw(0xE01FFC03, 0);
          sendIRCommand();
          return;
        }
        break;

      default:
        // Усі інші символи
        inputData += x;
        break;
    }
  }

  // DEL
  if (state.del && inputData.length() > 2) {
    inputData.remove(inputData.length() - 1);
  }

  // Enter (якщо ми в OS1)
  if (state.enter && isInputMode) {
    isInputMode = false;
    String savedText = inputData.substring(2);
    mesh.sendBroadcast(savedText);

    inputSprite.fillSprite(BLACK);
    inputSprite.pushSprite(0, 135 - 28);
  }
  // Enter (якщо projectorPilotMode)
  if (state.enter && projectorPilotMode) {
    IrSender.sendNECRaw(0xF40BFC03, 0);
    sendIRCommand();
  }

  // *** Виклик меню (OPT + M) => екран OS3
  if (state.opt && pressedKey == "m") {
    screen = OS3;
    // Ініціалізуємо меню "з нуля"
    currentMenu     = mainMenuItems;
    currentMenuSize = mainMenuCount;
    selectedIndex   = 0;
    menuStack.clear();
    return;
  }

  if (!isInputMode) {
    inputData = "";
  }
}

// handleInput() — дивимось, який screen
void handleInput() {
  if (screen == OS3) {
    // Входимо в логіку меню
    handleMenuInput();
  } else {
    // Стандартна логіка
    handleNormalInput();
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

  // Початковий екран
  screen = OS0;
}

//-------------------------------------------------------------
// LOOP
//-------------------------------------------------------------
void loop() {
  // Намалюємо поточний екран
  coreScreen();

  // Оновлення mesh
  mesh.update();

  // Оновлення M5Cardputer
  M5Cardputer.update();

  // Обробка клавіш
  handleInput();
}
