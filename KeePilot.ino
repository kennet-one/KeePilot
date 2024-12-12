#include "M5Cardputer.h"
#include "M5GFX.h"
#include "painlessMesh.h"
#include "mash_parameter.h"

Scheduler userScheduler; 
painlessMesh  mesh;

// Спрайт для тексту
M5Canvas canvas(&M5Cardputer.Display);

// Змінні
String inputData = "";
bool isInputMode = false;  // Чи активний режим введення

void setup() {
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );

  M5Cardputer.begin();

}

void loop() {

  mesh.update();

  M5Cardputer.update();  // Оновлюємо стан пристрою

  // Перевірка комбінації клавіш OPT+S
  if (M5Cardputer.Keyboard.isPressed()) {
    Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();
    if (state.opt && std::string(state.word.begin(), state.word.end()) == "s") {
      isInputMode = true;  // Увімкнути режим введення
      startInputMode();
    }
  }

    // Якщо активний режим введення
  if (isInputMode) {
    handleInput();
    
  }
}

void startInputMode() {
  isInputMode = true;  // Увімкнути режим введення
  inputData = "> ";    // Додаємо символ вводу

  canvas.createSprite(240, 28);  // Ширина: 240, Висота: 28
  canvas.fillSprite(0x404040);  // Темно-сірий фон
  canvas.setTextFont(&fonts::Font4);
  canvas.setTextSize(1);
  canvas.setTextColor(WHITE);
  canvas.setCursor(0, 0);
  canvas.print(inputData);
  canvas.pushSprite(0, 135 - 28);  // Відображаємо спрайт
}

void handleInput() {
  if (M5Cardputer.Keyboard.isChange()) {
    if (M5Cardputer.Keyboard.isPressed()) {
      Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();

      // Додаємо введений текст
      for (char c : state.word) {
        inputData += c;
      }

      // Видалення символів
      if (state.del && inputData.length() > 2) {
        inputData.remove(inputData.length() - 1);
      }

      // Завершення вводу (Enter)
      if (state.enter) {
        isInputMode = false;  // Вимикаємо режим вводу
        String savedText = inputData.substring(2);  // Зберігаємо текст без ">"
        inputData = "";  // Очищаємо змінну
        //Serial.println("Збережений текст: " + savedText);
        mesh.sendBroadcast(savedText);

        // Очищаємо спрайт
        canvas.fillSprite(BLACK);
        canvas.pushSprite(0, 135 - 28);  // Очищуємо область вводу
        }

        // Оновлюємо екран
      canvas.fillSprite(BLACK);
      canvas.setCursor(0, 0);
      canvas.print(inputData);
      canvas.pushSprite(0, 135 - 28);
    }
  }
}
