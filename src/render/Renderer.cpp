#include "Renderer.hpp"
#include "Actor.hpp"
#include "Mesh.hpp"
#include "MeshComponent.hpp"
#include "Shader.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <iostream>

Renderer::Renderer()
    : mViewMatrix(Matrix4::Identity), mProjectionMatrix(Matrix4::Identity),
      mMeshShader(nullptr) {}

Renderer::~Renderer() {

  // Delete shaders
  if (mMeshShader) {
    delete mMeshShader;
    mMeshShader = nullptr;
  }
}

bool Renderer::Initialize(float width, float height) {
  // Make sure we can create/compile shaders
  if (!LoadShaders()) {
    std::cerr << "Failed to load shaders." << std::endl;
    return false;
  }

  // Set the clear color to blue
  glClearColor(0.0f, 0.0f, 1.0f, 1.0f);

  // Enable depth testing
  glEnable(GL_DEPTH_TEST);

  // Enable backface culling
  // glEnable(GL_CULL_FACE);
  // glCullFace(GL_BACK); // Cull back faces
  // glFrontFace(GL_CCW); // Counter-clockwise winding is front-facing

  // Enable alpha blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Set default projection matrix based on framebuffer size (perspective)
  float fov = Math::ToRadians(90.0f);
  mProjectionMatrix =
      Matrix4::CreatePerspectiveFOV(fov, width, height, 0.1f, 200.0f);

  // Activate mesh shader
  if (mMeshShader) {
    mMeshShader->SetActive();
  }

  return true;
}

void Renderer::Shutdown() {
  // Unload shaders
  if (mMeshShader) {
    mMeshShader->Unload();
  }

  // Delete all cached meshes
  for (auto &pair : mMeshCache) {
    delete pair.second;
  }
  mMeshCache.clear();
}

void Renderer::Clear() {
  // Clear the color and depth buffers
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::DrawMeshesInstanced(const std::vector<MeshComponent *> &meshes,
                                   RendererMode mode) {
  if (meshes.empty() || !mMeshShader) {
    return;
  }

  // Group meshes by their mesh pointer and texture atlas
  struct MeshGroup {
    Mesh *mesh;
    std::vector<MeshComponent *> components;
  };

  std::vector<MeshGroup> groups;

  // Group meshes
  for (auto *meshComp : meshes) {
    if (!meshComp->IsVisible())
      continue;

    Mesh *mesh = &meshComp->GetMesh();

    // Find or create group
    bool found = false;
    for (auto &group : groups) {
      if (group.mesh == mesh) {
        group.components.push_back(meshComp);
        found = true;
        break;
      }
    }

    if (!found) {
      groups.push_back({mesh, {meshComp}});
    }
  }

  // Draw each group with instancing
  for (auto &group : groups) {
    if (group.components.empty())
      continue;

    // Setup instance buffer if not already done
    if (group.mesh->GetMaxInstances() == 0) {
      group.mesh->SetupInstanceBuffer(10000); // Max 10k instances per mesh type
    }

    // Prepare instance data: model matrix (16) + normal matrix (16) + color (3)
    // = 35 floats per instance
    std::vector<float> instanceData;
    instanceData.reserve(group.components.size() * 35);

    for (auto *meshComp : group.components) {
      Vector3 position =
          meshComp->GetOwner()->GetPosition() + meshComp->GetOffset();
      Vector3 size = meshComp->GetOwner()->GetScale();
      Quaternion rotation = Quaternion::Concatenate(
          meshComp->GetOwner()->GetRotation(), meshComp->GetRotation());

      // Model matrix (just transform, not MVP)
      Matrix4 model = Matrix4::CreateScale(size) *
                      Matrix4::CreateFromQuaternion(rotation) *
                      Matrix4::CreateTranslation(position);

      // Add model matrix (16 floats)
      for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
          instanceData.push_back(model.mat[row][col]);
        }
      }

      // Normal matrix (just rotation)
      Matrix4 normalMatrix = Matrix4::CreateFromQuaternion(rotation);

      // Add normal matrix (16 floats)
      for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
          instanceData.push_back(normalMatrix.mat[row][col]);
        }
      }

      // Add color (3 floats)
      Vector3 color = meshComp->GetColor();
      instanceData.push_back(color.x);
      instanceData.push_back(color.y);
      instanceData.push_back(color.z);
    }

    // Upload instance data
    group.mesh->UpdateInstanceBuffer(instanceData, group.components.size());

    // Set view-projection matrix uniform (same for all instances)
    Matrix4 viewProj = mViewMatrix * mProjectionMatrix;
    mMeshShader->SetMatrixUniform("uViewProjection", viewProj);

    // Bind texture atlas

    // Activate mesh VAO
    group.mesh->SetActive();

    // Draw all instances
    if (mode == RendererMode::LINES) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDrawElementsInstanced(GL_TRIANGLES, group.mesh->GetNumIndices(),
                              GL_UNSIGNED_INT, nullptr,
                              group.components.size());
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
      glDrawElementsInstanced(GL_TRIANGLES, group.mesh->GetNumIndices(),
                              GL_UNSIGNED_INT, nullptr,
                              group.components.size());
    }
  }
}

void Renderer::SetViewMatrix(const Matrix4 &view) { mViewMatrix = view; }

void Renderer::SetProjectionMatrix(const Matrix4 &projection) {
  mProjectionMatrix = projection;
}

void Renderer::SetCameraPos(const Vector3 &pos) { mCameraPos = pos; }

void Renderer::Present() {
  // Buffer swapping is now handled by SDL in main.cpp
  // This function kept for API compatibility but does nothing
}

bool Renderer::LoadShaders() {
  // Create mesh shader (Base.vert -> Mesh.frag)
  mMeshShader = new Shader();
  if (!mMeshShader->Load("./assets/shaders/Base.vert",
                         "./assets/shaders/Base.frag")) {
    delete mMeshShader;
    mMeshShader = nullptr;
    return false;
  }

  return true;
}

Mesh *Renderer::LoadMesh(const std::string &meshName) {
  // Check if mesh is already cached
  auto it = mMeshCache.find(meshName);
  if (it != mMeshCache.end()) {
    return it->second;
  }

  // Create new mesh based on name
  Mesh *mesh = nullptr;
  if (meshName == "cube") {
    mesh = new CubeMesh();
  } else if (meshName == "pyramid") {
    mesh = new PyramidMesh();
  } else if (meshName == "plane") {
    mesh = new PlaneMesh();
  } else if (meshName == "sphere") {
    mesh = new SphereMesh();
  } else {
    std::cerr << "LoadMesh: unknown mesh name '" << meshName << "'"
              << std::endl;
    return nullptr;
  }

  if (mesh) {
    mMeshCache[meshName] = mesh;
    std::cout << "Cached mesh: " << meshName << std::endl;
  }

  return mesh;
}

void Renderer::ActivateMeshShader() {
  if (!mMeshShader) {
    std::cerr << "Mesh shader not loaded" << std::endl;
    return;
  }

  // Activate mesh shader program
  mMeshShader->SetActive();

  // Set frame-level uniforms (uniforms that don't change per mesh)
  mMeshShader->SetMatrixUniform("uView", mViewMatrix);
  mMeshShader->SetVectorUniform("uCameraPos", mCameraPos);
  mMeshShader->SetIntegerUniform("uBloomPass", 0); // Default: not bloom pass
}

void Renderer::DrawSingleMesh(Mesh *mesh, const Vector3 &position,
                              const Vector3 &scale,
                              const Quaternion &rotation) {
  if (!mesh || !mMeshShader) {
    return;
  }

  // Setup instance buffer if not already done
  if (mesh->GetMaxInstances() == 0) {
    mesh->SetupInstanceBuffer(1000); // Setup with reasonable max instances
  }

  // Disable depth test for debug drawing
  glDisable(GL_DEPTH_TEST);

  // Activate the mesh VAO
  mesh->SetActive();

  // Set polygon mode to wireframe for debug drawing
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  // Set the view-projection matrix
  Matrix4 viewProj = mViewMatrix * mProjectionMatrix;
  mMeshShader->SetMatrixUniform("uViewProjection", viewProj);
  mMeshShader->SetMatrixUniform("uView", mViewMatrix);

  // Build model matrix from position, rotation, and scale
  // Use the same order as DrawMeshesInstanced
  Matrix4 model = Matrix4::CreateScale(scale) *
                  Matrix4::CreateFromQuaternion(rotation) *
                  Matrix4::CreateTranslation(position);

  // Normal matrix (for lighting, if needed)
  Matrix4 normalMatrix = Matrix4::CreateFromQuaternion(rotation);

  // Prepare instance data for a single mesh
  std::vector<float> instanceData;
  instanceData.reserve(35); // 35 floats per instance

  // Add model matrix (16 floats)
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      instanceData.push_back(model.mat[row][col]);
    }
  }

  // Add normal matrix (16 floats)
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      instanceData.push_back(normalMatrix.mat[row][col]);
    }
  }

  // Add green color (3 floats) for all debug colliders
  instanceData.push_back(0.0f); // R
  instanceData.push_back(1.0f); // G
  instanceData.push_back(0.0f); // B

  // Update instance buffer with single instance
  mesh->UpdateInstanceBuffer(instanceData, 1);

  // Draw using instanced rendering with 1 instance
  glDrawElementsInstanced(GL_TRIANGLES, mesh->GetNumIndices(), GL_UNSIGNED_INT,
                          nullptr, 1);

  // Restore polygon mode
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  // Re-enable depth test
  glEnable(GL_DEPTH_TEST);
}
