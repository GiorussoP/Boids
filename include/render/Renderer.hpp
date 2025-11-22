#pragma once
#include "Math.hpp"
#include "MeshComponent.hpp"
#include "Shader.hpp"

#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <vector>

enum class RendererMode { TRIANGLES, LINES };

class Renderer {
public:
  Renderer();
  ~Renderer();

  bool Initialize(float width, float height);
  void Shutdown();

  // Mesh management
  Mesh *LoadMesh(const std::string &meshName);

  Mesh *CreateHeightmapMesh(const std::string &path, float offset, float scale,
                            float heightScale);

  float GetHeightAt(float x, float z);
  Vector3 GetNormalAt(float x, float z);

  // Instanced drawing - draw multiple instances of the same mesh
  void DrawMeshesInstanced(const std::vector<MeshComponent *> &meshes,
                           RendererMode mode);

  void DrawSingleMesh(Mesh *mesh, const Vector3 &position, const Vector3 &scale,
                      const Quaternion &rotation = Quaternion::Identity);

  // Batch rendering - set frame-level uniforms once before drawing multiple

  void ActivateMeshShader();
  void ActivateShadowShader() {};

  void SetViewMatrix(const Matrix4 &view);
  void SetProjectionMatrix(const Matrix4 &projection);
  void SetCameraPos(const Vector3 &pos);
  void SetDownsampleFactor(int factor);
  void SetFOV(float fov);
  float GetScreenWidth() const;
  float GetScreenHeight() const;

  void Clear();
  void Present();

private:
  bool LoadShaders();

  // Projection and view matrices
  Matrix4 mViewMatrix;
  Matrix4 mProjectionMatrix;
  Vector3 mCameraPos;
  float mScreenWidth;
  float mScreenHeight;

  // Shaders
  Shader *mMeshShader;

  // Meshes
  std::unordered_map<std::string, Mesh *> mMeshCache;
  Mesh *mGroundMesh;

  Vector3 lightDir;

  // Heightmap data
  std::vector<float> mHeightmapData;
  int mHeightmapWidth;
  int mHeightmapHeight;
  float mHeightmapOffset;
  float mHeightmapScale;
  float mHeightmapHeightScale;
  int mDownsampleFactor;
};