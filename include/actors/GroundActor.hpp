#ifndef GROUND_ACTOR_HPP
#define GROUND_ACTOR_HPP

#include "Actor.hpp"
#include "MeshComponent.hpp"

class GroundActor : public Actor {
public:
  GroundActor(class Scene *scene);
  ~GroundActor() {};

private:
  MeshComponent *mMeshComponent;
};
#endif