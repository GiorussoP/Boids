#ifndef BIRD_ACTOR_HPP
#define BIRD_ACTOR_HPP

#include "Actor.hpp"
#include "MeshComponent.hpp"

class BirdActor : public Actor {
public:
  BirdActor(class Scene *scene);
  ~BirdActor() {};
  void OnUpdate(float deltaTime) override;

private:
  float mflapTimer;

  MeshComponent *mBody;
  MeshComponent *mLeftWing;
  MeshComponent *mRightWing;
  MeshComponent *mBeak;
  MeshComponent *mTail;
};

#endif