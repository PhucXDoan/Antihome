#define STB_IMAGE_IMPLEMENTATION true
#include <stb_image.h>
#include "unified.h"
#include "platform.h"
#include "utilities.cpp"

global constexpr vf2 VIEW_DIM       = { 350.0f, 175.0f };
global constexpr f32 HORT_TO_VERT_K = 0.927295218f * VIEW_DIM.x;
global constexpr f32 WALL_HEIGHT    = 2.7432f;
global constexpr f32 WALL_THICKNESS = 0.5f;
global constexpr f32 LUCIA_HEIGHT   = 1.4986f;
global constexpr vf2 WALLS[][2]     =
	{
		{ { -1.5f, -1.5f }, {  8.5f, -1.5f } },
		{ { -1.5f,  1.5f }, {  4.5f,  7.5f } },
		{ {  4.5f,  7.5f }, {  8.5f,  1.5f } },
		{ {  8.5f, -1.5f }, {  8.5f,  1.5f } }
	};

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
	ColumnMajorTexture ceiling;
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

	state->wall    = init_column_major_texture(DATA_DIR "wall.bmp");
	state->floor   = init_column_major_texture(DATA_DIR "floor.bmp");
	state->ceiling = init_column_major_texture(DATA_DIR "ceiling.bmp");
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	deinit_column_major_texture(&state->ceiling);
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

	vf2 displacement = state->lucia_velocity * SECONDS_PER_UPDATE;
	FOR_RANGE(4)
	{
		Intersection closest_intersection;
		closest_intersection.status   = IntersectionStatus::none;
		closest_intersection.position = { NAN, NAN };
		closest_intersection.normal   = { NAN, NAN };
		closest_intersection.distance = NAN;

		FOR_ELEMS(it, WALLS)
		{
			Intersection intersection = intersect_thick_line_segment(state->lucia_position, displacement, (*it)[0], (*it)[1], WALL_THICKNESS);

			if
			(
				closest_intersection.status == IntersectionStatus::none    ||
				intersection.status         == IntersectionStatus::outside &&  closest_intersection.status == IntersectionStatus::outside && intersection.distance < closest_intersection.distance ||
				intersection.status         == IntersectionStatus::inside  && (closest_intersection.status == IntersectionStatus::outside || intersection.distance > closest_intersection.distance)
			)
			{
				closest_intersection = intersection;
			}
		}

		if (closest_intersection.status == IntersectionStatus::none)
		{
			break;
		}
		else
		{
			state->lucia_position = closest_intersection.position;
			displacement          = dot(state->lucia_position + displacement - closest_intersection.position, { -closest_intersection.normal.y, closest_intersection.normal.x }) * vf2 { -closest_intersection.normal.y, closest_intersection.normal.x };
		}
	}
	state->lucia_position += displacement;

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

	constexpr i32 VIEW_PADDING = 10;

	fill(platform->surface, { 0.0f, 0.0f, 0.0f, 1.0f });
	fill(state->view      , { 1.0f, 0.0f, 0.0f, 1.0f });

	#if 1
	f32 lucia_eye_level = LUCIA_HEIGHT + 0.025f * (cosf(state->lucia_head_bob_keytime * TAU) - 1.0f);

	FOR_RANGE(x, VIEW_DIM.x)
	{
		vf2 ray_horizontal = polar(state->lucia_angle + (0.5f - x / VIEW_DIM.x) * state->lucia_fov);

		i32 wall_index    = -1;
		f32 wall_distance = NAN;
		f32 wall_portion  = NAN;

		FOR_ELEMS(it, WALLS)
		{
			f32 distance;
			f32 portion;
			if (ray_cast_line(&distance, &portion, state->lucia_position, ray_horizontal, (*it)[0], (*it)[1]) && IN_RANGE(portion, 0.0f, 1.0f) && (wall_index == -1 || distance < wall_distance))
			{
				wall_index    = it_index;
				wall_distance = distance;
				wall_portion  = portion;
			}
		}

		i32 pixel_starting_y = -1;
		i32 pixel_ending_y   = 0;
		if (wall_index != -1)
		{
			i32 starting_y = static_cast<i32>(VIEW_DIM.y / 2.0f - HORT_TO_VERT_K / state->lucia_fov * (WALL_HEIGHT - lucia_eye_level) / wall_distance);
			i32 ending_y   = static_cast<i32>(VIEW_DIM.y / 2.0f + HORT_TO_VERT_K / state->lucia_fov * lucia_eye_level / wall_distance);

			pixel_starting_y = MAXIMUM(0, starting_y);
			pixel_ending_y   = MINIMUM(ending_y, static_cast<i32>(VIEW_DIM.y));

			vf4* texture_column = &state->wall.colors[static_cast<i32>(wall_portion * (state->wall.w - 1.0f)) * state->wall.h];
			FOR_RANGE(y, pixel_starting_y, pixel_ending_y)
			{
				*(reinterpret_cast<u32*>(state->view->pixels) + y * state->view->w + x) =
					to_pixel
					(
						state->view,
						texture_column[static_cast<i32>(static_cast<f32>(ending_y - y) / (ending_y - starting_y) * state->wall.h)]
					);
			}
		}

		FOR_RANGE(y, 0, VIEW_DIM.y)
		{
			if (y == pixel_starting_y)
			{
				y = pixel_ending_y - 1;
			}
			else
			{
				ColumnMajorTexture* texture;
				f32                 texture_dimension;
				f32                 texture_level;
				if (y < VIEW_DIM.y / 2.0f)
				{
					texture           = &state->ceiling;
					texture_dimension = 4.0f;
					texture_level     = WALL_HEIGHT;
				}
				else
				{
					texture           = &state->floor;
					texture_dimension = 4.0f;
					texture_level     = 0.0f;
				}

				f32 pitch = (VIEW_DIM.y / 2.0f - y) * state->lucia_fov / HORT_TO_VERT_K;
				vf2 distances;
				vf2 portions;
				if
				(
					ray_cast_line(&distances.x, &portions.x, { state->lucia_position.x, lucia_eye_level }, normalize(vf2 { ray_horizontal.x, pitch }), { 0.0f, texture_level }, { texture_dimension, texture_level }) &&
					ray_cast_line(&distances.y, &portions.y, { state->lucia_position.y, lucia_eye_level }, normalize(vf2 { ray_horizontal.y, pitch }), { 0.0f, texture_level }, { texture_dimension, texture_level })
				)
				{
					f32 distance = sqrtf(square(distances.x) + square(distances.y) - square(lucia_eye_level));

					portions.x = fmodf(portions.x, 1.0f);
					portions.y = fmodf(portions.y, 1.0f);
					if (portions.x < 0.0f) { portions.x += 1.0f; }
					if (portions.y < 0.0f) { portions.y += 1.0f; }

					f32 k     = CLAMP(1.0f - distance / 16.0f, 0.0f, 1.0f);
					vf4 color = *(texture->colors + static_cast<i32>(portions.x * (texture->w - 1.0f)) * texture->h + static_cast<i32>(portions.y * texture->h));
					color.x  *= k;
					color.y  *= k;
					color.z  *= k;

					*(reinterpret_cast<u32*>(state->view->pixels) + y * state->view->w + x) = to_pixel(state->view, color);
				}
			}
		}
	}
	#else
	#define CONJUGATE(V) (vf2 { (V).x, -(V).y })

	const f32 PIXELS_PER_METER = 25.0f + 10.0f / state->lucia_fov;
	const vf2 ORIGIN           = state->lucia_position;

	FOR_ELEMS(it, WALLS)
	{
		const vf2 DIRECTION  = normalize((*it)[1] - (*it)[0]);
		const vf2 VERTICES[] =
			{
				(*it)[0] + (-DIRECTION + vf2 {  DIRECTION.y, -DIRECTION.x }) * WALL_THICKNESS,
				(*it)[1] + ( DIRECTION + vf2 {  DIRECTION.y, -DIRECTION.x }) * WALL_THICKNESS,
				(*it)[1] + ( DIRECTION + vf2 { -DIRECTION.y,  DIRECTION.x }) * WALL_THICKNESS,
				(*it)[0] + (-DIRECTION + vf2 { -DIRECTION.y,  DIRECTION.x }) * WALL_THICKNESS
			};

		draw_line
		(
			state->view,
			VIEW_DIM / 2.0f + CONJUGATE(-ORIGIN + (*it)[0]) * PIXELS_PER_METER,
			VIEW_DIM / 2.0f + CONJUGATE(-ORIGIN + (*it)[1]) * PIXELS_PER_METER,
			{ 1.0f, 1.0f, 1.0f, 1.0f }
		);

		FOR_RANGE(i, 4)
		{
			draw_line
			(
				state->view,
				VIEW_DIM / 2.0f + CONJUGATE(-ORIGIN + VERTICES[i]          ) * PIXELS_PER_METER,
				VIEW_DIM / 2.0f + CONJUGATE(-ORIGIN + VERTICES[(i + 1) % 4]) * PIXELS_PER_METER,
				{ 1.0f, 0.9f, 0.4f, 1.0f }
			);
		}
	}

	constexpr f32 LUCIA_DIM = 4.0f;
	fill(state->view, VIEW_DIM / 2.0f + CONJUGATE(-ORIGIN + state->lucia_position) * PIXELS_PER_METER - vf2 { LUCIA_DIM, LUCIA_DIM } / 2.0f, vf2 { LUCIA_DIM, LUCIA_DIM }, { 0.8f, 0.4f, 0.6f, 1.0f });
	draw_line
	(
		state->view,
		VIEW_DIM / 2.0f + CONJUGATE(-ORIGIN + state->lucia_position                                   ) * PIXELS_PER_METER,
		VIEW_DIM / 2.0f + CONJUGATE(-ORIGIN + state->lucia_position + polar(state->lucia_angle) * 1.0f) * PIXELS_PER_METER,
		{ 0.4f, 0.8f, 0.6f, 1.0f }
	);
	#endif

	SDL_Rect dst = { static_cast<i32>(VIEW_PADDING), static_cast<i32>(VIEW_PADDING), static_cast<i32>(WIN_DIM.x - VIEW_PADDING * 2.0f), static_cast<i32>((WIN_DIM.x - VIEW_PADDING * 2.0f) * VIEW_DIM.y / VIEW_DIM.x) };
	SDL_BlitScaled(state->view, 0, platform->surface, &dst);
}
