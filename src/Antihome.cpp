#include "unified.h"
#include "platform.h"
#include "utilities.cpp"

constexpr vf2 GAME_VIEW_RESOLUTION = { WIN_DIM.x / 2, WIN_DIM.x / 4 };

struct State
{
	u32          seed;
	SDL_Surface* view;
	vf2          lucia_velocity;
	vf2          lucia_position;
	f32          lucia_angle_velocity;
	f32          lucia_angle;
	f32          lucia_fov;

	SDL_Surface* wall;
};

extern "C" PROTOTYPE_INITIALIZE(initialize)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	*state = {};
	state->lucia_fov = TAU / 4.0f;

	SDL_SetRelativeMouseMode(SDL_TRUE);
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
			static_cast<i32>(GAME_VIEW_RESOLUTION.x),
			static_cast<i32>(GAME_VIEW_RESOLUTION.y),
			32,
			0x000000FF,
			0x0000FF00,
			0x00FF0000,
			0xFF000000
		);

	{
		SDL_Surface* bmp = SDL_LoadBMP(DATA_DIR "wall1.bmp");
		ASSERT(bmp);
		DEFER { SDL_FreeSurface(bmp); };

		state->wall = SDL_ConvertSurfaceFormat(bmp, SDL_PIXELFORMAT_RGBA8888, 0);
		ASSERT(state->wall);
	}
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	SDL_FreeSurface(state->wall);
	SDL_FreeSurface(state->view);
}

extern "C" PROTOTYPE_UPDATE(update)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	state->lucia_fov = CLAMP(state->lucia_fov - platform->scroll * 0.1f, 0.5f, 5.0f);

	state->lucia_angle_velocity -= 0.04f * state->lucia_fov * platform->cursor_delta.x;
	state->lucia_angle_velocity *= 0.5f;
	state->lucia_angle          += state->lucia_angle_velocity * SECONDS_PER_UPDATE;

	vf2 lucia_move = { 0.0f, 0.0f };
	if (HOLDING(Input::a))
	{
		lucia_move.x -= 1.0f;
	}
	if (HOLDING(Input::d))
	{
		lucia_move.x += 1.0f;
	}
	if (HOLDING(Input::s))
	{
		lucia_move.y -= 1.0f;
	}
	if (HOLDING(Input::w))
	{
		lucia_move.y += 1.0f;
	}

	if (+lucia_move)
	{
		lucia_move = rotate(normalize(lucia_move), state->lucia_angle);
	}

	state->lucia_velocity += 2.0f * lucia_move;
	state->lucia_velocity *= 0.6f;
	state->lucia_position += state->lucia_velocity * SECONDS_PER_UPDATE;

	return UpdateCode::resume;
}

extern "C" PROTOTYPE_RENDER(render)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	constexpr vf4 BG_COLOR = { 0.1f, 0.2f, 0.3f, 1.0f };

	fill_rect(platform->surface, { 0.0f, 0.0f }, WIN_DIM, { 0.1f, 0.1f, 0.1f, 1.0f });
	fill_rect(state->view, { 0.0f, 0.0f }, GAME_VIEW_RESOLUTION, BG_COLOR);

	constexpr i32 GAME_VIEW_PADDING = 10;

	FOR_RANGE(x, GAME_VIEW_RESOLUTION.x)
	{
		f32    angle_offset  = -(static_cast<f32>(x) / GAME_VIEW_RESOLUTION.x - 0.5f) * state->lucia_fov;
		vf2    ray_direction = { -sinf(state->lucia_angle + angle_offset), cosf(state->lucia_angle + angle_offset) };
		bool32 intersected   = false;
		f32    scalar        = INFINITY;
		f32    portion       = 0.0f;
		{
			f32 test_scalar  = INFINITY;
			f32 test_portion = 0.0f;
			if (ray_cast_to_wall(&test_scalar, &test_portion, state->lucia_position, ray_direction, { -1.0f, 3.0f }, { 1.0f, 3.0f }) && (!intersected || test_scalar < scalar))
			{
				intersected = true;
				scalar      = test_scalar;
				portion     = test_portion;
			}
			if (ray_cast_to_wall(&test_scalar, &test_portion, state->lucia_position, ray_direction, { -1.0f, 4.0f }, { 1.0f, 4.0f }) && (!intersected || test_scalar < scalar))
			{
				intersected = true;
				scalar      = test_scalar;
				portion     = test_portion;
			}
			if (ray_cast_to_wall(&test_scalar, &test_portion, state->lucia_position, ray_direction, { 1.0f, -4.0f }, { 1.0f, 4.0f }) && (!intersected || test_scalar < scalar))
			{
				intersected = true;
				scalar      = test_scalar;
				portion     = test_portion;
			}
		}

		if (intersected)
		{
			i32 height = static_cast<i32>(100.0f * state->lucia_fov / (scalar + 0.01f));

			FOR_RANGE(i, height)
			{
				i32 y = static_cast<i32>((GAME_VIEW_RESOLUTION.y - height) / 2.0f) + i;
				if (IN_RANGE(y, 0, GAME_VIEW_RESOLUTION.y))
				{
					set_color(state->view, x, y, get_sample(state->wall, { portion, static_cast<f32>(i) / height }));
				}
			}
		}
	}

	SDL_Rect dst = { static_cast<i32>(GAME_VIEW_PADDING), static_cast<i32>(GAME_VIEW_PADDING), static_cast<i32>(WIN_DIM.x - GAME_VIEW_PADDING * 2), static_cast<i32>((WIN_DIM.x - GAME_VIEW_PADDING * 2) * GAME_VIEW_RESOLUTION.y / GAME_VIEW_RESOLUTION.x) };
	SDL_BlitScaled(state->view, 0, platform->surface, &dst);
}
