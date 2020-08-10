#include "Particle.h"
#include "FixedMath.h"
#include "Platform.h"

ParticleSystem ParticleSystemManager::systems[MAX_PARTICLE_SYSTEMS];

void ParticleSystem::Init()
{
	life = 0;
}

void ParticleSystem::Step()
{
	for (int n = 0; n < PARTICLES_PER_SYSTEM; n++)
	{
		Particle& p = particles[n];
		if (p.IsActive())
		{
			p.velY += PARTICLE_GRAVITY;

			if (p.x + p.velX < -127 || p.x + p.velX > 127 || p.y + p.velY < -127)
			{
				p.x = -128;
				continue;
			}

			if (p.y + p.velY >= 128)
			{
				p.velY = p.velX = 0;
				p.y = 127;
			}

			p.x += p.velX;
			p.y += p.velY;
		}
	}
	
	life--;
}

void ParticleSystem::Draw(int x, int halfScale)
{
	int scale = 2 * halfScale;
	int8_t horizon = VIEWPORT_HEIGHT / 2; //Renderer::GetHorizon(x);
	
	for(int n = 0; n < PARTICLES_PER_SYSTEM; n++)
	{
		Particle& p = particles[n];
		if (p.IsActive())
		{
			int outX = x + ((p.x * scale) >> 8);
			int outY = horizon + ((p.y * scale) >> 8);

			if (outX >= 0 && outY >= 0 && outX < DISPLAY_WIDTH - 1 && outY < DISPLAY_HEIGHT - 1 && halfScale >= Renderer::wBuffer[outX])
			{
				Platform::PutPixel(outX, outY, p.colour);
				//Platform::PutPixel(outX + 1, outY, colour);
				//Platform::PutPixel(outX + 1, outY + 1, colour);
				//Platform::PutPixel(outX, outY + 1, colour);
			}
		}
	}
}

void ParticleSystem::Explode()
{
	for (int n = 0; n < PARTICLES_PER_SYSTEM; n++)
	{
		Particle& p = particles[n];
		p.x = (Random() & 31) - 16;
		p.y = (Random() & 31) - 16;

		p.velX = (Random() & 31) - 16;
		p.velY = (Random() & 31) - 25;
	}
	
	life = 22;
}

void ParticleSystemManager::Draw()
{
	for(int n = 0; n < MAX_PARTICLE_SYSTEMS; n++)
	{
		ParticleSystem& system = systems[n];
		if(system.IsActive())
		{
			int16_t screenX, screenW;

			if(Renderer::TransformAndCull(system.worldX, system.worldY, screenX, screenW))
			{
				QueuedDrawable* drawable = Renderer::CreateQueuedDrawable((uint8_t)screenW);
				if(drawable)
				{
					drawable->type = DT_ParticleSystem;
					drawable->x = (int8_t)screenX;
					drawable->inverseCameraDistance = (uint8_t)screenW;
					drawable->particleSystem = &system;
				}
			}
		}
	}
}

void ParticleSystemManager::Init()
{
	for (int n = 0; n < MAX_PARTICLE_SYSTEMS; n++)
	{
		ParticleSystem& system = systems[n];
		system.Init();
	}
}

void ParticleSystemManager::Update()
{
	for (int n = 0; n < MAX_PARTICLE_SYSTEMS; n++)
	{
		ParticleSystem& system = systems[n];
		if(system.IsActive())
		{
			system.Step();
		}
	}	
}

void ParticleSystemManager::CreateExplosion(int16_t worldX, int16_t worldY, uint8_t primaryColour, uint8_t secondaryColour)
{
	ParticleSystem* newSystem = nullptr;
	for (int n = 0; n < MAX_PARTICLE_SYSTEMS; n++)
	{
		ParticleSystem& system = systems[n];
		if(!system.IsActive())
		{
			newSystem = &system;
			break;
		}
	}	

	if (!newSystem)
	{
		newSystem = &systems[0];

		for (uint8_t n = 1; n < MAX_PARTICLE_SYSTEMS; n++)
		{
			if (systems[n].life < newSystem->life)
			{
				newSystem = &systems[n];
			}
		}
	}

	if (secondaryColour == 0xff)
		secondaryColour = primaryColour;

	newSystem->worldX = worldX;
	newSystem->worldY = worldY;
	newSystem->Explode();

	for (uint8_t n = 0; n < PARTICLES_PER_SYSTEM; n += 2)
	{
		newSystem->particles[n].colour = primaryColour;
		newSystem->particles[n + 1].colour = secondaryColour;
	}
}
