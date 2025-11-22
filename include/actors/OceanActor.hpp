#pragma once
#include "Actor.hpp"
#include "MeshComponent.hpp"

class OceanActor : public Actor {
public:
    OceanActor(class Scene* scene);
    ~OceanActor();

private:
    MeshComponent* mMeshComponent;
};