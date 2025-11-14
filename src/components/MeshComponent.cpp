#include "components/MeshComponent.hpp"
#include "actors/Actor.hpp"
#include "render/Mesh.hpp"

MeshComponent::MeshComponent(Actor *owner, Mesh &mesh, Vector3 offset,
                             Quaternion rotation, Vector3 color)
    : Component(owner), mMesh(mesh), mOffset(offset), mRotation(rotation),
      mColor(color), mIsVisible(true) {}

MeshComponent::~MeshComponent() {}
