#include "Enemy.h"
#include "Defines.h"
#include "Draw.h"
#include "Map.h"
#include "FixedMath.h"
#include "Game.h"
#include "Projectile.h"
#include "Generated/SpriteTypes.h"
#include "Sounds.h"
#include "Platform.h"
#include "Particle.h"

Enemy EnemyManager::enemies[MAX_ENEMIES];

const EnemyArchetype Enemy::archetypes[NumEnemyTypes] =
{
	{
		// Skeleton
		DrawEnemy,			// draw routine
		50,					// hp
		4,					// speed
		20,					// attackStrength
		3,					// attackDuration
		2,					// stunDuration
		false,				// isRanged
		96,					// sprite scale
		//AnchorType::Floor	// sprite anchor
	},
	{
		// Mage
		DrawEnemy,			// draw routine
		30,					// hp
		5,					// speed
		20,					// attackStrength
		3,					// attackDuration
		2,					// stunDuration
		true,				// isRanged
		96,					// sprite scale
		//AnchorType::Floor	// sprite anchor
	},
	{
		// Bat
		DrawEnemy,			// draw routine
		20,					// hp
		7,					// speed
		10,					// attackStrength
		2,					// attackDuration
		0,					// stunDuration
		false,				// isRanged
		80,					// sprite scale
		//AnchorType::Center	// sprite anchor
	},
	{
		// Spider
		DrawEnemy,			// draw routine
		10,					// hp
		7,					// speed
		5,					// attackStrength
		1,					// attackDuration
		0,					// stunDuration
		false,				// isRanged
		50,					// sprite scale
		//AnchorType::Floor	// sprite anchor
	}
};

void Enemy::Init(EnemyType initType, int16_t initX, int16_t initY)
{
	state = ES_Idle;
	type = initType;
	x = initX;
	y = initY;
	frameDelay = 0;
	targetCellX = cellX;
	targetCellY = cellY;
	hp = GetArchetype()->GetHP();
}

void Enemy::Damage(uint8_t amount)
{
	if (amount >= hp)
	{
		Game::stats.enemyKills[(int)type]++;
		type = ET_None;
		Platform::PlaySound(Sounds::Kill);
		ParticleSystemManager::CreateExplosion(x, y, 4, 8);
	}
	else
	{
		hp -= amount;
		Platform::PlaySound(Sounds::Hit);
		state = ES_Stunned;
		frameDelay = GetArchetype()->GetStunDuration();
	}
}

const EnemyArchetype* Enemy::GetArchetype() const
{
	if (type == ET_None)
		return nullptr;
	return &archetypes[(int)type];
}

int16_t Clamp(int16_t x, int16_t min, int16_t max)
{
	if(x < min)
		return min;
	if(x > max)
		return max;
	return x;
}

bool Enemy::TryPickCell(int8_t newX, int8_t newY)
{
	if(Map::IsBlocked(newX, newY))// && !engine.map.isDoor(newX, newZ))
		return false;
	if(Map::IsBlocked(targetCellX, newY)) // && !engine.map.isDoor(targetCellX, newZ))
		return false;
	if(Map::IsBlocked(newX, targetCellY)) // && !engine.map.isDoor(newX, targetCellZ))
		return false;

	for(int n = 0; n < MAX_ENEMIES; n++)
	{
		Enemy& other = EnemyManager::enemies[n];
		if(this != &other && other.IsValid())
		{
			if(other.targetCellX == newX && other.targetCellY == newY)
				return false;
		}
	}

	targetCellX = newX;
	targetCellY = newY;

	return true;
}

bool Enemy::TryPickCells(int8_t deltaX, int8_t deltaY)
{
	return TryPickCell(targetCellX + deltaX, targetCellY + deltaY)
		|| TryPickCell(targetCellX + deltaX, targetCellY) 
		|| TryPickCell(targetCellX, targetCellY + deltaY) 
		|| TryPickCell(targetCellX - deltaX, targetCellY + deltaY)
		|| TryPickCell(targetCellX + deltaX, targetCellY - deltaY);
}

uint8_t Enemy::GetPlayerCellDistance() const
{
	uint8_t dx = ABS(Game::player.cellX - cellX);
	uint8_t dy = ABS(Game::player.cellY - cellY);
	return dx > dy ? dx : dy;
}

void Enemy::PickNewTargetCell()
{
	int8_t deltaX = (int8_t) Clamp((Game::player.cellX) - targetCellX, -1, 1);
	int8_t deltaY = (int8_t) Clamp((Game::player.cellY) - targetCellY, -1, 1);
	uint8_t dodgeChance = (uint8_t) Random();

	if (GetArchetype()->GetIsRanged() && GetPlayerCellDistance() < 3)
	{
		deltaX = -deltaX;
		deltaY = -deltaY;
	}

	if(deltaX == 0)
	{
		if(dodgeChance < 64)
		{
			deltaX = -1;
		}
		else if(dodgeChance < 128)
		{
			deltaX = 1;
		}
	}
	else if(deltaY == 0)
	{
		if(dodgeChance < 64)
		{
			deltaY = -1;
		}
		else if(dodgeChance < 128)
		{
			deltaY = 1;
		}
	}

	TryPickCells(deltaX, deltaY);
}

void Enemy::StunMove()
{
	//int16_t targetX = Game::player.x;
	//int16_t targetY = Game::player.y;
	//
	//int16_t maxDelta = 3;
	//
	//int16_t deltaX = Clamp(targetX - x, -maxDelta, maxDelta);
	//int16_t deltaY = Clamp(targetY - y, -maxDelta, maxDelta);
	//
	//x -= deltaX;
	//y -= deltaY;
	
//	int16_t deltaX = (Random() % 16) - 8;
//	int16_t deltaY = (Random() % 16) - 8;
//	x += deltaX;
//	y += deltaY;
}

bool Enemy::TryMove()
{
	if(Map::IsSolid(targetCellX, targetCellY))
	{
		//engine.map.openDoorsAt(targetCellX, targetCellZ, Direction_None);
		return false;
	}

	int16_t targetX = (targetCellX * CELL_SIZE) + CELL_SIZE / 2;
	int16_t targetY = (targetCellY * CELL_SIZE) + CELL_SIZE / 2;

	int16_t maxDelta = GetArchetype()->GetMovementSpeed();

	int16_t deltaX = Clamp(targetX - x, -maxDelta, maxDelta);
	int16_t deltaY = Clamp(targetY - y, -maxDelta, maxDelta);

	x += deltaX;
	y += deltaY;

	if(IsOverlappingEntity(Game::player))
	{
		if (!GetArchetype()->GetIsRanged())
		{
			Game::player.Damage(GetArchetype()->GetAttackStrength());
			if (Game::player.hp == 0)
			{
				Game::stats.killedBy = type;
			}

			state = ES_Attacking;
			frameDelay = GetArchetype()->GetAttackDuration();
		}

		x -= deltaX;
		y -= deltaY;
		return false;
	}

	if(x == targetX && y == targetY)
	{
		PickNewTargetCell();
	}
	return true;	
}

bool Enemy::FireProjectile(angle_t angle)
{
	return ProjectileManager::FireProjectile(this, x, y, angle) != nullptr;
}

bool Enemy::TryFireProjectile()
{
	int8_t deltaX = (Game::player.cellX - cellX);
	int8_t deltaY = (Game::player.cellY - cellY);

	if (deltaX == 0)
	{
		if (deltaY < 0)
		{
			return FireProjectile(FIXED_ANGLE_270);
		}
		else if (deltaY > 0)
		{
			return FireProjectile(FIXED_ANGLE_90);
		}
	}
	else if (deltaY == 0)
	{
		if (deltaX < 0)
		{
			return FireProjectile(FIXED_ANGLE_180);
		}
		else if (deltaX > 0)
		{
			return FireProjectile(0);
		}
	}
	else if (deltaX == deltaY)
	{
		if (deltaX > 0)
		{
			return FireProjectile(FIXED_ANGLE_45);
		}
		else
		{
			return FireProjectile(FIXED_ANGLE_180 + FIXED_ANGLE_45);
		}
	}
	else if (deltaX == -deltaY)
	{
		if (deltaX > 0)
		{
			return FireProjectile(FIXED_ANGLE_270 + FIXED_ANGLE_45);
		}
		else
		{
			return FireProjectile(FIXED_ANGLE_90 + FIXED_ANGLE_45);
		}
	}

	return false;
}

bool Enemy::ShouldFireProjectile() const
{
	uint8_t distance = GetPlayerCellDistance();
	uint8_t chance = 16 / (distance > 0 ? distance : 1);

	return GetArchetype()->GetIsRanged() && (Random() & 0xff) < chance && CanSeePlayer(); // Map::IsClearLine(x, y, Game::player.x, Game::player.y);
}

bool Enemy::CanSeePlayer() const
{
	// Mechanism to split checks across 16 frames depending on cell x
	uint8_t frame = (uint8_t)(x >> 8) & 0xf;
	if ((Game::globalTickFrame & 0xf) == frame)
	{
		return Map::IsClearLine(x, y, Game::player.x, Game::player.y);
	}
	return false;
}

void Enemy::Tick()
{
	if (state == ES_Stunned)
	{
		StunMove();
	}

	if (frameDelay > 0)
	{
		if ((Game::globalTickFrame & 0xf) == 0)
		{
			frameDelay--;
		}
		return;
	}

	switch (state)
	{
	case ES_Idle:
		if (CanSeePlayer()) //Map::IsClearLine(x, y, Game::player.x, Game::player.y))
		{
			Platform::PlaySound(Sounds::SpotPlayer);
			state = ES_Moving;
		}
		break;
	case ES_Moving:
		TryMove();

		if (ShouldFireProjectile())
		{
			if (TryFireProjectile())
			{
				Platform::PlaySound(Sounds::Shoot);
				state = ES_Attacking;
				frameDelay = GetArchetype()->GetAttackDuration();
			}
		}
		break;
	case ES_Attacking:
		state = ES_Moving;
		break;
	case ES_Stunned:
		state = ES_Moving;
		break;
	}
}

void EnemyManager::Init()
{
	for(int n = 0; n < MAX_ENEMIES; n++)
	{
		enemies[n].Clear();
	}
}

void EnemyManager::Update()
{
	for (int n = 0; n < MAX_ENEMIES; n++)
	{
		Enemy& enemy = enemies[n];
		if(enemy.IsValid())
		{
			if (!Renderer::IsCellPotentiallyVisible(enemy.x >> 8, enemy.y >> 8))
				continue;
			enemy.Tick();
		}
	}
}

void EnemyManager::Draw()
{
	for (int n = 0; n < MAX_ENEMIES; n++)
	{
		Enemy& enemy = enemies[n];
		if(enemy.IsValid())
		{
			if (!Renderer::IsCellPotentiallyVisible(enemy.x >> 8, enemy.y >> 8))
				continue;

			bool invert = enemy.GetState() == ES_Stunned && (Renderer::globalRenderFrame & 1);
			int frameOffset = (enemy.GetType() == ET_Bat || enemy.GetState() == ES_Moving) && (Game::globalTickFrame & 8) == 0 ? 32 : 0;

			const EnemyArchetype* archetype = enemy.GetArchetype();
			Renderer::DrawObject(archetype->drawRoutine, enemy.x, enemy.y, 128 /*archetype->GetSpriteScale()*/, invert);
		}
	}
}

void EnemyManager::Spawn(EnemyType enemyType, int16_t x, int16_t y)
{
	for (int n = 0; n < MAX_ENEMIES; n++)
	{
		Enemy& enemy = enemies[n];
		if(!enemy.IsValid())
		{
			enemy.Init(enemyType, x, y);
			return;
		}
	}		
}

void EnemyManager::SpawnEnemies()
{
	for (uint8_t y = 0; y < MAP_HEIGHT; y++)
	{
		for (uint8_t x = 0; x < MAP_WIDTH; x++)
		{
			if (Map::GetCellSafe(x, y) == CT_Monster)
			{
				EnemyType type = (EnemyType)((Random() % ((int)(NumEnemyTypes))));
				EnemyManager::Spawn(type, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2);
				Map::SetCell(x, y, CT_Empty);
				break;
			}
		}
	}
	
}

Enemy* EnemyManager::GetOverlappingEnemy(Entity& entity)
{
	for (int n = 0; n < MAX_ENEMIES; n++)
	{
		Enemy& enemy = enemies[n];
		if (enemy.IsValid() && enemy.IsOverlappingEntity(entity))
		{
			return &enemy;
		}
	}

	return nullptr;
}

Enemy* EnemyManager::GetOverlappingEnemy(int16_t x, int16_t y)
{
	for (int n = 0; n < MAX_ENEMIES; n++)
	{
		Enemy& enemy = enemies[n];
		if (enemy.IsValid() && enemy.IsOverlappingPoint(x, y))
		{
			return &enemy;
		}
	}

	return nullptr;
}

