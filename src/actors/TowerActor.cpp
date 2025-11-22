#include "TowerActor.hpp"

#include "Scene.hpp"
#include "render/Renderer.hpp"

TowerActor::TowerActor(Scene *scene) : Actor(scene) {
  // Create and attach mesh component
  Mesh *towerMesh = scene->GetRenderer()->LoadMesh("cone");
  mMeshComponent =
      new MeshComponent(this, *towerMesh, Vector3(0.0f, 0.0f, 0.0f));
  mMeshComponent->SetColor(Color::Red);
  SetScale(Vector3(100.0f, 600.0f, 100.0f));
  SetPosition(Vector3(0.0f, 25.0f, 0.0f));
}

void TowerActor::OnUpdate(float deltaTime) {
  // for warnings
  (void)deltaTime;
}