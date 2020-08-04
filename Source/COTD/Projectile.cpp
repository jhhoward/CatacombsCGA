#include "Defines.h"
#include "Projectile.h"
#include "Map.h"
#include "FixedMath.h"
#include "Particle.h"
#include "Enemy.h"
#include "Generated/SpriteTypes.h"
#include "Platform.h"
#include "Sounds.h"

Projectile ProjectileManager::projectiles[MAX_PROJECTILES];

Projectile* ProjectileManager::FireProjectile(Entity* owner, int16_t x, int16_t y, angle_t angle)
{
	for(int i = 0; i < MAX_PROJECTILES; i++)
	{
		Projectile& p = projectiles[i];
		if(p.life == 0)
		{
			if (owner == &Game::player)
				p.ownerId = PLAYER_OWNER_ID;
			else
			{
				for (uint8_t n = 0; n < MAX_ENEMIES; n++)
				{
					if (&EnemyManager::enemies[n] == owner)
					{
						p.ownerId = n;
						break;
					}
				}
			}

			p.life = 255;
			p.x = x;
			p.y = y;
			p.angle = angle;
			return &p;
		}
	}

	return nullptr;
}

Entity* Projectile::GetOwner() const
{
	if (ownerId == PLAYER_OWNER_ID)
		return &Game::player;
	return &EnemyManager::enemies[ownerId];
}

void ProjectileManager::Update()
{
	for (int i = 0; i < MAX_PROJECTILES; i++)
	{
		Projectile& p = projectiles[i];
		if(p.life > 0)
		{
			p.life--;

			int16_t deltaX = FixedCos(p.angle) / 4;
			int16_t deltaY = FixedSin(p.angle) / 4;

			p.x += deltaX;
			p.y += deltaY;

			bool hitAnything = false;

			Entity* owner = p.GetOwner();

			if (Map::IsBlockedAtWorldPosition(p.x, p.y))
			{
				uint8_t cellX = p.cellX;
				uint8_t cellY = p.cellY;

				if (Map::GetCellSafe(cellX, cellY) == CT_Urn)
				{
					Map::SetCell(cellX, cellY, CT_Empty);
					ParticleSystemManager::CreateExplosion(cellX * CELL_SIZE + CELL_SIZE / 2, cellY * CELL_SIZE + CELL_SIZE / 2, 7, 8);

					switch ((Random() % 5))
					{
					case 0:
						EnemyManager::Spawn(ET_Spider, cellX * CELL_SIZE + CELL_SIZE / 2, cellY * CELL_SIZE + CELL_SIZE / 2);
						break;
					case 1:
						Map::SetCell(cellX, cellY, CT_Potion);
						break;
					case 2:
						Map::SetCell(cellX, cellY, CT_Coins);
						break;
					}
					Platform::PlaySound(Sounds::Kill);
				}
				else
				{
					Platform::PlaySound(Sounds::Hit);
				}

				hitAnything = true;
			}
			else
			{
				if (owner == &Game::player)
				{
					Enemy* overlappingEnemy = EnemyManager::GetOverlappingEnemy(p.x, p.y);
					if (overlappingEnemy)
					{
						overlappingEnemy->Damage(PLAYER_ATTACK_STRENGTH);

						hitAnything = true;
					}
				}
				else if(Game::player.IsOverlappingPoint(p.x, p.y))
				{
					const EnemyArchetype* enemyArchetype = ((Enemy*)owner)->GetArchetype();
					if (enemyArchetype)
					{
						Game::player.Damage(enemyArchetype->GetAttackStrength());
						if (Game::player.hp == 0)
						{
							Game::stats.killedBy = ((Enemy*)owner)->GetType();
						}
					}
					hitAnything = true;
				}
			}

			if (hitAnything)
			{
				ParticleSystemManager::CreateExplosion(p.x - deltaX, p.y - deltaY, 4, 14);
				p.life = 0;
			}
		}
	}	
}

void ProjectileManager::Init()
{
	for (int i = 0; i < MAX_PROJECTILES; i++)
	{
		Projectile& p = projectiles[i];
		p.life = 0;
	}
}

void ProjectileManager::Draw()
{
	for (int i = 0; i < MAX_PROJECTILES; i++)
	{
		Projectile& p = projectiles[i];
		if (p.life > 0)
		{
			//Renderer::DrawObject(p.ownerId == PLAYER_OWNER_ID ? projectileSpriteData : enemyProjectileSpriteData, p.x, p.y, 32);
			Renderer::DrawObject(p.ownerId == PLAYER_OWNER_ID ? DrawPlayerProjectile : DrawPlayerProjectile, p.x, p.y, 32);
		}
	}
}
