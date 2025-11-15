#include "render/Mesh.hpp"
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>

// ============== Base Mesh Class ==============

Mesh::Mesh()
    : mVertexArray(0), mVertexBuffer(0), mIndexBuffer(0), mInstanceBuffer(0),
      mNumVerts(0), mNumIndices(0), mMaxInstances(0) {}

Mesh::~Mesh() {
  if (mVertexBuffer != 0) {
    glDeleteBuffers(1, &mVertexBuffer);
    mVertexBuffer = 0;
  }
  if (mIndexBuffer != 0) {
    glDeleteBuffers(1, &mIndexBuffer);
    mIndexBuffer = 0;
  }
  if (mInstanceBuffer != 0) {
    glDeleteBuffers(1, &mInstanceBuffer);
    mInstanceBuffer = 0;
  }
  if (mVertexArray != 0) {
    glDeleteVertexArrays(1, &mVertexArray);
    mVertexArray = 0;
  }
}

void Mesh::Build(MeshData meshdata) {
  // Clean up existing buffers if any
  if (mVertexBuffer != 0) {
    glDeleteBuffers(1, &mVertexBuffer);
    mVertexBuffer = 0;
  }
  if (mIndexBuffer != 0) {
    glDeleteBuffers(1, &mIndexBuffer);
    mIndexBuffer = 0;
  }
  if (mVertexArray != 0) {
    glDeleteVertexArrays(1, &mVertexArray);
    mVertexArray = 0;
  }

  // Store triangles (for texture mapping)
  mTriangles = meshdata.triangles;

  // Convert vertices to flat array format: [x, y, z, nx, ny, nz]
  std::vector<float> vertexData;
  vertexData.reserve(meshdata.vertices.size() * 6);

  // Now build the vertex data
  for (size_t i = 0; i < meshdata.vertices.size(); ++i) {
    const Vertex &vertex = meshdata.vertices[i];

    // Position
    vertexData.push_back(vertex.position.x);
    vertexData.push_back(vertex.position.y);
    vertexData.push_back(vertex.position.z);

    // Normal
    vertexData.push_back(vertex.normal.x);
    vertexData.push_back(vertex.normal.y);
    vertexData.push_back(vertex.normal.z);
  }

  // Convert triangles to flat index array
  std::vector<unsigned int> indexData;
  indexData.reserve(meshdata.triangles.size() * 3);

  for (const auto &triangle : meshdata.triangles) {
    indexData.push_back(triangle.indices[0]);
    indexData.push_back(triangle.indices[1]);
    indexData.push_back(triangle.indices[2]);
  }

  mNumVerts = static_cast<unsigned int>(meshdata.vertices.size());
  mNumIndices = static_cast<unsigned int>(indexData.size());

  // Create vertex array object
  glGenVertexArrays(1, &mVertexArray);
  glBindVertexArray(mVertexArray);

  // Create and populate vertex buffer
  glGenBuffers(1, &mVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float),
               vertexData.data(), GL_STATIC_DRAW);

  // Create and populate index buffer
  glGenBuffers(1, &mIndexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexData.size() * sizeof(unsigned int),
               indexData.data(), GL_STATIC_DRAW);

  // Set up vertex attributes (6 floats per vertex)
  // Position attribute (location = 0)
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);

  // Normal attribute (location = 1)
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));

  // Unbind VAO
  glBindVertexArray(0);

  std::cout << "Mesh built with " << meshdata.vertices.size()
            << " vertices and " << meshdata.triangles.size() << " triangles"
            << std::endl;
}

void Mesh::SetActive() const { glBindVertexArray(mVertexArray); }

bool Mesh::LoadFromFile(const std::string &filename) {
  // Base implementation - to be overridden by derived classes
  std::cerr << "LoadFromFile not implemented for base Mesh class: " << filename
            << std::endl;
  return false;
}

// ============== CubeMesh Class ==============

CubeMesh::CubeMesh() { Build(GenerateCubeData()); }

MeshData CubeMesh::GenerateCubeData() {
  MeshData data;

  // Define 8 vertices for shared corners
  std::vector<Vector3> positions = {
      Vector3(-0.5f, -0.5f, -0.5f), // 0: back-bottom-left
      Vector3(0.5f, -0.5f, -0.5f),  // 1: back-bottom-right
      Vector3(0.5f, 0.5f, -0.5f),   // 2: back-top-right
      Vector3(-0.5f, 0.5f, -0.5f),  // 3: back-top-left
      Vector3(-0.5f, -0.5f, 0.5f),  // 4: front-bottom-left
      Vector3(0.5f, -0.5f, 0.5f),   // 5: front-bottom-right
      Vector3(0.5f, 0.5f, 0.5f),    // 6: front-top-right
      Vector3(-0.5f, 0.5f, 0.5f)    // 7: front-top-left
  };

  // Face normals
  std::vector<Vector3> faceNormals = {
      Vector3(0.0f, 0.0f, -1.0f), // back
      Vector3(1.0f, 0.0f, 0.0f),  // right
      Vector3(0.0f, 0.0f, 1.0f),  // front
      Vector3(-1.0f, 0.0f, 0.0f), // left
      Vector3(0.0f, 1.0f, 0.0f),  // top
      Vector3(0.0f, -1.0f, 0.0f)  // bottom
  };

  // For each vertex, average normals of adjacent faces
  std::vector<Vector3> vertexNormals(8, Vector3::Zero);

  // Back face (Z-): 0,1,2,3
  for (int i = 0; i < 4; i++)
    vertexNormals[i] += faceNormals[0];
  // Right face (X+): 1,5,6,2
  vertexNormals[1] += faceNormals[1];
  vertexNormals[5] += faceNormals[1];
  vertexNormals[6] += faceNormals[1];
  vertexNormals[2] += faceNormals[1];
  // Front face (Z+): 5,4,7,6
  for (int i = 4; i < 8; i++)
    vertexNormals[i] += faceNormals[2];
  // Left face (X-): 4,0,3,7
  vertexNormals[4] += faceNormals[3];
  vertexNormals[0] += faceNormals[3];
  vertexNormals[3] += faceNormals[3];
  vertexNormals[7] += faceNormals[3];
  // Top face (Y+): 3,2,6,7
  vertexNormals[3] += faceNormals[4];
  vertexNormals[2] += faceNormals[4];
  vertexNormals[6] += faceNormals[4];
  vertexNormals[7] += faceNormals[4];
  // Bottom face (Y-): 4,5,1,0
  vertexNormals[4] += faceNormals[5];
  vertexNormals[5] += faceNormals[5];
  vertexNormals[1] += faceNormals[5];
  vertexNormals[0] += faceNormals[5];

  // Normalize the averaged normals
  for (auto &normal : vertexNormals) {
    normal.Normalize();
  }

  // Create vertices
  for (int i = 0; i < 8; i++) {
    data.vertices.push_back(Vertex(positions[i], vertexNormals[i]));
  }

  // Define triangles with shared vertices
  // Back face
  data.triangles.push_back(Triangle(0, 2, 1));
  data.triangles.push_back(Triangle(0, 3, 2));
  // Right face
  data.triangles.push_back(Triangle(1, 2, 6));
  data.triangles.push_back(Triangle(1, 6, 5));
  // Front face
  data.triangles.push_back(Triangle(5, 6, 7));
  data.triangles.push_back(Triangle(5, 7, 4));
  // Left face
  data.triangles.push_back(Triangle(4, 7, 3));
  data.triangles.push_back(Triangle(4, 3, 0));
  // Top face
  data.triangles.push_back(Triangle(3, 6, 2));
  data.triangles.push_back(Triangle(3, 7, 6));
  // Bottom face
  data.triangles.push_back(Triangle(4, 1, 5));
  data.triangles.push_back(Triangle(4, 0, 1));

  return data;
}

PlaneMesh::PlaneMesh() { Build(GeneratePlaneData()); }

MeshData PlaneMesh::GeneratePlaneData() {
  MeshData data;

  Vector3 normal(0.0f, 1.0f, 0.0f);
  data.vertices.push_back(Vertex(Vector3(-0.5f, 0.0f, 0.5f), normal));
  data.vertices.push_back(Vertex(Vector3(0.5f, 0.0f, 0.5f), normal));
  data.vertices.push_back(Vertex(Vector3(0.5f, 0.0f, -0.5f), normal));
  data.vertices.push_back(Vertex(Vector3(-0.5f, 0.0f, -0.5f), normal));
  data.triangles.push_back(Triangle(0, 2, 1));
  data.triangles.push_back(Triangle(0, 3, 2));

  return data;
}

// ============== PyramidMesh Class ==============

PyramidMesh::PyramidMesh() { Build(GeneratePyramidData()); }

MeshData PyramidMesh::GeneratePyramidData() {
  MeshData data;

  Vector3 apex(0.0f, 0.5f, 0.0f);
  float baseHeight = -0.5f;

  // Front face (Z+)
  {
    Vector3 v0(-0.5f, baseHeight, 0.5f);
    Vector3 v1(0.5f, baseHeight, 0.5f);
    Vector3 edge1 = v1 - v0;
    Vector3 edge2 = apex - v0;
    Vector3 normal = Vector3::Cross(edge1, edge2);
    normal.Normalize();

    unsigned int baseIdx = data.vertices.size();
    data.vertices.push_back(Vertex(v0, normal));
    data.vertices.push_back(Vertex(v1, normal));
    data.vertices.push_back(Vertex(apex, normal));
    data.triangles.push_back(Triangle(baseIdx + 0, baseIdx + 2, baseIdx + 1));
  }

  // Right face (X+)
  {
    Vector3 v0(0.5f, baseHeight, 0.5f);
    Vector3 v1(0.5f, baseHeight, -0.5f);
    Vector3 edge1 = v1 - v0;
    Vector3 edge2 = apex - v0;
    Vector3 normal = Vector3::Cross(edge1, edge2);
    normal.Normalize();

    unsigned int baseIdx = data.vertices.size();
    data.vertices.push_back(Vertex(v0, normal));
    data.vertices.push_back(Vertex(v1, normal));
    data.vertices.push_back(Vertex(apex, normal));
    data.triangles.push_back(Triangle(baseIdx + 0, baseIdx + 2, baseIdx + 1));
  }

  // Back face (Z-)
  {
    Vector3 v0(0.5f, baseHeight, -0.5f);
    Vector3 v1(-0.5f, baseHeight, -0.5f);
    Vector3 edge1 = v1 - v0;
    Vector3 edge2 = apex - v0;
    Vector3 normal = Vector3::Cross(edge1, edge2);
    normal.Normalize();

    unsigned int baseIdx = data.vertices.size();
    data.vertices.push_back(Vertex(v0, normal));
    data.vertices.push_back(Vertex(v1, normal));
    data.vertices.push_back(Vertex(apex, normal));
    data.triangles.push_back(Triangle(baseIdx + 0, baseIdx + 2, baseIdx + 1));
  }

  // Left face (X-)
  {
    Vector3 v0(-0.5f, baseHeight, -0.5f);
    Vector3 v1(-0.5f, baseHeight, 0.5f);
    Vector3 edge1 = v1 - v0;
    Vector3 edge2 = apex - v0;
    Vector3 normal = Vector3::Cross(edge1, edge2);
    normal.Normalize();

    unsigned int baseIdx = data.vertices.size();
    data.vertices.push_back(Vertex(v0, normal));
    data.vertices.push_back(Vertex(v1, normal));
    data.vertices.push_back(Vertex(apex, normal));
    data.triangles.push_back(Triangle(baseIdx + 0, baseIdx + 2, baseIdx + 1));
  }

  // Bottom face (Y-)
  {
    Vector3 normal(0.0f, -1.0f, 0.0f);

    unsigned int baseIdx = data.vertices.size();
    data.vertices.push_back(Vertex(Vector3(-0.5f, baseHeight, -0.5f), normal));
    data.vertices.push_back(Vertex(Vector3(0.5f, baseHeight, -0.5f), normal));
    data.vertices.push_back(Vertex(Vector3(0.5f, baseHeight, 0.5f), normal));
    data.vertices.push_back(Vertex(Vector3(-0.5f, baseHeight, 0.5f), normal));

    data.triangles.push_back(Triangle(baseIdx + 0, baseIdx + 2, baseIdx + 1));
    data.triangles.push_back(Triangle(baseIdx + 0, baseIdx + 3, baseIdx + 2));
  }

  return data;
}
/*
      Vector3 normal(0.0f, 1.0f, 0.0f);
        unsigned int baseIdx = data.vertices.size();
        data.vertices.push_back(Vertex(Vector3(-0.5f, 0.5f, 0.5f), normal,
   Vector2(0.0f, 0.0f))); data.vertices.push_back(Vertex(Vector3(0.5f, 0.5f,
   0.5f), normal, Vector2(1.0f, 0.0f)));
        data.vertices.push_back(Vertex(Vector3(0.5f, 0.5f, -0.5f), normal,
   Vector2(1.0f, 1.0f))); data.vertices.push_back(Vertex(Vector3(-0.5f, 0.5f,
   -0.5f), normal, Vector2(0.0f, 1.0f)));
        data.triangles.push_back(Triangle(baseIdx+0, baseIdx+2, baseIdx+1,
   tile4)); data.triangles.push_back(Triangle(baseIdx+0, baseIdx+3, baseIdx+2,
   tile4));
*/

// ============== SphereMesh Class (Icosahedron) ==============

SphereMesh::SphereMesh() { Build(GenerateIcosahedronData()); }

MeshData SphereMesh::GenerateIcosahedronData() {
  MeshData data;

  // Golden ratio for icosahedron
  const float phi = (1.0f + Math::Sqrt(5.0f)) / 2.0f;
  const float a = 0.5f;
  const float b = a / phi;

  // 12 vertices of an icosahedron (scaled to fit in unit sphere)
  // Normalize to make it a unit sphere
  std::vector<Vector3> positions = {
      Vector3(-b, a, 0.0f),  // 0
      Vector3(b, a, 0.0f),   // 1
      Vector3(-b, -a, 0.0f), // 2
      Vector3(b, -a, 0.0f),  // 3

      Vector3(0.0f, -b, a),  // 4
      Vector3(0.0f, b, a),   // 5
      Vector3(0.0f, -b, -a), // 6
      Vector3(0.0f, b, -a),  // 7

      Vector3(a, 0.0f, -b),  // 8
      Vector3(a, 0.0f, b),   // 9
      Vector3(-a, 0.0f, -b), // 10
      Vector3(-a, 0.0f, b)   // 11
  };

  // Normalize all positions to make them lie on a unit sphere
  for (auto &pos : positions) {
    pos.Normalize();
    pos = pos * 0.5f; // Scale to fit in [-0.5, 0.5] bounds
  }

  // 20 triangular faces of an icosahedron
  std::vector<std::array<unsigned int, 3>> faces = {// 5 faces around point 0
                                                    {{0, 11, 5}},
                                                    {{0, 5, 1}},
                                                    {{0, 1, 7}},
                                                    {{0, 7, 10}},
                                                    {{0, 10, 11}},

                                                    // 5 adjacent faces
                                                    {{1, 5, 9}},
                                                    {{5, 11, 4}},
                                                    {{11, 10, 2}},
                                                    {{10, 7, 6}},
                                                    {{7, 1, 8}},

                                                    // 5 faces around point 3
                                                    {{3, 9, 4}},
                                                    {{3, 4, 2}},
                                                    {{3, 2, 6}},
                                                    {{3, 6, 8}},
                                                    {{3, 8, 9}},

                                                    // 5 adjacent faces
                                                    {{4, 9, 5}},
                                                    {{2, 4, 11}},
                                                    {{6, 2, 10}},
                                                    {{8, 6, 7}},
                                                    {{9, 8, 1}}};

  // Calculate vertex normals by averaging face normals
  std::vector<Vector3> normals(positions.size(), Vector3::Zero);
  std::vector<int> normalCounts(positions.size(), 0);

  for (const auto &face : faces) {
    // Calculate face normal
    Vector3 v0 = positions[face[0]];
    Vector3 v1 = positions[face[1]];
    Vector3 v2 = positions[face[2]];

    Vector3 edge1 = v1 - v0;
    Vector3 edge2 = v2 - v0;
    Vector3 faceNormal = Vector3::Cross(edge1, edge2);
    faceNormal.Normalize();

    // Add to vertex normals
    normals[face[0]] += faceNormal;
    normals[face[1]] += faceNormal;
    normals[face[2]] += faceNormal;
    normalCounts[face[0]]++;
    normalCounts[face[1]]++;
    normalCounts[face[2]]++;
  }

  // Average and normalize vertex normals
  for (size_t i = 0; i < normals.size(); ++i) {
    if (normalCounts[i] > 0) {
      normals[i] = normals[i] * (1.0f / normalCounts[i]);
      normals[i].Normalize();
    }
  }

  // Create vertices with normals
  for (size_t i = 0; i < positions.size(); ++i) {
    data.vertices.push_back(Vertex(positions[i], normals[i]));
  }

  // Create triangles
  for (const auto &face : faces) {
    data.triangles.push_back(Triangle(face[0], face[1], face[2]));
  }

  return data;
}

// ============== ConeMesh Class ==============

ConeMesh::ConeMesh() { Build(GenerateConeData()); }

MeshData ConeMesh::GenerateConeData() {
  MeshData data;

  const int segments = 8;
  const float radius = 0.5f;
  const float height = 1.0f;

  // Apex
  Vector3 apex(0.0f, height / 2.0f, 0.0f);
  data.vertices.push_back(Vertex(apex, Vector3(0.0f, 1.0f, 0.0f)));

  // Base vertices
  for (int i = 0; i < segments; i++) {
    float angle = 2.0f * Math::Pi * i / segments;
    Vector3 pos(radius * Math::Cos(angle), -height / 2.0f,
                radius * Math::Sin(angle));
    Vector3 normal =
        Vector3::Normalize(pos - Vector3(0.0f, -height / 2.0f, 0.0f));
    data.vertices.push_back(Vertex(pos, normal));
  }

  // Triangles for sides
  for (int i = 0; i < segments; i++) {
    int next = (i + 1) % segments;
    data.triangles.push_back(Triangle(0, 1 + i, 1 + next));
  }

  return data;
}

void Mesh::SetupInstanceBuffer(size_t maxInstances) {
  mMaxInstances = maxInstances;

  if (mInstanceBuffer != 0) {
    glDeleteBuffers(1, &mInstanceBuffer);
  }

  // Bind VAO to modify it
  glBindVertexArray(mVertexArray);

  // Create instance buffer
  glGenBuffers(1, &mInstanceBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, mInstanceBuffer);

  // Allocate buffer (35 floats per instance: 16 + 16 + 3)
  glBufferData(GL_ARRAY_BUFFER, maxInstances * 35 * sizeof(float), nullptr,
               GL_DYNAMIC_DRAW);

  // Setup instance attribute pointers
  // Model matrix (mat4) - locations 4, 5, 6, 7
  for (int i = 0; i < 4; i++) {
    glEnableVertexAttribArray(4 + i);
    glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, 35 * sizeof(float),
                          (void *)(i * 4 * sizeof(float)));
    glVertexAttribDivisor(4 + i, 1); // Advance once per instance
  }

  // Normal matrix (mat4) - locations 8, 9, 10, 11
  for (int i = 0; i < 4; i++) {
    glEnableVertexAttribArray(8 + i);
    glVertexAttribPointer(8 + i, 4, GL_FLOAT, GL_FALSE, 35 * sizeof(float),
                          (void *)((16 + i * 4) * sizeof(float)));
    glVertexAttribDivisor(8 + i, 1); // Advance once per instance
  }

  // Color (vec3) - location 12
  glEnableVertexAttribArray(12);
  glVertexAttribPointer(12, 3, GL_FLOAT, GL_FALSE, 35 * sizeof(float),
                        (void *)(32 * sizeof(float)));
  glVertexAttribDivisor(12, 1); // Advance once per instance

  // Unbind
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::UpdateInstanceBuffer(const std::vector<float> &instanceData,
                                size_t instanceCount) {
  if (mInstanceBuffer == 0) {
    std::cerr << "Instance buffer not set up. Call SetupInstanceBuffer first."
              << std::endl;
    return;
  }

  if (instanceCount > mMaxInstances) {
    std::cerr << "Instance count (" << instanceCount
              << ") exceeds max instances (" << mMaxInstances << ")"
              << std::endl;
    return;
  }

  glBindBuffer(GL_ARRAY_BUFFER, mInstanceBuffer);
  glBufferSubData(GL_ARRAY_BUFFER, 0, instanceData.size() * sizeof(float),
                  instanceData.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}
