#ifndef CAMERA_CONTROLLER_ACTOR_HPP
#define CAMERA_CONTROLLER_ACTOR_HPP

#include "actors/Actor.hpp"

class CameraControllerActor : public Actor {
public:
  CameraControllerActor(class Scene *scene);
  ~CameraControllerActor();

  void OnProcessInput() override;
  void OnUpdate(float deltaTime) override;

  float GetYaw() const { return mYaw; }
  float GetPitch() const { return mPitch; }

private:
  float mYaw;
  float mPitch;
  float mMoveSpeed = 20.0f;
  float mLookSpeed = 2.0f;
  Vector3 mMoveDir;
};

#endif