// @NOTE@ Credits
// "A Fast Voxel Traversal Algorithm for Ray Tracing" https://www.flipcode.com/archives/A%20faster%20voxel%20traversal%20algorithm%20for%20ray%20tracing.pdf

#define STB_IMAGE_IMPLEMENTATION true
#include <stb_image.h>
#include "unified.h"
#include "platform.h"
#include "utilities.cpp"

global constexpr vi2 VIEW_DIM       = { 350, 175 };
global constexpr f32 HORT_TO_VERT_K = 0.927295218f * VIEW_DIM.x;
global constexpr f32 WALL_HEIGHT    = 2.7432f;
global constexpr f32 WALL_THICKNESS = 0.25f;
global constexpr f32 LUCIA_HEIGHT   = 1.4986f;
global constexpr i32 MAP_DIM        = 16;
global constexpr f32 WALL_SPACING   = 3.0f;

flag_struct (WallVoxel, u8)
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

struct PathCoordinatesNode
{
	vi2                  coordinates;
	PathCoordinatesNode* next_node;
};

enum_loose (ThingType, u32)
{
	monster,
	battery,
	paper,
	hand,
	CAPACITY
};

struct Thing
{
	ThingType type;
	vf3       position;
	vf2       velocity;
	vf2       normal;
};

struct State
{
	MemoryArena          long_term_arena;
	MemoryArena          transient_arena;
	PathCoordinatesNode* available_path_coordinates_node;

	u32                  seed;
	f32                  time;
	SDL_Surface*         view;
	Pixel                frame_buffer[VIEW_DIM.x][VIEW_DIM.y];

	WallVoxel            wall_voxels[MAP_DIM][MAP_DIM];
	PathCoordinatesNode* monster_path;

	vf2                  lucia_velocity;
	vf3                  lucia_position;
	f32                  lucia_angle_velocity;
	f32                  lucia_angle;
	f32                  lucia_fov;
	f32                  lucia_head_bob_keytime;

	vf3                  flashlight_ray;
	f32                  flashlight_keytime;
	Thing*               hand_hovered_thing;

	Thing                monster;
	Thing                battery;
	Thing                paper;
	Thing                hand;

	ImgRGB               wall;
	ImgRGB               floor;
	ImgRGB               ceiling;
	ImgRGBA              hand_img;
	ImgRGBA              thing_imgs[ThingType::CAPACITY];
};

global constexpr struct { WallVoxel voxel; vf2 start; vf2 end; vf2 normal; } WALL_VOXEL_DATA[] =
	{
		{ WallVoxel::left         , { 0.0f, 0.0f }, { 0.0f, 1.0f }, {     -1.0f,      0.0f } },
		{ WallVoxel::bottom       , { 0.0f, 0.0f }, { 1.0f, 0.0f }, {      0.0f,      1.0f } },
		{ WallVoxel::back_slash   , { 1.0f, 0.0f }, { 0.0f, 1.0f }, { -INVSQRT2, -INVSQRT2 } },
		{ WallVoxel::forward_slash, { 0.0f, 0.0f }, { 1.0f, 1.0f }, { -INVSQRT2,  INVSQRT2 } }
	};

internal WallVoxel* get_wall_voxel(State* state, vi2 v)
{
	return &state->wall_voxels[mod(v.y, MAP_DIM)][mod(v.x, MAP_DIM)];
}

internal PathCoordinatesNode* allocate_path_coordinates_node(State* state)
{
	if (state->available_path_coordinates_node)
	{
		PathCoordinatesNode* node = state->available_path_coordinates_node;
		state->available_path_coordinates_node = state->available_path_coordinates_node->next_node;
		return node;
	}
	else
	{
		return memory_arena_push<PathCoordinatesNode>(&state->long_term_arena);
	}
}

internal PathCoordinatesNode* deallocate_path_coordinates_node(State* state, PathCoordinatesNode* node)
{
	PathCoordinatesNode* tail = node->next_node;
	node->next_node = state->available_path_coordinates_node;
	state->available_path_coordinates_node = node;
	return tail;
}

internal vf2 path_coordinates_to_position(vi2 coordinates)
{
	coordinates.y = mod(coordinates.y, MAP_DIM * 2);
	return
		vf2 {
			mod(coordinates.x, MAP_DIM) + (coordinates.y % 2 == 0 ? 0.5f : 0.0f),
			coordinates.y / 2.0f
		} * WALL_SPACING;
}

internal vi2 path_coordinates_to_map_coordinates(vi2 coordinates)
{
	return { mod(coordinates.x, MAP_DIM), mod(coordinates.y, MAP_DIM * 2) / 2 };
}

internal vi2 get_closest_open_path_coordinates(State* state, vf2 position)
{
	position.x = mod(position.x, MAP_DIM * WALL_SPACING);
	position.y = mod(position.y, MAP_DIM * WALL_SPACING);

	vi2 map_coordinates       = vxx(position / WALL_SPACING);
	vf2 remainder_coordinates = position / WALL_SPACING - map_coordinates;

	constexpr struct { vi2 delta_coordinates; WallVoxel side; vf2 offset; } SIDES[] =
		{
			{ { 0, 0 }, WallVoxel::bottom, { 0.5f, 0.0f } },
			{ { 0, 0 }, WallVoxel::left  , { 0.0f, 0.5f } },
			{ { 1, 0 }, WallVoxel::left  , { 1.0f, 0.5f } },
			{ { 0, 1 }, WallVoxel::bottom, { 0.5f, 1.0f } }
		};

	i32 best_index       = -1;
	f32 best_distance_sq = NAN;
	FOR_ELEMS(it, SIDES)
	{
		if (+(*get_wall_voxel(state, map_coordinates + it->delta_coordinates) & it->side))
		{
			continue;
		}
		if (+(*get_wall_voxel(state, map_coordinates) & WallVoxel::back_slash))
		{
			if (remainder_coordinates.x < 1.0f - remainder_coordinates.y)
			{
				if (!(it_index == 0 || it_index == 1))
				{
					continue;
				}
			}
			else if (!(it_index == 2 || it_index == 3))
			{
				continue;
			}
		}
		else if (+(*get_wall_voxel(state, map_coordinates) & WallVoxel::forward_slash))
		{
			if (remainder_coordinates.x < remainder_coordinates.y)
			{
				if (!(it_index == 1 || it_index == 3))
				{
					continue;
				}
			}
			else if (!(it_index == 0 || it_index == 2))
			{
				continue;
			}
		}

		f32 distance_sq = norm_sq(remainder_coordinates - it->offset);
		if (best_index == -1 || distance_sq < best_distance_sq)
		{
			best_index       = it_index;
			best_distance_sq = distance_sq;
		}
	}

	vi2 result = {};
	switch (best_index)
	{
		case 0: result = { map_coordinates.x    , map_coordinates.y * 2     }; break;
		case 1: result = { map_coordinates.x    , map_coordinates.y * 2 + 1 }; break;
		case 2: result = { map_coordinates.x + 1, map_coordinates.y * 2 + 1 }; break;
		case 3: result = { map_coordinates.x    , map_coordinates.y * 2 + 2 }; break;
		default: ASSERT(false);
	}

	result.x = mod(result.x, MAP_DIM    );
	result.y = mod(result.y, MAP_DIM * 2);
	return result;
}

internal f32 path_distance_function(vi2 a, vi2 b)
{
	a.x = mod(a.x, MAP_DIM    );
	a.y = mod(a.y, MAP_DIM * 2);
	b.x = mod(b.x, MAP_DIM    );
	b.y = mod(b.y, MAP_DIM * 2);

	f32 best = -1.0f;
	FOR_RANGE(i, 9)
	{
		vi2 deltas   = { abs(a.x - b.x + (i % 3 - 1) * MAP_DIM), abs(a.y - b.y + (i / 3 - 1) * MAP_DIM * 2) } ;
		f32 distance = abs(deltas.x - deltas.y) + min(deltas.x, deltas.y) * SQRT2;
		if (best == -1.0f || best > distance)
		{
			best = distance;
		}
	}

	return best;
}

internal vf2 ray_to_closest(vf2 position, vf2 target)
{
	position.x = mod(position.x, MAP_DIM * WALL_SPACING);
	position.y = mod(position.y, MAP_DIM * WALL_SPACING);
	target.x   = mod(target  .x, MAP_DIM * WALL_SPACING);
	target.y   = mod(target  .y, MAP_DIM * WALL_SPACING);

	i32 closest_index = -1;
	f32 best_distance = NAN;
	vf2 best_ray      = { NAN, NAN };
	FOR_RANGE(i, 9)
	{
		vf2 ray      = target - position + vi2 { i % 3 - 1, i / 3 - 1 } * MAP_DIM * WALL_SPACING;
		f32 distance = norm(ray);

		if (closest_index == -1 || best_distance > distance)
		{
			closest_index = i;
			best_distance = distance;
			best_ray      = ray;
		}
	}

	return best_ray;
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

	state->long_term_arena.size = static_cast<memsize>((platform->memory_capacity - sizeof(State)) * 0.75f);
	state->long_term_arena.base = platform->memory + sizeof(State);
	state->long_term_arena.used = 0;

	state->transient_arena.size = platform->memory_capacity - sizeof(State) - state->long_term_arena.size;
	state->transient_arena.base = platform->memory          + sizeof(State) + state->long_term_arena.size;
	state->transient_arena.used = 0;

	FOR_RANGE(start_walk_y, MAP_DIM)
	{
		FOR_RANGE(start_walk_x, MAP_DIM)
		{
			if (!+*get_wall_voxel(state, { start_walk_x, start_walk_y }) && rng(&state->seed) < 0.15f)
			{
				vi2 walk = { start_walk_x, start_walk_y };
				FOR_RANGE(64)
				{
					switch (static_cast<i32>(rng(&state->seed) * 4.0f))
					{
						case 0:
						{
							if (!+*get_wall_voxel(state, walk + vi2 { -1, 0 }) && !+(*get_wall_voxel(state, walk + vi2 { -1, -1 }) & WallVoxel::left) && !+(*get_wall_voxel(state, walk + vi2 { -2, 0 }) & WallVoxel::bottom))
							{
								walk.x = mod(walk.x - 1, MAP_DIM);
								*get_wall_voxel(state, walk) |= WallVoxel::bottom;
							}
						} break;

						case 1:
						{
							if (!+*get_wall_voxel(state, walk + vi2 { 1, 0 }) && !+(*get_wall_voxel(state, walk) & WallVoxel::bottom) && !+(*get_wall_voxel(state, walk + vi2 { 1, -1 }) & WallVoxel::left))
							{
								*get_wall_voxel(state, walk) |= WallVoxel::bottom;
								walk.x = mod(walk.x + 1, MAP_DIM);
							}
						} break;

						case 2:
						{
							if (!+*get_wall_voxel(state, walk + vi2 { 0, -1 }) && !+(*get_wall_voxel(state, walk + vi2 { -1, -1 }) & WallVoxel::bottom) && !+(*get_wall_voxel(state, walk + vi2 { 0, -2 }) & WallVoxel::left))
							{
								walk.y = mod(walk.y - 1, MAP_DIM);
								*get_wall_voxel(state, walk) |= WallVoxel::left;
							}
						} break;

						case 3:
						{
							if (!+*get_wall_voxel(state, walk + vi2 { 0, 1 }) && !+(*get_wall_voxel(state, walk) & WallVoxel::left) && !+(*get_wall_voxel(state, walk + vi2 { -1, 1 }) & WallVoxel::bottom))
							{
								*get_wall_voxel(state, walk) |= WallVoxel::left;
								walk.y = mod(walk.y + 1, MAP_DIM);
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
			if (*get_wall_voxel(state, { x, y }) == (WallVoxel::left | WallVoxel::bottom) && rng(&state->seed) < 0.5f)
			{
				*get_wall_voxel(state, { x, y }) = WallVoxel::back_slash;
			}
			else if (+(*get_wall_voxel(state, { x + 1, y }) & WallVoxel::left) && +(*get_wall_voxel(state, { x, y + 1 }) & WallVoxel::bottom) && rng(&state->seed) < 0.5f)
			{
				*get_wall_voxel(state, { x + 1, y     }) &= ~WallVoxel::left;
				*get_wall_voxel(state, { x    , y + 1 }) &= ~WallVoxel::bottom;
				*get_wall_voxel(state, { x    , y     }) |= WallVoxel::back_slash;
			}
			else if (+(*get_wall_voxel(state, { x, y }) & WallVoxel::bottom) && *get_wall_voxel(state, { x + 1, y }) == WallVoxel::left && rng(&state->seed) < 0.5f)
			{
				*get_wall_voxel(state, { x    , y }) &= ~WallVoxel::bottom;
				*get_wall_voxel(state, { x + 1, y }) &= ~WallVoxel::left;
				*get_wall_voxel(state, { x    , y }) |=  WallVoxel::forward_slash;
			}
			else if (+(*get_wall_voxel(state, { x, y }) & WallVoxel::left) && *get_wall_voxel(state, { x, y + 1 }) == WallVoxel::bottom && rng(&state->seed) < 0.5f)
			{
				*get_wall_voxel(state, { x, y     }) &= ~WallVoxel::left;
				*get_wall_voxel(state, { x, y + 1 }) &= ~WallVoxel::bottom;
				*get_wall_voxel(state, { x, y     }) |=  WallVoxel::forward_slash;
			}
		}
	}

	state->lucia_position.xy = { 0.75f * WALL_SPACING, 0.5f * WALL_SPACING };
	state->lucia_fov         = TAU / 3.0f;

	state->monster.type     = ThingType::monster;
	state->monster.position = { 2.5f * WALL_SPACING, 2.5f * WALL_SPACING };

	state->battery.type        = ThingType::battery;
	state->battery.position.xy = state->lucia_position.xy + vf2 { 0.5f * WALL_SPACING, 0.0f * WALL_SPACING };

	state->paper.type        = ThingType::paper;
	state->paper.position.xy = state->lucia_position.xy + vf2 { 0.5f * WALL_SPACING, 1.0f * WALL_SPACING };
	state->paper.normal      = { 0.5f, -1.0f };

	state->hand.type = ThingType::hand;

	state->hand_hovered_thing = &state->paper;

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

	state->wall    = init_img_rgb(DATA_DIR "wall.png");
	state->floor   = init_img_rgb(DATA_DIR "floor.png");
	state->ceiling = init_img_rgb(DATA_DIR "ceiling.png");

	state->thing_imgs[+ThingType::monster] = init_img_rgba(DATA_DIR "monster.png");
	state->thing_imgs[+ThingType::battery] = init_img_rgba(DATA_DIR "battery.png");
	state->thing_imgs[+ThingType::paper  ] = init_img_rgba(DATA_DIR "paper.png");
	state->thing_imgs[+ThingType::hand   ] = init_img_rgba(DATA_DIR "hand.png");
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	deinit_img_rgb(&state->wall);
	deinit_img_rgb(&state->floor);
	deinit_img_rgb(&state->ceiling);

	FOR_ELEMS(it, state->thing_imgs)
	{
		deinit_img_rgba(it);
	}
}

extern "C" PROTOTYPE_UPDATE(update)
{
	State* state = reinterpret_cast<State*>(platform->memory);
	state->transient_arena.used = 0;

	state->lucia_angle_velocity -= platform->cursor_delta.x * 0.01f / SECONDS_PER_UPDATE;
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

		vi2 step =
			{
				displacement.x < 0.0f ? -1 : 1,
				displacement.y < 0.0f ? -1 : 1
			};
		vf2 t_max =
			{
				((step.x == -1 ? floorf : ceilf)(state->lucia_position.x / WALL_SPACING) * WALL_SPACING - state->lucia_position.x) / displacement.x,
				((step.y == -1 ? floorf : ceilf)(state->lucia_position.y / WALL_SPACING) * WALL_SPACING - state->lucia_position.y) / displacement.y
			};
		vf2 t_delta =
			{
				step.x / displacement.x * WALL_SPACING,
				step.y / displacement.y * WALL_SPACING
			};
		vi2 coordinates =
			{
				static_cast<i32>(floorf(state->lucia_position.x / WALL_SPACING)),
				static_cast<i32>(floorf(state->lucia_position.y / WALL_SPACING))
			};
		FOR_RANGE(MAXIMUM(fabsf(displacement.x), fabsf(displacement.y)) + 1)
		{
			FOR_ELEMS(voxel_data, WALL_VOXEL_DATA)
			{
				if (+(*get_wall_voxel(state, coordinates) & voxel_data->voxel))
				{
					vf2 start = (coordinates + voxel_data->start) * WALL_SPACING;
					vf2 end   = (coordinates + voxel_data->end  ) * WALL_SPACING;

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

			if (closest_intersection.status != IntersectionStatus::none)
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

	if (state->monster_path)
	{
		vf2 ray = ray_to_closest(state->monster.position.xy, path_coordinates_to_position(state->monster_path->coordinates));
		if (norm(ray) < WALL_SPACING / 2.0f)
		{
			state->monster_path = deallocate_path_coordinates_node(state, state->monster_path);
		}
		else
		{
			state->monster.velocity = dampen(state->monster.velocity, normalize(ray) * 3.0f, 4.0f, SECONDS_PER_UPDATE);
		}
	}
	else
	{
		vf2 ray = ray_to_closest(state->monster.position.xy, state->lucia_position.xy);
		if (norm(ray) > 3.0f)
		{
			state->monster.velocity = dampen(state->monster.velocity, normalize(ray) * 3.0f, 4.0f, SECONDS_PER_UPDATE);
		}
		else
		{
			state->monster.velocity = dampen(state->monster.velocity, { 0.0f, 0.0f }, 4.0f, SECONDS_PER_UPDATE);
		}
	}

	// @TEMP@
	persist f32 TEMP;
	TEMP += 1.5f * SECONDS_PER_UPDATE;

	state->monster.position.xy += state->monster.velocity * SECONDS_PER_UPDATE;
	state->monster.position.x   = mod(state->monster.position.x, MAP_DIM * WALL_SPACING);
	state->monster.position.y   = mod(state->monster.position.y, MAP_DIM * WALL_SPACING);
	state->monster.position.z   = cosf(TEMP * 2.0f) * 0.15f + WALL_HEIGHT / 2.0f;
	state->monster.normal       = normalize(dampen(state->monster.normal, normalize(state->lucia_position.xy - state->monster.position.xy), 1.0f, SECONDS_PER_UPDATE));

	state->battery.position.z = 1.0f + sinf(TEMP * 2.0f) * 0.1f;
	state->battery.normal     = polar(TEMP);

	state->paper.position.z = 1.0f + sinf(TEMP * 2.0f) * 0.1f;
	state->paper.normal     = polar(TEMP);

	state->flashlight_keytime += 0.0025f + 0.05f * norm(state->lucia_velocity) * SECONDS_PER_UPDATE;
	if (state->flashlight_keytime > 1.0f)
	{
		state->flashlight_keytime -= 1.0f;
	}

	state->flashlight_ray.xy  = dampen(state->flashlight_ray.xy, polar(state->lucia_angle + sinf(state->flashlight_keytime * TAU * 2.0f) * 0.03f), 16.0f, SECONDS_PER_UPDATE);
	state->flashlight_ray.z   = sinf(state->flashlight_keytime * TAU) * 0.05f;
	state->flashlight_ray     = normalize(state->flashlight_ray);

	persist vi2 TEMP_OLD_CLOSEST = get_closest_open_path_coordinates(state, state->lucia_position.xy);
	if (get_closest_open_path_coordinates(state, state->lucia_position.xy) != TEMP_OLD_CLOSEST)
	{
		TEMP_OLD_CLOSEST = get_closest_open_path_coordinates(state, state->lucia_position.xy);

		vi2 starting_coordinates = get_closest_open_path_coordinates(state, state->monster.position.xy);
		vi2 ending_coordinates   = get_closest_open_path_coordinates(state, state->lucia_position.xy);

		struct PathVertex
		{
			bool32 is_set;
			f32    best_weight;
			vi2    prev_coordinates;
		};

		struct PathQueueNode
		{
			f32            estimated_length;
			vi2            prev_coordinates;
			vi2            coordinates;
			PathQueueNode* next_node;
		};

		PathVertex path_vertices[MAP_DIM * 2][MAP_DIM] = {};
		path_vertices[starting_coordinates.y][starting_coordinates.x].is_set = true;

		PathQueueNode* path_queue = memory_arena_push<PathQueueNode>(&state->transient_arena);
		path_queue->estimated_length = path_distance_function(starting_coordinates, ending_coordinates);
		path_queue->prev_coordinates = { -1, -1 };
		path_queue->coordinates      = starting_coordinates;
		path_queue->next_node        = 0;

		PathQueueNode* available_path_queue_node = 0;

		while (path_queue && path_queue->coordinates != ending_coordinates)
		{
			PathQueueNode* head = path_queue;
			path_queue = path_queue->next_node;

			struct ADJACENT { WallVoxel side; vi2 delta_coordinates; };
			constexpr ADJACENT HORI[] =
				{
					{ WallVoxel::bottom, { -1, -1 } },
					{ WallVoxel::bottom, {  0, -1 } },
					{ WallVoxel::left  , { -1,  0 } },
					{ WallVoxel::left  , {  1,  0 } },
					{ WallVoxel::bottom, { -1,  1 } },
					{ WallVoxel::bottom, {  0,  1 } }
				};

			constexpr ADJACENT VERT[] =
				{
					{ WallVoxel::bottom, { 0, -2 } },
					{ WallVoxel::left  , { 0, -1 } },
					{ WallVoxel::left  , { 1, -1 } },
					{ WallVoxel::left  , { 0,  1 } },
					{ WallVoxel::left  , { 1,  1 } },
					{ WallVoxel::bottom, { 0,  2 } }
				};

			FOR_ELEMS(it, head->coordinates.y % 2 == 0 ? VERT : HORI)
			{
				vi2 next_coordinates =
					{
						mod(head->coordinates.x + it->delta_coordinates.x, MAP_DIM    ),
						mod(head->coordinates.y + it->delta_coordinates.y, MAP_DIM * 2)
					};

				if (next_coordinates == head->prev_coordinates || +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(next_coordinates)) & it->side))
				{
					continue;
				}

				if (head->coordinates.y % 2 == 0)
				{
					if (it->delta_coordinates.y < 0)
					{
						if
						(
							it->delta_coordinates != vi2 { 1, -1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates) + vi2 { 0, -1 }) & WallVoxel::back_slash   ) ||
							it->delta_coordinates != vi2 { 0, -1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates) + vi2 { 0, -1 }) & WallVoxel::forward_slash)
						)
						{
							continue;
						}
					}
					else if
					(
						it->delta_coordinates != vi2 { 0, 1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates)) & WallVoxel::back_slash   ) ||
						it->delta_coordinates != vi2 { 1, 1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates)) & WallVoxel::forward_slash)
					)
					{
						continue;
					}
				}
				else if (it->delta_coordinates.x < 0)
				{
					if
					(
						it->delta_coordinates != vi2 { -1,  1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates) + vi2 { -1, 0 }) & WallVoxel::back_slash   ) ||
						it->delta_coordinates != vi2 { -1, -1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates) + vi2 { -1, 0 }) & WallVoxel::forward_slash)
					)
					{
						continue;
					}
				}
				else if
				(
					it->delta_coordinates != vi2 { 0, -1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates)) & WallVoxel::back_slash   ) ||
					it->delta_coordinates != vi2 { 0,  1 } && +(*get_wall_voxel(state, path_coordinates_to_map_coordinates(head->coordinates)) & WallVoxel::forward_slash)
				)
				{
					continue;
				}

				f32         next_weight = path_vertices[head->coordinates.y][head->coordinates.x].best_weight + path_distance_function(head->coordinates, next_coordinates);
				PathVertex* next_vertex = &path_vertices[next_coordinates.y][next_coordinates.x];

				if (!next_vertex->is_set || next_vertex->best_weight > next_weight)
				{
					next_vertex->is_set           = true;
					next_vertex->best_weight      = next_weight;
					next_vertex->prev_coordinates = head->coordinates;

					PathQueueNode** repeated_node = &path_queue;

					while (*repeated_node && (*repeated_node)->coordinates != next_coordinates)
					{
						repeated_node = &(*repeated_node)->next_node;
					}

					if (*repeated_node)
					{
						PathQueueNode* tail = (*repeated_node)->next_node;
						(*repeated_node)->next_node = available_path_queue_node;
						available_path_queue_node = *repeated_node;
						*repeated_node = tail;
					}

					f32             next_estimated_length = next_weight + path_distance_function(next_coordinates, ending_coordinates);
					PathQueueNode** post_node             = &path_queue;
					while (*post_node && (*post_node)->estimated_length < next_estimated_length)
					{
						post_node = &(*post_node)->next_node;
					}

					PathQueueNode* new_node;
					if (available_path_queue_node)
					{
						new_node                  = available_path_queue_node;
						available_path_queue_node = available_path_queue_node->next_node;
					}
					else
					{
						new_node = memory_arena_push<PathQueueNode>(&state->transient_arena);
					}

					new_node->estimated_length = next_estimated_length;
					new_node->prev_coordinates = head->coordinates;
					new_node->coordinates      = next_coordinates;
					new_node->next_node        = *post_node;
					*post_node = new_node;
				}
			}

			head->next_node = available_path_queue_node;
			available_path_queue_node = head;
		}

		if (path_queue && path_queue->coordinates == ending_coordinates)
		{
			while (state->monster_path)
			{
				state->monster_path = deallocate_path_coordinates_node(state, state->monster_path);
			}

			vi2 coordinates = ending_coordinates;
			while (true)
			{
				PathCoordinatesNode* path_coordinates_node = allocate_path_coordinates_node(state);
				path_coordinates_node->coordinates = coordinates;
				path_coordinates_node->next_node   = state->monster_path;
				state->monster_path = path_coordinates_node;

				if (coordinates == starting_coordinates)
				{
					break;
				}

				coordinates = path_vertices[coordinates.y][coordinates.x].prev_coordinates;
			}
		}
		else
		{
			ASSERT(false);
		}
	}

	if (state->hand_hovered_thing)
	{
		vf2 ray = ray_to_closest(state->hand_hovered_thing->position.xy, state->lucia_position.xy);

		state->hand.position.xy = state->hand_hovered_thing->position.xy + ray / 2.0f;
		state->hand.position.z  = lerp(state->hand_hovered_thing->position.z, state->lucia_position.z, 0.75f);
		state->hand.normal      = normalize(ray) * 0.15f;
	}

	return UpdateCode::resume;
}

extern "C" PROTOTYPE_RENDER(render)
{
	State* state = reinterpret_cast<State*>(platform->memory);
	state->transient_arena.used = 0;

	constexpr i32 VIEW_PADDING = 10;

	fill(platform->surface, { 0.05f, 0.10f, 0.15f });
	memset(state->frame_buffer, 0, sizeof(state->frame_buffer));

	#if 1
	constexpr f32 AMBIENT_LIGHT_RADIUS = 4.0f;
	constexpr f32 FLASHLIGHT_POW       = 8.0f;

	#if 0
	persist u64 DEBUG_total;
	persist u64 DEBUG_counter;
	LARGE_INTEGER DEBUG_li0;
	QueryPerformanceCounter(&DEBUG_li0);

	LARGE_INTEGER DEBUG_li1;
	QueryPerformanceCounter(&DEBUG_li1);
	DEBUG_total   += DEBUG_li1.QuadPart - DEBUG_li0.QuadPart;
	DEBUG_counter += 1;
	if (DEBUG_counter > 8'000)
	{
		DEBUG_printf("%f\n", static_cast<f64>(DEBUG_total) / DEBUG_counter);
		DEBUG_total   = 0;
		DEBUG_counter = 0;
	}
	#endif

	FOR_RANGE(x, VIEW_DIM.x)
	{
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
			FOR_ELEMS(voxel_data, WALL_VOXEL_DATA)
			{
				if (+(*get_wall_voxel(state, coordinates) & voxel_data->voxel))
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
							(coordinates + voxel_data->start) * WALL_SPACING,
							(coordinates + voxel_data->end  ) * WALL_SPACING
						)
						&& IN_RANGE(portion, 0.0f, 1.0f)
						&& (!wall_exists || distance < wall_distance)
					)
					{
						wall_exists   = true;
						wall_normal   = voxel_data->normal;
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

		i32 starting_y       = 0;
		i32 ending_y         = 0;
		i32 pixel_starting_y = 0;
		i32 pixel_ending_y   = 0;
		if (wall_exists)
		{
			starting_y       = static_cast<i32>(VIEW_DIM.y / 2.0f - HORT_TO_VERT_K / state->lucia_fov *                state->lucia_position.z  / (wall_distance + 0.1f));
			ending_y         = static_cast<i32>(VIEW_DIM.y / 2.0f + HORT_TO_VERT_K / state->lucia_fov * (WALL_HEIGHT - state->lucia_position.z) / (wall_distance + 0.1f));
			pixel_starting_y = MAXIMUM(0, starting_y);
			pixel_ending_y   = MINIMUM(ending_y, VIEW_DIM.y);
		}

		FOR_RANGE(y, 0, VIEW_DIM.y)
		{
			vf3 ray          = normalize({ ray_horizontal.x, ray_horizontal.y, (y - VIEW_DIM.y / 2.0f) * state->lucia_fov / HORT_TO_VERT_K });
			f32 flashlight_k = powf(CLAMP(dot(ray, state->flashlight_ray), 0.0f, 1.0f), FLASHLIGHT_POW);

			if (IN_RANGE(y, pixel_starting_y, pixel_ending_y))
			{
				f32 k =
					flashlight_k
						* square(CLAMP(1.0f - wall_distance / 32.0f, 0.0f, 1.0f))
					+ fabsf(dot(ray.xy, wall_normal))
						* square(CLAMP(1.0f - wall_distance / AMBIENT_LIGHT_RADIUS, 0.0f, 1.0f));

				write_pixel
				(
					&state->frame_buffer[x][y],
					{
						*(state->wall.rgb + static_cast<i32>(wall_portion * (state->wall.dim.x - 1.0f)) * state->wall.dim.y + static_cast<i32>(static_cast<f32>(y - starting_y) / (ending_y - starting_y) * state->wall.dim.y))
							* CLAMP(k, 0.0f, 1.0f),
						1.0f / wall_distance
					}
				);
			}
			else
			{
				f32 zk       = ((y < VIEW_DIM.y / 2 ? 0 : WALL_HEIGHT) - state->lucia_position.z) / ray.z;
				vf2 portions = (state->lucia_position.xy + zk * ray.xy) / 4.0f;
				portions.x   = mod(portions.x, 1.0f);
				portions.y   = mod(portions.y, 1.0f);

				f32 distance = sqrtf(square(zk) * 2.0f - square(state->lucia_position.z));
				f32 k        =
					flashlight_k
						* square(CLAMP(1.0f - distance / 32.0f, 0.0f, 1.0f))
					+ fabsf(ray.z)
						* square(CLAMP(1.0f - distance / AMBIENT_LIGHT_RADIUS, 0.0f, 1.0f));

				ImgRGB* img = y < VIEW_DIM.y / 2 ? &state->floor : &state->ceiling;
				write_pixel
				(
					&state->frame_buffer[x][y],
					{
						*(img->rgb + static_cast<i32>(portions.x * (img->dim.x - 1.0f)) * img->dim.y + static_cast<i32>(portions.y * img->dim.y))
							* CLAMP(k, 0.0f, 1.0f),
						1.0f / distance
					}
				);
			}
		}

		struct ThingNode
		{
			Thing*     thing;
			f32        distance;
			f32        portion;
			ThingNode* next_node;
		};

		ThingNode* intersected_thing_node = 0;

		Thing* things[] = { &state->monster, &state->battery, &state->paper, &state->hand };
		FOR_ELEMS(it, things)
		{
			Thing* thing = *it;

			FOR_RANGE(i, 0, 9)
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
						vi2 { i % 3 - 1, i / 3 - 1 } * MAP_DIM * WALL_SPACING + thing->position.xy - rotate90(thing->normal) / 2.0f,
						vi2 { i % 3 - 1, i / 3 - 1 } * MAP_DIM * WALL_SPACING + thing->position.xy + rotate90(thing->normal) / 2.0f
					)
					&& IN_RANGE(portion, 0.0f, 1.0f)
				)
				{
					ThingNode** post_node = &intersected_thing_node;
					while (*post_node && (*post_node)->distance > distance)
					{
						post_node = &(*post_node)->next_node;
					}

					ThingNode* new_node = memory_arena_push<ThingNode>(&state->transient_arena);
					new_node->thing     = *it;
					new_node->distance  = distance;
					new_node->portion   = portion;
					new_node->next_node = *post_node;
					*post_node = new_node;
				}
			}
		}

		for (ThingNode* node = intersected_thing_node; node; node = node->next_node)
		{
			i32 thing_starting_y       = static_cast<i32>(VIEW_DIM.y / 2.0f + HORT_TO_VERT_K / state->lucia_fov * (node->thing->position.z - norm(node->thing->normal) / 2.0f - state->lucia_position.z) / (node->distance + 0.1f));
			i32 thing_ending_y         = static_cast<i32>(VIEW_DIM.y / 2.0f + HORT_TO_VERT_K / state->lucia_fov * (node->thing->position.z + norm(node->thing->normal) / 2.0f - state->lucia_position.z) / (node->distance + 0.1f));
			i32 thing_pixel_starting_y = MAXIMUM(0, thing_starting_y);
			i32 thing_pixel_ending_y   = MINIMUM(thing_ending_y, VIEW_DIM.y);

			FOR_RANGE(y, thing_pixel_starting_y, thing_pixel_ending_y)
			{
				vf4* thing_pixel =
					state->thing_imgs[+node->thing->type].rgba
						+ static_cast<i32>(node->portion * state->thing_imgs[+node->thing->type].dim.x) * state->thing_imgs[+node->thing->type].dim.y
						+ static_cast<i32>(static_cast<f32>(y - thing_starting_y) / (thing_ending_y - thing_starting_y) * state->thing_imgs[+node->thing->type].dim.y);

				if (thing_pixel->w)
				{
					vf3 ray          = normalize({ ray_horizontal.x, ray_horizontal.y, (y - VIEW_DIM.y / 2.0f) * state->lucia_fov / HORT_TO_VERT_K });
					f32 flashlight_k = powf(CLAMP(dot(ray, state->flashlight_ray), 0.0f, 1.0f), FLASHLIGHT_POW);
					f32 k =
						flashlight_k
							* square(CLAMP(1.0f - node->distance / 32.0f, 0.0f, 1.0f))
						+ fabsf(dot(ray, { node->thing->normal.x, node->thing->normal.y, 0.0f }))
							* square(CLAMP(1.0f - node->distance / AMBIENT_LIGHT_RADIUS, 0.0f, 1.0f));

					write_pixel
					(
						&state->frame_buffer[x][y],
						{
							lerp
							(
								state->frame_buffer[x][y].color,
								thing_pixel->xyz * CLAMP(k, 0.0f, 1.0f),
								thing_pixel->w
							),
							1.0f / node->distance
						}
					);
				}
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
	#else
	fill(state->view, { 1.0f, 1.0f, 1.0f });

	vf2 cam_pos = state->lucia_position.xy;

	#if 0
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
	#endif

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
	FOR_RANGE(4)
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

	FOR_RANGE(i, MAP_DIM)
	{
		draw_line
		(
			state->view,
			VIEW_DIM / 2.0f + conjugate(vi2 { i,       0 } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
			VIEW_DIM / 2.0f + conjugate(vi2 { i, MAP_DIM } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
			{ 0.8f, 0.8f, 0.8f }
		);
		draw_line
		(
			state->view,
			VIEW_DIM / 2.0f + conjugate(vi2 {       0, i } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
			VIEW_DIM / 2.0f + conjugate(vi2 { MAP_DIM, i } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
			{ 0.8f, 0.8f, 0.8f }
		);
	}

	FOR_RANGE(y, MAP_DIM)
	{
		FOR_RANGE(x, MAP_DIM)
		{
			if (+(*get_wall_voxel(state, { x, y }) & WallVoxel::left))
			{
				draw_line
				(
					state->view,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 1.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					{ 0.0f, 0.0f, 0.0f }
				);
			}

			if (+(*get_wall_voxel(state, { x, y }) & WallVoxel::bottom))
			{
				draw_line
				(
					state->view,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 1.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					{ 0.0f, 0.0f, 0.0f }
				);
			}

			if (+(*get_wall_voxel(state, { x, y }) & WallVoxel::back_slash))
			{
				draw_line
				(
					state->view,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 1.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 1.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					{ 0.0f, 0.0f, 0.0f }
				);
			}

			if (+(*get_wall_voxel(state, { x, y }) & WallVoxel::forward_slash))
			{
				draw_line
				(
					state->view,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 0.0f, y + 0.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					VIEW_DIM / 2.0f + conjugate(vf2 { x + 1.0f, y + 1.0f } * WALL_SPACING - cam_pos) * PIXELS_PER_METER,
					{ 0.0f, 0.0f, 0.0f }
				);
			}

			constexpr f32 DIM = 1.0f;
			fill
			(
				state->view,
				VIEW_DIM / 2.0f + conjugate((vf2 { x + 0.5f, y + 0.0f }) * WALL_SPACING - cam_pos) * PIXELS_PER_METER - vf2 { DIM, DIM } / 2.0f,
				vf2 { DIM, DIM },
				{ 0.0f, 0.25f, 0.0f }
			);
			fill
			(
				state->view,
				VIEW_DIM / 2.0f + conjugate((vf2 { x + 0.0f, y + 0.5f }) * WALL_SPACING - cam_pos) * PIXELS_PER_METER - vf2 { DIM, DIM } / 2.0f,
				vf2 { DIM, DIM },
				{ 0.0f, 0.25f, 0.0f }
			);
		}
	}

	for (PathCoordinatesNode* node = state->monster_path; node && node->next_node; node = node->next_node)
	{
		draw_line
		(
			state->view,
			VIEW_DIM / 2.0f + conjugate(path_coordinates_to_position(node           ->coordinates) - cam_pos) * PIXELS_PER_METER - vf2 { 2.5f, 2.5f } / 4.0f,
			VIEW_DIM / 2.0f + conjugate(path_coordinates_to_position(node->next_node->coordinates) - cam_pos) * PIXELS_PER_METER - vf2 { 2.5f, 2.5f } / 4.0f,
			abs(node->coordinates.x - node->next_node->coordinates.x) < 3 && abs(node->coordinates.y - node->next_node->coordinates.y) < 3
				? vf3 { 0.6f, 0.1f, 0.9f }
				: vf3 { 0.8f, 0.8f, 1.0f }
		);
	}

	fill(state->view, VIEW_DIM / 2.0f + conjugate(state->monster.position.xy - cam_pos) * PIXELS_PER_METER - vf2 { 2.5f, 2.5f }, { 5.0f, 5.0f }, { 0.0f, 0.0f, 1.0f });
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
