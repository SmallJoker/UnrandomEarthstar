#include "headers.h"
#include "image.h"
#include "tile.h"
#include "util/args_parser.h"
#include "util/timer.h"
#include "util/unittest.h"

#include <algorithm> // std::sort

#include <unordered_map>

int checkIntegrity(v2s16 pos, Tile *tile)
{
	int diff = 0;
	int n = 0;
	for (int i = 0; i < TP_TOTAL; ++i) {
		Tile *fix = Tile::getAtPos(pos + tile_pos_to_dir[i]);
		if (!fix)
			continue;
		diff += tile->faces[i].getDistance(fix->faces[swapTilePos(i)]);
		n++;
	}
	if (n == 0)
		return 0;

	return diff / n;
}

int closestMatchLoop(Image &img)
{
	static int min_diff = 0;

	struct result_t {
		Tile *t1;
		Tile *t2;
		TILE_POS face;
		int diff;
	};

	std::vector<result_t> ranking;

	TILE_POS face;
	int d, d2;

	ranking.clear();
	ranking.reserve(g_pool.size());
	for (Tile *t1 : g_pool) {
		Tile::pushSeen();
		t1->recursiveExecS(nullptr);

		tilecall_t add_tile = [&] (Tile *t2) {
			d = t1->getDistance(t2, &face);

			if (d < min_diff)// || d > min_diff + 10)
				return;

			g_mapdata->clear();
			g_mapdata->reserve(g_pool.size());
			Tile *old_neighbour = t1->getNeighbour(face);

			if (t1->link(t2, face)) {
				d2 = t1->getDistanceAll();
				if (d2 < d * 1.2f) {
					ranking.push_back(result_t {
						.t1 = t1,
						.t2 = t2,
						.face = face,
						.diff = d
					});
				}
			}

			if (old_neighbour)
				t1->link(old_neighbour, face);
			else
				t1->unlink(face);
		};

		for (Tile *t2 : g_pool)
			t2->recursiveExecS(add_tile);
		Tile::popSeen();
	}

	std::sort(ranking.begin(), ranking.end(),
			[](const result_t &a, const result_t &b) {
		return a.diff < b.diff;
	});

	int n = 0;
	int moved = 0;
	for (auto &res : ranking) {
		g_mapdata->clear();
		Tile *old_neighbour = res.t1->getNeighbour(res.face);
		if (res.t1->link(res.t2, res.face)) {
			LOG(PP(res.t1->original_pos) << " <--> " << PP(res.t2->original_pos)
				<< "  diff=" << res.diff
				<< ", face=" << (int)res.face);
			// OK
			moved++;
			min_diff = res.diff;
		} else {
			if (old_neighbour) {
				if (!res.t1->link(old_neighbour, res.face))
					ERROR("Undo failed. Link code is broken!");
			} else {
				res.t1->unlink(res.face);
			}
		}

		if (moved >= 40 || ++n > 100)
			break;
	}
	return moved;
}

int closestMatchLoop2(Image &img)
{
	static int loop_n = 0; loop_n++;
	// Find closest edges
	int min_diff = 0xFFFF;
	int total_moved = 0;
	int last_length = 1;

	TILE_POS f_face;
	Tile *f_tile;
	int f_diff;

	tilecall_t f_find_closest = [&] (Tile *tile) {
		if (tile->link_count >= TP_TOTAL)
			return;

		for (Tile *other : g_pool) {
			if (other == tile)
				continue;

			if (other->link_count >= TP_TOTAL)
				continue;

			if (other->getSeenDiff() != 1)
				continue; // connected ones are 0

			TILE_POS face;
			int d = tile->getDistance(other, &face);
			if (d >= 0 && d < f_diff) {
				f_diff = d;
				f_tile = other;
				f_face = face;
			}
		}
	};

	for (Tile *tile : g_pool) {
		if (tile->link_count >= TP_TOTAL)
			continue;

		f_tile = nullptr;
		f_diff = 0xFFFF;

		Tile::pushSeen();
		int n = tile->recursiveExecS(nullptr);
		f_find_closest(tile);
		Tile::popSeen();

		if (!f_tile)
			continue;

		LOG("Link " << PP(tile->original_pos) << " to " << PP(f_tile->original_pos)
			<< " :: diff=" << f_diff << ", face=" << (int)f_face
			<< ", len=" << n + 1);

		Tile *old_neighbour = tile->getNeighbour(f_face);
		if (!tile->link(f_tile, f_face)) {
			// Undo
			tile->link(old_neighbour, f_face);
			WARN(" ^ Link failed!");
			continue;
		}

		int moved = 1;

		// Statistics and debug
		// Refresh the image for each step
#if 1
	#if 1
		Unittest::updateImage(&img, true);
		getchar();
	#else
		int new_length = Unittest::updateImage(&img, true);
		if (new_length > last_length) {
			last_length = new_length;
			getchar();
		}
	#endif
#endif

		total_moved += moved;
		if (f_diff < min_diff)
			min_diff = f_diff;
	}

	LOG("Loop " << loop_n << ": diff=" << min_diff
		<< ", moved=" << total_moved);

	return min_diff;
}

int main(int argc, char **argv)
{
	//Unittest test;

	// difficult: 
	CLIArgStr ca_file("f", "images/simple.png");
	CLIArgS64 ca_xt("x", 4);
	CLIArgS64 ca_yt("y", 4);
	// 21 x 30
	CLIArg::parseArgs(argc, argv);

	LOG("Startup....");

	Image img(ca_file.get());
	img.read(v2u16(ca_xt.get(), ca_yt.get()));
	img.smoothen();
	LOG("Read image");

	int i = 0;
	int moved = 0;
	do {
		moved = closestMatchLoop(img);
		if (++i == 30) {
			i = 0;
			Unittest::updateImage(&img, true);
			getchar();
		}
	} while (moved > 0);

	Tile *center;
	Tile::sortAllUnsafe(center);
	img.plotTile(center);
	img.save("images/out.png");
	return 0;
}

