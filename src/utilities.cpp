#define PRESSED(INPUT)  (platform->inputs[+(INPUT)].curr && !platform->inputs[+(INPUT)].prev)
#define HOLDING(INPUT)  (platform->inputs[+(INPUT)].curr)
#define RELEASED(INPUT) (!platform->inputs[+(INPUT)].curr && platform->inputs[+(INPUT)].prev)

global constexpr f32 TAU = 6.28318530717958647692f;

// @TODO@ Use an actual RNG lol.
// @NOTE@ Is in interval [0, 65536).
global constexpr u16 RAND_TABLE[] =
	{
		0x9b9d, 0x4e65, 0x8ec9, 0x30e9, 0x5477, 0xe845, 0xab16, 0x910d,
		0xfdf3, 0x73dc, 0x424b, 0xc8f9, 0x27a1, 0xe07a, 0x6803, 0xb8c1,
		0x9525, 0x1c29, 0xcf1f, 0x3e18, 0xc463, 0x165f, 0x7018, 0xeae4,
		0x7e41, 0x945e, 0xbbbf, 0x5ac7, 0x15a4, 0xa666, 0xa8f1, 0xbc54,
		0xe604, 0x3393, 0x5cad, 0x061f, 0x98b7, 0x93fd, 0xc762, 0xa37b,
		0x1880, 0x26cd, 0xb131, 0x3f59, 0xae82, 0x264f, 0x9355, 0xc0e8,
		0x5e46, 0x76bd, 0xd8f2, 0x2e69, 0xb158, 0x0317, 0x127f, 0x936a,
		0x0ccd, 0x8497, 0xd08d, 0x92ad, 0xa3d3, 0xcf59, 0x1dc6, 0xbb4b,
		0x7ecb, 0xb664, 0x93c7, 0xd3f1, 0xd23c, 0x9b2f, 0x5fae, 0x8c17,
		0xdfee, 0xd19c, 0x3bff, 0x6ac7, 0x078c, 0x8282, 0x9fe3, 0xa7fd,
		0xa653, 0xd182, 0x31ad, 0x0938, 0x5ac8, 0xcd79, 0x8622, 0x4e64,
		0xeb1f, 0x3da6, 0x133d, 0x6f45, 0xe061, 0x6449, 0xe19e, 0xf63b,
		0x5440, 0xe667, 0x9be6, 0x27f1, 0xfbee, 0x407b, 0x3c76, 0x54b6,
		0x8061, 0x6e0b, 0x36f5, 0xdd0d, 0xbaa5, 0xe8dc, 0x2a46, 0x4f8f,
		0x63ab, 0xda6c, 0x5950, 0x1df6, 0xd32a, 0xf42b, 0x68dd, 0xe9d2,
		0xb825, 0x31b8, 0x2a7c, 0x49f0, 0xe859, 0x4041, 0x2fce, 0x72f9,
		0x8224, 0x9615, 0x888f, 0x2050, 0x9172, 0x5744, 0xe53a, 0xfaef,
		0xf191, 0xd627, 0x2dae, 0x69da, 0xe4b3, 0x4a30, 0x9436, 0x80cd,
		0xb863, 0x7ac0, 0xd72b, 0xb49c, 0x20fc, 0xa0c6, 0x29c2, 0xb683,
		0xc820, 0x36d8, 0xeb86, 0x3bc5, 0xd2f4, 0xa04e, 0x90b4, 0xe5f0,
		0x44df, 0x442b, 0xf29c, 0x72db, 0x0fdf, 0x366f, 0x36e7, 0x734a,
		0x00c9, 0x40c1, 0xbddf, 0x7fcd, 0xff1e, 0x31a9, 0xd998, 0x67c7,
		0x328f, 0xbf19, 0xaa42, 0x165a, 0x3975, 0x99fb, 0x0df0, 0xe0ac,
		0x6cf0, 0x6c4d, 0x7a87, 0x73b5, 0xc3fc, 0x8878, 0xe755, 0x01ab,
		0x2d9e, 0x19dc, 0x2f14, 0xcaca, 0x4510, 0x2983, 0xfa28, 0xd81a,
		0xd4ba, 0x757a, 0xfe60, 0x58fd, 0xb899, 0x24e8, 0x90ac, 0x1a92,
		0x4b26, 0xb050, 0xd8e1, 0x96d1, 0x6ad2, 0x330d, 0x9e1a, 0xcfc4,
		0xaabd, 0x0394, 0x1a64, 0x59da, 0x4f65, 0xac6f, 0x381d, 0x1c57,
		0xbc45, 0xab88, 0x0f90, 0x2dd3, 0x468b, 0x71ac, 0xe1d1, 0x4e64,
		0x1f1e, 0xbb90, 0xa3d7, 0xa04a, 0xd59b, 0x8778, 0x38ed, 0xd353,
		0x5e6f, 0xd907, 0xbbcf, 0xe53e, 0xe37a, 0x7c8d, 0x9d9d, 0xb302,
		0xda05, 0xd0df, 0x0cbf, 0x2efe, 0x38aa, 0xbd2d, 0x1a53, 0x0f2b
	};

enum struct IntersectionStatus : u32
{
	none,
	outside,
	inside
};

struct Intersection
{
	IntersectionStatus status;
	vf2                position;
	vf2                normal;
	f32                distance;
};

struct ColumnMajorTexture
{
	i32  w;
	i32  h;
	vf3* colors;
};

internal f32 rng(u32* seed)
{
	return RAND_TABLE[++*seed % ARRAY_CAPACITY(RAND_TABLE)] / 65536.0f;
}

internal i32 rng(u32* seed, i32 start, i32 end)
{
	return static_cast<i32>(rng(seed) * (end - start) + start);
}

internal f32 rng(u32* seed, f32 start, f32 end)
{
	return rng(seed) * (end - start) + start;
}

internal constexpr f32 square(f32 x) { return x * x; }

internal constexpr f32 lerp(f32 a, f32 b, f32 t) { return a * (1.0f - t) + b * t; }
internal constexpr vf2 lerp(vf2 a, vf2 b, f32 t) { return a * (1.0f - t) + b * t; }
internal constexpr vf4 lerp(vf4 a, vf4 b, f32 t) { return a * (1.0f - t) + b * t; }

internal constexpr f32 dampen(f32 a, f32 b, f32 k, f32 dt) { return lerp(a, b, 1.0f - expf(-k * dt)); }
internal constexpr vf4 dampen(vf4 a, vf4 b, f32 k, f32 dt) { return lerp(a, b, 1.0f - expf(-k * dt)); }

internal constexpr f32 dot(vf2 u, vf2 v) { return u.x * v.x + u.y * v.y; }

internal constexpr vf4 hadamard_product(vf4 u, vf4 v) { return { u.x * v.x, u.y * v.y, u.z * v.z, u.w * v.w }; }

internal f32 norm(vf2 v) { return sqrtf(v.x * v.x + v.y * v.y            ); }
internal f32 norm(vf3 v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }

internal vf2 normalize(vf2 v) { return v / norm(v); }
internal vf3 normalize(vf3 v) { return v / norm(v); }

internal f32 distance(vf2 u, vf2 v) { return norm(u - v); }

internal vf2 polar(f32 angle) { return { cosf(angle), sinf(angle) }; }

internal vf2 rotate(vf2 v, f32 angle)
{
	return { v.x * cosf(angle) - v.y * sinf(angle), v.x * sinf(angle) + v.y * cosf(angle) };
}

internal bool32 ray_cast_line(f32* scalar, f32* portion, vf2 position, vf2 ray, vf2 start, vf2 end)
{
	f32 scalar_denom = (start.x - end.x) * ray.y - (start.y - end.y) * ray.x;

	if (fabs(scalar_denom) < 0.01f)
	{
		return false;
	}

	*scalar = ((start.x - end.x) * (start.y - position.y) - (start.y - end.y) * (start.x - position.x)) / scalar_denom;

	if (*scalar < 0.0f)
	{
		return false;
	}

	f32 portion_c     = start.x * ray.y - start.y * ray.x;
	f32 portion_denom = portion_c + ray.x * end.y - ray.y * end.x;

	if (fabs(portion_denom) < 0.01f)
	{
		return false;
	}

	*portion = (portion_c + ray.x * position.y - ray.y * position.x) / portion_denom;

	return true;
}

internal Intersection intersect_thick_line_segment(vf2 position, vf2 ray, vf2 start, vf2 end, f32 thickness)
{
	vf2 dir        = normalize(end - start);
	vf2 vertices[] =
		{
			start + (-dir + vf2 {  dir.y, -dir.x }) * thickness,
			end   + ( dir + vf2 {  dir.y, -dir.x }) * thickness,
			end   + ( dir + vf2 { -dir.y,  dir.x }) * thickness,
			start + (-dir + vf2 { -dir.y,  dir.x }) * thickness
		};

	Intersection closest_intersection;
	closest_intersection.status   = IntersectionStatus::none;
	closest_intersection.position = { NAN, NAN };
	closest_intersection.normal   = { NAN, NAN };
	closest_intersection.distance = NAN;

	vf2 current_dir = dir;
	FOR_RANGE(i, 4)
	{
		refering u = vertices[i];
		refering v = vertices[(i + 1) % 4];
		vf2      n = { current_dir.y, -current_dir.x };

		Intersection intersection;
		intersection.status   = IntersectionStatus::none;
		intersection.position = { NAN, NAN };
		intersection.normal   = { NAN, NAN };
		intersection.distance = NAN;

		if (closest_intersection.status != IntersectionStatus::inside && dot(position - u, n) >= 0.0f)
		{
			if (dot(ray, n) < 0.0f)
			{
				f32 denom = ray.x * (u.y - v.y) - ray.y * (u.x - v.x);
				if (fabs(denom) > 0.001f)
				{
					intersection.position = (ray * (u.y * v.x - u.x * v.y) + (ray.x * position.y - ray.y * position.x) * (u - v)) / denom;
					intersection.distance = norm(intersection.position - position);

					if (IN_RANGE(dot(intersection.position - u, current_dir), 0.0f, norm(u - v)) && intersection.distance < norm(ray))
					{
						intersection.status = IntersectionStatus::outside;
					}
				}
			}
		}
		else if (IN_RANGE(dot(position - u, -n), 0.0f, thickness))
		{
			intersection.position = dot(position - u, current_dir) * current_dir + u;

			if (IN_RANGE(dot(intersection.position - u, current_dir), 0.0f, norm(u - v)))
			{
				intersection.status   = IntersectionStatus::inside;
				intersection.distance = norm(intersection.position - position);
			}
		}

		if (intersection.status != IntersectionStatus::none && (closest_intersection.status == IntersectionStatus::none || intersection.distance < closest_intersection.distance))
		{
			closest_intersection        = intersection;
			closest_intersection.normal = n;
		}

		current_dir = { -current_dir.y, current_dir.x };
	}

	return closest_intersection;
}

internal u32 to_pixel(SDL_Surface* surface, vf3 color)
{
	return SDL_MapRGB(surface->format, static_cast<u8>(color.x * 255.0f), static_cast<u8>(color.y * 255.0f), static_cast<u8>(color.z * 255.0f));
}

internal void fill(SDL_Surface* surface, vf2 position, vf2 dimensions, vf3 color)
{
	SDL_Rect rect = { static_cast<i32>(position.x), static_cast<i32>(position.y), static_cast<i32>(dimensions.x), static_cast<i32>(dimensions.y) };
	SDL_FillRect(surface, &rect, to_pixel(surface, color));
}

internal void fill(SDL_Surface* surface, vf3 color)
{
	SDL_Rect rect = { 0, 0, surface->w, surface->h };
	SDL_FillRect(surface, &rect, to_pixel(surface, color));
}

// @TODO@ Use arena.
internal ColumnMajorTexture init_column_major_texture(strlit filepath)
{
	ColumnMajorTexture texture;

	i32  iw;
	i32  ih;
	u32* img = reinterpret_cast<u32*>(stbi_load(filepath, &iw, &ih, 0, STBI_rgb_alpha));
	DEFER { stbi_image_free(img); };
	ASSERT(img);

	texture.w      = iw;
	texture.h      = ih;
	texture.colors = reinterpret_cast<vf3*>(malloc(texture.w * texture.h * sizeof(vf3)));

	FOR_RANGE(y, texture.h)
	{
		FOR_RANGE(x, texture.w)
		{
			u32 pixel = *(img + y * texture.w + x);
			*(texture.colors + x * texture.h + (texture.h - 1 - y)) =
				{
					static_cast<f32>(pixel >>  0 & 0xFF) / 0xFF,
					static_cast<f32>(pixel >>  8 & 0xFF) / 0xFF,
					static_cast<f32>(pixel >> 16 & 0xFF) / 0xFF,
				};
		}
	}

	return texture;
}

internal void deinit_column_major_texture(ColumnMajorTexture* texture)
{
	free(texture->colors);
}

internal void draw_line(SDL_Surface* surface, vf2 start, vf2 end, vf3 color)
{
	u32 pixel  = to_pixel(surface, color);
	vf2 p      = end - start;
	f32 length = norm(p);
	vf2 np     = p / length;
	p = start;

	FOR_RANGE(length)
	{
		if (IN_RANGE(p.x, 0.0f, surface->w) && IN_RANGE(p.y, 0.0f, surface->h))
		{
			*(reinterpret_cast<u32*>(surface->pixels) + static_cast<i32>(p.y) * surface->w + static_cast<i32>(p.x)) = pixel;
		}
		p += np;
	}
}
