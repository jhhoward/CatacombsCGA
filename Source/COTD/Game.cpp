#include "Defines.h"
#include "Game.h"
#include "FixedMath.h"
#include "Draw.h"
#include "Map.h"
#include "Projectile.h"
#include "Particle.h"
#include "MapGenerator.h"
#include "Platform.h"
#include "Entity.h"
#include "Enemy.h"
#include "Menu.h"
#include "DOSLib.h"
#include "Profiler.h"

Player Game::player;
const char* Game::displayMessage = nullptr;
uint8_t Game::displayMessageTime = 0;
Game::State Game::state = GS_Menu;
uint8_t Game::floor = 1;
uint8_t Game::globalTickFrame = 0;
Stats Game::stats;
Menu Game::menu;

#include <stdio.h>

void Game::Init()
{
	Platform::Log("Init");
	menu.Init();
	ParticleSystemManager::Init();
	ProjectileManager::Init();
	EnemyManager::Init();

	StartGame();
}

void Game::StartGame()
{
	floor = 1;
	stats.Reset();
	player.Init();
	SwitchState(GS_EnteringLevel);
}

void Game::SwitchState(State newState)
{
	if(state != newState)
	{
		state = newState;
		menu.ResetTimer();
	}
}

void Game::ShowMessage(const char* message)
{
	const uint8_t messageDisplayTime = 90;

	displayMessage = message;
	displayMessageTime = messageDisplayTime;
}

void Game::NextLevel()
{
	if (floor == 10)
	{
		GameOver();
	}
	else
	{
		floor++;
		SwitchState(GS_EnteringLevel);
	}
}

void Game::StartLevel()
{
	Platform::Log("Starting level..");
	ParticleSystemManager::Init();
	ProjectileManager::Init();
	EnemyManager::Init();
	MapGenerator::Generate();
	EnemyManager::SpawnEnemies();

	player.NextLevel();

	Platform::ExpectLoadDelay();
	SwitchState(GS_InGame);
}

void Game::Draw(backbuffer_t backBuffer)
{
	PROFILE_SECTION(Draw);

	switch(state)
	{
		case GS_Menu:
			menu.Draw();
			break;
		case GS_EnteringLevel:
			menu.DrawEnteringLevel();
			break;
		case GS_InGame:
		{
			Renderer::Render(backBuffer, player);
		}
			break;
		case GS_GameOver:
			menu.DrawGameOver();
			break;
		case GS_FadeOut:
			menu.FadeOut();
			break;
	}
}

void Game::TickInGame()
{
	if (displayMessageTime > 0)
	{
		displayMessageTime--;
		if (displayMessageTime == 0)
			displayMessage = nullptr;
	}

	player.Tick();

#if WITH_DOORS
	Map::Tick();
#endif

	
	ProjectileManager::Update();
	ParticleSystemManager::Update();
	EnemyManager::Update();
	
	if (Map::GetCellSafe(player.x / CELL_SIZE, player.y / CELL_SIZE) == CT_Exit)
	{
		NextLevel();
	}

	if (player.hp == 0)
	{
		GameOver();
	}
}

void Game::Tick()
{
	globalTickFrame++;

	switch(state)
	{
		case GS_InGame:
			TickInGame();
			return;
		case GS_EnteringLevel:
			menu.TickEnteringLevel();
			return;
		case GS_Menu:
			menu.Tick();
			return;
		case GS_GameOver:
			menu.TickGameOver();
			return;
	}
}

void Game::GameOver()
{
	SwitchState(GS_FadeOut);
}

void Stats::Reset()
{
	killedBy = ET_None;
	chestsOpened = 0;
	coinsCollected = 0;
	crownsCollected = 0;
	scrollsCollected = 0;

	for(int n = 0; n < NumEnemyTypes; n++)
	{
		enemyKills[n] = 0;
	}
}
