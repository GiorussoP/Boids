#ifndef SCENE_HPP
#define SCENE_HPP
#include "Math.hpp"
#include <SDL2/SDL.h>
#include <unordered_set>
#include <vector>

class Scene {
public:
  Scene();

  bool Initialize();
  void RunLoop();
  void Shutdown();
  void Quit() { mIsRunning = false; }

  // Actor functions
  void InitializeActors();
  void UpdateActors(float deltaTime);

  void AddActor(class Actor *actor);
  void RemoveActor(class Actor *actor);

  // Renderer getter
  class Renderer *GetRenderer() { return mRenderer; }

  // Camera functions
  Vector3 &GetCameraPos() { return mCameraPos; }
  void SetCameraPos(Vector3 position);

  Vector3 &GetCameraForward() { return mCameraForward; }
  void SetCameraForward(const Vector3 forward) { mCameraForward = forward; }

  Vector3 GetCameraUp() const { return mCameraUp; }
  void SetCameraUp(const Vector3 up) { mCameraUp = up; }

  Quaternion GetCameraRotation() const {
    return Math::LookRotation(mCameraForward, mCameraUp);
  }

  void SetCameraRotation(const Quaternion rotation) {
    mCameraForward = Vector3::Transform(Vector3::UnitZ, rotation);
    mCameraUp = Vector3::Transform(Vector3::UnitY, rotation);
  }

private:
  void ProcessInput();
  void UpdateScene(float deltaTime);
  void GenerateOutput();
  void CheckCollisions();

  // Track if we're updating actors right now
  bool mUpdatingActors;

  // All the actors in the scene
  std::vector<class Actor *> mActors;
  std::vector<class Actor *> mPendingActors;

  // SDL window
  SDL_Window *mWindow;

  // OpenGL context
  SDL_GLContext mGLContext;

  // Renderer
  class Renderer *mRenderer;

  // Camera state
  Vector3 mCameraPos;
  Vector3 mCameraForward;
  Vector3 mCameraUp;

  // Scene state
  Uint32 mTicksCount;
  bool mIsRunning;
  bool mIsDebugging;
  bool mIsPaused;

  // Camera controller
  class CameraControllerActor *mCameraController;
};

#endif