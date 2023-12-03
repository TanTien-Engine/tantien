#define QUOTE(...) #__VA_ARGS__
static const char* dbModuleSource = QUOTE(

foreign class RTree
{
	init() {}
	init(db) {}

	foreign load_from_file(filepath)

	foreign insert(poly)
	foreign query(key)
	foreign delete(key)

	foreign update(hist)

	foreign insert_with_time(poly, time)
	foreign query_with_time(key, tmin, tmax)

	foreign get_all_leaves()
	foreign query_leaves(region)

	foreign pick_aabbs(pos, dir)
	foreign pick_polys(pos, dir)
}

foreign class RKey
{
	init() {}
	init(aabb) {}

	foreign region()
	foreign id()
}

foreign class RFile
{
	init(filename) {}
}

);