#pragma once

#include <stdint.h>
#include "Defines.h"
#include "Draw.h"
#include "Game.h"

#define PARTICLE_GRAVITY 3
#define MAX_PARTICLE_SYSTEMS 3

struct Particle
{
	int8_t x, y;
	int8_t velX, velY;
	uint8_t colour;

	inline bool IsActive() { return x != -128; }
};

struct ParticleSystem
{
	int16_t worldX, worldY;
	uint8_t life;
	Particle particles[PARTICLES_PER_SYSTEM];
	
	bool IsActive() { return life > 0; }

	void Init();
	void Step();
	void Draw(int x, int scale);
	void Explode();
};

class ParticleSystemManager
{
public:
	static ParticleSystem systems[MAX_PARTICLE_SYSTEMS];
	
	static void Init();
	static void Draw();
	static void Update();
	static void CreateExplosion(int16_t x, int16_t y, uint8_t primaryColour, uint8_t secondaryColour = 0xff);
};
