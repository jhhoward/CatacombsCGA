#pragma once

#include <stdint.h>
#include "Entity.h"
#include "FixedMath.h"

#define PLAYER_MAX_HP 100
#define PLAYER_MAX_MANA 100
#define PLAYER_MANA_FIRE_COST 20
#define PLAYER_MANA_RECHARGE_RATE 1
#define PLAYER_ATTACK_STRENGTH 10
#define PLAYER_COLLISION_SIZE 48
#define PLAYER_LOOK_AHEAD_DISTANCE 60
#define PLAYER_POTION_STRENGTH 25

class Player : public Entity
{
public:
	angle_t angle;
	int16_t velocityX, velocityY;
	int8_t angularVelocity;

	uint8_t shakeTime;
	uint8_t damageTime;
	uint8_t reloadTime;

	uint8_t hp;
	uint8_t mana;

	void Init();
	void NextLevel();
	void Tick();
	void Fire();
	void Move(int16_t deltaX, int16_t deltaY);
	bool CheckCollisions();
	void Damage(uint8_t amount);
	bool IsWorldColliding() const;
};
