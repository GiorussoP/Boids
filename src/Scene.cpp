#include "Scene.hpp"

#include "Input.hpp"
#include "actors/BirdActor.hpp"
#include "actors/CameraControllerActor.hpp"
#include "actors/GroundActor.hpp"
#include "actors/TowerActor.hpp"
#include "render/Mesh.hpp"
#include "render/Renderer.hpp"

#include <GL/glew.h>
#include <algorithm>
#include <iostream>
#include <unordered_set>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int FPS = 60;
const int FRAME_TIME = 1000 / FPS;
const float MIDI_UPDATE_INTERVAL = 0.001f; // Update MIDI every 1ms

Scene::Scene()
    : mUpdatingActors(false), mWindow(nullptr), mGLContext(nullptr),
      mRenderer(nullptr), mCameraPos(Vector3(0.0f, 10.0f, 0.0f)),
      mCameraForward(0.0f, -1.0f, 0.0f), mCameraUp(0.0f, 1.0f, 0.0f),
      mCameraMode(CameraMode::Free), mFlockCenter(Vector3::Zero),
      mFlockVelocity(Vector3::Zero), mTicksCount(0), mIsRunning(true),
      mIsDebugging(false), mCameraController(nullptr), mLeader(nullptr) {}

bool Scene::Initialize() {
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
    return false;
  }

  // Set OpenGL attributes
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  // Create window
  mWindow = SDL_CreateWindow(
      "TP Final - Mellodica", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      WINDOW_WIDTH, WINDOW_HEIGHT,
      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  if (!mWindow) {
    std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
    return false;
  }

  // Create OpenGL context
  mGLContext = SDL_GL_CreateContext(mWindow);
  if (!mGLContext) {
    std::cerr << "Failed to create OpenGL context: " << SDL_GetError()
              << std::endl;
    return false;
  }

  // Initialize GLEW
  glewExperimental = GL_TRUE;
  GLenum glewError = glewInit();
  if (glewError != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(glewError)
              << std::endl;
    return false;
  }

  // Enable backface culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  // Enable VSync
  SDL_GL_SetSwapInterval(1);

  // Create renderer
  mRenderer = new Renderer();
  if (!mRenderer->Initialize(static_cast<float>(WINDOW_WIDTH),
                             static_cast<float>(WINDOW_HEIGHT))) {
    std::cerr << "Failed to initialize renderer" << std::endl;
    return false;
  }

  // Set initial projection matrix
  float fov = Math::ToRadians(90.0f);
  Matrix4 perspectiveProjection = Matrix4::CreatePerspectiveFOV(
      fov, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT),
      0.1f, 1000.0f);
  mRenderer->SetProjectionMatrix(perspectiveProjection);

  mTicksCount = SDL_GetTicks();

  InitializeActors();

  return true;
}

void Scene::InitializeActors() {
  new GroundActor(this);
  mCameraController = new CameraControllerActor(this);

  new TowerActor(this);

  mLeader = new BirdActor(this, BirdType::Leader);
  mLeader->SetPosition(Vector3(20.0f, 50.0f, 0.0f));
  mLeader->GetComponent<RigidBodyComponent>()->SetVelocity(
      Vector3(0.0f, 0.0f, 20.0f));

  for (int i = 0; i < 10; i++) {
    BirdActor *bird = new BirdActor(this);
    bird->SetPosition(
        Vector3((std::rand() % 20 - 10), 20, (std::rand() % 20) - 10));
    bird->GetComponent<RigidBodyComponent>()->SetVelocity(
        Vector3((std::rand() % 41 - 20), (std::rand() % 41 - 20),
                (std::rand() % 41 - 20)));
    mBirds.emplace_back(bird);
  }
}
void Scene::SetCameraPos(Vector3 position) { mCameraPos = position; }

void Scene::RunLoop() {
  while (mIsRunning) {
    // Calculate actual elapsed time since last frame
    Uint32 currentTicks = SDL_GetTicks();
    float deltaTime = (currentTicks - mTicksCount) / 1000.0f;
    mTicksCount = currentTicks;

    // Cap deltaTime to prevent huge jumps (max 0.25 seconds)
    if (deltaTime > 0.05f) {
      deltaTime = 0.05f;
    }

    // Update scene with deltaTime
    ProcessInput();
    UpdateScene(deltaTime);

    GenerateOutput();

    // Frame rate limiting - delay to maintain 60 FPS
    Uint32 frameTime = SDL_GetTicks() - currentTicks;
    if (frameTime < FRAME_TIME) {
      SDL_Delay(FRAME_TIME - frameTime);
    }
  }
}

void Scene::Shutdown() {
  std::cout << "Shutdown: Starting cleanup..." << std::endl;

  // Close the window
  if (mWindow) {
    std::cout << "Shutdown: Closing window..." << std::endl;
    SDL_DestroyWindow(mWindow);
    mWindow = nullptr;
    std::cout << "Shutdown: Window closed" << std::endl;
  }

  mBirds.clear();

  // Delete all actors
  std::cout << "Shutdown: Deleting " << mActors.size() << " actors..."
            << std::endl;
  while (!mActors.empty()) {
    delete mActors.back();
  }
  mActors.clear();
  std::cout << "Shutdown: All actors deleted" << std::endl;

  // Shutdown renderer
  std::cout << "Shutdown: Shutting down renderer..." << std::endl;
  mRenderer->Shutdown();
  delete mRenderer;
  mRenderer = nullptr;
  std::cout << "Shutdown: Renderer deleted" << std::endl;

  if (mGLContext) {
    std::cout << "Shutdown: Deleting GL context..." << std::endl;
    SDL_GL_DeleteContext(mGLContext);
    mGLContext = nullptr;
  }

  std::cout << "Shutdown: Quitting SDL..." << std::endl;
  SDL_Quit();
  std::cout << "Shutdown: Complete!" << std::endl;
}

void Scene::AddActor(Actor *actor) {
  if (mUpdatingActors) {
    mPendingActors.emplace_back(actor);
  } else {
    mActors.emplace_back(actor);
  }
}

void Scene::RemoveActor(Actor *actor) {
  // Check pending actors
  auto iter = std::find(mPendingActors.begin(), mPendingActors.end(), actor);
  if (iter != mPendingActors.end()) {
    // Swap to end and pop
    std::iter_swap(iter, mPendingActors.end() - 1);
    mPendingActors.pop_back();
  }

  // Check active actors
  iter = std::find(mActors.begin(), mActors.end(), actor);
  if (iter != mActors.end()) {
    std::iter_swap(iter, mActors.end() - 1);
    mActors.pop_back();
  }
}

void Scene::ProcessInput() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      mIsRunning = false;
      break;
    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
        // Handle window resize
        glViewport(0, 0, event.window.data1, event.window.data2);

        float fov = Math::ToRadians(90.0f);
        Matrix4 perspectiveProjection = Matrix4::CreatePerspectiveFOV(
            fov, static_cast<float>(event.window.data1),
            static_cast<float>(event.window.data2), 0.1f, 1000.0f);
        mRenderer->SetProjectionMatrix(perspectiveProjection);
      }
      break;
    }
  }

  Input::Update();

  if (Input::WasKeyPressed(SDL_SCANCODE_C)) {
    // Cycle through camera modes
    mCameraMode =
        static_cast<CameraMode>((static_cast<int>(mCameraMode) + 1) % 4);
  }

  if (Input::WasKeyPressed(SDL_SCANCODE_ESCAPE)) {
    mIsRunning = false;
  }

  if (Input::WasKeyPressed(SDL_SCANCODE_F11)) {
    // Toggle fullscreen (use desktop fullscreen for best compatibility)
    if (mWindow) {
      Uint32 flags = SDL_GetWindowFlags(mWindow);
      if (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) {
        SDL_SetWindowFullscreen(mWindow, 0); // windowed
      } else {
        SDL_SetWindowFullscreen(mWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
      }
    }
  }

  if (Input::WasKeyPressed(SDL_SCANCODE_F1)) {
    mIsDebugging = !mIsDebugging;

    std::cout << "=== DEBUG MODE " << (mIsDebugging ? "ENABLED" : "DISABLED")
              << " ===" << std::endl;
  }

  if (Input::WasKeyPressed(SDL_SCANCODE_EQUALS)) {
    BirdActor *bird = new BirdActor(this);
    bird->SetPosition(mFlockCenter + Vector3((std::rand() % 41 - 20),
                                             (std::rand() % 41 - 20),
                                             (std::rand() % 41) - 20));
    bird->GetComponent<RigidBodyComponent>()->SetVelocity(
        Vector3((std::rand() % 41 - 20), (std::rand() % 41 - 20),
                (std::rand() % 41 - 20)));
    mBirds.emplace_back(bird);
  }
  if (Input::WasKeyPressed(SDL_SCANCODE_MINUS)) {
    if (!mBirds.empty()) {
      auto b = mBirds.back();
      mBirds.pop_back();
      RemoveActor(b);
    }
  }

  for (auto &actor : mActors) {
    actor->ProcessInput();
  }
}

void Scene::UpdateActors(float deltaTime) {
  mUpdatingActors = true;

  Vector3 flockCenter = Vector3::Zero;
  Vector3 flockVelocity = Vector3::Zero;
  int birdCount = 0;

  // Update all active actors
  for (auto actor : mActors) {
    actor->Update(deltaTime);
    if (actor->GetState() == ActorState::Active) {
      if (auto bird = dynamic_cast<BirdActor *>(actor)) {
        flockCenter += bird->GetPosition();
        flockVelocity +=
            bird->GetComponent<RigidBodyComponent>()->GetVelocity();
        birdCount++;
      }
    }
  }
  if (birdCount > 0) {
    mFlockCenter = flockCenter * (1.0 / static_cast<float>(birdCount));
    mFlockVelocity = flockVelocity * (1.0 / static_cast<float>(birdCount));
  }
  mUpdatingActors = false;

  // Move pending actors to active and register with chunk grid
  for (auto pending : mPendingActors) {
    mActors.emplace_back(pending);
  }
  mPendingActors.clear();

  // Delete dead actors
  std::vector<Actor *> deadActors;
  for (auto actor : mActors) {
    if (actor->GetState() == ActorState::Destroy) {
      deadActors.emplace_back(actor);
    }
  }

  for (auto actor : deadActors) {
    delete actor;
  }
}

void Scene::UpdateScene(float deltaTime) {
  UpdateActors(deltaTime);

  // Check collisions after all actors have been updated
  CheckCollisions();
}

void Scene::CheckCollisions() {
  // Get visible actors from chunk grid

  // Collect all colliders from active actors
  std::vector<ColliderComponent *> colliders;
  for (auto actor : mActors) {
    auto collider = actor->GetComponent<ColliderComponent>();
    if (collider) {
      colliders.push_back(collider);
    }
  }

  // Check collisions between all pairs
  for (size_t i = 0; i < colliders.size(); i++) {
    for (size_t j = i + 1; j < colliders.size(); j++) {
      ColliderComponent *colliderA = colliders[i];
      ColliderComponent *colliderB = colliders[j];

      // Check if they intersect
      if (colliderA->Intersect(*colliderB)) {
        // Get penetration vector for A (how much to push A out of B)
        Vector3 penetrationA = colliderA->DetectCollision(*colliderB);

        // Determine who gets pushed based on static flags
        bool aIsStatic = colliderA->IsStatic();
        bool bIsStatic = colliderB->IsStatic();

        if (!aIsStatic && !bIsStatic) {
          // Both dynamic - split the penetration
          colliderA->GetOwner()->OnCollision(penetrationA * 0.5f, colliderB);
          colliderB->GetOwner()->OnCollision(penetrationA * -0.5f, colliderA);
        } else if (!aIsStatic && bIsStatic) {
          // A is dynamic, B is static - only push A
          colliderA->GetOwner()->OnCollision(penetrationA, colliderB);
        } else if (aIsStatic && !bIsStatic) {
          // A is static, B is dynamic - only push B
          colliderB->GetOwner()->OnCollision(penetrationA * -1.0f, colliderA);
        }
        // else: both static - no response needed
      }
    }
  }
}

void Scene::GenerateOutput() {
  // Clear the screen
  mRenderer->Clear();

  // Update camera
  Vector3 targetPos = mCameraPos + mCameraForward;
  mRenderer->SetViewMatrix(
      Matrix4::CreateLookAt(mCameraPos, targetPos, mCameraUp));
  mRenderer->SetCameraPos(mCameraPos);
  RendererMode mode =
      mIsDebugging ? RendererMode::LINES : RendererMode::TRIANGLES;

  // Get visible actors from chunk grid
  // std::vector<Actor*> mActiveActors =
  // mChunkGrid->GetVisibleActors(mCameraPos);

  // Collect meshes and sprites from active actors
  std::vector<MeshComponent *> activeMeshes;

  for (auto actor : mActors) {
    auto &components = actor->GetComponents();
    for (auto component : components) {
      if (auto mesh = dynamic_cast<MeshComponent *>(component)) {
        if (mesh->IsVisible()) {
          activeMeshes.push_back(mesh);
        }
      }
    }
  }

  // Render meshes with instancing (batch draw)
  mRenderer->ActivateMeshShader();
  mRenderer->DrawMeshesInstanced(activeMeshes, mode);

  /*
  if (mIsDebugging) {
    for (auto actor : mActors) {
      auto &components = actor->GetComponents();
      for (auto component : components) {
        component->DebugDraw(mRenderer);
      }
    }
  }
  */

  SDL_GL_SwapWindow(mWindow);
}
