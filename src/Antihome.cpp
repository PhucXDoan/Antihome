#include "unified.h"
#include "platform.h"
#include "utilities.cpp"

constexpr vf2 GAME_VIEW_RESOLUTION = { WIN_DIM.x / 2, WIN_DIM.x / 4 };
constexpr f32 FOV                  = TAU / 4.0f;

struct State
{
	u32          seed;
	SDL_Surface* view;
	vf2          lucia_velocity;
	vf2          lucia_position;
	f32          lucia_angle_velocity;
	f32          lucia_angle;
};

extern "C" PROTOTYPE_INITIALIZE(initialize)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	*state = {};
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

	f32 lucia_turn = 0.0f;
	if (HOLDING(Input::left))
	{
		lucia_turn += 1.0f;
	}
	if (HOLDING(Input::right))
	{
		lucia_turn -= 1.0f;
	}

	state->lucia_angle_velocity += 3.0f * FOV * lucia_turn;
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

	fill_rect(platform->surface, { 0.0f, 0.0f }, WIN_DIM, { 0.0f, 0.0f, 0.0f, 1.0f });
	fill_rect(state->view, { 0.0f, 0.0f }, GAME_VIEW_RESOLUTION, BG_COLOR);

	constexpr i32 GAME_VIEW_PADDING = 10;

	FOR_RANGE(x, GAME_VIEW_RESOLUTION.x)
	{
		f32 angle_offset  = -(static_cast<f32>(x) / GAME_VIEW_RESOLUTION.x - 0.5f) * FOV;
		vf2 ray_direction = { -sinf(state->lucia_angle + angle_offset), cosf(state->lucia_angle + angle_offset) };
		vf4 color         = { 0.0f, 0.0f, 0.0f, 0.0f };

		f32 best_scalar = INFINITY;
		f32 best_portion;
		f32 scalar;
		f32 portion;

		if (ray_cast_to_wall(&scalar, &portion, state->lucia_position, ray_direction, { -1.0f, 3.0f }, { 1.0f, 3.0f }) && scalar < best_scalar)
		{
			color        = { 1.0f, 0.0f, 0.0f, 1.0f };
			best_scalar  = scalar;
			best_portion = portion;
		}
		if (ray_cast_to_wall(&scalar, &portion, state->lucia_position, ray_direction, { -1.0f, 4.0f }, { 1.0f, 4.0f }) && scalar < best_scalar)
		{
			color        = { 0.0f, 1.0f, 0.0f, 1.0f };
			best_scalar  = scalar;
			best_portion = portion;
		}
		if (ray_cast_to_wall(&scalar, &portion, state->lucia_position, ray_direction, { 1.0f, -4.0f }, { 1.0f, 4.0f }) && scalar < best_scalar)
		{
			color        = { 0.0f, 0.0f, 1.0f, 1.0f };
			best_scalar  = scalar;
			best_portion = portion;
		}

		if (best_scalar != INFINITY)
		{
			i32 height = static_cast<i32>(100.0f / (best_scalar + 0.01f));

			FOR_RANGE(y, MAXIMUM(static_cast<i32>((GAME_VIEW_RESOLUTION.y - height) / 2.0f), 0), MINIMUM(static_cast<i32>((GAME_VIEW_RESOLUTION.y - height) / 2.0f) + height, GAME_VIEW_RESOLUTION.y - 1))
			{
				set_pixel(state->view, x, y, color);
			}
		}
	}

	SDL_Rect dst = { static_cast<i32>(GAME_VIEW_PADDING), static_cast<i32>(GAME_VIEW_PADDING), static_cast<i32>(WIN_DIM.x - GAME_VIEW_PADDING * 2), static_cast<i32>((WIN_DIM.x - GAME_VIEW_PADDING * 2) * GAME_VIEW_RESOLUTION.y / GAME_VIEW_RESOLUTION.x) };
	SDL_BlitScaled(state->view, 0, platform->surface, &dst);
}
