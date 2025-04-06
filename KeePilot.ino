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
  ACTION_GARLAND,
};

// Тепер кожен MenuItem має ще й ідентифікатор дії
struct MenuItem {
  const char* title;    
  bool isAction;
  MenuItem* submenu;
  int submenuCount;
  ActionID actionID;    // поле з enum
  uint32_t nodeId; // Додали ID ноди для перевірки online-статусу
};

bool isNodeOnline(uint32_t nodeId) {
  auto nodes = mesh.getNodeList();
  return std::find(nodes.begin(), nodes.end(), nodeId) != nodes.end();
};

void performAction(ActionID id) {
  switch (id) {
    case ACTION_BEDSIDE:
      mesh.sendSingle(635035530,"bedside");
      break;

    case ACTION_LAMPK:
      mesh.sendSingle(434115122,"lam");
      break;

    case ACTION_GARLAND:
      mesh.sendSingle(2224853816,"garland");
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
  }
}

MenuItem bedsideSubmenu[] = {
  {"ON/OFF",      true, nullptr, 0, ACTION_BEDSIDE},
};

MenuItem lampkSubmenu[] = {
  {"ON/OFF",      true, nullptr, 0, ACTION_LAMPK},
};

MenuItem garlandSubmenu[] = {
  {"ON/OFF",   true, nullptr, 0, ACTION_GARLAND},
};

// Головне меню
MenuItem mainMenuItems[] = {
  {"bedside", false, bedsideSubmenu, 1, ACTION_NONE, 635035530},
  {"lampk",   false, lampkSubmenu,   1, ACTION_NONE, 434115122},
  {"garland", false, garlandSubmenu, 1, ACTION_NONE, 2224853816},
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
// Анімація індикатора передавання
//-------------------------------------------------------------
void sendIRCommand(int x,int y) {
  indicatorSprite.fillScreen(BLACK);
  indicatorSprite.fillCircle(8, 8, 7, GREEN);
  indicatorSprite.pushSprite(x, y);
  delay(500);

  indicatorSprite.fillScreen(BLACK);
  indicatorSprite.fillCircle(8, 8, 7, YELLOW);
  indicatorSprite.pushSprite(x, y);
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
      float voltage        = voltageMilliVolt / 1000.0;

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
      inputSprite.fillScreen(BLACK);
      inputSprite.pushSprite(0, 0);

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

      mainScreenSprite.fillScreen(BLACK);

      mainScreenSprite.setFont(&fonts::Font4);
      mainScreenSprite.setTextSize(1);
      mainScreenSprite.setTextColor(GREEN);
      mainScreenSprite.setTextDatum(textdatum_t::top_left);

      String menuTitle = "Menu";
      
      if (!menuStack.empty()) {// Якщо стек не порожній, значить ми в підменю
        MenuState &parentState = menuStack.back();
        MenuItem  &parentItem  = parentState.menu[parentState.selected];
        
        menuTitle += " -> ";
        menuTitle += parentItem.title;
      }

      mainScreenSprite.drawString(menuTitle, 10, 0);// Малюємо зібраний заголовок

      mainScreenSprite.setTextColor(WHITE);// Далі малюємо риску, а потім - пункти меню:
      mainScreenSprite.drawFastHLine(0, 20, mainScreenSprite.width(), RED);

      int startY = 30;
      for (int i = 0; i < currentMenuSize; i++) {
        int y = startY + i * 20;

        bool online = isNodeOnline(currentMenu[i].nodeId);

        if (i == selectedIndex) {
          // Виділяємо пункт
          mainScreenSprite.fillRect(0, y, mainScreenSprite.width(), 20, BLUE);
          mainScreenSprite.setTextColor(BLACK);
        } else {
          mainScreenSprite.setTextColor(online ? WHITE : DARKGREY);
        }
        mainScreenSprite.setTextSize(1);
        String titleText = String(currentMenu[i].title);
        mainScreenSprite.drawString(titleText, 10, y);

        int16_t mainTextWidth = mainScreenSprite.textWidth(titleText);

        if (!currentMenu[i].isAction) { // якшо не головне меню то убрати надпис
          mainScreenSprite.setTextSize(0.5);

          String statusPart = online ? " [online]" : " [offline]";
          mainScreenSprite.drawString(statusPart, 20 + mainTextWidth, y + 5);
          mainScreenSprite.setTextSize(1);
        }
      }
      mainScreenSprite.pushSprite(0, 0);
      }
      break;

    default:
      break;
  }
}

//-------------------------------------------------------------
// ОБРОБКА КЛАВІАТУРИ 
//-------------------------------------------------------------
void handleInput() {
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
        menuStack.clear();
        inputSprite.fillSprite(BLACK);
        inputSprite.pushSprite(0, 135 - 28);

        screen = OS0;
        return;

      case 's':// OPT+s => Введення (OS1)
        if (state.opt) {
          isInputMode        = true;
          projectorPilotMode = false;
          inputData          = "> ";

          screen = OS1;
          return;
        }
        if (isInputMode) {
          inputData += x;
        }
        break;

      case 'p':// OPT+p => Пілот (OS2)
        if (state.opt) {
          isInputMode        = false;
          projectorPilotMode = true;

          screen = OS2;
          return;
        }
        if (projectorPilotMode) { 
          IrSender.sendNECRaw(0xE21DFC03, 0);
          sendIRCommand(32, 105);
          break;
        }
        if (isInputMode) {
          inputData += x;
        }
        break;

      case 'm':// Виклик меню екран OS3
        if (state.opt) {
          screen = OS3;

          currentMenu     = mainMenuItems;
          currentMenuSize = mainMenuCount;
          selectedIndex   = 0;
          menuStack.clear();
          break;
        }
        if (isInputMode) {
          inputData += x;
        }
        break;

      case ';': // Вверх
        if (screen == OS3) {
          selectedIndex--;
          if (selectedIndex < 0) selectedIndex = currentMenuSize - 1;

          return;
        }
        if (projectorPilotMode) {
          IrSender.sendNECRaw(0xF609FC03, 0);
          sendIRCommand(32, 105);
          return;
        }
        break;

      case '.': // Вниз
        if (screen == OS3) {
          selectedIndex++;
          if (selectedIndex >= currentMenuSize) selectedIndex = 0;

          return;
        }
        if (projectorPilotMode) {
          IrSender.sendNECRaw(0xFE01FC03, 0);
          sendIRCommand(32, 105);
          return;
        }
        break;

      case ',': // вліво
        if (projectorPilotMode) {
          IrSender.sendNECRaw(0xF708FC03, 0);
          sendIRCommand(32, 105);
          return;
        }
        if (screen == OS3) {
          if (!menuStack.empty()) {
            MenuState st = menuStack.back();
            menuStack.pop_back();

            currentMenu     = st.menu;
            currentMenuSize = st.menuSize;
            selectedIndex   = st.selected;
          } else {
            screen = OS0;
          }
        }
        break;

      case '/': // вправо
        if (screen == OS3) {
          MenuItem& item = currentMenu[selectedIndex];
        if (item.isAction) {// Виконуємо дію
          performAction(item.actionID);
          sendIRCommand(220, 0);
        // Лишаємося в цьому ж меню
        } else {// Перехід у підменю
          menuStack.push_back({currentMenu, currentMenuSize, selectedIndex});
          currentMenu     = item.submenu;
          currentMenuSize = item.submenuCount;
          selectedIndex   = 0;
        }
        return;
      }
        if (projectorPilotMode) {
          IrSender.sendNECRaw(0xF50AFC03, 0);
          sendIRCommand(32, 105);
          return;
        }
        break;

      case ' ':
        if (projectorPilotMode) {
          IrSender.sendNECRaw(0xE01FFC03, 0);
          sendIRCommand(32, 105);
          return;
        }
        break;

      default:// Усі інші символи
        if (isInputMode) {
          inputData += x;
        }
        break;
    }
  }

  if ((state.del && inputData.length() > 2) && ( isInputMode )) {// DEL
    inputData.remove(inputData.length() - 1);
  }

  if (state.enter && isInputMode) {// Enter (якщо ми в OS1)
    isInputMode = false;
    String savedText = inputData.substring(2);
    mesh.sendBroadcast(savedText);

    inputSprite.fillSprite(BLACK);
    inputSprite.pushSprite(0, 135 - 28);
  }
  
  if (state.enter && projectorPilotMode) {// Enter (якщо projectorPilotMode)
    IrSender.sendNECRaw(0xF40BFC03, 0);
    sendIRCommand(32, 105);
  }

  if (!isInputMode) {
    inputData = "";
  }
}

//-------------------------------------------------------------
void setup() {
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);// Ініціалізація mesh

  auto cfg = M5.config();// Ініціалізація M5Cardputer
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setTextDatum(middle_center);
  M5Cardputer.Display.setFont(&fonts::Orbitron_Light_24);
  M5Cardputer.Display.setTextSize(1);

  IrSender.begin(DISABLE_LED_FEEDBACK);// Ініціалізація IR
  IrSender.setSendPin(IR_TX_PIN);


  int w = M5Cardputer.Display.width();  // Створення спрайтів
  int h = M5Cardputer.Display.height(); //
  mainScreenSprite.createSprite(w, h);  //
  pilotModeSprite.createSprite(w, h);   //
  indicatorSprite.createSprite(16, 16); //
  inputSprite.createSprite(240, 28);    //

  screen = OS0;// Початковий екран
}

void loop() {
  coreScreen();

  mesh.update();

  M5Cardputer.update();

  handleInput();
}
