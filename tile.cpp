#include "tile.h"

std::unordered_map<Tile *, v2s16> *g_mapdata =
	new std::unordered_map<Tile *, v2s16>();
std::vector<Tile *> g_pool;


v2s16 tile_pos_to_dir[TP_TOTAL] = {
	v2s16(-1, 0), v2s16(0, -1), v2s16(1, 0), v2s16(0, 1) 
};

int Tile::s_seen_max = 1;

int Face::getDistance(const Face &other) const
{
	int diff = 0;
	for (int i = 0; i < SEGNUM; ++i)
		diff += ABS(colors[i] - other.colors[i]);

	/*int variety = 0;
	for (int i = 0; i < SEGNUM - 1; ++i)
		variety += ABS(colors[i] - colors[i + 1]);*/

	diff += ((int)512 - other.variance - variance);

	if (diff < 0)
		diff = 0;

	return diff;
}

Tile::Tile(const v2u16 &tilepos, const v2u16 &original)
{
	original_pos = original;

	for (int i = 0; i < TP_TOTAL; ++i)
		neighbours[i] = nullptr;
	link_count = 0;
}

Tile *Tile::getAtPos(const v2s16 &pos)
{
	for (auto &it : *g_mapdata) {
		if (it.second == pos)
			return it.first;
	}
	return nullptr;
}

bool Tile::link(Tile *other, TILE_POS face)
{
	// Not neighbours
	if (face == TP_TOTAL)
		return false;

	if (neighbours[face] == other)
		return true;

	// Unlink previous neighbour
	unlink(face);

	TILE_POS o_face = swapTilePos(face);
	if (other->neighbours[o_face]) {
		WARN("CONFLICT: " << PP(other->original_pos)
			<< " already linked with " << PP(other->neighbours[o_face]->original_pos));
	
		other->unlink(o_face);
		return false;
	}

	// Link with new neighbour
	neighbours[face] = other;
	link_count++;

	other->neighbours[o_face] = this;
	other->link_count++;

	g_mapdata->clear();
	if (!makeMap(v2s16())) {
		VERBOSE("Failed to link " << PP(other->original_pos)
			<< " to " << PP(original_pos) << " (not planar)");
		unlink(face);
		return false;
	}

	VERBOSE("Link face=" << (int)face << ", len=" << getWeight() << std::endl
		<< "\t" << PP(original_pos) << " ---> " << PP(other->original_pos));
	checkLinks();
	other->checkLinks();
	return true;
}

bool Tile::unlink(TILE_POS face)
{
	if (face == TP_TOTAL)
		return false;

	Tile *other = neighbours[face];
	if (!other)
		return false;

	TILE_POS o_face = swapTilePos(face);
	if (!other->neighbours[o_face]) {
		ERROR("Neighbour face=" << (int)face
			<< " not linked with " << PP(original_pos));
	}

	other->neighbours[o_face] = nullptr;
	other->link_count--;
	neighbours[face] = nullptr;
	link_count--;

	checkLinks();
	other->checkLinks();
	return true;
}

void Tile::checkLinks()
{
	int old_count = link_count;
	link_count = 0;
	for (int i = 0; i < TP_TOTAL; ++i) {
		if (!neighbours[i])
			continue;

		link_count++;
		for (int j = 0; j < i; ++j) {
			if (neighbours[j] == neighbours[i])
				ERROR("1 tile, two neighbours: " << PP(original_pos));
		}
	}

	if (old_count != link_count)
		WARN("old=" << old_count << " new=" << link_count);
}

int Tile::getDistance(Tile *other, TILE_POS *best_match) const
{
	int d, diff = 0xFFFF;

	for (int i = 0; i < TP_TOTAL; ++i) {
#if 0
		if (neighbours[i] && neighbours[i] != other)
			continue;

		TILE_POS o_face = swapTilePos(i);
		if (other->neighbours[o_face] && other->neighbours[o_face] != this)
			continue;
#else
		if (neighbours[i])
			continue;
		TILE_POS o_face = swapTilePos(i);
		if (other->neighbours[o_face])
			continue;
#endif
	
		d = faces[i].getDistance(other->faces[o_face]);

		if (d < diff) {
			diff = d;
			*best_match = (TILE_POS)i;
		}
	}
	/*if (original_pos == v2u16(128, 192)// (136, 192)
		&& other->original_pos == v2u16(192, 896)
	) {
		LOG("match: " << (int)*best_match << " diff = " << diff);
		getchar();
	}*/
	VERBOSE(PP(original_pos) << " -- " << PP(other->original_pos)
		<< " :: face=" << (int)*best_match << ", diff=" << diff);

	if (diff == 0xFFFF)
		return -1;

	return diff;
}

Tile *Tile::getNeighbour(TILE_POS face)
{
	if (face == TP_TOTAL)
		ERROR("Wrong face");

	return neighbours[face];
}

int Tile::getDistanceAll() const
{
	int d = 0;
	int n = 0;

	for (int i = 0; i < TP_TOTAL; ++i) {
		if (!neighbours[i])
			continue;

		d += faces[i].getDistance(neighbours[i]->faces[swapTilePos(i)]);
		n++;
	}
	if (n == 0)
		return -1;

	return d / n;
}

int Tile::recursiveExecS(tilecall_t func)
{
	if (m_seen > s_seen_max)
		ERROR("Seen counter exceeds limits! diff=" << m_seen - s_seen_max);

	if (m_seen == s_seen_max)
		return 0;
	m_seen = s_seen_max;

	int executed = 1;
	if (func)
		func(this);

	for (int i = 0; i < TP_TOTAL; ++i) {
		if (neighbours[i])
			executed += neighbours[i]->recursiveExecS(func);
	}
	return executed;
}

int Tile::getWeight()
{
	pushSeen();
	int n = recursiveExecS(nullptr);
	popSeen();
	return n;
}

int Tile::getDimensions(v2s16 &dim_min, v2s16 &dim_max)
{
	dim_min = v2s16(0x7FFF, 0x7FFF);
	dim_max = v2s16(-0x7FFF, -0x7FFF);

	tilecall_t f_measure = [&] (Tile *tile) {
		auto it = g_mapdata->find(tile);
		if (it == g_mapdata->end()) {
			Tile::dumpMap();
			ERROR("Tile not in map: " << PP(tile->original_pos));
		}

		const v2s16 &pos = it->second;
		if (pos.X > dim_max.X)
			dim_max.X = pos.X;
		if (pos.Y > dim_max.Y)
			dim_max.Y = pos.Y;

		if (pos.X < dim_min.X)
			dim_min.X = pos.X;
		if (pos.Y < dim_min.Y)
			dim_min.Y = pos.Y;
	};

	pushSeen();
	int n = recursiveExecS(f_measure); // SEEN
	popSeen();
	return n;
}

bool Tile::makeMap(v2s16 pos)
{
	auto it = g_mapdata->find(this);
	if (it != g_mapdata->end())
		return it->second == pos;

	for (auto &it : *g_mapdata) {
		if (it.second == pos)
			return false;
	}
	g_mapdata->insert(std::pair<Tile *, v2s16>(this, pos));

	for (int i = 0; i < TP_TOTAL; ++i) {
		if (!neighbours[i])
			continue;

		if (!neighbours[i]->makeMap(pos + tile_pos_to_dir[i]))
			return false;
	}
	return true;
}

void Tile::dumpMap()
{
	std::cout << "==== Map dump: " << g_mapdata->size() << " entries" << std::endl;
	for (auto &it : *g_mapdata) {
		std::cout << PP(it.second) << " <-- "
			<< PP(it.first->original_pos) << std::endl;
	}
}

int Tile::sortAllUnsafe(Tile *&center)
{
	int max_length = 0;

	// Find the longest chain
	pushSeen();
	for (Tile *tile : g_pool) {
		int n = tile->recursiveExecS(nullptr); // SEEN
		if (n > max_length) {
			center = tile;
			max_length = n;
		}
	}
	popSeen();

	g_mapdata->clear();
	bool ok = center->makeMap(v2s16());
	if (!ok)
		WARN("Map collision");
	//Tile::dumpMap();

	v2s16 dim_min, dim_max;
	center->getDimensions(dim_min, dim_max);

	// Remove offsets
	pushSeen();
	center->recursiveExecS([&](Tile *tile) {
		auto it = g_mapdata->find(tile);
		it->second = it->second - dim_min;
	});
	dim_max = dim_max - dim_min;
	dim_min = v2s16();
	popSeen();

	const int SPACE = 3;
	pushSeen();
	center->recursiveExecS(nullptr); // already mapped
	dim_max.X += SPACE;
	for (Tile *tile : g_pool) {
		if (tile->getSeenDiff() == 0)
			continue; // seen

		auto *real_map = g_mapdata;
		g_mapdata = new std::unordered_map<Tile *, v2s16>();
		tile->makeMap(v2s16());

		v2s16 adj_min, adj_max;
		tile->getDimensions(adj_min, adj_max);

		// Append positions to the sides of "real_map"
		for (auto &it : *g_mapdata) {
			it.second = v2s16(
				it.second.X - adj_min.X + dim_max.X,
				it.second.Y - adj_min.Y + dim_min.Y
			);
			VERBOSE(PP(it.second));
			real_map->insert(it);
		}
		dim_max.X += SPACE + (adj_max.X - adj_min.X);
		// Find taller one
		dim_max.Y = dim_max.Y > (adj_max.Y - adj_min.Y) ?
			dim_max.Y : (adj_max.Y - adj_min.Y);

		if (dim_max.X >= SPACE * 20) {
			dim_min.Y += dim_max.Y + SPACE;
			dim_max.X = 0;
			dim_max.Y = 0;
		}

		delete g_mapdata;
		g_mapdata = real_map;
	}
	dim_max.Y += dim_min.Y;
	dim_min = v2s16();
	popSeen();

	LOG("Longest: " << max_length << std::endl
		<< "\tmin=" << PP(dim_min)
		<< ", max=" << PP(dim_max));
	return max_length;
}

void Tile::popSeen()
{
	if (!s_seen_max)
		ERROR("Seen == 0");

	for (Tile *tile : g_pool) {
		if (tile->m_seen == s_seen_max)
			--tile->m_seen;
	}
	s_seen_max--;
}
