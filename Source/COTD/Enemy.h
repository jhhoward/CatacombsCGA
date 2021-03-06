#pragma once

#include <stdint.h>
#include "Entity.h"
#include "Defines.h"
#include "Draw.h"

#define MAX_ENEMIES 24

enum EnemyType
{
	ET_Skeleton,
	ET_Mage,
	ET_Bat,
	ET_Spider,
	NumEnemyTypes,
	ET_None = NumEnemyTypes
};

enum EnemyState
{
	ES_Idle,
	ES_Moving,
	ES_Attacking,
	ES_Stunned,
	ES_Dying,
	ES_Dead
};

struct EnemyArchetype
{
	drawRoutine_t drawRoutine;

	uint8_t hp;
	uint8_t movementSpeed;
	uint8_t attackStrength;
	uint8_t attackDuration;
	uint8_t stunDuration;
	uint8_t isRanged;
	uint8_t spriteScale;

	uint8_t GetHP() const				{ return hp; }
	uint8_t GetMovementSpeed() const	{ return movementSpeed; }
	uint8_t GetAttackStrength() const	{ return attackStrength; }
	uint8_t GetAttackDuration() const	{ return attackDuration; }
	uint8_t GetStunDuration() const		{ return stunDuration; }
	bool GetIsRanged() const			{ return isRanged != 0; }
	uint8_t GetSpriteScale() const		{ return spriteScale; }
};

class Enemy : public Entity
{
public:
	void Init(EnemyType type, int16_t x, int16_t y);
	void Tick();
	bool IsValid() const { return type != ET_None; }
	void Damage(uint8_t amount);
	void Clear() { type = ET_None; }
	const EnemyArchetype* GetArchetype() const;
	EnemyState GetState() const { return state; }
	EnemyType GetType() const { return type; }

private:
	static const EnemyArchetype archetypes[NumEnemyTypes];

	bool CanSeePlayer() const;

	bool ShouldFireProjectile() const;
	bool FireProjectile(angle_t angle);
	bool TryMove();
	void StunMove();
	bool TryFireProjectile();
	void PickNewTargetCell();
	bool TryPickCells(int8_t deltaX, int8_t deltaY);
	bool TryPickCell(int8_t newX, int8_t newY);
	uint8_t GetPlayerCellDistance() const;

	EnemyType type;
	EnemyState state;
	uint8_t frameDelay;
	uint8_t hp;
	uint8_t targetCellX, targetCellY;
};

class EnemyManager
{
public:
	static Enemy enemies[MAX_ENEMIES];
	
	static void Spawn(EnemyType enemyType, int16_t x, int16_t y);
	static void SpawnEnemies();

	static Enemy* GetOverlappingEnemy(Entity& entity);
	static Enemy* GetOverlappingEnemy(int16_t x, int16_t y);
	
	static void Init();
	static void Draw();
	static void Update();
};
