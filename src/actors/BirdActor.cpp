#include "actors/BirdActor.hpp"

#include "Scene.hpp"
#include "render/Renderer.hpp"

BirdActor::BirdActor(Scene *scene) : Actor(scene), mflapTimer(0.0f) {

  Mesh *pyramidMesh = scene->GetRenderer()->LoadMesh("pyramid");

  // Mesh *cubeMesh = scene->GetRenderer()->LoadMesh("cube");
  mBody = new MeshComponent(this, *pyramidMesh, Vector3(0.0f, 0.0f, 0.0f),
                            Vector3(0.75f, 2.0f, 0.75f),
                            Quaternion(Vector3::UnitX, -Math::PiOver2));
  mBody->SetColor(Color::White);

  mLeftWing = new MeshComponent(this, *pyramidMesh, Vector3(-0.5f, 0.0f, 0.0f),
                                Vector3(0.1f, 2.0f, 1.5f));
  mLeftWing->SetColor(Color::White);

  mRightWing = new MeshComponent(this, *pyramidMesh, Vector3(0.5f, 0.0f, 0.0f),
                                 Vector3(0.1f, 2.0f, 1.5f));

  mRightWing->SetColor(Color::White);

  mBeak = new MeshComponent(this, *pyramidMesh, Vector3(0.0f, 0.0f, 1.25f),
                            Vector3(0.5f, 0.5f, 0.5f),
                            Quaternion(Vector3::UnitX, Math::PiOver2));
  mBeak->SetColor(Color::Red);

  mTail = new MeshComponent(this, *pyramidMesh, Vector3(0.0f, 0.0f, -1.25f),
                            Vector3(1.5f, 0.5f, 0.2f),
                            Quaternion(Vector3::UnitX, Math::PiOver2));
  mTail->SetColor(Color::White);
}

void BirdActor::OnUpdate(float deltaTime) {
  // Flap wings
  mflapTimer += deltaTime * 5.0f;
  float wingAngle = Math::Sin(mflapTimer) * Math::ToRadians(45.0f);

  mLeftWing->SetRotation(
      Quaternion::Concatenate(Quaternion(Vector3::UnitZ, Math::PiOver2),
                              Quaternion(Vector3::UnitZ, wingAngle)));
  mLeftWing->SetOffset(Vector3(-1.0f, 1.0f * Math::Sin(-wingAngle), 0.0f));

  mRightWing->SetRotation(
      Quaternion::Concatenate(Quaternion(Vector3::UnitZ, -Math::PiOver2),
                              Quaternion(Vector3::UnitZ, -wingAngle)));
  mRightWing->SetOffset(Vector3(1.0f, 1.0f * Math::Sin(-wingAngle), 0.0f));
  // mRightWing->SetRotation(rightWingRot);

  Vector3 pos =
      Vector3::Transform(GetPosition(), Matrix4::CreateRotationY(deltaTime));
  // SetPosition(pos);

  mRotation = Quaternion::Concatenate(
      mRotation,
      Quaternion(Vector3::UnitY, deltaTime * Math::ToRadians(20.0f)));
}
