#define STB_IMAGE_IMPLEMENTATION true
#include <stb_image.h>
#include "unified.h"
#include "platform.h"
#include "utilities.cpp"

global constexpr vf2 VIEW_DIM     = { 350.0f, 175.0f };
global constexpr f32 WALL_HEIGHT  = 2.7432f;
global constexpr f32 LUCIA_HEIGHT = 1.4986f;

struct State
{
	u32                seed;
	SDL_Surface*       view;

	vf2                lucia_velocity;
	vf2                lucia_position;
	f32                lucia_angle_velocity;
	f32                lucia_angle;
	f32                lucia_fov;
	f32                lucia_head_bob_keytime;

	ColumnMajorTexture wall;
	ColumnMajorTexture floor;
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

	state->wall  = init_column_major_texture(DATA_DIR "wall.bmp");
	state->floor = init_column_major_texture(DATA_DIR "floor.bmp");
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	deinit_column_major_texture(&state->floor);
	deinit_column_major_texture(&state->wall);
}

extern "C" PROTOTYPE_UPDATE(update)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	state->lucia_angle_velocity -= platform->cursor_delta.x * 0.25f;
	state->lucia_angle_velocity *= 0.4f;
	state->lucia_angle          += state->lucia_angle_velocity * SECONDS_PER_UPDATE;
	if (state->lucia_angle < 0.0f)
	{
		state->lucia_angle += TAU;
	}
	else if (state->lucia_angle >= TAU)
	{
		state->lucia_angle -= TAU;
	}

	vf2 lucia_move = { 0.0f, 0.0f };
	if (HOLDING(Input::s))
	{
		lucia_move.x -= 1.0f;
	}
	if (HOLDING(Input::w))
	{
		lucia_move.x += 1.0f;
	}
	if (HOLDING(Input::d))
	{
		lucia_move.y -= 1.0f;
	}
	if (HOLDING(Input::a))
	{
		lucia_move.y += 1.0f;
	}

	if (+lucia_move)
	{
		state->lucia_velocity += rotate(normalize(lucia_move), state->lucia_angle) * 2.0f;
	}

	state->lucia_velocity *= HOLDING(Input::shift) ? 0.75f : 0.6f;
	state->lucia_position += state->lucia_velocity * SECONDS_PER_UPDATE;

	state->lucia_head_bob_keytime += 0.5f * norm(state->lucia_velocity) * SECONDS_PER_UPDATE;
	if (state->lucia_head_bob_keytime > 1.0f)
	{
		state->lucia_head_bob_keytime -= 1.0f;
	}

	state->lucia_fov += platform->scroll * 0.1f;

	return UpdateCode::resume;
}

extern "C" PROTOTYPE_RENDER(render)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	constexpr f32 MAGIC_K      = 0.927295218f * VIEW_DIM.x;
	constexpr i32 VIEW_PADDING = 10;
	constexpr vf2 WALLS[][2]   =
		{
			{ { -1.5f, -1.5f }, {  1.5f, -1.5f } },
			{ { -1.5f,  1.5f }, {  1.5f,  1.5f } },
			{ { -1.5f, -1.5f }, { -1.5f,  1.5f } },
			{ {  1.5f, -1.5f }, {  1.5f,  1.5f } },
		};

	fill(platform->surface, { 0.0f, 0.0f, 0.0f, 1.0f });
	fill(state->view      , { 0.1f, 0.2f, 0.3f, 1.0f });

	f32 lucia_eye_level = LUCIA_HEIGHT + 0.025f * (cosf(state->lucia_head_bob_keytime * TAU) - 1.0f);

	FOR_RANGE(x, VIEW_DIM.x)
	{
		vf2 ray_horizontal = polar(state->lucia_angle + (0.5f - x / VIEW_DIM.x) * state->lucia_fov);
		FOR_RANGE(y, VIEW_DIM.y)
		{
			f32 pitch = (0.5f - y / VIEW_DIM.y) * state->lucia_fov / MAGIC_K * VIEW_DIM.y;

			f32 floor_distance_x;
			f32 floor_portion_x;
			f32 floor_distance_y;
			f32 floor_portion_y;
			if
			(
				ray_cast_line_segment(&floor_distance_x, &floor_portion_x, { state->lucia_position.x, lucia_eye_level }, normalize(vf2 { ray_horizontal.x, pitch }), { -1.5f, 0.0f }, { 1.5f, 0.0f }) &&
				ray_cast_line_segment(&floor_distance_y, &floor_portion_y, { state->lucia_position.y, lucia_eye_level }, normalize(vf2 { ray_horizontal.y, pitch }), { -1.5f, 0.0f }, { 1.5f, 0.0f })
			)
			{
				*(reinterpret_cast<u32*>(state->view->pixels) + y * state->view->w + x) =
					to_pixel
					(
						state->view,
						*(state->floor.colors + static_cast<i32>(floor_portion_x * (state->floor.w - 1.0f)) * state->floor.h + static_cast<i32>(floor_portion_y * state->floor.h))
					);
			}
		}

		i32 wall_index    = -1;
		f32 wall_distance = -0.f;
		f32 wall_portion  = -0.f;

		FOR_ELEMS(it, WALLS)
		{
			f32 distance;
			f32 portion;
			if (ray_cast_line_segment(&distance, &portion, state->lucia_position, ray_horizontal, (*it)[0], (*it)[1]) && (wall_index == -1 || distance < wall_distance))
			{
				wall_index    = it_index;
				wall_distance = distance;
				wall_portion  = portion;
			}
		}

		if (wall_index != -1)
		{
			i32 starting_y = static_cast<i32>(VIEW_DIM.y / 2.0f - MAGIC_K / state->lucia_fov * (WALL_HEIGHT - lucia_eye_level) / wall_distance);
			i32 ending_y   = static_cast<i32>(VIEW_DIM.y / 2.0f + MAGIC_K / state->lucia_fov * lucia_eye_level / wall_distance);

			vf4* texture_column = &state->wall.colors[static_cast<i32>(wall_portion * (state->wall.w - 1.0f)) * state->wall.h];
			FOR_RANGE(y, MAXIMUM(0, starting_y), MINIMUM(ending_y, VIEW_DIM.y))
			{
				*(reinterpret_cast<u32*>(state->view->pixels) + y * state->view->w + x) =
					to_pixel
					(
						state->view,
						texture_column[static_cast<i32>(static_cast<f32>(ending_y - y) / (ending_y - starting_y) * state->wall.h)]
					);
			}
		}
	}

	SDL_Rect dst = { static_cast<i32>(VIEW_PADDING), static_cast<i32>(VIEW_PADDING), static_cast<i32>(WIN_DIM.x - VIEW_PADDING * 2.0f), static_cast<i32>((WIN_DIM.x - VIEW_PADDING * 2.0f) * VIEW_DIM.y / VIEW_DIM.x) };
	SDL_BlitScaled(state->view, 0, platform->surface, &dst);
}
