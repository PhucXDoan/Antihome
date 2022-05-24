// @TODO@ Better system.
internal f32 rng(u32 seed)
{
	srand(seed);
	return rand() / (RAND_MAX + 1.0f);
}

internal f32 rng(u32* seed)
{
	srand(++*seed);
	*seed += rand();
	return rand() / (RAND_MAX + 1.0f);
}

internal i32 rng(u32* seed, i32 start, i32 end)
{
	return static_cast<i32>(rng(seed) * (end - start) + start);
}

internal f32 rng(u32* seed, f32 start, f32 end)
{
	return rng(seed) * (end - start) + start;
}
