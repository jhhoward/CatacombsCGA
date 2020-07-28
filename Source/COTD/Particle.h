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

	inline bool IsActive() { return x != -128; }
};

struct ParticleSystem
{
	int16_t worldX, worldY;
	bool isWhite : 1;
	uint8_t life : 7;
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
	static void CreateExplosion(int16_t x, int16_t y, bool isWhite = false);
};
