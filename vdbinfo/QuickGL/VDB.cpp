

#include <openvdb/openvdb.h>
#include <OpenGL/gl.h>

struct VDBGrid
{
  openvdb::GridBase::Ptr grid;
  std::string file;
};

typedef std::vector< VDBGrid* > VDBGrids;
static VDBGrids sVDBGrids;

extern "C" void loadVDB( const char *filename )
{
  static struct VDBInit {
    VDBInit() { openvdb::initialize(); }
    ~VDBInit() { openvdb::uninitialize(); }
  } sVDBInit;

  std::vector< VDBGrid* > &grids = sVDBGrids;

  openvdb::io::File file(filename);
  file.open();
  for ( openvdb::io::File::NameIterator nameIter = file.beginName(); nameIter != file.endName(); ++nameIter)
  {
    grids.push_back( new VDBGrid );
    grids.back()->grid = file.readGrid(nameIter.gridName());
    grids.back()->file = filename;
    #if 0
    {
        SurfaceGeo    srf(grids.back()->buffer, grids.back()->grid->getGridClass() == openvdb::GRID_LEVEL_SET ? 0.0 : 0.01);
        processTypedScalarGrid(grids.back()->grid, srf);
    }
    #endif
    #if 0
    {
        ActiveVoxelGeo    srf(grids.back()->buffer);
        processTypedScalarGrid(grids.back()->grid, srf);
    }
    #endif
    #if 0
    {
        ActiveScalarValuesOp srf(grids.back()->buffer, &grids.back()->buffer);
        processTypedScalarGrid(grids.back()->grid, srf);
    }
    #endif
    #if 0
    {
        ActiveVectorValuesOp srf(grids.back()->buffer);
        processTypedVectorGrid(grids.back()->grid, srf);
    }
    #endif
      //grids.push_back( new VDBPrimitive(file.readGrid(nameIter.gridName()), new BufferObject) );
  }
  file.close();

assert(!sVDBGrids.empty());

auto voxDim = grids.back()->grid->evalActiveVoxelDim();
printf("voxDim: %d, %d, %d\n", voxDim[0], voxDim[1], voxDim[2]);
auto xform = sVDBGrids[0]->grid->transform();
printf("xform: linear: %d, type: %s\n", xform.isLinear(), xform.mapType().c_str());
    
auto bbox = sVDBGrids.back()->grid->evalActiveVoxelBoundingBox();
printf("box:\n"
       "  %d, %d, %d\n"
       "  %d, %d, %d\n",
       bbox.min()[0], bbox.min()[1], bbox.min()[2],
       bbox.max()[0], bbox.max()[1], bbox.max()[2]);
}


static void drawVDBCube( openvdb::GridBase::Ptr& grid ) {
    auto xform = grid->transform();
    openvdb::math::Mat4d mat4 = xform.baseMap()->getAffineMap()->getMat4();
/*
    //auto mat4 = xform.getBaseMap()->getAffineMap()->getMat4();
    // xform.map<openvdb::math::AffineMap>()->getMat4();
    // openvdb::math::Mat4d mat4 = openvdb::math::Mat4d::identity();
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            sModelMatrix[r][c] = mat4[r][c];

    sModelMatrix = koala::Matrix4::kIdentity;
    UseProgram(Programs.constant, sModelMatrix);
*/

    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    glBegin(GL_QUADS);
        openvdb::CoordBBox bbox = grid->evalActiveVoxelBoundingBox();
        openvdb::Coord Pc[8];
        bbox.getCornerPoints(Pc);
        // for (int i = 0; i < 8; ++i) printf("coords[%d]: %d, %d, %d\n", i, Pc[i][0], Pc[i][1], Pc[i][2]);
        /*
coords[0]: 1, -9, -10
coords[1]: 1, -9, 10
coords[2]: 1, 9, -10
coords[3]: 1, 9, 10
coords[4]: 19, -9, -10
coords[5]: 19, -9, 10
coords[6]: 19, 9, -10
coords[7]: 19, 9, 10
*/
#if 1
        openvdb::Vec3d Pd[8];
        for (int i = 0; i < 8; ++i) {
            Pd[i] = grid->indexToWorld(Pc[i]);
            //printf("coords[%d]: %d, %d, %d\n", i, Pc[i][0], Pc[i][1], Pc[i][2]);
            //printf(" world[%d]: %f, %f, %f\n", i, Pd[i][0], Pd[i][1], Pd[i][2]);
        }
    #define glVertex3V(i) glVertex3dv(&Pd[i][0])
#else
    #define glVertex3V(i) glVertex3iv(Pc[i].data())
#endif

        glVertex3V(0);
        glVertex3V(1);
        glVertex3V(3);
        glVertex3V(2);

        glVertex3V(4);
        glVertex3V(5);
        glVertex3V(7);
        glVertex3V(6);

        glVertex3V(0);
        glVertex3V(1);
        glVertex3V(5);
        glVertex3V(4);

        glVertex3V(3);
        glVertex3V(2);
        glVertex3V(6);
        glVertex3V(7);

        // Near & Far
        glVertex3V(0);
        glVertex3V(2);
        glVertex3V(6);
        glVertex3V(4);

        glVertex3V(1);
        glVertex3V(3);
        glVertex3V(7);
        glVertex3V(5);

    glEnd();
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
}

extern "C" void drawVDBCube() {
    int i = 0;
    for (auto& vdb: sVDBGrids) {
        glColor4f(i % 3 == 0 ? 1 : 0,
                  i % 3 == 1 ? 1 : 0,
                  i % 3 == 2 ? 1 : 0,
                  1);
        drawVDBCube(vdb->grid);
        ++i;
    }
}
