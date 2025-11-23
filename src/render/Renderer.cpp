#include "Renderer.hpp"
#include "Actor.hpp"
#include "Mesh.hpp"
#include "MeshComponent.hpp"
#include "Shader.hpp"
#include <GL/glew.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <cstring>
#include <iostream>

Renderer::Renderer()
    : mViewMatrix(Matrix4::Identity), mProjectionMatrix(Matrix4::Identity),
      mMeshShader(nullptr), mShadowShader(nullptr), mGroundMesh(nullptr),
      lightDir(Vector3::Normalize(Vector3(-1.5f, -0.5f, 0.5f))), mShadowFBO(0),
      mShadowMap(0), mDownsampleFactor(8) {}

Renderer::~Renderer() {

  // Delete shaders
  if (mMeshShader) {
    delete mMeshShader;
    mMeshShader = nullptr;
  }
  if (mShadowShader) {
    delete mShadowShader;
    mShadowShader = nullptr;
  }

  // Delete shadow map
  if (mShadowFBO) {
    glDeleteFramebuffers(1, &mShadowFBO);
  }
  if (mShadowMap) {
    glDeleteTextures(1, &mShadowMap);
  }
}

bool Renderer::Initialize(float width, float height) {
  // Make sure we can create/compile shaders
  if (!LoadShaders()) {
    std::cerr << "Failed to load shaders." << std::endl;
    return false;
  }

  // Create shadow map
  glGenFramebuffers(1, &mShadowFBO);
  glGenTextures(1, &mShadowMap);
  glBindTexture(GL_TEXTURE_2D, mShadowMap);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOW_MAP_SIZE,
               SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  glBindFramebuffer(GL_FRAMEBUFFER, mShadowFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         mShadowMap, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Shadow framebuffer not complete!" << std::endl;
    return false;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Set the clear color to sky blue
  glClearColor(1.0f, 0.7785f, 0.4848f, 1.0f);

  // Enable depth testing
  glEnable(GL_DEPTH_TEST);

  // Enable backface culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK); // Cull back faces
  glFrontFace(GL_CCW); // Counter-clockwise winding is front-facing

  // Enable alpha blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Set default projection matrix based on framebuffer size (perspective)
  float fov = Math::ToRadians(40.0f);
  mProjectionMatrix =
      Matrix4::CreatePerspectiveFOV(fov, width, height, 0.1f, 1500.0f);

  mScreenWidth = width;
  mScreenHeight = height;

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
  if (mShadowShader) {
    mShadowShader->Unload();
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
          meshComp->GetOwner()->GetPosition() +
          Vector3::Transform(meshComp->GetOffset() *
                                 meshComp->GetOwner()->GetScale(),
                             meshComp->GetOwner()->GetRotation());
      Vector3 size = meshComp->GetOwner()->GetScale() * meshComp->GetScale();
      Quaternion rotation = Quaternion::Concatenate(
          meshComp->GetRotation(), meshComp->GetOwner()->GetRotation());

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

    // Activate mesh VAO
    group.mesh->SetActive();

    // Set uIsGround
    mMeshShader->SetFloatUniform("uIsGround",
                                 group.mesh == mGroundMesh ? 1.0f : 0.0f);

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

void Renderer::SetScreenSize(float width, float height) {
  mScreenWidth = width;
  mScreenHeight = height;
  glViewport(0, 0, width, height);
}

float Renderer::GetScreenWidth() const { return mScreenWidth; }

float Renderer::GetScreenHeight() const { return mScreenHeight; }

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

  // Create shadow shader
  mShadowShader = new Shader();
  if (!mShadowShader->Load("./assets/shaders/Shadow.vert",
                           "./assets/shaders/Shadow.frag")) {
    delete mShadowShader;
    mShadowShader = nullptr;
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
  } else if (meshName == "cone") {
    mesh = new ConeMesh();
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

void Renderer::RenderShadows(const std::vector<MeshComponent *> &meshes) {
  if (meshes.empty() || !mShadowShader) {
    return;
  }

  // Bind shadow framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, mShadowFBO);
  glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
  glClear(GL_DEPTH_BUFFER_BIT);

  // Disable color writes
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  // Set light view and projection
  Vector3 lightPos = lightDir * -1000.0f;     // Position behind the scene
  Vector3 target = Vector3(0.0f, 0.0f, 0.0f); // Center of scene
  Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
  Matrix4 lightView = Matrix4::CreateLookAt(lightPos, target, up);
  Matrix4 lightProj =
      Matrix4::CreateOrtho(-500.0f, 500.0f, -500.0f, 500.0f, 0.1f, 2000.0f);
  Matrix4 lightViewProj = lightView * lightProj;

  // Activate shadow shader
  ActivateShadowShader();
  mShadowShader->SetMatrixUniform("uLightViewProjection", lightViewProj);

  // Group meshes by mesh pointer
  struct MeshGroup {
    Mesh *mesh;
    std::vector<MeshComponent *> components;
  };

  std::vector<MeshGroup> groups;

  for (auto *meshComp : meshes) {
    if (!meshComp->IsVisible())
      continue;

    Mesh *mesh = &meshComp->GetMesh();

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

  // Draw each group
  for (auto &group : groups) {
    if (group.components.empty())
      continue;

    if (group.mesh->GetMaxInstances() == 0) {
      group.mesh->SetupInstanceBuffer(10000);
    }

    std::vector<float> instanceData;
    instanceData.reserve(group.components.size() *
                         35); // 35 floats per instance

    for (auto *meshComp : group.components) {
      Vector3 position =
          meshComp->GetOwner()->GetPosition() +
          Vector3::Transform(meshComp->GetOffset() *
                                 meshComp->GetOwner()->GetScale(),
                             meshComp->GetOwner()->GetRotation());
      Vector3 size = meshComp->GetOwner()->GetScale() * meshComp->GetScale();
      Quaternion rotation = Quaternion::Concatenate(
          meshComp->GetRotation(), meshComp->GetOwner()->GetRotation());

      Matrix4 model = Matrix4::CreateScale(size) *
                      Matrix4::CreateFromQuaternion(rotation) *
                      Matrix4::CreateTranslation(position);

      // Add model matrix (16 floats)
      for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
          instanceData.push_back(model.mat[row][col]);
        }
      }

      // Add normal matrix (identity, 16 floats)
      Matrix4 normalMatrix = Matrix4::Identity;
      for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
          instanceData.push_back(normalMatrix.mat[row][col]);
        }
      }

      // Add color (0,0,0, 3 floats)
      instanceData.push_back(0.0f);
      instanceData.push_back(0.0f);
      instanceData.push_back(0.0f);
    }

    group.mesh->UpdateInstanceBuffer(instanceData, group.components.size());

    group.mesh->SetActive();

    glDrawElementsInstanced(GL_TRIANGLES, group.mesh->GetNumIndices(),
                            GL_UNSIGNED_INT, nullptr, group.components.size());
  }

  // Restore state
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  // Restore viewport later in main render
}

void Renderer::ActivateShadowShader() {
  if (!mShadowShader) {
    std::cerr << "Shadow shader not loaded" << std::endl;
    return;
  }

  mShadowShader->SetActive();
}

void Renderer::ActivateMeshShader() {
  if (!mMeshShader) {
    std::cerr << "Mesh shader not loaded" << std::endl;
    return;
  }

  // Activate mesh shader program
  mMeshShader->SetActive();

  // Set view-projection matrix uniform (same for all instances)
  Matrix4 viewProj = mViewMatrix * mProjectionMatrix;
  mMeshShader->SetMatrixUniform("uViewProjection", viewProj);
  mMeshShader->SetVectorUniform("uLightDir", lightDir);
  mMeshShader->SetVectorUniform("uLightColor", Vector3(1.65f, 1.45f, 0.8f));
  mMeshShader->SetVectorUniform("uAmbientColor", Vector3(0.2f, 0.2f, 0.2f));
  mMeshShader->SetVectorUniform("uCameraPos", mCameraPos);
  mMeshShader->SetVectorUniform("uFogColor", Vector3(1.0f, 0.7785f, 0.4848f));
  mMeshShader->SetFloatUniform("uFogDensity", 0.007f);

  // Bind shadow map
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mShadowMap);
  mMeshShader->SetIntegerUniform("uShadowMap", 0);

  // Set light view-projection for shadow mapping
  Vector3 lightPos = lightDir * -1000.0f;
  Vector3 target = Vector3(0.0f, 0.0f, 0.0f);
  Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
  Matrix4 lightView = Matrix4::CreateLookAt(lightPos, target, up);
  Matrix4 lightProj =
      Matrix4::CreateOrtho(-500.0f, 500.0f, -500.0f, 500.0f, 0.1f, 2000.0f);
  Matrix4 lightViewProj = lightView * lightProj;
  mMeshShader->SetMatrixUniform("uLightViewProjection", lightViewProj);
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

  // Set uIsGround
  mMeshShader->SetFloatUniform("uIsGround", mesh == mGroundMesh ? 1.0f : 0.0f);

  // Draw using instanced rendering with 1 instance
  glDrawElementsInstanced(GL_TRIANGLES, mesh->GetNumIndices(), GL_UNSIGNED_INT,
                          nullptr, 1);

  // Restore polygon mode
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  // Re-enable depth test
  glEnable(GL_DEPTH_TEST);
}

float Renderer::GetHeightAt(float x, float z) {
  const int factor = mDownsampleFactor;
  float localX = (x + mHeightmapOffset) / (mHeightmapScale * factor);
  float localZ = (z + mHeightmapOffset) / (mHeightmapScale * factor);
  int ix = (int)floor(localX);
  int iz = (int)floor(localZ);
  float fx = localX - ix;
  float fz = localZ - iz;
  ix = Math::Clamp(ix, 0, mHeightmapWidth - 2);
  iz = Math::Clamp(iz, 0, mHeightmapHeight - 2);
  float h00 = mHeightmapData[iz * mHeightmapWidth + ix];
  float h10 = mHeightmapData[iz * mHeightmapWidth + ix + 1];
  float h01 = mHeightmapData[(iz + 1) * mHeightmapWidth + ix];
  float h11 = mHeightmapData[(iz + 1) * mHeightmapWidth + ix + 1];
  float h0 = h00 * (1 - fx) + h10 * fx;
  float h1 = h01 * (1 - fx) + h11 * fx;
  return h0 * (1 - fz) + h1 * fz;
}

Vector3 Renderer::GetNormalAt(float x, float z) {
  const int factor = mDownsampleFactor;
  float localX = (x + mHeightmapOffset) / (mHeightmapScale * factor);
  float localZ = (z + mHeightmapOffset) / (mHeightmapScale * factor);
  int ix = (int)Math::Clamp(localX, 0.0f, mHeightmapWidth - 1.0f);
  int iy = (int)Math::Clamp(localZ, 0.0f, mHeightmapHeight - 1.0f);

  float h = mHeightmapData[iy * mHeightmapWidth + ix];
  float hx_plus = (ix < mHeightmapWidth - 1)
                      ? mHeightmapData[iy * mHeightmapWidth + ix + 1]
                      : h;
  float hx_minus = (ix > 0) ? mHeightmapData[iy * mHeightmapWidth + ix - 1] : h;
  float hz_plus = (iy < mHeightmapHeight - 1)
                      ? mHeightmapData[(iy + 1) * mHeightmapWidth + ix]
                      : h;
  float hz_minus =
      (iy > 0) ? mHeightmapData[(iy - 1) * mHeightmapWidth + ix] : h;

  float dx = (hx_plus - hx_minus) / (2.0f * mHeightmapScale * factor);
  float dz = (hz_plus - hz_minus) / (2.0f * mHeightmapScale * factor);

  Vector3 normal = Vector3(-dx, 1.0f, -dz);
  normal.Normalize();
  return normal;
}

Mesh *Renderer::CreateHeightmapMesh(const std::string &path, float offset,
                                    float scale, float heightScale) {
  SDL_Surface *img = IMG_Load(path.c_str());
  const int W = img->w;
  const int H = img->h;
  const int BPP = img->format->BytesPerPixel;
  const int pitch = img->pitch;
  uint8_t *pix = (uint8_t *)img->pixels;

  const int factor = mDownsampleFactor; // Downsample factor
  const int newW = W / factor;
  const int newH = H / factor;

  mHeightmapWidth = newW;
  mHeightmapHeight = newH;
  mHeightmapOffset = offset;
  mHeightmapScale = scale;
  mHeightmapHeightScale = heightScale;
  mHeightmapData.resize(newW * newH);

  // Downsample heightmap
  for (int iy = 0; iy < newH; iy++) {
    for (int ix = 0; ix < newW; ix++) {
      float sum = 0.0f;
      for (int dy = 0; dy < factor; dy++) {
        for (int dx = 0; dx < factor; dx++) {
          int px = ix * factor + dx;
          int py = iy * factor + dy;
          if (px < W && py < H) {
            uint8_t *row_start = &pix[py * pitch];
            uint8_t *p = &row_start[px * BPP];
            uint16_t hval_16bit;
            if (BPP >= 2) {
              memcpy(&hval_16bit, p, 2);
              sum += float(hval_16bit) / 65535.0f;
            } else {
              hval_16bit = *p;
              sum += float(hval_16bit) / 255.0f;
            }
          }
        }
      }
      float hn = sum / (factor * factor);
      float Y = hn * heightScale;
      mHeightmapData[iy * newW + ix] = Y;
    }
  }

  std::vector<Vertex> vertices;
  vertices.reserve(newW * newH);

  for (int iy = 0; iy < newH; iy++) {
    for (int ix = 0; ix < newW; ix++) {
      float Y = mHeightmapData[iy * newW + ix];
      Vertex v;
      v.position = Vector3(float(ix * factor) * scale - offset, Y,
                           float(iy * factor) * scale - offset);
      v.normal = Vector3(0, 1, 0);
      v.texture = Vector2(float(ix) / (newW - 1), float(iy) / (newH - 1));
      vertices.push_back(v);
    }
  }

  // Compute normals for mesh
  for (int y = 0; y < newH - 1; y++) {
    for (int x = 0; x < newW - 1; x++) {
      int i0 = y * newW + x;
      int i1 = i0 + 1;
      int i2 = (y + 1) * newW + x;
      int i3 = i2 + 1;
      // Normals for triangle 1 (i0, i1, i2)
      Vector3 v01 = vertices[i1].position - vertices[i0].position;
      Vector3 v02 = vertices[i2].position - vertices[i0].position;
      Vector3 normal1 = Vector3::Normalize(Vector3::Cross(v02, v01));
      // Normals for triangle 2 (i1, i3, i2)
      Vector3 v13 = vertices[i3].position - vertices[i1].position;
      Vector3 v12 = vertices[i2].position - vertices[i1].position;
      Vector3 normal2 = Vector3::Normalize(Vector3::Cross(v12, v13));
      vertices[i0].normal += normal1;
      vertices[i1].normal += normal1 + normal2;
      vertices[i2].normal += normal1 + normal2;
      vertices[i3].normal += normal2;
    }
  }
  for (Vertex &v : vertices) {
    v.normal.Normalize();
  }
  // Generate triangles for mesh
  std::vector<Triangle> triangles;
  triangles.reserve((newW - 1) * (newH - 1) * 2);
  for (int y = 0; y < newH - 1; y++) {
    for (int x = 0; x < newW - 1; x++) {
      int i0 = y * newW + x;
      int i1 = i0 + 1;
      int i2 = (y + 1) * newW + x;
      int i3 = i2 + 1;
      triangles.push_back(
          {(unsigned int)i0, (unsigned int)i1, (unsigned int)i2});
      triangles.push_back(
          {(unsigned int)i1, (unsigned int)i3, (unsigned int)i2});
    }
  }

  SDL_FreeSurface(img);

  Mesh *mesh = new Mesh();

  MeshData data;
  data.vertices = std::move(vertices);
  data.triangles = std::move(triangles);
  mesh->Build(data);

  mGroundMesh = mesh;

  return mesh;
}