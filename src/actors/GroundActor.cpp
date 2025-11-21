#include "actors/GroundActor.hpp"

#include "Scene.hpp"
#include "components/MeshComponent.hpp"
#include "render/Mesh.hpp"
#include "render/Renderer.hpp"
#include <iostream>

GroundActor::GroundActor(Scene *scene) : Actor(scene) {

  const std::string heightmapPath = "assets/heightmap/heightmap2.png";
  const float heightScale = 100.0f; // Ajuste esta altura conforme necessário
  const float horizontalScaleFactor = 1000.0f / 1025.0f;
  const float offset = horizontalScaleFactor * 0.5f * 1000.0f;

  Mesh *groundMesh =
      scene->GetRenderer()->CreateHeightmapMesh(heightmapPath, heightScale);

  mMeshComponent =
      new MeshComponent(this, *groundMesh, Vector3(-offset, 0.0, -offset));

  mMeshComponent->SetColor(Color::Green);
  SetScale(Vector3(horizontalScaleFactor, 1.0f, horizontalScaleFactor));
}