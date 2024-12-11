#include "painlessMesh.h"
#include "mash_parameter.h"
#include "M5Cardputer.h"
#include "M5GFX.h"

Scheduler userScheduler; 
painlessMesh  mesh;

M5Canvas canvas(&M5Cardputer.Display);

void setup() {

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  //mesh.onReceive(&receivedCallback);

  M5Cardputer.begin();
    
  // Створюємо спрайт
  canvas.createSprite(240, 135);  // Ширина: 128, Висота: 64
  canvas.fillSprite(BLACK);      // Заливаємо спрайт чорним

  // Малюємо текст і фігури на спрайті
  canvas.setTextColor(WHITE);
  canvas.setCursor(10, 10);
  canvas.println("Hello, Sprite!");
  canvas.drawRect(20, 20, 50, 30, RED);

  // Виводимо спрайт на екран
  canvas.pushSprite(0, 0);
}

void loop() {
  mesh.update();
}
