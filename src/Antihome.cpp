#define STB_IMAGE_IMPLEMENTATION true
#include <stb_image.h>
#include "unified.h"
#include "platform.h"
#include "utilities.cpp"

constexpr vf2 VIEW_DIM = { WIN_DIM.x / 4, WIN_DIM.x / 8 };
constexpr f32 LUCIA_HEIGHT    = 1.4986f;

struct State
{
	u32                seed;
	SDL_Surface*       view;

	vf2                lucia_velocity;
	vf2                lucia_position;
	f32                lucia_angle_velocity;
	f32                lucia_angle;
	f32                lucia_fov;

	ColumnMajorTexture wall;
};

extern "C" PROTOTYPE_INITIALIZE(initialize)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	*state = {};

	state->lucia_fov = TAU / 3.0f;

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
			static_cast<i32>(VIEW_DIM.x),
			static_cast<i32>(VIEW_DIM.y),
			32,
			0x000000FF,
			0x0000FF00,
			0x00FF0000,
			0xFF000000
		);

	state->wall = init_column_major_texture(DATA_DIR "wall.bmp");
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	deinit_column_major_texture(&state->wall);
}

extern "C" PROTOTYPE_UPDATE(update)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	state->lucia_angle_velocity -= platform->cursor_delta.x * 0.25f;
	state->lucia_angle_velocity *= 0.5f;
	state->lucia_angle          += state->lucia_angle_velocity * SECONDS_PER_UPDATE;

	vf2 lucia_move = { 0.0f, 0.0f };
	if (HOLDING(Input::w))
	{
		lucia_move.x += 1.0f;
	}
	if (HOLDING(Input::s))
	{
		lucia_move.x -= 1.0f;
	}
	if (HOLDING(Input::a))
	{
		lucia_move.y += 1.0f;
	}
	if (HOLDING(Input::d))
	{
		lucia_move.y -= 1.0f;
	}

	if (+lucia_move)
	{
		lucia_move = rotate(normalize(lucia_move), state->lucia_angle);
	}

	state->lucia_velocity += lucia_move; // @TODO@ Framerate independence?
	state->lucia_velocity *= 0.5f;
	state->lucia_position += state->lucia_velocity * SECONDS_PER_UPDATE;

	return UpdateCode::resume;
}

extern "C" PROTOTYPE_RENDER(render)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	constexpr i32 VIEW_PADDING = 10;

	fill(platform->surface, { 0.0f, 0.0f, 0.0f, 1.0f });
	fill(state->view      , { 0.1f, 0.2f, 0.3f, 1.0f });

	FOR_RANGE(x, VIEW_DIM.x)
	{
		vf2 ray = polar(state->lucia_angle + (0.5f - x / VIEW_DIM.x) * state->lucia_fov);

		f32 scalar;
		f32 portion;
		if (ray_cast_to_wall(&scalar, &portion, state->lucia_position, ray, { -0.5f, 1.0f }, { 0.5f, 1.0f }))
		{
			i32 projected_wall_height = static_cast<i32>(50.0f / scalar);
			i32 starting_y            = static_cast<i32>((VIEW_DIM.y - projected_wall_height) / 2.0f);

			vf4* texture_column = &state->wall.colors[static_cast<i32>(portion * state->wall.w) * state->wall.h];
			FOR_RANGE(i, MAXIMUM(0, -starting_y), MINIMUM(projected_wall_height, VIEW_DIM.y - starting_y))
			{
				*(reinterpret_cast<u32*>(state->view->pixels) + (starting_y + i) * state->view->w + x) =
					to_pixel
					(
						state->view,
						texture_column[static_cast<i32>(static_cast<f32>(i) / projected_wall_height * state->wall.h)]
					);
			}
		}
	}

	SDL_Rect dst = { static_cast<i32>(VIEW_PADDING), static_cast<i32>(VIEW_PADDING), static_cast<i32>(WIN_DIM.x - VIEW_PADDING * 2.0f), static_cast<i32>((WIN_DIM.x - VIEW_PADDING * 2.0f) * VIEW_DIM.y / VIEW_DIM.x) };
	SDL_BlitScaled(state->view, 0, platform->surface, &dst);
}
