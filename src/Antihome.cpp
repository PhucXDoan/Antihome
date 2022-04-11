#include "unified.h"
#include "platform.h"
#include "utilities.cpp"

constexpr vf2 VIEW_RESOLUTION  = { WIN_DIM.x / 2, WIN_DIM.x / 4 };
constexpr f32 PIXELS_PER_METER = 150.0f;
constexpr f32 WALL_HEIGHT      = 2.7432f;
constexpr f32 LUCIA_HEIGHT     = 1.4986f;

struct Wall
{
	vf2 position_start;
	vf2 position_end;
	f32 texture_start;
	f32 texture_end;
};

global Wall WALLS[] =
	{
		{ { -1.0f, -1.0f }, { -0.5f, -1.0f }, 0.00f, 0.25f },
		{ {  0.5f, -1.0f }, {  1.0f, -1.0f }, 0.75f, 1.00f },
		{ { -1.0f,  1.0f }, {  1.0f,  1.0f }, 0.00f, 1.00f },
		{ { -1.0f, -1.0f }, { -1.0f,  1.0f }, 0.00f, 1.00f },
		{ {  1.0f, -1.0f }, {  1.0f,  1.0f }, 0.00f, 1.00f },

		{ { -1.0f - 1.0f, -1.0f - 1.0f }, { -0.5f - 1.0f, -1.0f - 1.0f }, 0.00f, 0.25f },
		{ {  0.5f + 1.0f, -1.0f - 1.0f }, {  1.0f + 1.0f, -1.0f - 1.0f }, 0.75f, 1.00f },
		{ { -1.0f - 1.0f,  1.0f + 1.0f }, {  1.0f + 1.0f,  1.0f + 1.0f }, 0.00f, 1.00f },
		{ { -1.0f - 1.0f, -1.0f - 1.0f }, { -1.0f - 1.0f,  1.0f + 1.0f }, 0.00f, 1.00f },
		{ {  1.0f + 1.0f, -1.0f - 1.0f }, {  1.0f + 1.0f,  1.0f + 1.0f }, 0.00f, 1.00f }
	};

struct State
{
	u32          seed;
	SDL_Surface* view;
	vf2          lucia_velocity;
	vf2          lucia_position;
	f32          lucia_angle_velocity;
	f32          lucia_angle;
	f32          lucia_fov;
	Mipmap       texture_wall;
};

extern "C" PROTOTYPE_INITIALIZE(initialize)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	*state = {};
	state->lucia_fov = 0.375f * TAU;

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
			static_cast<i32>(VIEW_RESOLUTION.x),
			static_cast<i32>(VIEW_RESOLUTION.y),
			32,
			0x000000FF,
			0x0000FF00,
			0x00FF0000,
			0xFF000000
		);

	state->texture_wall = init_mipmap(DATA_DIR "wooden_wall_mipmap.bmp", 6);
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	deinit_mipmap(&state->texture_wall);
	SDL_FreeSurface(state->view);
}

extern "C" PROTOTYPE_UPDATE(update)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	state->lucia_fov = CLAMP(state->lucia_fov - platform->scroll * 0.1f, 0.5f, 5.0f);

	state->lucia_angle_velocity -= 0.1f * platform->cursor_delta.x;
	state->lucia_angle_velocity *= 0.5f;
	state->lucia_angle          += state->lucia_angle_velocity * SECONDS_PER_UPDATE;
	state->lucia_angle           = fmodf(state->lucia_angle, TAU);

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

	constexpr i32 VIEW_PADDING = 10;

	fill_rect(platform->surface, { 0.0f, 0.0f }, WIN_DIM        , { 0.0f, 0.0f, 0.0f, 1.0f });
	fill_rect(state->view      , { 0.0f, 0.0f }, VIEW_RESOLUTION, { 0.1f, 0.2f, 0.3f, 1.0f });

	FOR_RANGE(x, VIEW_RESOLUTION.x)
	{
		vf2   ray_direction       = polar(state->lucia_angle - (x / VIEW_RESOLUTION.x - 0.5f) * state->lucia_fov);
		Wall* intersected_wall    = 0;
		f32   intersected_scalar  = 0.0f;
		f32   intersected_portion = 0.0f;

		FOR_ELEMS(it, WALLS)
		{
			f32 scalar;
			f32 portion;
			if (ray_cast_to_wall(&scalar, &portion, state->lucia_position, ray_direction, it->position_start, it->position_end) && (!intersected_wall || scalar < intersected_scalar))
			{
				intersected_wall    = it;
				intersected_scalar  = scalar;
				intersected_portion = portion;
			}
		}

		if (intersected_wall && intersected_scalar > 0.01f)
		{
			i32 projected_wall_height = static_cast<i32>(WALL_HEIGHT / intersected_scalar * PIXELS_PER_METER);

			f32 mipmap_value   = intersected_scalar * (1.0f / 1.5f + 2.0f * (1.0f - fabsf(dot(ray_direction, rotate(normalize(intersected_wall->position_end - intersected_wall->position_start), TAU / 4.0f)))));
			i32 mipmap_level   = static_cast<i32>(mipmap_value);
			f32 mipmap_portion = mipmap_value - mipmap_level;

			FOR_RANGE(i, projected_wall_height)
			{
				i32 y = static_cast<i32>(VIEW_RESOLUTION.y / 2.0f + i - static_cast<i32>(LUCIA_HEIGHT / intersected_scalar * PIXELS_PER_METER));
				if (IN_RANGE(y, 0, VIEW_RESOLUTION.y))
				{
					vf2 uv = { lerp(intersected_wall->texture_start, intersected_wall->texture_end, intersected_portion), static_cast<f32>(i) / projected_wall_height };
					set_color
					(
						state->view,
						x,
						y,
						mipmap_level >= state->texture_wall.levels - 1
							? get_mipmap_sample_at_level(&state->texture_wall, uv, state->texture_wall.levels - 1)
							: lerp
								(
									get_mipmap_sample_at_level(&state->texture_wall, uv, mipmap_level    ),
									get_mipmap_sample_at_level(&state->texture_wall, uv, mipmap_level + 1),
									mipmap_portion
								)
					);
				}
			}
		}
	}

	SDL_Rect dst = { static_cast<i32>(VIEW_PADDING), static_cast<i32>(VIEW_PADDING), static_cast<i32>(WIN_DIM.x - VIEW_PADDING * 2.0f), static_cast<i32>((WIN_DIM.x - VIEW_PADDING * 2.0f) * VIEW_RESOLUTION.y / VIEW_RESOLUTION.x) };
	SDL_BlitScaled(state->view, 0, platform->surface, &dst);
}
