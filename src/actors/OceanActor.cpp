#include "OceanActor.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include <iostream>

OceanActor::OceanActor(Scene *scene) : Actor(scene) {
  // Load the plane mesh
  Mesh *planeMesh = scene->GetRenderer()->LoadMesh("plane");
  if (!planeMesh) {
    std::cerr << "Failed to load plane mesh for ocean" << std::endl;
    return;
  }

  // Create a giant blue plane for the ocean
  mMeshComponent = new MeshComponent(this, *planeMesh, Vector3::Zero,
                                     Vector3(2000.0f, 1.0f, 2000.0f));
  mMeshComponent->SetColor(Vector3(0.0f, 0.2f, 0.8f)); // Blue color
  SetPosition(Vector3(0.0f, 220.0f, 0.0f));
}

OceanActor::~OceanActor() {
  // MeshComponent is deleted by Actor base class
}