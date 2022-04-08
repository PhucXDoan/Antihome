#include "unified.h"
#include "platform.h"

#define PRESSED(INPUT) (platform->inputs[+(INPUT)].curr && !platform->inputs[+(INPUT)].prev)
#define HELD(INPUT)    (platform->inputs[+(INPUT)].curr)

internal void set_pixel(SDL_Surface* surface, i32 x, i32 y, vf4 color)
{
	*reinterpret_cast<u32*>(reinterpret_cast<u8*>(surface->pixels) + y * surface->pitch + x * surface->format->BytesPerPixel) =
		(static_cast<u32>(static_cast<u8>(color.w * 255.0f)) << 24) |
		(static_cast<u32>(static_cast<u8>(color.z * 255.0f)) << 16) |
		(static_cast<u32>(static_cast<u8>(color.y * 255.0f)) <<  8) |
		(static_cast<u32>(static_cast<u8>(color.x * 255.0f)) <<  0);
}

struct State
{
	SDL_Surface* view;
};

extern "C" PROTOTYPE_INITIALIZE(initialize)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);
}

extern "C" PROTOTYPE_BOOT_UP(boot_up)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	// @TODO@ More robustiness needed here.
	state->view =
		SDL_CreateRGBSurface
		(
			0,
			300,
			200,
			32,
			0x000000FF,
			0x0000FF00,
			0x00FF0000,
			0xFF000000
		);

	FOR_RANGE(x, 300)
	{
		set_pixel(state->view, x, 0, { 1.0f, 0.0f, 0.0f, 1.0f });
	}

	FOR_RANGE(y, 200)
	{
		set_pixel(state->view, 0, y, { 0.0f, 1.0f, 0.0f, 1.0f });
	}

	FOR_RANGE(z, 200)
	{
		set_pixel(state->view, z, z, { 0.0f, 0.0f, 1.0f, 0.5f });
	}
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	SDL_FreeSurface(state->view);
}

extern "C" PROTOTYPE_UPDATE(update)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	if (PRESSED(Input::q))
	{
		DEBUG_printf("q\n");
	}

	if (HELD(Input::e))
	{
		DEBUG_printf("e\n");
	}

	return UpdateCode::resume;
}

extern "C" PROTOTYPE_RENDER(render)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	SDL_FillRect(platform->surface, 0, 0);

	SDL_Rect dst = { 200, 200, 300, 400 };

	SDL_BlitSurface(state->view, 0, platform->surface, &dst);
}
