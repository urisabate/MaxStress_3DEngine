#ifndef __PhysBody3D_H__
#define __PhysBody3D_H__

#include "p2List.h"

class btRigidBody;
class Module;
class btQuaternion;
class btVector3;

enum SensorType {
	None = 0,
	LapSensor,
	HalfLap,
	Car1,
	Car2,
	Respawn
};

// =================================================
struct PhysBody3D
{
	friend class ModulePhysics3D;

public:
	PhysBody3D(btRigidBody* body);
	~PhysBody3D();

	void Push(float x, float y, float z);
	void GetTransform(float* matrix) const;
	void SetTransform(const float* matrix) const;
	void SetSensor()const;
	void SetPos(float x, float y, float z);
	btVector3 GetPos() const;
	void SetRotation(btQuaternion rotation) const;
	btQuaternion GetRotation() const;

private:
	btRigidBody* body = nullptr;

public:
	p2List<Module*> collision_listeners;
	SensorType type = None;
};

#endif // __PhysBody3D_H__