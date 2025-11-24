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
  // Behaviour
  if (mType == Leader) {
    Vector3 center(40.0f, 0.0f, -90.0f);
    Vector3 offset = mPosition - center;
    Vector3 desiredVel =
        Vector3::Normalize(Vector3(
            -offset.z, std::round(Math::Sin(offset.x * 0.1f)) * 55.0f - 7.0f,
            offset.x)) *
        10.0f;

    // Prevent getting too high
    if (mPosition.y > 450.0f) {
      desiredVel.y = Math::Min(desiredVel.y, -20.0f);
    }
    if (mPosition.y < 225.0f) {
      desiredVel.y = Math::Max(desiredVel.y, +20.0f);
    }

    mRigidBody->SetVelocity(desiredVel);

  } else {
    BirdActor *leader = mScene->GetLeader();
    Vector3 toLeader = leader->GetPosition() - GetPosition();
    toLeader.Normalize();
    Vector3 desiredToLeader = toLeader * 50.0f;

    const float neighborRadius = 50.0f;
    Vector3 avgVelocity = Vector3::Zero;
    Vector3 flockCenter = Vector3::Zero;
    int neighborCount = 0;

    for (auto actor : mScene->GetActors()) {
      BirdActor *otherBird = dynamic_cast<BirdActor *>(actor);
      if (otherBird != nullptr && otherBird != this) {
        Vector3 toOther = otherBird->GetPosition() - GetPosition();
        float distSq = toOther.LengthSq();
        if (distSq < neighborRadius * neighborRadius) {
          avgVelocity += otherBird->GetRigidBody().GetVelocity();
          flockCenter += otherBird->GetPosition();
          neighborCount++;
        }
      }
    }

    Vector3 desiredVelocity;
    if (neighborCount > 0) {
      avgVelocity = avgVelocity * (1.0f / (float)neighborCount);
      flockCenter = flockCenter * (1.0f / (float)neighborCount);
      Vector3 toFlockCenter = flockCenter - GetPosition();
      toFlockCenter.Normalize();
      Vector3 desiredToCenter = toFlockCenter;
      desiredVelocity =
          (desiredToLeader + avgVelocity + desiredToCenter) * (1.0f / 3.0f);
    } else {
      desiredVelocity = desiredToLeader;
    }

    Vector3 steering = desiredVelocity - mRigidBody->GetVelocity();
    if (steering.LengthSq() > 0.1f) {
      mRigidBody->ApplyForce(steering);
    }

    // Avoid crowding
    for (auto actor : mScene->GetActors()) {
      BirdActor *otherBird = dynamic_cast<BirdActor *>(actor);
      if (otherBird != nullptr && otherBird != this) {
        Vector3 toOther = otherBird->GetPosition() - GetPosition();
        float distSq = toOther.LengthSq();
        if (distSq < 30.0f) {
          Vector3 repulsion =
              -10.0f * toOther * (1.0f / std::max(distSq, 0.0001f));
          if (repulsion.LengthSq() > 0.1f) {
            mRigidBody->ApplyForce(repulsion);
          }
        }
      }
    }

    // Terrain avoidance
    float height = mScene->GetRenderer()->GetHeightAt(mPosition.x, mPosition.z);
    const float margin = 10.0f;
    if (mPosition.y < height + margin) {
      Vector3 normal =
          mScene->GetRenderer()->GetNormalAt(mPosition.x, mPosition.z);
      float distance = height + margin - mPosition.y;
      Vector3 force = normal * 50.0f * distance;
      mRigidBody->ApplyForce(force);
    }

    // Anticipate terrain ahead
    Vector3 vel = mRigidBody->GetVelocity();
    if (vel.LengthSq() > 0.1f) {
      Vector3 aheadVel = vel;
      aheadVel.Normalize();
      Vector3 ahead = mPosition + aheadVel * 5.0f;
      float heightAhead = mScene->GetRenderer()->GetHeightAt(ahead.x, ahead.z);
      if (heightAhead > mPosition.y + 2.0f) {
        Vector3 normal =
            mScene->GetRenderer()->GetNormalAt(mPosition.x, mPosition.z);
        Vector3 force = normal * 100.0f;
        mRigidBody->ApplyForce(force);
      }
    }
  }

  // Rotation

  Vector3 velocity = mRigidBody->GetVelocity();
  velocity.Normalize();
  float pitch = -Math::ASin(velocity.y);
  float yaw = Math::Atan2(velocity.x, velocity.z);
  float deltaYaw = yaw - mPrevYaw;
  mPrevYaw = yaw;
  if (velocity.LengthSq() > 0.1f) {

    float roll =
        -Math::Clamp(deltaYaw / deltaTime, -Math::Pi / 4.0f, Math::Pi / 4.0f);

    Quaternion targetRotation = Quaternion::Concatenate(
        Quaternion::Concatenate(Quaternion(Vector3::UnitZ, roll),
                                Quaternion(Vector3::UnitX, pitch)),
        Quaternion(Vector3::UnitY, yaw));

    SetRotation(Quaternion::Slerp(mRotation, targetRotation,
                                  Math::Clamp(20.0f * deltaTime, 0.0f, 1.0f)));
  }

  // Animation
  float turnAmmount = -Math::Clamp(deltaYaw / Math::Max(deltaTime, 0.0001f),
                                   -Math::PiOver2 / 3.0f, Math::PiOver2 / 3.0f);

  if (mRigidBody->GetVelocity().y >= -8.0f) {

    mflapTimer +=
        deltaTime * (2000.f / Math::Clamp(Vector2(mRigidBody->GetVelocity().x,
                                                  mRigidBody->GetVelocity().z)
                                              .LengthSq(),
                                          80.0f, 100.0f));

    float wingAngle = Math::Sin(mflapTimer) * Math::ToRadians(45.0f);

    mLeftWing->SetRotation(
        Quaternion::Concatenate(Quaternion(Vector3::UnitZ, Math::PiOver2),
                                Quaternion(Vector3::UnitZ, wingAngle)));
    mLeftWing->SetOffset(Vector3(-1.0f, Math::Sin(-wingAngle), 0.0f));

    mRightWing->SetRotation(
        Quaternion::Concatenate(Quaternion(Vector3::UnitZ, -Math::PiOver2),
                                Quaternion(Vector3::UnitZ, -wingAngle)));
    mRightWing->SetOffset(Vector3(1.0f, Math::Sin(-wingAngle), 0.0f));
  } else {

    mLeftWing->SetRotation(
        Quaternion::Concatenate(Quaternion(Vector3::UnitZ, Math::PiOver2),
                                Quaternion(Vector3::UnitZ, turnAmmount)));
    mLeftWing->SetOffset(Vector3(-1.0f, Math::Sin(-turnAmmount), 0.0f));

    mRightWing->SetRotation(
        Quaternion::Concatenate(Quaternion(Vector3::UnitZ, -Math::PiOver2),
                                Quaternion(Vector3::UnitZ, turnAmmount)));
    mRightWing->SetOffset(Vector3(1.0f, Math::Sin(turnAmmount), 0.0f));
  }
}
