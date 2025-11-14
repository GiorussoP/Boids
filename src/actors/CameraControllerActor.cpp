#include "actors/CameraControllerActor.hpp"

#include "Input.hpp"
#include "Math.hpp"
#include "Scene.hpp"

CameraControllerActor::CameraControllerActor(Scene *scene)
    : Actor(scene), mYaw(0.0f), mPitch(Math::ToRadians(-45.0f)),
      mMoveDir(Vector3(0.0f, 0.0f, 0.0f)) {
  // Set initial camera forward
  Vector3 forward =
      Vector3(Math::Cos(mPitch) * Math::Cos(mYaw), Math::Sin(mPitch),
              Math::Cos(mPitch) * Math::Sin(mYaw));
  scene->SetCameraForward(forward);
}

CameraControllerActor::~CameraControllerActor() {}

void CameraControllerActor::OnProcessInput() {
  // Look controls with arrow keys
  if (Input::IsKeyDown(SDL_SCANCODE_UP)) {
    mPitch += mLookSpeed * 0.016f; // Approximate deltaTime for input processing
    mPitch =
        Math::Clamp(mPitch, -Math::ToRadians(80.0f), Math::ToRadians(80.0f));
  }
  if (Input::IsKeyDown(SDL_SCANCODE_DOWN)) {
    mPitch -= mLookSpeed * 0.016f;
    mPitch =
        Math::Clamp(mPitch, -Math::ToRadians(80.0f), Math::ToRadians(80.0f));
  }
  if (Input::IsKeyDown(SDL_SCANCODE_LEFT)) {
    mYaw += mLookSpeed * 0.016f;
  }
  if (Input::IsKeyDown(SDL_SCANCODE_RIGHT)) {
    mYaw -= mLookSpeed * 0.016f;
  }

  // Movement controls with WASD
  Vector3 forward =
      Vector3(Math::Cos(mPitch) * Math::Cos(mYaw), Math::Sin(mPitch),
              Math::Cos(mPitch) * Math::Sin(mYaw));
  Vector3 right = Vector3::Cross(GetScene()->GetCameraUp(), forward);
  right.Normalize();

  mMoveDir = Vector3(0.0f, 0.0f, 0.0f);
  if (Input::IsKeyDown(SDL_SCANCODE_W)) {
    mMoveDir += forward;
  }
  if (Input::IsKeyDown(SDL_SCANCODE_S)) {
    mMoveDir -= forward;
  }
  if (Input::IsKeyDown(SDL_SCANCODE_A)) {
    mMoveDir -= right;
  }
  if (Input::IsKeyDown(SDL_SCANCODE_D)) {
    mMoveDir += right;
  }
  if (Input::IsKeyDown(SDL_SCANCODE_SPACE)) {
    mMoveDir += GetScene()->GetCameraUp();
  }
  if (Input::IsKeyDown(SDL_SCANCODE_LSHIFT)) {
    mMoveDir -= GetScene()->GetCameraUp();
  }
}

void CameraControllerActor::OnUpdate(float deltaTime) {
  Scene *scene = GetScene();

  if (mMoveDir.LengthSq() > 0.0f) {
    mMoveDir.Normalize();
    Vector3 newPos = scene->GetCameraPos() + mMoveDir * mMoveSpeed * deltaTime;
    scene->SetCameraPos(newPos);
  }

  // Update camera forward based on yaw and pitch
  Vector3 forward =
      Vector3(Math::Cos(mPitch) * Math::Cos(mYaw), Math::Sin(mPitch),
              Math::Cos(mPitch) * Math::Sin(mYaw));
  scene->SetCameraForward(forward);
}