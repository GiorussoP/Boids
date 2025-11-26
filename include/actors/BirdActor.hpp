#ifndef BIRD_ACTOR_HPP
#define BIRD_ACTOR_HPP

#include "Actor.hpp"
#include "MeshComponent.hpp"
#include "RigidBodyComponent.hpp"

enum BirdType { Leader, Follower };

class BirdActor : public Actor {
public:
  BirdActor(class Scene *scene, BirdType type = Follower);
  ~BirdActor() {};
  void OnUpdate(float deltaTime) override;

  RigidBodyComponent &GetRigidBody() { return *mRigidBody; }

private:
  float mflapTimer;
  float mPrevYaw;
  float mRandomAngle;

  MeshComponent *mBody;
  MeshComponent *mLeftWing;
  MeshComponent *mRightWing;
  MeshComponent *mBeak;
  MeshComponent *mTail;

  MeshComponent *mFishHead;
  MeshComponent *mFishBody;
  MeshComponent *mFishTail;

  BirdType mType;

  RigidBodyComponent *mRigidBody;
};

#endif