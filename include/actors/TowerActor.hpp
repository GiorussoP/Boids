#ifndef TOWER_ACTOR_HPP
#define TOWER_ACTOR_HPP

#include "Actor.hpp"
#include "Mesh.hpp"
#include "MeshComponent.hpp"

class TowerActor : public Actor {
public:
  TowerActor(class Scene *scene);
  ~TowerActor() {};

private:
  MeshComponent *mMeshComponent;
};

#endif