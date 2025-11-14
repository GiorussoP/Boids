#ifndef MESH_COMPONENT_HPP
#define MESH_COMPONENT_HPP
#include "Math.hpp"
#include "Mesh.hpp"
#include "components/Component.hpp"
#include <vector>

class MeshComponent : public Component {
public:
  MeshComponent(class Actor *owner, Mesh &mesh, Vector3 offset = Vector3::Zero,
                Vector3 scale = Vector3::One,
                Quaternion rotation = Quaternion::Identity,
                Vector3 color = Color::Red);
  ~MeshComponent();

  bool IsVisible() const { return mIsVisible; }
  void SetIsVisible(bool isVisible) { mIsVisible = isVisible; }

  Vector3 GetOffset() const { return mOffset; }
  void SetOffset(const Vector3 &offset) { mOffset = offset; }

  Vector3 GetScale() const { return mScale; }
  void SetScale(const Vector3 &scale) { mScale = scale; }

  Quaternion GetRotation() const { return mRotation; }
  void SetRotation(const Quaternion &rotation) { mRotation = rotation; }

  Vector3 GetColor() const { return mColor; }
  void SetColor(const Vector3 &color) { mColor = color; }

  Mesh &GetMesh() const { return mMesh; }

protected:
  Mesh &mMesh;
  Vector3 mOffset;
  Vector3 mScale;
  Quaternion mRotation;
  Vector3 mColor;

  bool mIsVisible;
};

#endif