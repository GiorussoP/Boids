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
  if (mScene->GetCameraMode() != CameraMode::Free) {
    return;
  }
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
  Vector3 forward, up;

  switch (mScene->GetCameraMode()) {
  case CameraMode::Free:
    if (mMoveDir.LengthSq() > 0.0f) {
      mMoveDir.Normalize();
      Vector3 newPos =
          mScene->GetCameraPos() + mMoveDir * mMoveSpeed * deltaTime;
      mScene->SetCameraPos(newPos);
    }

    // Update camera forward based on yaw and pitch
    forward = Vector3(Math::Cos(mPitch) * Math::Cos(mYaw), Math::Sin(mPitch),
                      Math::Cos(mPitch) * Math::Sin(mYaw));
    mScene->SetCameraForward(forward);
    mScene->SetCameraUp(Vector3::UnitY);
    break;
  case CameraMode::Fixed:
    // Look to flock center

    mScene->SetCameraPos(Vector3(0.0f, 30.0f, 0.0f));
    forward =
        Vector3::Normalize(mScene->GetFlockCenter() - mScene->GetCameraPos());
    mScene->SetCameraRotation(Math::LookRotation(forward, Vector3::UnitY));
    // mScene->SetCameraForward(forward);
    // mScene->SetCameraUp(Vector3::UnitY);

    break;
  case CameraMode::FollowBoid: {
    Vector3 flockCenter = mScene->GetFlockCenter();
    Vector3 cameraPos =
        Vector3::Lerp(mScene->GetCameraPos(), flockCenter, 0.05f);
    forward = Vector3::Normalize(flockCenter - cameraPos);
    mScene->SetCameraForward(forward);
    mScene->SetCameraPos(cameraPos);
  }
  case CameraMode::SeeBoid: {
    /*
    auto leader = mScene->GetLeader();
    if (leader) {
      Vector3 leaderPos = leader->GetPosition();
      Vector3 cameraPos =
          Vector3::Lerp(mScene->GetCameraPos(), leaderPos, 0.05f);
      forward = Vector3::Normalize(leaderPos - cameraPos);
      mScene->SetCameraForward(forward);
      mScene->SetCameraPos(cameraPos);
    }
    */
  } break;
  default:
    break;
  }
}