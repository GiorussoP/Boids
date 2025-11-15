#include "actors/BirdActor.hpp"

#include "Scene.hpp"
#include "render/Renderer.hpp"
#include <algorithm>

BirdActor::BirdActor(Scene *scene, BirdType type)
    : Actor(scene), mflapTimer(((float)(std::rand() % 100)) / Math::Pi),
      mPrevYaw(0.0f), mType(type) {

  Mesh *pyramidMesh = scene->GetRenderer()->LoadMesh("pyramid");

  // Mesh *cubeMesh = scene->GetRenderer()->LoadMesh("cube");
  mBody = new MeshComponent(this, *pyramidMesh, Vector3(0.0f, 0.0f, 0.0f),
                            Vector3(0.75f, 2.0f, 0.75f),
                            Quaternion(Vector3::UnitX, -Math::PiOver2));

  mLeftWing = new MeshComponent(this, *pyramidMesh, Vector3(-0.5f, 0.0f, 0.0f),
                                Vector3(0.1f, 2.0f, 1.5f));

  mRightWing = new MeshComponent(this, *pyramidMesh, Vector3(0.5f, 0.0f, 0.0f),
                                 Vector3(0.1f, 2.0f, 1.5f));

  mBeak = new MeshComponent(this, *pyramidMesh, Vector3(0.0f, 0.0f, 1.25f),
                            Vector3(0.5f, 0.5f, 0.5f),
                            Quaternion(Vector3::UnitX, Math::PiOver2));
  mBeak->SetColor(Color::Orange);

  mTail = new MeshComponent(this, *pyramidMesh, Vector3(0.0f, 0.0f, -1.25f),
                            Vector3(1.5f, 0.5f, 0.2f),
                            Quaternion(Vector3::UnitX, Math::PiOver2));

  Vector3 color = (type == Leader) ? Color::Yellow : Color::White;
  mBody->SetColor(color);
  mLeftWing->SetColor(color);
  mRightWing->SetColor(color);
  mTail->SetColor(color);

  mRigidBody = new RigidBodyComponent(this, 1.0f, 0.0f);

  mScale = Vector3(0.3f, 0.3f, 0.3f);
}

void BirdActor::OnUpdate(float deltaTime) {
  // Animation
  mflapTimer += deltaTime * 25.0f;
  float wingAngle = Math::Sin(mflapTimer) * Math::ToRadians(45.0f);

  mLeftWing->SetRotation(
      Quaternion::Concatenate(Quaternion(Vector3::UnitZ, Math::PiOver2),
                              Quaternion(Vector3::UnitZ, wingAngle)));
  mLeftWing->SetOffset(Vector3(-1.0f, 1.0f * Math::Sin(-wingAngle), 0.0f));
  mRightWing->SetRotation(
      Quaternion::Concatenate(Quaternion(Vector3::UnitZ, -Math::PiOver2),
                              Quaternion(Vector3::UnitZ, -wingAngle)));
  mRightWing->SetOffset(Vector3(1.0f, 1.0f * Math::Sin(-wingAngle), 0.0f));

  // Behaviour
  if (mType == Leader) {
    // Orbit the origin
    mRigidBody->SetVelocity(
        Vector3::Normalize(Vector3(
            -mPosition.z, Math::Sin(mPosition.x * 0.1f) * 10.0f, mPosition.x)) *
        20.0f);

  } else {
    BirdActor *leader = mScene->GetLeader();
    Vector3 toLeader = leader->GetPosition() - GetPosition();
    toLeader.Normalize();
    Vector3 desiredToLeader = toLeader * 25.0f;

    const float neighborRadius = 20.0f;
    Vector3 avgVelocity = Vector3::Zero;
    int neighborCount = 0;

    for (auto actor : mScene->GetActors()) {
      BirdActor *otherBird = dynamic_cast<BirdActor *>(actor);
      if (otherBird != nullptr && otherBird != this) {
        Vector3 toOther = otherBird->GetPosition() - GetPosition();
        float distSq = toOther.LengthSq();
        if (distSq < neighborRadius * neighborRadius) {
          avgVelocity +=
              otherBird->GetComponent<RigidBodyComponent>()->GetVelocity();
          neighborCount++;
        }
      }
    }

    Vector3 desiredVelocity;
    if (neighborCount > 0) {
      avgVelocity = avgVelocity * (1.0f / (float)neighborCount);
      desiredVelocity = (desiredToLeader + avgVelocity) * 0.5f;
    } else {
      desiredVelocity = desiredToLeader;
    }

    Vector3 steering = desiredVelocity - mRigidBody->GetVelocity();
    mRigidBody->ApplyForce(steering);

    // Avoid crowding
    for (auto actor : mScene->GetActors()) {
      BirdActor *otherBird = dynamic_cast<BirdActor *>(actor);
      if (otherBird != nullptr && otherBird != this) {
        Vector3 toOther = otherBird->GetPosition() - GetPosition();
        float distSq = toOther.LengthSq();
        if (distSq < 10.0f) {
          toOther.Normalize();
          mRigidBody->ApplyForce(-1.0f * toOther * 50.0f);
        }
      }
    }
  }

  // Rotation
  Vector3 velocity = mRigidBody->GetVelocity();
  if (velocity.LengthSq() > 0.0f) {
    velocity.Normalize();
    float pitch = Math::ASin(-velocity.y);
    float yaw = Math::Atan2(velocity.x, velocity.z);
    float deltaYaw = yaw - mPrevYaw;
    mPrevYaw = yaw;
    float roll = Math::Clamp(deltaYaw * 10.0f, -Math::Pi / 2, Math::Pi / 2);
    SetRotation(Quaternion::Concatenate(
        Quaternion::Concatenate(
            Quaternion(Vector3::UnitY, yaw),

            Quaternion(Vector3::Normalize(velocity), pitch)),

        Quaternion(Vector3::UnitZ, roll)));
  }
}