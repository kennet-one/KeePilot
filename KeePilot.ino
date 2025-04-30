#include "M5Cardputer.h"
#include "M5GFX.h"
#include "painlessMesh.h"
#include "mash_parameter.h"
#include <IRremote.hpp>

/* ---------- MESH ---------- */
Scheduler		userScheduler;
painlessMesh	mesh;

/* ---------- INPUT --------- */
String			inputData = "";
bool			isInputMode = false;

/* ---------- IR ------------ */
#define IR_TX_PIN 44
bool			projectorPilotMode = false;

/* ---------- SCROLL -------- */
const int		LINE_HEIGHT		= 20;
const int		VIEWPORT_TOP_Y	= 30;
const int		HEADER_HEIGHT	= 20;
const uint16_t	SCROLL_INTERVAL	= 8;
int				maxVisibleItems;
int				firstVisibleIdx	= 0;
bool			needScrollAnim	= false;
int8_t			scrollDir		= 0;
int				scrollPxDone	= 0;
uint32_t		lastScrollTick	= 0;

/* ---------- SPRITES ------- */
LGFX_Sprite		headerSprite(&M5Cardputer.Display);
LGFX_Sprite		menuSprite(&M5Cardputer.Display);
LGFX_Sprite		pilotSprite(&M5Cardputer.Display);
LGFX_Sprite		indicatorSprite(&M5Cardputer.Display);
LGFX_Sprite		inputSprite(&M5Cardputer.Display);

/* ---------- FLAGS --------- */
bool			menuDirty		= true;

/* ---------- STATES -------- */
int	garlandState = -1;
int	bdsdState	 = -1;
int	lamState	 = -1;

/* ---------- ENUMS --------- */
enum CORESCREEN { OS0, OS1, OS2, OS3, OS4 } screen = OS0;

enum ActionID {
	ACTION_NONE = 0,
	ACTION_BEDSIDE,
	ACTION_LAMPK,
	ACTION_GARLAND,
	ACTION_HUMI
};

struct MenuItem {
	const char*	title;
	bool		isAction;
	MenuItem*	submenu;
	int			submenuCount;
	ActionID	actionID;
	uint32_t	nodeId;
};

/* ---------- MENU DATA ----- */
MenuItem bedsideSubmenu[] = { { "ON/OFF", true, nullptr, 0, ACTION_BEDSIDE } };
MenuItem lampkSubmenu[]   = { { "ON/OFF", true, nullptr, 0, ACTION_LAMPK  } };
MenuItem garlandSubmenu[] = { { "ON/OFF", true, nullptr, 0, ACTION_GARLAND } };
MenuItem humidifierSubmenu[] = {
	{ "ON/OFF", true, nullptr, 0, ACTION_HUMI },
	{ "PUMP", true, nullptr, 0, ACTION_HUMI },
	{ "FLOW", true, nullptr, 0, ACTION_HUMI },
	{ "IONIC", true, nullptr, 0, ACTION_HUMI },
	{ "LIGHT", true, nullptr, 0, ACTION_HUMI },
	{ "LIGHT COLOR", true, nullptr, 0, ACTION_HUMI },
};

MenuItem mainMenuItems[] = {
	{ "bedside",    false, bedsideSubmenu,	 1, ACTION_NONE, 635035530 },
	{ "lampk",      false, lampkSubmenu,	 1, ACTION_NONE, 434115122 },
	{ "garland",    false, garlandSubmenu,  1, ACTION_NONE, 2224853816 },
	{ "humidifier", false, humidifierSubmenu, 6, ACTION_NONE, 2661345693 }
};
int				mainMenuCount	= 4;
MenuItem*		currentMenu		= mainMenuItems;
int				currentMenuSize = mainMenuCount;
int				selectedIndex	= 0;

/* ---------- STACK --------- */
struct MenuState { MenuItem* menu; int menuSize; int selected; };
std::vector<MenuState> menuStack;

/* ---------- HELPERS ------- */
bool isNodeOnline(uint32_t id) {
	auto lst = mesh.getNodeList();
	return std::find(lst.begin(), lst.end(), id) != lst.end();
}

void performAction(ActionID id) {
	switch (id) {
		case ACTION_BEDSIDE:	mesh.sendSingle(635035530, "bedside");		 break;
		case ACTION_LAMPK:		mesh.sendSingle(434115122, "lam");			 break;
		case ACTION_GARLAND:	mesh.sendSingle(2224853816, "garland");		 break;
		case ACTION_HUMI:		mesh.sendSingle(2224853816, "echo_turb");	 break;
		default: break;
	}
}

/* ---------- INDICATOR IR -- */
void sendIRCommand(int x, int y) {
	indicatorSprite.fillScreen(BLACK);
	indicatorSprite.fillCircle(8, 8, 7, GREEN);
	indicatorSprite.pushSprite(x, y);
	delay(300);
	indicatorSprite.fillScreen(BLACK);
	indicatorSprite.fillCircle(8, 8, 7, YELLOW);
	indicatorSprite.pushSprite(x, y);
	delay(300);
}

/* ---------- ANIMATION ----- */
void animateScroll() {
	if (!needScrollAnim) return;
	if (millis() - lastScrollTick < SCROLL_INTERVAL) return;
	lastScrollTick = millis();

	scrollPxDone += scrollDir;
	menuSprite.pushSprite(0, VIEWPORT_TOP_Y - scrollPxDone);

	if (scrollDir > 0)
		M5Cardputer.Display.fillRect(
			0,
			VIEWPORT_TOP_Y + menuSprite.height() - scrollDir,
			menuSprite.width(), scrollDir, BLACK);
	else
		M5Cardputer.Display.fillRect(
			0,
			VIEWPORT_TOP_Y - scrollDir,
			menuSprite.width(), -scrollDir, BLACK);

	if (abs(scrollPxDone) >= LINE_HEIGHT) {
		needScrollAnim = false;
		scrollPxDone   = 0;
		menuDirty      = true;
	}
}

/* ---------- CORE SCREEN --- */
void coreScreen() {
	switch (screen) {
		case OS0: {
			headerSprite.fillScreen(BLACK);
			int v = M5Cardputer.Power.getBatteryVoltage();
			char t[10]; sprintf(t, "%.2fV", v / 1000.0);
			headerSprite.setFont(&fonts::Font4);
			headerSprite.setTextSize(1);
			headerSprite.setTextColor(WHITE);
			headerSprite.setTextDatum(top_right);
			headerSprite.drawString(t, headerSprite.width() - 4, 0);
			headerSprite.pushSprite(0, 0);
			break;
		}

		case OS3: {
			/* --- header --- */
			headerSprite.fillScreen(BLACK);
			headerSprite.setFont(&fonts::Font4);
			headerSprite.setTextSize(1);
			headerSprite.setTextColor(GREEN);
			headerSprite.setTextDatum(textdatum_t::top_left);

			String title = "Menu";
			if (!menuStack.empty()) {
				auto& p = menuStack.back();
				title += " -> ";
				title += p.menu[p.selected].title;
			}
			headerSprite.drawString(title, 10, 0);
			headerSprite.setTextColor(WHITE);
			headerSprite.drawFastHLine(0, 18, headerSprite.width(), RED);
			headerSprite.pushSprite(0, 0);

			/* --- list --- */
			if (menuDirty) {
				menuSprite.fillScreen(BLACK);
				menuSprite.setFont(&fonts::Font4);
				menuSprite.setTextSize(1);
				menuSprite.setTextDatum(textdatum_t::top_left);

				for (int row = 0; row < maxVisibleItems; row++) {
					int idx = firstVisibleIdx + row;
					if (idx >= currentMenuSize) break;
					int y = row * LINE_HEIGHT;
					bool on = isNodeOnline(currentMenu[idx].nodeId);

					menuSprite.setTextColor(
						(idx == selectedIndex) ? BLACK : (on ? WHITE : DARKGREY));

					if (idx == selectedIndex)
						menuSprite.fillRect(0, y, menuSprite.width(), LINE_HEIGHT, BLUE);

					String s = currentMenu[idx].title;
					if (currentMenu == garlandSubmenu && idx == 0)
						s += garlandState == 1 ? " [ON]" :
							 garlandState == 0 ? " [OFF]" : " [???]";
					if (currentMenu == bedsideSubmenu && idx == 0)
						s += bdsdState == 1 ? " [ON]" :
							 bdsdState == 0 ? " [OFF]" : " [???]";
					if (currentMenu == lampkSubmenu && idx == 0)
						s += lamState == 1 ? " [ON]" :
							 lamState == 0 ? " [OFF]" : " [???]";

					menuSprite.drawString(s, 10, y);

					if (!currentMenu[idx].isAction) {
						int16_t w = menuSprite.textWidth(s);
						menuSprite.setTextSize(0.5);
						menuSprite.drawString(on ? " [online]" : " [offline]",
							20 + w, y + 5);
						menuSprite.setTextSize(1);
					}
				}
				menuDirty = false;
			}
			menuSprite.pushSprite(0, VIEWPORT_TOP_Y);
			break;
		}

		default: break;
	}
}

/* ---------- RX CALLBACK --- */
void receivedCallback(uint32_t, String& msg) {
	bool ch = false;
	if (msg == "garl0") { garlandState = 0; ch = true; }
	else if (msg == "garl1") { garlandState = 1; ch = true; }
	else if (msg == "bdsdl0") { bdsdState = 0; ch = true; }
	else if (msg == "bdsdl1") { bdsdState = 1; ch = true; }
	else if (msg == "La0") { lamState = 0; ch = true; }
	else if (msg == "La1") { lamState = 1; ch = true; }
	if (ch) menuDirty = true;
}

/* ---------- INPUT ---------- */
void handleInput() {
	if (!M5Cardputer.Keyboard.isChange()) return;
	if (!M5Cardputer.Keyboard.isPressed()) return;

	auto state = M5Cardputer.Keyboard.keysState();
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

      case 's': // OPT+s => Введення (OS1)
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

      case 'p': // OPT+p => Пілот (OS2)
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

      case 'm': // Виклик меню (OS3)
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
					selectedIndex = (selectedIndex - 1 + currentMenuSize) % currentMenuSize;
					if (!needScrollAnim) menuDirty = true;
					if (selectedIndex < firstVisibleIdx) {
						firstVisibleIdx--;
						scrollDir      = -1;
						needScrollAnim = true;
						scrollPxDone   = 0;
						lastScrollTick = millis();
          }
        }
        return;

        if (projectorPilotMode) {
          IrSender.sendNECRaw(0xF609FC03, 0);
          sendIRCommand(32, 105);
          return;
        }
        break;
      case '.': // Вниз
				if (screen == OS3) {
					selectedIndex = (selectedIndex + 1) % currentMenuSize;
					if (!needScrollAnim) menuDirty = true;
					if (selectedIndex > firstVisibleIdx + maxVisibleItems - 1) {
						firstVisibleIdx++;
						scrollDir      = +1;
						needScrollAnim = true;
						scrollPxDone   = 0;
						lastScrollTick = millis();
          return;
          }
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
						auto st = menuStack.back(); menuStack.pop_back();
						currentMenu     = st.menu;
						currentMenuSize = st.menuSize;
						selectedIndex   = st.selected;
						firstVisibleIdx = std::max(0, selectedIndex - maxVisibleItems + 1);
						menuDirty       = true;
          } else {
            screen = OS0;
          }
        }
        break;

      case '/': // вправо
        if (screen == OS3) {
          MenuItem& item = currentMenu[selectedIndex];
          if (item.isAction) {
            // Виконуємо дію
            performAction(item.actionID);
            sendIRCommand(220, 0);
            // Лишаємося в тому ж меню
          } else {
            // Переходимо в підменю
            if (&item == &mainMenuItems[2]) { 
              mesh.sendSingle(2224853816, "garland_echo");
              garlandState = -1; // поки що невідомо, чекаємо відповіді
            }
            if (&item == &mainMenuItems[0]) { 
              mesh.sendSingle(635035530, "bedside_echo");
              bdsdState = -1; 
            }
            if (&item == &mainMenuItems[1]) { 
              mesh.sendSingle(434115122, "lamech");
              bdsdState = -1; 
            }
            menuStack.push_back({currentMenu, currentMenuSize, selectedIndex});
						currentMenu     = item.submenu;
						currentMenuSize = item.submenuCount;
						selectedIndex   = 0;
						firstVisibleIdx = 0;
						menuDirty       = true;
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

      default: // Усі інші символи
        if (isInputMode) {
          inputData += x;
        }
        break;
    }
  }

  if ((state.del && inputData.length() > 2) && isInputMode) { // DEL
    inputData.remove(inputData.length() - 1);
  }

  if (state.enter && isInputMode) { // Enter (якщо ми в OS1)
    isInputMode = false;
    String savedText = inputData.substring(2);
    mesh.sendBroadcast(savedText);

    inputSprite.fillSprite(BLACK);
    inputSprite.pushSprite(0, 135 - 28);
  }
  
  if (state.enter && projectorPilotMode) { // Enter (якщо projectorPilotMode)
    IrSender.sendNECRaw(0xF40BFC03, 0);
    sendIRCommand(32, 105);
  }

  if (!isInputMode) {
    inputData = "";
  }
}

/* ---------- SETUP --------- */
void setup() {
	mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
	mesh.onReceive(&receivedCallback);

	auto cfg = M5.config();
	M5Cardputer.begin(cfg, true);
	M5Cardputer.Display.setRotation(1);

	int w = M5Cardputer.Display.width();
	int h = M5Cardputer.Display.height();
	headerSprite.createSprite(w, HEADER_HEIGHT);
	menuSprite.createSprite(w, h - VIEWPORT_TOP_Y);
	pilotSprite.createSprite(w, h);
	indicatorSprite.createSprite(16, 16);
	inputSprite.createSprite(240, 28);
	maxVisibleItems = (h - VIEWPORT_TOP_Y) / LINE_HEIGHT;

	IrSender.begin(DISABLE_LED_FEEDBACK);
	IrSender.setSendPin(IR_TX_PIN);
}

/* ---------- LOOP ---------- */
void loop() {
	if (!needScrollAnim) coreScreen();
	mesh.update();
	M5Cardputer.update();
	handleInput();
	animateScroll();
}
