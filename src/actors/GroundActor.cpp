#include "actors/GroundActor.hpp"

#include "Scene.hpp"
#include "components/MeshComponent.hpp"
#include "render/Mesh.hpp"
#include "render/Renderer.hpp"
#include <iostream>

GroundActor::GroundActor(Scene *scene) : Actor(scene) {

  const std::string heightmapPath = "assets/heightmap/heightmap2.png";
  const float heightScale = scene->GetHeightmapHeightScale();
  const float horizontalScaleFactor = scene->GetHeightmapScale();
  const float offset = scene->GetHeightmapOffset();

  Mesh *groundMesh = scene->GetRenderer()->CreateHeightmapMesh(
      heightmapPath, offset, horizontalScaleFactor, heightScale);

  mMeshComponent = new MeshComponent(this, *groundMesh, Vector3::Zero);

  mMeshComponent->SetColor(Color::Green);
  SetScale(Vector3::One);
}