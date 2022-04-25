// @NOTE@ Credits
// "A Fast Voxel Traversal Algorithm for Ray Tracing" https://www.flipcode.com/archives/A%20faster%20voxel%20traversal%20algorithm%20for%20ray%20tracing.pdf

#define STB_IMAGE_IMPLEMENTATION true
#include <stb_image.h>
#include "unified.h"
#include "platform.h"
#include "utilities.cpp"

global constexpr vi2 VIEW_DIM               = { 350, 175 };
global constexpr f32 HORT_TO_VERT_K         = 0.927295218f * VIEW_DIM.x;
global constexpr f32 WALL_HEIGHT            = 2.7432f;
global constexpr f32 WALL_THICKNESS         = 0.25f;
global constexpr f32 LUCIA_HEIGHT           = 1.4986f;
global constexpr i32 MAP_DIM                = 64;
global constexpr f32 WALL_SPACING           = 3.0f;
global constexpr vf2 WALL_LAYOUT_DATA[4][3] =
	{
		{ { 0.0f, 0.0f }, { 0.0f, 1.0f }, {     -1.0f,      0.0f } },
		{ { 0.0f, 0.0f }, { 1.0f, 0.0f }, {      0.0f,      1.0f } },
		{ { 1.0f, 0.0f }, { 0.0f, 1.0f }, { -INVSQRT2, -INVSQRT2 } },
		{ { 0.0f, 0.0f }, { 1.0f, 1.0f }, { -INVSQRT2,  INVSQRT2 } }
	};

flag_struct (WallLayout, u8)
{
	left          = 1 << 0,
	bottom        = 1 << 1,
	back_slash    = 1 << 2,
	forward_slash = 1 << 3
};

struct Pixel
{
	vf3 color;
	f32 inv_depth;
};

struct State
{
	u32                seed;
	SDL_Surface*       view;
	Pixel              frame_buffer[VIEW_DIM.x][VIEW_DIM.y];
	vf2                lucia_velocity;
	vf3                lucia_position;
	f32                lucia_angle_velocity;
	f32                lucia_angle;
	f32                lucia_fov;
	f32                lucia_head_bob_keytime;
	vf3                flashlight_ray;
	f32                flashlight_keytime;
	WallLayout         wall_layouts[MAP_DIM * MAP_DIM];
	ColumnMajorTexture wall;
	ColumnMajorTexture floor;
	ColumnMajorTexture ceiling;
	Image              monster_img;
	Sprite             monster_sprite;
};

internal WallLayout* get_wall_layout(State* state, i32 x, i32 y)
{
	return &state->wall_layouts[((y % MAP_DIM) + MAP_DIM) % MAP_DIM * MAP_DIM + ((x % MAP_DIM) + MAP_DIM) % MAP_DIM];
}

internal void write_pixel(Pixel* pixel, Pixel new_pixel)
{
	if (pixel->inv_depth <= new_pixel.inv_depth)
	{
		*pixel = new_pixel;
	}
}

extern "C" PROTOTYPE_INITIALIZE(initialize)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	*state = {};

	state->lucia_position  = { 0.5f, 0.5f, LUCIA_HEIGHT };
	state->lucia_fov       = TAU / 3.0f;

	FOR_RANGE(start_walk_y, MAP_DIM)
	{
		FOR_RANGE(start_walk_x, MAP_DIM)
		{
			if (!+*get_wall_layout(state, start_walk_x, start_walk_y) && rng(&state->seed) < 0.15f)
			{
				i32 walk_x = start_walk_x;
				i32 walk_y = start_walk_y;
				FOR_RANGE(64)
				{
					switch (static_cast<i32>(rng(&state->seed) * 4.0f))
					{
						case 0:
						{
							if (!+*get_wall_layout(state, walk_x - 1, walk_y) && !+(*get_wall_layout(state, walk_x - 1, walk_y - 1) & WallLayout::left) && !+(*get_wall_layout(state, walk_x - 2, walk_y) & WallLayout::bottom))
							{
								walk_x = (walk_x - 1 + MAP_DIM) % MAP_DIM;
								*get_wall_layout(state, walk_x, walk_y) |= WallLayout::bottom;
							}
						} break;

						case 1:
						{
							if (!+*get_wall_layout(state, walk_x + 1, walk_y) && !+(*get_wall_layout(state, walk_x, walk_y) & WallLayout::bottom) && !+(*get_wall_layout(state, walk_x + 1, walk_y - 1) & WallLayout::left))
							{
								*get_wall_layout(state, walk_x, walk_y) |= WallLayout::bottom;
								walk_x = (walk_x + 1) % MAP_DIM;
							}
						} break;

						case 2:
						{
							if (!+*get_wall_layout(state, walk_x, walk_y - 1) && !+(*get_wall_layout(state, walk_x - 1, walk_y - 1) & WallLayout::bottom) && !+(*get_wall_layout(state, walk_x, walk_y - 2) & WallLayout::left))
							{
								walk_y = (walk_y - 1 + MAP_DIM) % MAP_DIM;
								*get_wall_layout(state, walk_x, walk_y) |= WallLayout::left;
							}
						} break;

						case 3:
						{
							if (!+*get_wall_layout(state, walk_x, walk_y + 1) && !+(*get_wall_layout(state, walk_x, walk_y) & WallLayout::left) && !+(*get_wall_layout(state, walk_x - 1, walk_y + 1) & WallLayout::bottom))
							{
								*get_wall_layout(state, walk_x, walk_y) |= WallLayout::left;
								walk_y = (walk_y + 1) % MAP_DIM;
							}
						} break;
					}
				}
			}
		}
	}

	FOR_RANGE(y, MAP_DIM)
	{
		FOR_RANGE(x, MAP_DIM)
		{
			if (*get_wall_layout(state, x, y) == (WallLayout::left | WallLayout::bottom) && rng(&state->seed) < 0.5f)
			{
				*get_wall_layout(state, x, y) = WallLayout::back_slash;
			}
			else if (+(*get_wall_layout(state, x + 1, y) & WallLayout::left) && +(*get_wall_layout(state, x, y + 1) & WallLayout::bottom) && rng(&state->seed) < 0.5f)
			{
				*get_wall_layout(state, x + 1, y    ) &= ~WallLayout::left;
				*get_wall_layout(state, x    , y + 1) &= ~WallLayout::bottom;
				*get_wall_layout(state, x    , y    ) |= WallLayout::back_slash;
			}
			else if (+(*get_wall_layout(state, x, y) & WallLayout::bottom) && *get_wall_layout(state, x + 1, y) == WallLayout::left && rng(&state->seed) < 0.5f)
			{
				*get_wall_layout(state, x    , y) &= ~WallLayout::bottom;
				*get_wall_layout(state, x + 1, y) &= ~WallLayout::left;
				*get_wall_layout(state, x    , y) |=  WallLayout::forward_slash;
			}
			else if (+(*get_wall_layout(state, x, y) & WallLayout::left) && *get_wall_layout(state, x, y + 1) == WallLayout::bottom && rng(&state->seed) < 0.5f)
			{
				*get_wall_layout(state, x, y    ) &= ~WallLayout::left;
				*get_wall_layout(state, x, y + 1) &= ~WallLayout::bottom;
				*get_wall_layout(state, x, y    ) |=  WallLayout::forward_slash;
			}
		}
	}

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
			VIEW_DIM.x,
			VIEW_DIM.y,
			32,
			0x000000FF,
			0x0000FF00,
			0x00FF0000,
			0xFF000000
		);

	state->wall        = init_column_major_texture(DATA_DIR "wall.png");
	state->floor       = init_column_major_texture(DATA_DIR "floor.png");
	state->ceiling     = init_column_major_texture(DATA_DIR "ceiling.png");
	state->monster_img = init_image(DATA_DIR "sprite.png");

	state->monster_sprite.img = &state->monster_img;
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	deinit_image(&state->monster_img);
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

		FOR_RANGE(y, MAP_DIM)
		{
			FOR_RANGE(x, MAP_DIM)
			{
				FOR_ELEMS(layout_position, WALL_LAYOUT_DATA)
				{
					if (+(*get_wall_layout(state, x, y) & static_cast<WallLayout>(1 << layout_position_index)))
					{
						vf2 start = (vf2 { static_cast<f32>(x), static_cast<f32>(y) } + (*layout_position)[0]) * WALL_SPACING;
						vf2 end   = (vf2 { static_cast<f32>(x), static_cast<f32>(y) } + (*layout_position)[1]) * WALL_SPACING;

						Intersection intersection = intersect_thick_line_segment(state->lucia_position.xy, displacement, start, end, WALL_THICKNESS);

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
				}
			}
		}

		if (closest_intersection.status == IntersectionStatus::none)
		{
			break;
		}
		else
		{
			state->lucia_position.xy = closest_intersection.position;
			displacement             = dot(state->lucia_position.xy + displacement - closest_intersection.position, { -closest_intersection.normal.y, closest_intersection.normal.x }) * vf2 { -closest_intersection.normal.y, closest_intersection.normal.x };
		}
	}
	state->lucia_position.xy += displacement;
	state->lucia_position.z   = LUCIA_HEIGHT + 0.1f * (cosf(state->lucia_head_bob_keytime * TAU) - 1.0f);

	state->lucia_position.x = mod(state->lucia_position.x, MAP_DIM * WALL_SPACING);
	state->lucia_position.y = mod(state->lucia_position.y, MAP_DIM * WALL_SPACING);

	state->lucia_head_bob_keytime += 0.001f + 0.35f * norm(state->lucia_velocity) * SECONDS_PER_UPDATE;
	if (state->lucia_head_bob_keytime > 1.0f)
	{
		state->lucia_head_bob_keytime -= 1.0f;
	}

	state->lucia_fov += platform->scroll * 0.1f;

	// @TEMP@
	persist f32 TEMP;
	TEMP += 1.5f * SECONDS_PER_UPDATE;

	if (norm(state->monster_sprite.position.xy - state->lucia_position.xy) > 1.5f)
	{
		state->monster_sprite.position.xy = dampen(state->monster_sprite.position.xy, state->lucia_position.xy, 0.5f, SECONDS_PER_UPDATE);
	}
	state->monster_sprite.position.z = (cosf(TEMP * 2.0f) * 0.15f) + WALL_HEIGHT / 2.0f;

	state->monster_sprite.orientation_x = 0.001f * normalize(-cross(normalize(state->lucia_position - state->monster_sprite.position), { 0.0f, 0.0f, 1.0f })) * static_cast<f32>(state->monster_sprite.img->w);
	state->monster_sprite.orientation_y = 0.001f * vf3 { 0.0f, 0.0f, static_cast<f32>(state->monster_sprite.img->h) };

	state->flashlight_keytime += 0.0025f + 0.05f * norm(state->lucia_velocity) * SECONDS_PER_UPDATE;
	if (state->flashlight_keytime > 1.0f)
	{
		state->flashlight_keytime -= 1.0f;
	}

	state->flashlight_ray.xy  = dampen(state->flashlight_ray.xy, polar(state->lucia_angle + sinf(state->flashlight_keytime * TAU * 2.0f) * 0.03f), 16.0f, SECONDS_PER_UPDATE);
	state->flashlight_ray.z   = sinf(state->flashlight_keytime * TAU) * 0.05f;
	state->flashlight_ray     = normalize(state->flashlight_ray);

	return UpdateCode::resume;
}

extern "C" PROTOTYPE_RENDER(render)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	constexpr i32 VIEW_PADDING = 10;

	fill(platform->surface, { 0.05f, 0.10f, 0.15f });
	memset(state->frame_buffer, 0, sizeof(state->frame_buffer));

	constexpr f32 AMBIENT_LIGHT_RADIUS = 4.0f;
	constexpr f32 FLASHLIGHT_POW       = 8.0f;

	#if 1
	FOR_RANGE(x, VIEW_DIM.x)
	{
		#if 0
		persist u64 DEBUG_total;
		persist u64 DEBUG_counter;
		LARGE_INTEGER DEBUG_li0;
		QueryPerformanceCounter(&DEBUG_li0);

		LARGE_INTEGER DEBUG_li1;
		QueryPerformanceCounter(&DEBUG_li1);

		DEBUG_total   += DEBUG_li1.QuadPart - DEBUG_li0.QuadPart;
		DEBUG_counter += 1;
		if (DEBUG_counter > 4096)
		{
			DEBUG_printf("%llu\n", DEBUG_total);
			DEBUG_total   = 0;
			DEBUG_counter = 0;
		}
		#endif

		vf2 ray_horizontal = polar(state->lucia_angle + (0.5f - static_cast<f32>(x) / VIEW_DIM.x) * state->lucia_fov);

		bool32 wall_exists   = false;
		vf2    wall_normal   = { NAN, NAN };
		f32    wall_distance = NAN;
		f32    wall_portion  = NAN;

		vi2 step =
			{
				ray_horizontal.x < 0.0f ? -1 : 1,
				ray_horizontal.y < 0.0f ? -1 : 1
			};
		vf2 t_max =
			{
				((step.x == -1 ? floorf : ceilf)(state->lucia_position.x / WALL_SPACING) * WALL_SPACING - state->lucia_position.x) / ray_horizontal.x,
				((step.y == -1 ? floorf : ceilf)(state->lucia_position.y / WALL_SPACING) * WALL_SPACING - state->lucia_position.y) / ray_horizontal.y
			};
		vf2 t_delta =
			{
				step.x / ray_horizontal.x * WALL_SPACING,
				step.y / ray_horizontal.y * WALL_SPACING
			};
		vi2 coordinates =
			{
				static_cast<i32>(floorf(state->lucia_position.x / WALL_SPACING)),
				static_cast<i32>(floorf(state->lucia_position.y / WALL_SPACING))
			};
		FOR_RANGE(MAP_DIM * MAP_DIM)
		{
			FOR_ELEMS(layout_data, WALL_LAYOUT_DATA)
			{
				if (+(state->wall_layouts[mod(coordinates.y, MAP_DIM) * MAP_DIM + mod(coordinates.x, MAP_DIM)] & static_cast<WallLayout>(1 << layout_data_index)))
				{
					f32 distance;
					f32 portion;
					if
					(
						ray_cast_line
						(
							&distance,
							&portion,
							state->lucia_position.xy,
							ray_horizontal,
							(coordinates + (*layout_data)[0]) * WALL_SPACING,
							(coordinates + (*layout_data)[1]) * WALL_SPACING
						)
						&& IN_RANGE(portion, 0.0f, 1.0f)
						&& (!wall_exists || distance < wall_distance)
					)
					{
						wall_exists   = true;
						wall_normal   = (*layout_data)[2];
						wall_distance = distance;
						wall_portion  = portion;
					}
				}
			}

			if (wall_exists)
			{
				break;
			}

			if (t_max.x < t_max.y)
			{
				t_max.x       += t_delta.x;
				coordinates.x += step.x;
			}
			else
			{
				t_max.y       += t_delta.y;
				coordinates.y += step.y;
			}
		}

		i32 pixel_starting_y = -1;
		i32 pixel_ending_y   = 0;
		if (wall_exists)
		{
			i32 starting_y = static_cast<i32>(VIEW_DIM.y / 2.0f - HORT_TO_VERT_K / state->lucia_fov *                state->lucia_position.z  / (wall_distance + 0.1f));
			i32 ending_y   = static_cast<i32>(VIEW_DIM.y / 2.0f + HORT_TO_VERT_K / state->lucia_fov * (WALL_HEIGHT - state->lucia_position.z) / (wall_distance + 0.1f));

			pixel_starting_y = MAXIMUM(0, starting_y);
			pixel_ending_y   = MINIMUM(ending_y, VIEW_DIM.y);

			FOR_RANGE(y, pixel_starting_y, pixel_ending_y)
			{
				vf3 ray = normalize({ ray_horizontal.x, ray_horizontal.y, (y - VIEW_DIM.y / 2.0f) * state->lucia_fov / HORT_TO_VERT_K });
				f32 k   =
					powf(CLAMP(dot(ray, state->flashlight_ray), 0.0f, 1.0f), FLASHLIGHT_POW) * square(CLAMP(1.0f - wall_distance / 32.0f, 0.0f, 1.0f))
					+ fabsf(dot(ray, { wall_normal.x, wall_normal.y, 0.0f })) * square(CLAMP(1.0f - wall_distance / AMBIENT_LIGHT_RADIUS, 0.0f, 1.0f));
				write_pixel
				(
					&state->frame_buffer[x][y],
					{
						*(state->wall.colors + static_cast<i32>(wall_portion * (state->wall.w - 1.0f)) * state->wall.h + static_cast<i32>(static_cast<f32>(y - starting_y) / (ending_y - starting_y) * state->wall.h)) * CLAMP(k, 0.0f, 1.0f),
						1.0f / wall_distance
					}
				);
			}
		}

		FOR_RANGE(y, 0, VIEW_DIM.y)
		{
			if (y == pixel_starting_y)
			{
				y = MAXIMUM(pixel_starting_y + 1, pixel_ending_y - 1);
				continue;
			}

			ColumnMajorTexture* texture;
			f32                 texture_dimension;
			f32                 texture_level;
			if (y < VIEW_DIM.y / 2.0f)
			{
				texture           = &state->floor;
				texture_dimension = 4.0f;
				texture_level     = 0.0f;
			}
			else
			{
				texture           = &state->ceiling;
				texture_dimension = 4.0f;
				texture_level     = WALL_HEIGHT;
			}

			f32 pitch = (y - VIEW_DIM.y / 2.0f) * state->lucia_fov / HORT_TO_VERT_K;
			vf3 ray   = normalize({ ray_horizontal.x, ray_horizontal.y, pitch });
			vf2 distances;
			vf2 portions;
			if
			(
				ray_cast_line(&distances.x, &portions.x, { state->lucia_position.x, state->lucia_position.z }, { ray.x, ray.z }, { 0.0f, texture_level }, { texture_dimension, texture_level }) &&
				ray_cast_line(&distances.y, &portions.y, { state->lucia_position.y, state->lucia_position.z }, { ray.y, ray.z }, { 0.0f, texture_level }, { texture_dimension, texture_level })
			)
			{
				portions.x = mod(portions.x, 1.0f);
				portions.y = mod(portions.y, 1.0f);

				f32 distance = sqrtf(square(distances.x) + square(distances.y) - square(state->lucia_position.z));
				f32 k        =
					powf(CLAMP(dot(ray, state->flashlight_ray), 0.0f, 1.0f), FLASHLIGHT_POW) * square(CLAMP(1.0f - distance / 32.0f, 0.0f, 1.0f))
					+ fabsf(dot(ray, { 0.0f, 0.0f, 1.0f })) * square(CLAMP(1.0f - distance / AMBIENT_LIGHT_RADIUS, 0.0f, 1.0f));

				write_pixel
				(
					&state->frame_buffer[x][y],
					{
						*(texture->colors + static_cast<i32>(portions.x * (texture->w - 1.0f)) * texture->h + static_cast<i32>(portions.y * texture->h)) * CLAMP(k, 0.0f, 1.0f),
						1.0f / distance
					}
				);
			}
		}

		FOR_RANGE(y, 0, VIEW_DIM.y)
		{
			vf3 ray = normalize({ ray_horizontal.x, ray_horizontal.y, (y - VIEW_DIM.y / 2.0f) * state->lucia_fov / HORT_TO_VERT_K });

			f32 distance;
			vf2 portion;
			if
			(
				ray_cast_plane
				(
					&distance,
					&portion,
					state->lucia_position + 0.5f * (state->monster_sprite.orientation_x + state->monster_sprite.orientation_y),
					ray,
					state->monster_sprite.position,
					state->monster_sprite.orientation_x,
					state->monster_sprite.orientation_y
				)
				&& IN_RANGE(portion.x, 0.0f, 1.0f)
				&& IN_RANGE(portion.y, 0.0f, 1.0f)
			)
			{
				vf4* sprite_rgba = state->monster_sprite.img->pixels + static_cast<i32>((1.0f - portion.y) * (state->monster_sprite.img->h - 1.0f)) * state->monster_sprite.img->w + static_cast<i32>(portion.x * state->monster_sprite.img->w);
				f32 k            =
					powf(CLAMP(dot(ray, state->flashlight_ray), 0.0f, 1.0f), FLASHLIGHT_POW) * square(CLAMP(1.0f - distance / 32.0f, 0.0f, 1.0f))
					+ fabsf(dot(ray, { 0.0f, 0.0f, 1.0f })) * square(CLAMP(1.0f - distance / AMBIENT_LIGHT_RADIUS, 0.0f, 1.0f));

				write_pixel
				(
					&state->frame_buffer[x][y],
					{
						lerp
						(
							state->frame_buffer[x][y].color,
							sprite_rgba->xyz * CLAMP(k, 0.0f, 1.0f),
							sprite_rgba->w
						),
						1.0f / distance
					}
				);
			}
		}
	}

	FOR_RANGE(y, VIEW_DIM.y)
	{
		FOR_RANGE(x, VIEW_DIM.x)
		{
			*(reinterpret_cast<u32*>(state->view->pixels) + (VIEW_DIM.y - 1 - y) * state->view->w + x) =
				SDL_MapRGB
				(
					state->view->format,
					static_cast<u8>(state->frame_buffer[x][y].color.x * 255.0f),
					static_cast<u8>(state->frame_buffer[x][y].color.y * 255.0f),
					static_cast<u8>(state->frame_buffer[x][y].color.z * 255.0f)
				);
		}
	}
	#elif 0
	const f32 PIXELS_PER_METER = 25.0f + 10.0f / state->lucia_fov;
	const vf2 ORIGIN           = state->lucia_position.xy;

	FOR_RANGE(y, MAP_DIM)
	{
		FOR_RANGE(x, MAP_DIM)
		{
			FOR_ELEMS(layout_position, WALL_LAYOUT_DATA)
			{
				if (+(*get_wall_layout(state, x, y) & static_cast<WallLayout>(1 << layout_position_index)))
				{
					vf2 start = (vf2 { static_cast<f32>(x), static_cast<f32>(y) } + (*layout_position)[0]) * WALL_SPACING;
					vf2 end   = (vf2 { static_cast<f32>(x), static_cast<f32>(y) } + (*layout_position)[1]) * WALL_SPACING;

					const vf2 DIRECTION  = normalize(end - start);
					const vf2 VERTICES[] =
						{
							start + (-DIRECTION + vf2 {  DIRECTION.y, -DIRECTION.x }) * WALL_THICKNESS,
							end   + ( DIRECTION + vf2 {  DIRECTION.y, -DIRECTION.x }) * WALL_THICKNESS,
							end   + ( DIRECTION + vf2 { -DIRECTION.y,  DIRECTION.x }) * WALL_THICKNESS,
							start + (-DIRECTION + vf2 { -DIRECTION.y,  DIRECTION.x }) * WALL_THICKNESS
						};

					draw_line
					(
						state->view,
						VIEW_DIM / 2.0f + conjugate(-ORIGIN + start) * PIXELS_PER_METER,
						VIEW_DIM / 2.0f + conjugate(-ORIGIN + end  ) * PIXELS_PER_METER,
						{ 1.0f, 1.0f, 1.0f }
					);

					FOR_RANGE(j, 4)
					{
						draw_line
						(
							state->view,
							VIEW_DIM / 2.0f + conjugate(-ORIGIN + VERTICES[j]          ) * PIXELS_PER_METER,
							VIEW_DIM / 2.0f + conjugate(-ORIGIN + VERTICES[(j + 1) % 4]) * PIXELS_PER_METER,
							{ 1.0f, 0.0f, 0.0f }
						);
					}
				}
			}
		}
	}

	constexpr f32 LUCIA_DIM = 4.0f;
	fill(state->view, VIEW_DIM / 2.0f + conjugate(-ORIGIN + state->lucia_position.xy) * PIXELS_PER_METER - vf2 { LUCIA_DIM, LUCIA_DIM } / 2.0f, vf2 { LUCIA_DIM, LUCIA_DIM }, { 0.0f, 0.0f, 1.0f });
	draw_line
	(
		state->view,
		VIEW_DIM / 2.0f + conjugate(-ORIGIN + state->lucia_position.xy                                   ) * PIXELS_PER_METER,
		VIEW_DIM / 2.0f + conjugate(-ORIGIN + state->lucia_position.xy + polar(state->lucia_angle) * 1.0f) * PIXELS_PER_METER,
		{ 0.4f, 0.8f, 0.6f }
	);
	#else
	fill(state->view, { 1.0f, 1.0f, 1.0f });

	persist vf2 cam_pos = vf2 { MAP_DIM * WALL_SPACING, MAP_DIM * WALL_SPACING } / 2.0f;

	constexpr f32 SPEED = 32.0f;
	if (HOLDING(Input::left))
	{
		cam_pos.x -= SPEED * SECONDS_PER_UPDATE;
	}
	if (HOLDING(Input::right))
	{
		cam_pos.x += SPEED * SECONDS_PER_UPDATE;
	}
	if (HOLDING(Input::down))
	{
		cam_pos.y -= SPEED * SECONDS_PER_UPDATE;
	}
	if (HOLDING(Input::up))
	{
		cam_pos.y += SPEED * SECONDS_PER_UPDATE;
	}

	constexpr f32 PIXELS_PER_METER = 5.0f;

	vf2 ray  = polar(state->lucia_angle);
	vi2 step =
		{
			ray.x < 0.0f ? -1 : 1,
			ray.y < 0.0f ? -1 : 1
		};

	vf2 t_max =
		{
			step.x == -1 ? (floorf(state->lucia_position.x / WALL_SPACING) - state->lucia_position.x / WALL_SPACING) / (ray.x / WALL_SPACING) : (ceilf(state->lucia_position.x / WALL_SPACING) - state->lucia_position.x / WALL_SPACING) / (ray.x / WALL_SPACING),
			step.y == -1 ? (floorf(state->lucia_position.y / WALL_SPACING) - state->lucia_position.y / WALL_SPACING) / (ray.y / WALL_SPACING) : (ceilf(state->lucia_position.y / WALL_SPACING) - state->lucia_position.y / WALL_SPACING) / (ray.y / WALL_SPACING)
		};

	vf2 t_delta =
		{
			step.x / (ray.x / WALL_SPACING),
			step.y / (ray.y / WALL_SPACING)
		};

	vi2 pos = { static_cast<i32>(floorf(state->lucia_position.x / WALL_SPACING)), static_cast<i32>(floorf(state->lucia_position.y / WALL_SPACING)) };
	FOR_RANGE(32)
	{
		fill
		(
			state->view,
			VIEW_DIM / 2.0f + conjugate(pos * WALL_SPACING - cam_pos) * PIXELS_PER_METER - vf2 { 0.0f, WALL_SPACING * PIXELS_PER_METER },
			vf2 { WALL_SPACING, WALL_SPACING } * PIXELS_PER_METER,
			{ 0.5f, 0.7f, 0.9f }
		);

		if (t_max.x < t_max.y)
		{
			t_max.x += t_delta.x;
			pos.x   += step.x;
		}
		else
		{
			t_max.y += t_delta.y;
			pos.y   += step.y;
		}
	}

	FOR_RANGE(y, MAP_DIM)
	{
		FOR_RANGE(x, MAP_DIM)
		{
			if (+(*get_wall_layout(state, x, y) & WallLayout::left))
			{
				draw_line
				(
					state->view,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 1.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					{ 0.0f, 0.0f, 0.0f }
				);
			}

			if (+(*get_wall_layout(state, x, y) & WallLayout::bottom))
			{
				draw_line
				(
					state->view,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 1.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					{ 0.0f, 0.0f, 0.0f }
				);
			}

			if (+(*get_wall_layout(state, x, y) & WallLayout::back_slash))
			{
				draw_line
				(
					state->view,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 1.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 1.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					{ 0.0f, 0.0f, 0.0f }
				);
			}

			if (+(*get_wall_layout(state, x, y) & WallLayout::forward_slash))
			{
				draw_line
				(
					state->view,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 1.0f, y + 1.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					{ 0.0f, 0.0f, 0.0f }
				);
			}
		}
	}

	fill(state->view, VIEW_DIM / 2.0f + conjugate(state->lucia_position.xy - cam_pos) * PIXELS_PER_METER - vf2 { 2.5f, 2.5f } / 2.0f, { 2.5f, 2.5f }, { 1.0f, 0.0f, 0.0f });
	draw_line
	(
		state->view,
		VIEW_DIM / 2.0f + conjugate(state->lucia_position.xy - cam_pos                                    ) * PIXELS_PER_METER - vf2 { 2.5f, 2.5f } / 4.0f,
		VIEW_DIM / 2.0f + conjugate(state->lucia_position.xy - cam_pos + polar(state->lucia_angle) * 6.0f) * PIXELS_PER_METER - vf2 { 2.5f, 2.5f } / 4.0f,
		{ 0.6f, 0.1f, 0.1f }
	);
	#endif

	SDL_Rect dst = { VIEW_PADDING, VIEW_PADDING, WIN_DIM.x - VIEW_PADDING * 2, (WIN_DIM.x - VIEW_PADDING * 2) * VIEW_DIM.y / VIEW_DIM.x };
	SDL_BlitScaled(state->view, 0, platform->surface, &dst);
}
