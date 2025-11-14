#include "actors/GroundActor.hpp"

#include "Scene.hpp"
#include "components/MeshComponent.hpp"
#include "render/Mesh.hpp"
#include "render/Renderer.hpp"

GroundActor::GroundActor(Scene *scene) : Actor(scene) {
  // Create and attach mesh component
  Mesh *groundMesh = scene->GetRenderer()->LoadMesh("plane");
  mMeshComponent =
      new MeshComponent(this, *groundMesh, Vector3(0.0f, 0.0f, 0.0f));
  mMeshComponent->SetColor(Color::Green);
  SetScale(Vector3(100.0f, 1.0f, 100.0f));
}