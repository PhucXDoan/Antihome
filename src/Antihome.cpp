#define STB_IMAGE_IMPLEMENTATION true
#include <stb_image.h>
#include "unified.h"
#include "platform.h"
#include "utilities.cpp"

global constexpr vi2 VIEW_DIM                    = { 350, 175 };
global constexpr f32 HORT_TO_VERT_K              = 0.927295218f * VIEW_DIM.x;
global constexpr f32 WALL_HEIGHT                 = 2.7432f;
global constexpr f32 WALL_THICKNESS              = 0.25f;
global constexpr f32 LUCIA_HEIGHT                = 1.4986f;
global constexpr i32 MAP_DIM                     = 8;
global constexpr f32 WALL_SPACING                = 3.0f; // @TODO@ Must be integer.
global constexpr vf2 WALL_LAYOUT_POSITIONS[4][2] =
	{
		{ { 0.0f, 0.0f }, { 0.0f, 1.0f } },
		{ { 0.0f, 0.0f }, { 1.0f, 0.0f } },
		{ { 1.0f, 0.0f }, { 0.0f, 1.0f } },
		{ { 0.0f, 0.0f }, { 1.0f, 1.0f } }
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
	f32 depth;
};

struct State
{
	u32                seed;
	SDL_Surface*       view;
	Pixel              frame_buffer[VIEW_DIM.x][VIEW_DIM.y];
	vf2                lucia_velocity;
	vf2                lucia_position;
	f32                lucia_angle_velocity;
	f32                lucia_angle;
	f32                lucia_fov;
	f32                lucia_head_bob_keytime;
	WallLayout         wall_layouts[MAP_DIM * MAP_DIM];
	ColumnMajorTexture wall;
	ColumnMajorTexture floor;
	ColumnMajorTexture ceiling;

	Sprite sprite;
	vf3    sprite_position;
};

internal WallLayout* get_wall_layout(State* state, i32 x, i32 y)
{
	return &state->wall_layouts[((y % MAP_DIM) + MAP_DIM) % MAP_DIM * MAP_DIM + ((x % MAP_DIM) + MAP_DIM) % MAP_DIM];
}

internal void write_pixel(Pixel* pixel, Pixel new_pixel)
{
	if (pixel->depth <= new_pixel.depth)
	{
		*pixel = new_pixel;
	}
}

extern "C" PROTOTYPE_INITIALIZE(initialize)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	*state = {};

	state->lucia_position  = { 0.5f, 0.5f };
	state->lucia_fov       = TAU / 3.0f;
	state->sprite_position = { 1.0f, 2.0f, 0.0f };

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

	state->wall    = init_column_major_texture(DATA_DIR "wall.png");
	state->floor   = init_column_major_texture(DATA_DIR "floor.png");
	state->ceiling = init_column_major_texture(DATA_DIR "ceiling.png");
	state->sprite  = init_sprite(DATA_DIR "sprite.png");
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	ASSERT(sizeof(State) <= platform->memory_capacity);
	State* state = reinterpret_cast<State*>(platform->memory);

	deinit_sprite(&state->sprite);
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
				FOR_ELEMS(layout_position, WALL_LAYOUT_POSITIONS)
				{
					if (+(*get_wall_layout(state, x, y) & static_cast<WallLayout>(1 << layout_position_index)))
					{
						vf2 start = (vf2 { static_cast<f32>(x), static_cast<f32>(y) } + (*layout_position)[0]) * WALL_SPACING;
						vf2 end   = (vf2 { static_cast<f32>(x), static_cast<f32>(y) } + (*layout_position)[1]) * WALL_SPACING;

						Intersection intersection = intersect_thick_line_segment(state->lucia_position, displacement, start, end, WALL_THICKNESS);

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
			state->lucia_position   = closest_intersection.position;
			//state->lucia_position.x = fmodf((state->lucia_position.x + MAP_DIM * WALL_SPACING), MAP_DIM * WALL_SPACING);
			//state->lucia_position.y = fmodf((state->lucia_position.y + MAP_DIM * WALL_SPACING), MAP_DIM * WALL_SPACING);
			displacement            = dot(state->lucia_position + displacement - closest_intersection.position, { -closest_intersection.normal.y, closest_intersection.normal.x }) * vf2 { -closest_intersection.normal.y, closest_intersection.normal.x };
		}
	}
	state->lucia_position  += displacement;
	//state->lucia_position.x = fmodf((state->lucia_position.x + MAP_DIM * WALL_SPACING), MAP_DIM * WALL_SPACING);
	//state->lucia_position.y = fmodf((state->lucia_position.y + MAP_DIM * WALL_SPACING), MAP_DIM * WALL_SPACING);

	state->lucia_head_bob_keytime += 0.35f * norm(state->lucia_velocity) * SECONDS_PER_UPDATE;
	if (state->lucia_head_bob_keytime > 1.0f)
	{
		state->lucia_head_bob_keytime -= 1.0f;
	}

	state->lucia_fov += platform->scroll * 0.1f;

	// @TEMP@
	persist f32 TEMP;
	TEMP += 1.5f * SECONDS_PER_UPDATE;
	state->sprite_position.z = WALL_HEIGHT * (0.5f + sinf(TEMP) * 0.1f);

	return UpdateCode::resume;
}

extern "C" PROTOTYPE_RENDER(render)
{
	State* state = reinterpret_cast<State*>(platform->memory);

	constexpr i32 VIEW_PADDING = 10;

	fill(platform->surface, { 0.05f, 0.10f, 0.15f });
	memset(state->frame_buffer, 0, sizeof(state->frame_buffer));

	#if 1
	f32 lucia_eye_level = LUCIA_HEIGHT + 0.1f * (cosf(state->lucia_head_bob_keytime * TAU) - 1.0f);

	FOR_RANGE(x, VIEW_DIM.x)
	{
		vf2 ray_horizontal = polar(state->lucia_angle + (0.5f - static_cast<f32>(x) / VIEW_DIM.x) * state->lucia_fov);

		bool32 wall_exists   = false;
		f32    wall_distance = NAN;
		f32    wall_portion  = NAN;

		FOR_RANGE(wall_y, MAP_DIM)
		{
			FOR_RANGE(wall_x, MAP_DIM)
			{
				FOR_ELEMS(layout_position, WALL_LAYOUT_POSITIONS)
				{
					if (+(*get_wall_layout(state, wall_x, wall_y) & static_cast<WallLayout>(1 << layout_position_index)))
					{
						//FOR_RANGE(i, 9) // @TEMP@
						i32 i = 4;
						{
							vf2 offset = (vf2 { static_cast<f32>(i % 3), static_cast<f32>(i / 3) } - vf2 { 1.0f, 1.0f }) * MAP_DIM;

							vf2 start  = (offset + vf2 { static_cast<f32>(wall_x), static_cast<f32>(wall_y) } + (*layout_position)[0]) * WALL_SPACING;
							vf2 end    = (offset + vf2 { static_cast<f32>(wall_x), static_cast<f32>(wall_y) } + (*layout_position)[1]) * WALL_SPACING;

							f32 distance;
							f32 portion;
							if (ray_cast_line(&distance, &portion, state->lucia_position, ray_horizontal, start, end) && IN_RANGE(portion, 0.0f, 1.0f) && (!wall_exists || distance < wall_distance))
							{
								wall_exists   = true;
								wall_distance = distance;
								wall_portion  = portion;
							}
						}
					}
				}
			}
		}

		i32 pixel_starting_y = -1;
		i32 pixel_ending_y   = 0;
		if (wall_exists)
		{
			i32 starting_y = static_cast<i32>(VIEW_DIM.y / 2.0f - HORT_TO_VERT_K / state->lucia_fov *                lucia_eye_level  / (wall_distance + 0.1f));
			i32 ending_y   = static_cast<i32>(VIEW_DIM.y / 2.0f + HORT_TO_VERT_K / state->lucia_fov * (WALL_HEIGHT - lucia_eye_level) / (wall_distance + 0.1f));

			pixel_starting_y = MAXIMUM(0, starting_y);
			pixel_ending_y   = MINIMUM(ending_y, VIEW_DIM.y);

			FOR_RANGE(y, pixel_starting_y, pixel_ending_y)
			{
				write_pixel
				(
					&state->frame_buffer[x][y],
					{
						*(state->wall.colors + static_cast<i32>(wall_portion * (state->wall.w - 1.0f)) * state->wall.h + static_cast<i32>(static_cast<f32>(y - starting_y) / (ending_y - starting_y) * state->wall.h))
							* CLAMP(1.0f - wall_distance / 16.0f, 0.0f, 1.0f),
						0.0f
					}
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
				vf2 distances;
				vf2 portions;
				if
				(
					ray_cast_line(&distances.x, &portions.x, { state->lucia_position.x, lucia_eye_level }, normalize(vf2 { ray_horizontal.x, pitch }), { 0.0f, texture_level }, { texture_dimension, texture_level }) &&
					ray_cast_line(&distances.y, &portions.y, { state->lucia_position.y, lucia_eye_level }, normalize(vf2 { ray_horizontal.y, pitch }), { 0.0f, texture_level }, { texture_dimension, texture_level })
				)
				{
					f32 distance = sqrtf(square(distances.x) + square(distances.y) - square(lucia_eye_level));

					portions.x = mod(portions.x, 1.0f);
					portions.y = mod(portions.y, 1.0f);

					write_pixel
					(
						&state->frame_buffer[x][y],
						{
							*(texture->colors + static_cast<i32>(portions.x * (texture->w - 1.0f)) * texture->h + static_cast<i32>(portions.y * texture->h))
								* CLAMP(1.0f - distance / 16.0f, 0.0f, 1.0f),
							0.0f
						}
					);
				}
			}
		}
	}

	vi2 sprite_screen_position =
		{
			static_cast<i32>(VIEW_DIM.x * (0.5f - centerize_angle(mod(argument(normalize(state->sprite_position.xy - state->lucia_position)) - state->lucia_angle, TAU)) / state->lucia_fov)),
			static_cast<i32>(VIEW_DIM.y / 2.0f + HORT_TO_VERT_K / state->lucia_fov * normalize(state->sprite_position - vf3 { state->lucia_position.x, state->lucia_position.y, lucia_eye_level }).z)
		};

	vi2 sprite_screen_dimensions =
		{
			static_cast<i32>(0.1f / (0.1f + norm(state->sprite_position - vf3 { state->lucia_position.x, state->lucia_position.y, lucia_eye_level })) * state->sprite.w),
			static_cast<i32>(0.1f / (0.1f + norm(state->sprite_position - vf3 { state->lucia_position.x, state->lucia_position.y, lucia_eye_level })) * state->sprite.h)
		};

	sprite_screen_position -= sprite_screen_dimensions / 2;

	FOR_RANGE(ix, sprite_screen_dimensions.x)
	{
		FOR_RANGE(iy, sprite_screen_dimensions.y)
		{
			if (IN_RANGE(sprite_screen_position.x + ix, 0, VIEW_DIM.x) && IN_RANGE(sprite_screen_position.y + iy, 0, VIEW_DIM.y))
			{
				vf4* sprite_rgba =
					state->sprite.pixels +
					static_cast<i32>((1.0f - static_cast<f32>(iy) / sprite_screen_dimensions.y) * (state->sprite.h - 1.0f)) * state->sprite.w +
					static_cast<i32>(static_cast<f32>(ix) / sprite_screen_dimensions.x * state->sprite.w);

				write_pixel
				(
					&state->frame_buffer[sprite_screen_position.x + ix][sprite_screen_position.y + iy],
					{
						lerp
						(
							state->frame_buffer[sprite_screen_position.x + ix][sprite_screen_position.y + iy].color,
							sprite_rgba->xyz,
							sprite_rgba->w
						),
						0.0f
					}
				);
			}
		}
	}
	#elif 0
	const f32 PIXELS_PER_METER = 25.0f + 10.0f / state->lucia_fov;
	const vf2 ORIGIN           = state->lucia_position;

	FOR_RANGE(y, MAP_DIM)
	{
		FOR_RANGE(x, MAP_DIM)
		{
			FOR_ELEMS(layout_position, WALL_LAYOUT_POSITIONS)
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
						{ 0.0f, 0.0f, 0.0f }
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
	fill(state->view, VIEW_DIM / 2.0f + conjugate(-ORIGIN + state->lucia_position) * PIXELS_PER_METER - vf2 { LUCIA_DIM, LUCIA_DIM } / 2.0f, vf2 { LUCIA_DIM, LUCIA_DIM }, { 0.0f, 0.0f, 1.0f });
	draw_line
	(
		state->view,
		VIEW_DIM / 2.0f + conjugate(-ORIGIN + state->lucia_position                                   ) * PIXELS_PER_METER,
		VIEW_DIM / 2.0f + conjugate(-ORIGIN + state->lucia_position + polar(state->lucia_angle) * 1.0f) * PIXELS_PER_METER,
		{ 0.4f, 0.8f, 0.6f }
	);
	#else
	persist vf2 cam_pos = { MAP_DIM * 1.5f, MAP_DIM * 1.5f };

	constexpr f32 SPEED = 8.0f;
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

	FOR_RANGE(iy, 3)
	{
		FOR_RANGE(ix, 3)
		{
			constexpr f32 PIXELS_PER_METER = 5.0f;
			constexpr f32 DOT_DIM          = 1.0f;

			vf2 offset = VIEW_DIM / 2.0f + conjugate({ ix * MAP_DIM - cam_pos.x, iy * MAP_DIM - cam_pos.y }) * PIXELS_PER_METER;

			fill(state->view, offset + conjugate({ 0.0f, MAP_DIM * PIXELS_PER_METER }), { MAP_DIM * PIXELS_PER_METER, MAP_DIM * PIXELS_PER_METER }, { 1.0f - iy / 2.0f, 1.0f - ix / 2.0f, 0.75f });

			FOR_RANGE(y, MAP_DIM)
			{
				FOR_RANGE(x, MAP_DIM)
				{
					fill(state->view, offset + conjugate({ x + 0.0f, y + 0.0f }) * PIXELS_PER_METER - vf2 { DOT_DIM, DOT_DIM } / 2.0f, { DOT_DIM, DOT_DIM }, { 0.0f, 0.0f, 0.0f });

					if (+(*get_wall_layout(state, x, y) & WallLayout::left))
					{
						draw_line
						(
							state->view,
							offset + conjugate({ x + 0.0f, y + 0.0f }) * PIXELS_PER_METER - vf2 { DOT_DIM, DOT_DIM } / 2.0f,
							offset + conjugate({ x + 0.0f, y + 1.0f }) * PIXELS_PER_METER - vf2 { DOT_DIM, DOT_DIM } / 2.0f,
							{ 0.25f, 0.25f, 0.25f }
						);
					}

					if (+(*get_wall_layout(state, x, y) & WallLayout::bottom))
					{
						draw_line
						(
							state->view,
							offset + conjugate({ x + 0.0f, y + 0.0f }) * PIXELS_PER_METER - vf2 { DOT_DIM, DOT_DIM } / 2.0f,
							offset + conjugate({ x + 1.0f, y + 0.0f }) * PIXELS_PER_METER - vf2 { DOT_DIM, DOT_DIM } / 2.0f,
							{ 0.25f, 0.25f, 0.25f }
						);
					}

					if (+(*get_wall_layout(state, x, y) & WallLayout::back_slash))
					{
						draw_line
						(
							state->view,
							offset + conjugate({ x + 0.0f, y + 1.0f }) * PIXELS_PER_METER - vf2 { DOT_DIM, DOT_DIM } / 2.0f,
							offset + conjugate({ x + 1.0f, y + 0.0f }) * PIXELS_PER_METER - vf2 { DOT_DIM, DOT_DIM } / 2.0f,
							{ 0.25f, 0.25f, 0.25f }
						);
					}

					if (+(*get_wall_layout(state, x, y) & WallLayout::forward_slash))
					{
						draw_line
						(
							state->view,
							offset + conjugate({ x + 0.0f, y + 0.0f }) * PIXELS_PER_METER - vf2 { DOT_DIM, DOT_DIM } / 2.0f,
							offset + conjugate({ x + 1.0f, y + 1.0f }) * PIXELS_PER_METER - vf2 { DOT_DIM, DOT_DIM } / 2.0f,
							{ 0.25f, 0.25f, 0.25f }
						);
					}
				}
			}
		}
	}

	//FOR_RANGE(y, MAP_DIM - 1)
	//{
	//	FOR_RANGE(x, MAP_DIM)
	//	{
	//		fill(state->view, VIEW_DIM / 2.0f + (conjugate({ x * 1.0f, y + 0.5f }) + vf2 { -(MAP_DIM - 1.0f) / 2.0f, (MAP_DIM - 1.0f) / 2.0f }) * PIXELS_PER_METER - vf2 { POINT_DIM, POINT_DIM } / 2.0f, { POINT_DIM, POINT_DIM }, { 0.5f, 1.0f, 0.5f });
	//	}
	//}
	#endif

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

	SDL_Rect dst = { VIEW_PADDING, VIEW_PADDING, WIN_DIM.x - VIEW_PADDING * 2, (WIN_DIM.x - VIEW_PADDING * 2) * VIEW_DIM.y / VIEW_DIM.x };
	SDL_BlitScaled(state->view, 0, platform->surface, &dst);
}
