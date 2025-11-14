#ifndef ACTOR_HPP
#define ACTOR_HPP
#include "ColliderComponent.hpp"
#include "Input.hpp"
#include "Math.hpp"
#include <SDL2/SDL.h>
#include <vector>

enum class ActorState { Active, Paused, Destroy };

class Actor {
public:
  Actor(class Scene *Scene);
  virtual ~Actor();

  // Update function called from Scene (not overridable)
  void Update(float deltaTime);

  // ProcessInput function called from Scene (not overridable)
  void ProcessInput();

  // Position getter/setter
  const Vector3 &GetPosition() const { return mPosition; }
  void SetPosition(const Vector3 pos);

  // Scale getter/setter
  Vector3 GetScale() const { return mScale; }
  void SetScale(const Vector3 scale) { mScale = scale; }

  // Rotation getter/setter (for 2D sprites, rotation around Y axis)
  Quaternion GetRotation() const { return mRotation; }
  void SetRotation(const Quaternion rotation) { mRotation = rotation; }

  // State getter/setter
  ActorState GetState() const { return mState; }
  void SetState(ActorState state) { mState = state; }

  // Scene getter
  class Scene *GetScene() { return mScene; }

  // Components getter
  const std::vector<class Component *> &GetComponents() const {
    return mComponents;
  }

  // Called when the actor collides
  virtual void OnCollision(Vector3 penetration, ColliderComponent *other) {
    (void)penetration;
    (void)other;
  };

  // Returns component of type T, or null if doesn't exist
  template <typename T> T *GetComponent() const {
    for (auto c : mComponents) {

      T *t = dynamic_cast<T *>(c);
      if (t != nullptr) {
        return t;
      }
    }
    return nullptr;
  }

protected:
  class Scene *mScene;

  // Any actor-specific update code (overridable)
  virtual void OnUpdate(float deltaTime) { (void)deltaTime; };

  // Any actor-specific input code (overridable)
  virtual void OnProcessInput() {};

  // Actor's state
  ActorState mState;

  // Transform
  Vector3 mPosition;
  Vector3 mScale;
  Quaternion mRotation;

  // Components
  std::vector<class Component *> mComponents;

private:
  friend class Component;

  // Adds component to Actor (called automatically in component constructor)
  void AddComponent(class Component *c);
};

#endif