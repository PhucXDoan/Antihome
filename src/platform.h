#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_FontCache.h>
#include <SDL_Mixer.h>

global constexpr vi2 WIN_DIM            = { 800, 600 };
global constexpr f32 SECONDS_PER_UPDATE = 1.0f / 30.0f;

enum_loose (Input, u32)
{
	a,
	d,
	e,
	q,
	s,
	w,
	n0,
	n1,
	n2,
	n3,
	n4,
	n5,
	n6,
	n7,
	n8,
	n9,
	left,
	right,
	down,
	up,
	space,
	backspace,
	shift,
	escape,

	CAPACITY
};

struct InputState
{
	u8 curr;
	u8 prev;
};

enum struct UpdateCode : u32
{
	resume,
	terminate
};

struct Platform
{
	SDL_Surface* surface;
	memsize      memory_capacity;
	byte*        memory;
	InputState   inputs[Input::CAPACITY];
};

#define PROTOTYPE_INITIALIZE(NAME) void NAME(Platform* platform)
typedef PROTOTYPE_INITIALIZE(PrototypeInitialize);

#define PROTOTYPE_BOOT_UP(NAME) void NAME(Platform* platform)
typedef PROTOTYPE_BOOT_UP(PrototypeBootUp);

#define PROTOTYPE_BOOT_DOWN(NAME) void NAME(Platform* platform)
typedef PROTOTYPE_BOOT_DOWN(PrototypeBootDown);

#define PROTOTYPE_UPDATE(NAME) UpdateCode NAME(Platform* platform)
typedef PROTOTYPE_UPDATE(PrototypeUpdate);

#define PROTOTYPE_RENDER(NAME) void NAME(Platform* platform)
typedef PROTOTYPE_RENDER(PrototypeRender);
