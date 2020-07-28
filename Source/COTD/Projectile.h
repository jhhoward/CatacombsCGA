#pragma once

#include <stdint.h>
#include "Entity.h"

#define PLAYER_OWNER_ID 0xff
#define MAX_PROJECTILES 4

class Projectile : public Entity
{
public:
	uint8_t angle;
	uint8_t life;
	uint8_t ownerId;

	Entity* GetOwner() const;
};

class ProjectileManager
{
public:
	static Projectile projectiles[MAX_PROJECTILES];

	static Projectile* FireProjectile(Entity* owner, int16_t x, int16_t y, uint8_t angle);
	static void Init();
	static void Draw();
	static void Update();
};
