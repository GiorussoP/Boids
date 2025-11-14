#include "TowerActor.hpp"

#include "Scene.hpp"
#include "render/Renderer.hpp"

TowerActor::TowerActor(Scene *scene) : Actor(scene) {
  // Create and attach mesh component
  Mesh *towerMesh = scene->GetRenderer()->LoadMesh("cube");
  mMeshComponent = new MeshComponent(
      this, *towerMesh, Vector3(0.0f, 0.0f, 0.0f), Quaternion::Identity);
  mMeshComponent->SetColor(Color::Yellow);
  SetScale(Vector3(2.0f, 30.0f, 2.0f));
  SetPosition(Vector3(5.0f, 15.0f, 0.0f));
}

void TowerActor::OnUpdate(float deltaTime) {
  // Rotate tower slowly
  Quaternion rot(Vector3::UnitY, Math::ToRadians(15.0f) * deltaTime);
  SetRotation(Quaternion::Concatenate(GetRotation(), rot));
}