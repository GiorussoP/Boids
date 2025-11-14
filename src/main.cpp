#include "Scene.hpp"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  Scene scene;
  bool success = scene.Initialize();
  if (success) {
    scene.RunLoop();
  }
  scene.Shutdown();

  return 0;
}
