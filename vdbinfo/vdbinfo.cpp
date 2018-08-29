
#include <openvdb/openvdb.h>
#include <openvdb/tools/Interpolation.h>
#include <string>
#include <stdio.h>

//
//
//

static unsigned sTraceLevel;
struct Tracer {
	unsigned mLevel;
	std::string mStr;
	static void print(const char* fmt, ...) {
		printf("%s", std::string(sTraceLevel, ' ').c_str());
		va_list ap;
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
		printf("\n");
	}

	Tracer(const char* fmt, ...) : mLevel(sTraceLevel++) {
		char buf[512];
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);
		mStr = buf;
		printf("%s>> %s\n", std::string(mLevel, ' ').c_str(), mStr.c_str());
	}
	~Tracer() {
		printf("%s<< %s\n", std::string(mLevel, ' ').c_str(), mStr.c_str());
		--sTraceLevel;
	}
};

#define TRACEOBJ(fmt, ...)  Tracer  __tobj(fmt, ##__VA_ARGS__)
#define TRACE(fmt, ...)     do { Tracer::print(fmt, ##__VA_ARGS__); } while(0)

static void SHOW(const char* what, size_t n) {
	TRACE("%s : %lu", what, n);
}
static void SHOW(const char* what, const openvdb::Vec3R& c) {
	TRACE("%s : (%f, %f, %f)", what, c[0], c[1], c[2]);
}
static void SHOW(const char* what, const openvdb::Coord& c) {
	TRACE("%s : (%d, %d, %d)", what, c[0], c[1], c[2]);
}
static void SHOW(const char* what, const openvdb::CoordBBox& bb) {
	TRACEOBJ(what);
	SHOW("min", bb.min());
	SHOW("max", bb.max());
}

class VDBParser
{
public:
	VDBParser() {}
	~VDBParser() {}

	void parse(const char* filename) {
		openvdb::io::File f(filename);
		f.open();
		if (!f.isOpen())
			return;

		for (openvdb::io::File::NameIterator nameItr = f.beginName(), nameEnd = f.endName();
			 nameItr != nameEnd; ++nameItr) {
			readGid(f, *nameItr);
		}
		f.beginName();
	}
	void readMetadata(const openvdb::GridBase::Ptr& grid) {
		TRACEOBJ("metadata:");
		for (openvdb::GridBase::ConstMetaIterator metaItr = grid->beginMeta(), metaEnd = grid->endMeta();
		     metaItr != metaEnd; ++metaItr) {
			const openvdb::Name& mdName = metaItr->first;
			TRACE("'%s' : '%s'", mdName.c_str(), metaItr->second->str().c_str());
		}

	}
	bool readGid(openvdb::io::File& f, const openvdb::Name& name) {
		TRACEOBJ("grid: %s", name.c_str());
		openvdb::GridBase::Ptr grid = f.readGrid(name);
		if (grid->getGridClass() != openvdb::GRID_FOG_VOLUME)
			return true;

		readMetadata(grid);

		SHOW("activevox", grid->activeVoxelCount());
		SHOW("activedim", grid->evalActiveVoxelDim());
		SHOW("activebox", grid->evalActiveVoxelBoundingBox());
		SHOW("voxelsize", grid->voxelSize());
#if 1
		openvdb::FloatGrid::Ptr fGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(grid);

		using GridType = openvdb::FloatGrid;
		using TreeType = GridType::TreeType;
		using RootType = TreeType::RootNodeType;   // level 3 RootNode
		assert(RootType::LEVEL == 3);
		using Int1Type = RootType::ChildNodeType;  // level 2 InternalNode
		using Int2Type = Int1Type::ChildNodeType;  // level 1 InternalNode
		using LeafType = TreeType::LeafNodeType;   // level 0 LeafNode

		SHOW("leafCount", fGrid->tree().leafCount());
		SHOW("activeLeafVoxelCount", fGrid->tree().activeLeafVoxelCount());
		SHOW("leafDimensons", fGrid->tree().cbeginLeaf()->getNodeBoundingBox().dim());
        SHOW("LeafType::DIM", LeafType::DIM);
        SHOW("LeafType::NUM_VOXELS", LeafType::NUM_VOXELS);
		size_t leafIter = 0, onVoxels = 0, offVoxels = 0;
		for (auto cLeafItr = fGrid->tree().cbeginLeaf(); cLeafItr; ++cLeafItr) {
			++leafIter;
			onVoxels += cLeafItr->onVoxelCount();
			offVoxels += cLeafItr->offVoxelCount();
		}
        size_t totalPerLeaf = (onVoxels + offVoxels) / leafIter;
		SHOW("leafIter", leafIter);
		SHOW("onVoxels", onVoxels);
		SHOW("offVoxels", offVoxels);
		SHOW("totalVoxels", onVoxels + offVoxels);
		SHOW("totalPerLeaf", totalPerLeaf);

		SHOW("cube total", pow(onVoxels + offVoxels, 1.0/3.0));
		SHOW("cube / leaf total", pow((onVoxels + offVoxels) / fGrid->tree().leafCount(), 1.0/3.0));

        for (auto cLeafItr = fGrid->tree().cbeginLeaf(); cLeafItr; ++cLeafItr) {
            assert(totalPerLeaf == cLeafItr->onVoxelCount() + cLeafItr->offVoxelCount());
            assert(totalPerLeaf == cLeafItr->SIZE);
        }
        
return true;

		const openvdb::Vec3R ijk(0, 0, 0);
		GridType::ConstAccessor accessor = fGrid->getConstAccessor();
		GridType::ValueType v0 = openvdb::tools::PointSampler::sample(accessor, ijk);
		GridType::ValueType v1 = openvdb::tools::BoxSampler::sample(accessor, ijk);
		GridType::ValueType v2 = openvdb::tools::QuadraticSampler::sample(accessor, ijk);

		for (auto iter = fGrid->tree().beginNode(); iter; ++iter) {
		}

		size_t counts[RootType::LEVEL+1] = { 0,0,0,0 };
		size_t iterations = 0;
		size_t tVoxels = 0;
		for (auto iter = fGrid->tree().beginNode(); iter; ++iter) {
			//TRACE("depth %u", iter.getDepth());
		    if (0)
		    switch (iter.getDepth()) {
		    case 0: { RootType* node = nullptr; iter.getNode(node); TRACE("%p", node); break; }
		    case 1: { Int1Type* node = nullptr; iter.getNode(node); TRACE("%p", node); break; }
		    case 2: { Int2Type* node = nullptr; iter.getNode(node); TRACE("%p", node); break; }
		    case 3: { LeafType* node = nullptr; iter.getNode(node); TRACE("%p", node); break; }
		    }
		    ++counts[iter.getDepth()];
		    ++iterations;
		    if (iter.getDepth() == 3) {
			SHOW("     depth", iter.getDepth());
			SHOW("     coord", iter.getCoord());
			//SHOW("      bbox", iter.getBoundingBox());
			openvdb::CoordBBox bbox;
			SHOW("      size", iter.getBoundingBox().dim());
			LeafType* node;
			iter.getNode(node);
			assert(node);
			size_t voxels = 0;
			for (auto vIter = node->cbeginValueOn(); vIter; ++vIter) {
				SHOW("       pos", vIter.pos());
				SHOW("    offset", node->coordToOffset(vIter.getCoord()));
				SHOW("     coord", vIter.getCoord());
				SHOW("     value", vIter.getValue());
				assert(vIter.pos() == node->coordToOffset(vIter.getCoord()));
				++voxels;
			}
			TRACE("voxels  %lu", voxels);
			tVoxels += voxels;
			}
			
			static_assert(decltype(iter)::LEAF_DEPTH == 3, "BAD LEAF DEPTH");

		}
		TRACE("iterations  %lu", iterations);
		TRACE("tVoxels  %lu", tVoxels);
		for (unsigned i = 0; i <= RootType::LEVEL; ++i)
			TRACE("depth %u %lu", i, counts[i]);
		TRACE("total  %lu", counts[0] + counts[1] + counts[2] + counts[3]);

		iterations = 0;
		::memset(counts, 0, sizeof(counts));
		for (auto iter = fGrid->cbeginValueOn(); iter; ++iter) {
			SHOW("     depth", iter.getDepth());
			SHOW("leaf depth", iter.getLeafDepth());
			SHOW("     coord", iter.getCoord());
			assert(iter.getLeafDepth() == 3);
			++iterations;
		    ++counts[iter.getDepth()];
		}
		TRACE("iterations  %lu", iterations);
		for (unsigned i = 0; i <= RootType::LEVEL; ++i)
			TRACE("depth %u %lu", i, counts[i]);
		TRACE("total  %lu", counts[0] + counts[1] + counts[2] + counts[3]);


		{
		openvdb::FloatGrid::Accessor acc(fGrid->getAccessor());
		openvdb::Coord ijk(-4,-4,-4);
		TRACEOBJ("Coord: %d, %d, %d", ijk[0], ijk[1], ijk[2]);
		SHOW("value", acc.getValue(ijk));
		SHOW("depth", acc.getValueDepth(ijk));
		}
		{
			openvdb::Vec3R ijk(0.5,0.5,0.5);
			ijk = fGrid->transform().worldToIndex(ijk);
			GridType::ValueType v0 = openvdb::tools::PointSampler::sample(fGrid->tree(), ijk);
			TRACEOBJ("FractCoord: %f, %f, %f", ijk[0], ijk[1], ijk[2]);
			SHOW("value", v0);

		}
#endif
		return true;
	}
};

int main(int argc, char const **argv)
{
try {
	openvdb::initialize();

	TRACEOBJ("VDBINFO");
	VDBParser vdbs;
	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			vdbs.parse(argv[i]);
		}
	} else
		vdbs.parse("sphere.vdb");
} catch (std::exception& e) {
	TRACE("ERROR: %s", e.what());
}
	openvdb::uninitialize();
	return 0;
}
