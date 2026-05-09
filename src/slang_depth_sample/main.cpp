#include <Windows.h>
#include "app.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
  yab::App app;
  return app.Run();
}
