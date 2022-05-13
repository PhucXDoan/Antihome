#define PRESSED(INPUT)  (!!platform->inputs[+(INPUT)].curr && !platform->inputs[+(INPUT)].prev)
#define HOLDING(INPUT)  (!!platform->inputs[+(INPUT)].curr)
#define RELEASED(INPUT) (!platform->inputs[+(INPUT)].curr && !!platform->inputs[+(INPUT)].prev)

global constexpr f32 TAU           = 6.28318530717f;
global constexpr f32 SQRT2         = 1.41421356237f;
global constexpr f32 INVSQRT2      = 0.70710678118f;
global constexpr i32 MIPMAP_LEVELS = 4;

// @TODO@ Use an actual RNG lol.
// @NOTE@ Is in interval [0, 65536).
global constexpr u16 RAND_TABLE[] =
	{
		0x9b9d, 0x4e65, 0x8ec9, 0x30e9, 0x5477, 0xe845, 0xab16, 0x910d, 0xfdf3, 0x73dc, 0x424b, 0xc8f9, 0x27a1, 0xe07a, 0x6803, 0xb8c1,
		0x9525, 0x1c29, 0xcf1f, 0x3e18, 0xc463, 0x165f, 0x7018, 0xeae4, 0x7e41, 0x945e, 0xbbbf, 0x5ac7, 0x15a4, 0xa666, 0xa8f1, 0xbc54,
		0xe604, 0x3393, 0x5cad, 0x061f, 0x98b7, 0x93fd, 0xc762, 0xa37b, 0x1880, 0x26cd, 0xb131, 0x3f59, 0xae82, 0x264f, 0x9355, 0xc0e8,
		0x5e46, 0x76bd, 0xd8f2, 0x2e69, 0xb158, 0x0317, 0x127f, 0x936a, 0x0ccd, 0x8497, 0xd08d, 0x92ad, 0xa3d3, 0xcf59, 0x1dc6, 0xbb4b,
		0x7ecb, 0xb664, 0x93c7, 0xd3f1, 0xd23c, 0x9b2f, 0x5fae, 0x8c17, 0xdfee, 0xd19c, 0x3bff, 0x6ac7, 0x078c, 0x8282, 0x9fe3, 0xa7fd,
		0xa653, 0xd182, 0x31ad, 0x0938, 0x5ac8, 0xcd79, 0x8622, 0x4e64, 0xeb1f, 0x3da6, 0x133d, 0x6f45, 0xe061, 0x6449, 0xe19e, 0xf63b,
		0x5440, 0xe667, 0x9be6, 0x27f1, 0xfbee, 0x407b, 0x3c76, 0x54b6, 0x8061, 0x6e0b, 0x36f5, 0xdd0d, 0xbaa5, 0xe8dc, 0x2a46, 0x4f8f,
		0x63ab, 0xda6c, 0x5950, 0x1df6, 0xd32a, 0xf42b, 0x68dd, 0xe9d2, 0xb825, 0x31b8, 0x2a7c, 0x49f0, 0xe859, 0x4041, 0x2fce, 0x72f9,
		0x8224, 0x9615, 0x888f, 0x2050, 0x9172, 0x5744, 0xe53a, 0xfaef, 0xf191, 0xd627, 0x2dae, 0x69da, 0xe4b3, 0x4a30, 0x9436, 0x80cd,
		0xb863, 0x7ac0, 0xd72b, 0xb49c, 0x20fc, 0xa0c6, 0x29c2, 0xb683, 0xc820, 0x36d8, 0xeb86, 0x3bc5, 0xd2f4, 0xa04e, 0x90b4, 0xe5f0,
		0x44df, 0x442b, 0xf29c, 0x72db, 0x0fdf, 0x366f, 0x36e7, 0x734a, 0x00c9, 0x40c1, 0xbddf, 0x7fcd, 0xff1e, 0x31a9, 0xd998, 0x67c7,
		0x328f, 0xbf19, 0xaa42, 0x165a, 0x3975, 0x99fb, 0x0df0, 0xe0ac, 0x6cf0, 0x6c4d, 0x7a87, 0x73b5, 0xc3fc, 0x8878, 0xe755, 0x01ab,
		0x2d9e, 0x19dc, 0x2f14, 0xcaca, 0x4510, 0x2983, 0xfa28, 0xd81a, 0xd4ba, 0x757a, 0xfe60, 0x58fd, 0xb899, 0x24e8, 0x90ac, 0x1a92,
		0x4b26, 0xb050, 0xd8e1, 0x96d1, 0x6ad2, 0x330d, 0x9e1a, 0xcfc4, 0xaabd, 0x0394, 0x1a64, 0x59da, 0x4f65, 0xac6f, 0x381d, 0x1c57,
		0xbc45, 0xab88, 0x0f90, 0x2dd3, 0x468b, 0x71ac, 0xe1d1, 0x4e64, 0x1f1e, 0xbb90, 0xa3d7, 0xa04a, 0xd59b, 0x8778, 0x38ed, 0xd353,
		0x5e6f, 0xd907, 0xbbcf, 0xe53e, 0xe37a, 0x7c8d, 0x9d9d, 0xb302, 0xda05, 0xd0df, 0x0cbf, 0x2efe, 0x38aa, 0xbd2d, 0x1a53, 0x0f2b
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

struct ImgRGBA
{
	vi2  dim;
	vf4* rgba;
};

struct RGB
{
	u8 r;
	u8 g;
	u8 b;
	u8 PADDING_;
};

struct Mipmap
{
	vi2  base_dim;
	RGB* data;
};

internal constexpr vf2 conjugate(vf2 v) { return {  v.x, -v.y }; }
internal constexpr vf2 rotate90(vf2 v)  { return { -v.y,  v.x }; }

internal constexpr f32 square(f32 x) { return x * x; }
internal constexpr f32 cube(f32 x) { return x * x * x; }

internal constexpr vf4 lerp(vf4 a, vf4 b, f32 t) { return a * (1.0f - t) + b * t; }
internal constexpr vf3 lerp(vf3 a, vf3 b, f32 t) { return a * (1.0f - t) + b * t; }
internal constexpr vf2 lerp(vf2 a, vf2 b, f32 t) { return a * (1.0f - t) + b * t; }
internal constexpr f32 lerp(f32 a, f32 b, f32 t) { return a * (1.0f - t) + b * t; }

internal constexpr f32 dampen(f32 a, f32 b, f32 k, f32 dt) { return lerp(a, b, 1.0f - expf(-k * dt)); }
internal constexpr vf2 dampen(vf2 a, vf2 b, f32 k, f32 dt) { return lerp(a, b, 1.0f - expf(-k * dt)); }
internal constexpr vf3 dampen(vf3 a, vf3 b, f32 k, f32 dt) { return lerp(a, b, 1.0f - expf(-k * dt)); }
internal constexpr vf4 dampen(vf4 a, vf4 b, f32 k, f32 dt) { return lerp(a, b, 1.0f - expf(-k * dt)); }

internal constexpr f32 dot(vf3 u, vf3 v) { return u.x * v.x + u.y * v.y + u.z * v.z; }
internal constexpr f32 dot(vf2 u, vf2 v) { return u.x * v.x + u.y * v.y;             }

internal constexpr vf3 cross(vf3 u, vf3 v) { return { u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x }; }

internal constexpr vf4 hadamard_product(vf4 u, vf4 v) { return { u.x * v.x, u.y * v.y, u.z * v.z, u.w * v.w }; }

internal constexpr vf3 monochrome(f32 x) { return { x, x, x }; }

internal f32 norm_sq(vf3 v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
internal f32 norm_sq(vf2 v) { return v.x * v.x + v.y * v.y            ; }

internal f32 norm(vf3 v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }
internal f32 norm(vf2 v) { return sqrtf(v.x * v.x + v.y * v.y            ); }

internal vf3 normalize(vf3 v) { return v / norm(v); }
internal vf2 normalize(vf2 v) { return v / norm(v); }

internal f32 distance(vf3 u, vf3 v) { return norm(u - v); }
internal f32 distance(vf2 u, vf2 v) { return norm(u - v); }

internal i32 mod(i32 x, i32 m) { return (x % m + m) % m; }
internal f32 mod(f32 x, f32 m) { f32 y = fmodf(x, m); return y < 0.0f ? y + m : y; }

internal vf2 polar(f32 angle) { return { cosf(angle), sinf(angle) }; }

internal vf2 rotate(vf2 v, f32 angle)
{
	return { v.x * cosf(angle) - v.y * sinf(angle), v.x * sinf(angle) + v.y * cosf(angle) };
}

internal constexpr u32 pack_color(vf3 color)
{
	return
		(static_cast<u8>(color.x * 255.0f) << 24) |
		(static_cast<u8>(color.y * 255.0f) << 16) |
		(static_cast<u8>(color.z * 255.0f) <<  8);
}

internal constexpr vf3 unpack_color(u32 pixel)
{
	return
		{
			static_cast<f32>((pixel >> 24) & 0xFF) / 255.0f,
			static_cast<f32>((pixel >> 16) & 0xFF) / 255.0f,
			static_cast<f32>((pixel >>  8) & 0xFF) / 255.0f
		};
}

internal ImgRGBA init_img_rgba(strlit filepath)
{
	ImgRGBA img;

	i32  iw;
	i32  ih;
	u32* stbimg = reinterpret_cast<u32*>(stbi_load(filepath, &iw, &ih, 0, STBI_rgb_alpha));
	DEFER { stbi_image_free(stbimg); };
	ASSERT(stbimg);

	img.dim  = { iw, ih };
	img.rgba = reinterpret_cast<vf4*>(malloc(img.dim.x * img.dim.y * sizeof(vf4)));

	FOR_RANGE(y, img.dim.y)
	{
		FOR_RANGE(x, img.dim.x)
		{
			u32 pixel = *(stbimg + y * img.dim.x + x);
			*(img.rgba + x * img.dim.y + (img.dim.y - 1 - y)) =
				{
					static_cast<f32>(pixel >>  0 & 0xFF) / 0xFF,
					static_cast<f32>(pixel >>  8 & 0xFF) / 0xFF,
					static_cast<f32>(pixel >> 16 & 0xFF) / 0xFF,
					static_cast<f32>(pixel >> 24 & 0xFF) / 0xFF
				};
		}
	}

	return img;
}

internal void deinit_img_rgba(ImgRGBA* img)
{
	free(img->rgba);
}

internal vf4 img_color_at(ImgRGBA* img, vf2 uv)
{
	ASSERT(0.0f <= uv.x && uv.x <= 1.0f);
	ASSERT(0.0f <= uv.y && uv.y <= 1.0f);
	return img->rgba[static_cast<i32>(uv.x * (img->dim.x - 1.0f)) * img->dim.y + static_cast<i32>(uv.y * (img->dim.y - 1.0f))];
}

internal Mipmap init_mipmap(strlit file_path)
{
	Mipmap mipmap;

	vi2  absolute_dim;
	u32* stbimg = reinterpret_cast<u32*>(stbi_load(file_path, &absolute_dim.x, &absolute_dim.y, 0, STBI_rgb_alpha));
	DEFER { stbi_image_free(stbimg); };
	ASSERT(stbimg);

	mipmap.base_dim = { absolute_dim.x * 2 / 3, absolute_dim.y };
	mipmap.data     = reinterpret_cast<RGB*>(malloc((mipmap.base_dim.x * mipmap.base_dim.y * 2 - mipmap.base_dim.x * mipmap.base_dim.y * 2 / (1 << MIPMAP_LEVELS)) * sizeof(RGB)));

	vi2  stbimg_coordinates = { 0, 0 };
	RGB* mipmap_pixel = mipmap.data;
	FOR_RANGE(i, MIPMAP_LEVELS)
	{
		FOR_RANGE(ix, mipmap.base_dim.x / (1 << i))
		{
			FOR_RANGE(iy, mipmap.base_dim.y / (1 << i))
			{
				u32 stbimg_pixel = *(stbimg + (stbimg_coordinates.y + iy) * absolute_dim.x + stbimg_coordinates.x + ix);
				*mipmap_pixel++ =
					{
						static_cast<u8>(stbimg_pixel >>  0 & 0xFF),
						static_cast<u8>(stbimg_pixel >>  8 & 0xFF),
						static_cast<u8>(stbimg_pixel >> 16 & 0xFF)
					};
			}
		}

		stbimg_coordinates = { mipmap.base_dim.x, mipmap.base_dim.y - mipmap.base_dim.y / (1 << i) };
	}

	return mipmap;
}

internal void deinit_mipmap(Mipmap* mipmap)
{
	free(mipmap->data);
}

internal vf3 mipmap_color_at(Mipmap* mipmap, f32 level, vf2 uv)
{
	ASSERT(0.0f <= uv.x && uv.x <= 1.0f);
	ASSERT(0.0f <= uv.y && uv.y <= 1.0f);

	i32 l = static_cast<i32>(clamp(level, 0.0f, MIPMAP_LEVELS - 1.0f));
	RGB p =
		*(
			mipmap->data
				+ mipmap->base_dim.x * mipmap->base_dim.y * 4 / 3 - mipmap->base_dim.x * mipmap->base_dim.y * 4 / 3 / (1 << (l * 2))
				+ static_cast<i32>(uv.x * (mipmap->base_dim.x / (1 << l) - 1.0f)) * (mipmap->base_dim.y / (1 << l))
				+ static_cast<i32>((1.0f - uv.y) * (mipmap->base_dim.y / (1 << l) - 1.0f))
		);

	if (IN_RANGE(level, 0.0f, MIPMAP_LEVELS - 1.0f))
	{
		RGB q =
			*(
				mipmap->data
					+ mipmap->base_dim.x * mipmap->base_dim.y * 4 / 3 - mipmap->base_dim.x * mipmap->base_dim.y * 4 / 3 / (1 << ((l + 1) * 2))
					+ static_cast<i32>(uv.x * (mipmap->base_dim.x / (1 << (l + 1)) - 1.0f)) * (mipmap->base_dim.y / (1 << (l + 1)))
					+ static_cast<i32>((1.0f - uv.y) * (mipmap->base_dim.y / (1 << (l + 1)) - 1.0f))
			);

		return vf3 { lerp(p.r, q.r, level - l), lerp(p.g, q.g, level - l), lerp(p.b, q.b, level - l) } / 255.0f;
	}
	else
	{
		return vf3 { p.r / 255.0f, p.g / 255.0f, p.b / 255.0f };
	}
}

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

internal bool32 ray_cast_line(f32* scalar, f32* portion, vf2 position, vf2 ray, vf2 start, vf2 end)
{
	*scalar = ((start.x - end.x) * (start.y - position.y) - (start.y - end.y) * (start.x - position.x)) / ((start.x - end.x) * ray.y - (start.y - end.y) * ray.x);

	if (*scalar < 0.0f)
	{
		return false;
	}
	else
	{
		f32 portion_c = start.x * ray.y - start.y * ray.x;
		*portion = (portion_c + ray.x * position.y - ray.y * position.x) / (portion_c + ray.x * end.y - ray.y * end.x);

		return true;
	}
}

enum struct Orientation : u8
{
	collinear,
	clockwise,
	counterclockwise
};

bool32 is_point_on_line_segment(vf2 p, vf2 q, vf2 r)
{
	return q.x <= max(p.x, r.x) && q.x >= min(p.x, r.x) && q.y <= max(p.y, r.y) && q.y >= min(p.y, r.y);
}

internal Orientation orientation_of(vf2 p, vf2 q, vf2 r)
{
	f32 n = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);

	if (n == 0.0f)
	{
		return Orientation::collinear;
	}
	else if (n > 0.0f)
	{
		return Orientation::clockwise;
	}
	else
	{
		return Orientation::counterclockwise;
	}
}

bool32 is_line_segment_intersecting(vf2 p1, vf2 q1, vf2 p2, vf2 q2)
{
	Orientation o1 = orientation_of(p1, q1, p2);
	Orientation o2 = orientation_of(p1, q1, q2);
	Orientation o3 = orientation_of(p2, q2, p1);
	Orientation o4 = orientation_of(p2, q2, q1);

	return
		o1 != o2 && o3 != o4
		|| o1 == Orientation::collinear && is_point_on_line_segment(p1, p2, q1)
		|| o2 == Orientation::collinear && is_point_on_line_segment(p1, q2, q1)
		|| o3 == Orientation::collinear && is_point_on_line_segment(p2, p1, q2)
		|| o4 == Orientation::collinear && is_point_on_line_segment(p2, q1, q2);
}

internal bool32 ray_cast_plane(f32* scalar, vf2* portion, vf3 position, vf3 ray, vf3 plane_origin, vf3 plane_dx, vf3 plane_dy)
{
	__m128 mx = _mm_setr_ps(plane_dx.y, plane_dx.z, plane_dx.x, 0.0f);
	__m128 my = _mm_setr_ps(plane_dy.z, plane_dy.x, plane_dy.y, 0.0f);
	__m128 ma =
		_mm_sub_ps
		(
			_mm_mul_ps(mx, my),
			_mm_mul_ps
			(
				_mm_setr_ps(plane_dy.y, plane_dx.x, plane_dx.y, 0.0f),
				_mm_setr_ps(plane_dx.z, plane_dy.z, plane_dy.x, 0.0f)
			)
		);

	__m128 mb = _mm_mul_ps(ma, _mm_setr_ps(ray.x, ray.y, ray.z, 0.0f));
	mb = _mm_hadd_ps(mb, mb);

	f32 det = _mm_cvtss_f32(_mm_hadd_ps(mb, mb));
	__m128 mv  =
		_mm_sub_ps
		(
			_mm_setr_ps(plane_origin.x, plane_origin.y, plane_origin.z, 0.0f),
			_mm_setr_ps(position.x, position.y, position.z, 0.0f)
		);

	mb = _mm_mul_ps(ma, mv);
	mb = _mm_hadd_ps(mb, mb);
	*scalar = _mm_cvtss_f32(_mm_hadd_ps(mb, mb)) / det;

	if (*scalar >= 0.0f)
	{
		__m128 mr0 = _mm_setr_ps(ray.y, ray.z, ray.x, 0.0f);
		__m128 mr1 = _mm_setr_ps(ray.z, ray.x, ray.y, 0.0f);

		ma =
			_mm_mul_ps
			(
				_mm_sub_ps
				(
					_mm_mul_ps(mr0, my),
					_mm_mul_ps(mr1, _mm_setr_ps(plane_dy.y, plane_dy.z, plane_dy.x, 0.0f))
				),
				mv
			);

		ma = _mm_hadd_ps(ma, ma);
		portion->x = _mm_cvtss_f32(_mm_hadd_ps(ma, ma)) / det;

		ma =
			_mm_mul_ps
			(
				_mm_sub_ps
				(
					_mm_mul_ps(mr1, mx),
					_mm_mul_ps(mr0, _mm_setr_ps(plane_dx.z, plane_dx.x, plane_dx.y, 0.0f))
				),
				mv
			);

		ma = _mm_hadd_ps(ma, ma);
		portion->y = _mm_cvtss_f32(_mm_hadd_ps(ma, ma)) / det;

		return true;
	}
	else
	{
		return false;
	}
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

internal void set_color(SDL_Renderer* renderer, vf3 color)
{
	SDL_SetRenderDrawColor
	(
		renderer,
		static_cast<u8>(color.x * 255.0f),
		static_cast<u8>(color.y * 255.0f),
		static_cast<u8>(color.z * 255.0f),
		255
	);
}

internal void set_color(SDL_Renderer* renderer, vf4 color)
{
	SDL_SetRenderDrawColor
	(
		renderer,
		static_cast<u8>(color.x * 255.0f),
		static_cast<u8>(color.y * 255.0f),
		static_cast<u8>(color.z * 255.0f),
		static_cast<u8>(color.w * 255.0f)
	);
}

internal void draw_filled_rect(SDL_Renderer* renderer, vi2 position, vi2 dimensions)
{
	SDL_Rect rect = { position.x, position.y, dimensions.x, dimensions.y };
	SDL_RenderFillRect(renderer, &rect);
}

internal void draw_rect(SDL_Renderer* renderer, vi2 position, vi2 dimensions)
{
	SDL_Rect rect = { position.x, position.y, dimensions.x, dimensions.y };
	SDL_RenderDrawRect(renderer, &rect);
}

internal void draw_line(SDL_Renderer* renderer, vi2 start, vi2 end)
{
	ASSERT(fabs(start.x - end.x) + fabs(start.y - end.y) < 4096.0f);
	SDL_RenderDrawLine(renderer, start.x, start.y, end.x, end.y);
}

internal void draw_circle(SDL_Renderer* renderer, vi2 center, i32 radius)
{
	ASSERT(radius < 256);

	vi2 p     = { radius - 1, 0 };
	vi2 t     = { 1, 1 };
	i32 error = t.x - 2 * radius;

	while (p.x >= p.y)
	{
		SDL_RenderDrawPoint(renderer, center.x + p.x, center.y - p.y);
		SDL_RenderDrawPoint(renderer, center.x + p.x, center.y + p.y);
		SDL_RenderDrawPoint(renderer, center.x - p.x, center.y - p.y);
		SDL_RenderDrawPoint(renderer, center.x - p.x, center.y + p.y);
		SDL_RenderDrawPoint(renderer, center.x + p.y, center.y - p.x);
		SDL_RenderDrawPoint(renderer, center.x + p.y, center.y + p.x);
		SDL_RenderDrawPoint(renderer, center.x - p.y, center.y - p.x);
		SDL_RenderDrawPoint(renderer, center.x - p.y, center.y + p.x);

		if (error <= 0)
		{
			p.y   += 1;
			error += t.y;
			t.y   += 2;
		}

		if (error > 0)
		{
			p.x   -= 1;
			t.x   += 2;
			error += t.x - 2 * radius;
		}
	}
}

internal void draw_filled_circle(SDL_Renderer* renderer, vi2 center, i32 radius)
{
	ASSERT(radius < 256);

	vi2 p     = { radius - 1, 0 };
	vi2 t     = { 1, 1 };
	i32 error = t.x - 2 * radius;

	while (p.x >= p.y)
	{
		SDL_RenderDrawLine(renderer, center.x - p.x, center.y - p.y, center.x + p.x, center.y - p.y);
		SDL_RenderDrawLine(renderer, center.x - p.y, center.y - p.x, center.x + p.y, center.y - p.x);
		SDL_RenderDrawLine(renderer, center.x - p.x, center.y + p.y, center.x + p.x, center.y + p.y);
		SDL_RenderDrawLine(renderer, center.x - p.y, center.y + p.x, center.x + p.y, center.y + p.x);

		if (error <= 0)
		{
			p.y   += 1;
			error += t.y;
			t.y   += 2;
		}

		if (error > 0)
		{
			p.x   -= 1;
			t.x   += 2;
			error += t.x - 2 * radius;
		}
	}
}

template <typename... ARGUMENTS>
internal void draw_text(SDL_Renderer* renderer, FC_Font* font, vf2 coordinates, FC_AlignEnum alignment, f32 scalar, vf4 rgba, strlit fstr, ARGUMENTS... arguments)
{
	FC_DrawEffect
	(
		font,
		renderer,
		coordinates.x,
		coordinates.y,
		FC_MakeEffect
		(
			alignment,
			FC_MakeScale(scalar, scalar),
			FC_MakeColor
			(
				static_cast<u8>(rgba.x * 255.0f),
				static_cast<u8>(rgba.y * 255.0f),
				static_cast<u8>(rgba.z * 255.0f),
				static_cast<u8>(rgba.w * 255.0f)
			)
		),
		fstr,
		arguments...
	);
}

template <typename... ARGUMENTS>
internal void draw_boxed_text(SDL_Renderer* renderer, FC_Font* font, vi2 coordinates, vi2 dimensions, FC_AlignEnum alignment, f32 scalar, vf4 rgba, strlit fstr, ARGUMENTS... arguments)
{
	FC_DrawBoxEffect
	(
		font,
		renderer,
		{ coordinates.x, coordinates.y, static_cast<i32>(dimensions.x / scalar), dimensions.y },
		FC_MakeEffect
		(
			alignment,
			FC_MakeScale(scalar, scalar),
			FC_MakeColor
			(
				static_cast<u8>(rgba.x * 255.0f),
				static_cast<u8>(rgba.y * 255.0f),
				static_cast<u8>(rgba.z * 255.0f),
				static_cast<u8>(rgba.w * 255.0f)
			)
		),
		fstr,
		arguments...
	);
}

internal void blit_texture(SDL_Renderer* renderer, SDL_Texture* texture, vi2 position, vi2 dimensions)
{
	SDL_Rect dst = { position.x, position.y, dimensions.x, dimensions.y };
	SDL_RenderCopy(renderer, texture, 0, &dst);
}

internal i32 iterate_repeated_movement(Platform* platform, Input negative_input, Input positive_input, f32* current_repeat_countdown, f32 repeat_threshold = 0.3f, f32 repeat_frequency = 0.1f)
{
	i32 delta = 0;

	if (HOLDING(negative_input) != HOLDING(positive_input))
	{
		if (*current_repeat_countdown <= 0.0f)
		{
			if (HOLDING(negative_input))
			{
				delta                     = -1;
				*current_repeat_countdown =
					PRESSED(negative_input)
						? repeat_threshold
						: repeat_frequency;
			}
			else
			{
				delta                     = 1;
				*current_repeat_countdown =
					PRESSED(positive_input)
						? repeat_threshold
						: repeat_frequency;
			}
		}
	}
	else
	{
		*current_repeat_countdown = 0.0f;
	}

	return delta;
}
