# Introduction #

Below are some examples of how to use Alembic.  They are mostly taken from the various tests in lib/Alembic/**Foo**/Tests/.

## Setting and Getting Data ##

```
//-*****************************************************************************
//-*****************************************************************************
// EXAMPLE1 - INTRODUCTION
//
// Hello Alembic User! This is the first Example Usage file, and so we'll
// start by targeting the thing you'd most often want to do - write and read
// animated, geometric primitives. To do this, we will be using two main
// libraries: Alembic::Abc, which provides the basic Alembic Abstractions,
// and Alembic::AbcGeom, which implements specific Geometric primitives
// on top of Alembic::Abc.
//-*****************************************************************************
//-*****************************************************************************

//-*****************************************************************************
//-*****************************************************************************
// INCLUDES
//
// Each Library includes the entirety of its public self in a file named "All.h"
// file. So, you can typically just do include lines like the following.
//-*****************************************************************************
//-*****************************************************************************

// Alembic Includes
#include <Alembic/AbcGeom/All.h>
// This is required to tell Alembic which implementation to use.  In this case,
// the HDF5 implementation, currently the only one available.
#include <Alembic/AbcCoreHDF5/All.h>

// Other includes
#include <iostream>
#include <assert.h>

// We include some global mesh data to test with from an external source
// to keep this example code clean.
#include <Alembic/AbcGeom/Tests/MeshData.h>

//-*****************************************************************************
//-*****************************************************************************
// NAMESPACES
//
// Each library has a namespace which is the same as the name of the library.
// We shorten those here for brevity.
//-*****************************************************************************
//-*****************************************************************************

using namespace std;
using namespace Alembic::AbcGeom; // Contains Abc, AbcCoreAbstract

//-*****************************************************************************
//-*****************************************************************************
// WRITING OUT AN ANIMATED MESH
//
// Here we'll create an "Archive", which is Alembic's term for the actual
// file on disk containing all of the scene geometry. The Archive will contain
// a single animated Transform with a single static PolyMesh as its child.
//-*****************************************************************************
//-*****************************************************************************
void Example1_MeshOut()
{
    // Create an OArchive.
    // Like std::iostreams, we have a completely separate-but-parallel class
    // hierarchy for output and for input (OArchive, IArchive, and so on). This
    // maintains the important abstraction that Alembic is for storage,
    // representation, and archival (as opposed to being a dynamic scene
    // manipulation framework).
    OArchive archive(

        // The hard link to the implementation.
        Alembic::AbcCoreHDF5::WriteArchive(),

        // The file name.
        // Because we're an OArchive, this is creating (or clobbering)
        // the archive with this filename.
        "polyMesh1.abc" );

    // Create a PolyMesh class.
    // An OPolyMesh is-an OObject that has-an OPolyMeshSchema
    // An OPolyMeshSchema is-an OCompoundProperty
    OPolyMesh meshyObj( OObject( archive, kTop ), "meshy" );
    OPolyMeshSchema &mesh = meshyObj.getSchema();

    // UVs and Normals use GeomParams, which can be written or read
    // as indexed or not, as you'd like.

    // The typed GeomParams have an inner Sample class that is used
    // for setting and getting data.
    OV2fGeomParam::Sample uvsamp( V2fArraySample( (const V2f *)g_uvs,
                                                  g_numUVs ),
                                  kFacevaryingScope );
    // indexed normals
    ON3fGeomParam::Sample nsamp( N3fArraySample( (const N3f *)g_normals,
                                                 g_numNormals ),
                                 kFacevaryingScope );

    // Set a mesh sample.
    // We're creating the sample inline here,
    // but we could create a static sample and leave it around,
    // only modifying the parts that have changed.

    // Alembic is strongly typed. P3fArraySample is for an array
    // of 32-bit points, which are the mesh vertices. g_verts etc.
    // are defined in MeshData.cpp.
    OPolyMeshSchema::Sample mesh_samp(
        P3fArraySample( ( const V3f * )g_verts, g_numVerts ),
        Int32ArraySample( g_indices, g_numIndices ),
        Int32ArraySample( g_counts, g_numCounts ),
        uvsamp, nsamp );

    // not actually the right data; just making it up
    Box3d cbox;
    cbox.extendBy( V3d( 1.0, -1.0, 0.0 ) );
    cbox.extendBy( V3d( -1.0, 1.0, 3.0 ) );
    mesh_samp.setChildBounds( cbox );

    // Set the sample twice.
    // Because the data is the same in both samples, Alembic will
    // store only one copy, but note that two samples have been set.
    mesh.set( mesh_samp );
    mesh.set( mesh_samp );


    // Alembic objects close themselves automatically when they go out
    // of scope. So - we don't have to do anything to finish
    // them off!
    std::cout << "Writing: " << archive.getName() << std::endl;
}

//-*****************************************************************************
void Example1_MeshIn()
{
    IArchive archive( Alembic::AbcCoreHDF5::ReadArchive(), "polyMesh1.abc" );
    std::cout << "Reading: " << archive.getName() << std::endl;

    IPolyMesh meshyObj( IObject( archive, kTop ), "meshy" );
    IPolyMeshSchema &mesh = meshyObj.getSchema();
    IN3fGeomParam N = mesh.getNormalsParam();
    IV2fGeomParam uv = mesh.getUVsParam();

    // N and uv are GeomParams, which can be stored as indexed or not.
    assert( ! N.isIndexed() );
    assert( ! uv.isIndexed() );

    IPolyMeshSchema::Sample mesh_samp;
    mesh.get( mesh_samp );

    assert( mesh_samp.getSelfBounds().min == V3d( -1.0, -1.0, -1.0 ) );

    assert( mesh_samp.getSelfBounds().max == V3d( 1.0, 1.0, 1.0 ) );

    // "ArbGeomParams" are a container that should only contain
    // GeomParams.
    ICompoundProperty arbattrs = mesh.getArbGeomParams();

    // We didn't set any on write, so on read, it should be an invalid container
    // Invalid Properties of any type evaluate as boolean False; valid 
    // Properties, even if they are empty, will evaluate as True.
    assert( ! arbattrs );

    // getExpandedValue() takes an optional ISampleSelector, and returns
    // an IGeomParam::Sample.  IGeomParam::Sample::getVals() returns an 
    // Abc::TypedArraySamplePtr
    N3fArraySamplePtr nsp = N.getExpandedValue().getVals();

    // GeomParams and Properties have a method, isConstant(), that returns
    // if there are fewer than two unique samples in them.
    assert( N.isConstant() );
    assert( uv.isConstant() );

    // An N3f instance is a typedef from an Imath::Vec3f, and is interpreted
    // by Alembic as a normal.
    N3f n0 = (*nsp)[0];

    // TypedArraySample::size() returns the number of elements.
    // In this case, there are g_numNormals elements.
    for ( size_t i = 0 ; i < nsp->size() ; ++i )
    {
        std::cout << i << "th normal: " << (*nsp)[i] << std::endl;
    }

    assert( n0 == N3f( -1.0f, 0.0f, 0.0f ) );

    IV2fGeomParam::Sample uvsamp = uv.getIndexedValue();

    assert( (*(uvsamp.getIndices()))[1] == 1 );
    V2f uv2 = (*(uvsamp.getVals()))[2];
    assert( uv2 == V2f( 1.0f, 1.0f ) );
    std::cout << "2th UV: " << uv2 << std::endl;

    std::cout << "Mesh num vertices: "
              << mesh_samp.getPositions()->size() << std::endl;

    std::cout << "0th vertex from the mesh sample with get method: "
              << mesh_samp.getPositions()->get()[0] << std::endl;

    std::cout << "0th vertex from the mesh sample with operator*(): "
              << (*(mesh_samp.getPositions()))[0] << std::endl;

}
```

## Accumulating a Transform ##

In this example, given a node in an Alembic Archive, we'll figure out what the final xform is.  We proceed from the leaf to the root.  This code is in examples/bin/AbcEcho/AbcBoundsEcho.cpp.

```
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>

#include <ImathBoxAlgo.h>

#include <iostream>

//-*****************************************************************************
using namespace ::Alembic::AbcGeom;

static Box3d g_bounds;

//-*****************************************************************************
void accumXform( M44d &xf, IObject obj )
{
    if ( IXform::matches( obj.getHeader() ) )
    {
        IXform x( obj, kWrapExisting );
        XformSample xs;
        x.getSchema().get( xs );
        // AbcGeom::XformSample::getMatrix() will compute an Imath::M44d that
        // represents the condensed xform for this AbcGeom::XformSample.
        xf *= xs.getMatrix();
    }
}

//-*****************************************************************************
M44d getFinalMatrix( IObject &iObj )
{
    M44d xf;
    xf.makeIdentity();

    IObject parent = iObj.getParent();

    // Once the Archive's Top Object is reached, IObject::getParent() will
    // return an invalid IObject, and that will evaluate to False.
    while ( parent )
    {
        accumXform( xf, parent );
        parent = parent.getParent();
    }

    return xf;
}

//-*****************************************************************************
Box3d getBounds( IObject iObj )
{
    Box3d bnds;
    bnds.makeEmpty();

    M44d xf = getFinalMatrix( iObj );

    if ( IPolyMesh::matches( iObj.getMetaData() ) )
    {
        IPolyMesh mesh( iObj, kWrapExisting );
        IPolyMeshSchema ms = mesh.getSchema();
        V3fArraySamplePtr positions = ms.getValue().getPositions();
        size_t numPoints = positions->size();

        // we're computing the bounds here every time, but could retrieve them
        // from the meshes by getting the SelfBounds Property.
        for ( size_t i = 0 ; i < numPoints ; ++i )
        {
            bnds.extendBy( (*positions)[i] );
        }
    }
    else if ( ISubD::matches( iObj.getMetaData() ) )
    {
        ISubD mesh( iObj, kWrapExisting );
        ISubDSchema ms = mesh.getSchema();
        V3fArraySamplePtr positions = ms.getValue().getPositions();
        size_t numPoints = positions->size();

        for ( size_t i = 0 ; i < numPoints ; ++i )
        {
            bnds.extendBy( (*positions)[i] );
        }
    }

    bnds.extendBy( Imath::transform( bnds, xf ) );

    g_bounds.extendBy( bnds );

    return bnds;
}

//-*****************************************************************************
void visitObject( IObject iObj )
{
    std::string path = iObj.getFullName();

    const MetaData &md = iObj.getMetaData();

    // only bother with meshes
    if ( IPolyMeshSchema::matches( md ) || ISubDSchema::matches( md ) )
    {
        Box3d bnds = getBounds( iObj );
        std::cout << path << " " << bnds.min << " " << bnds.max << std::endl;
    }

    // now the child objects
    for ( size_t i = 0 ; i < iObj.getNumChildren() ; i++ )
    {
        visitObject( IObject( iObj, iObj.getChildHeader( i ).getName() ) );
    }
}

//-*****************************************************************************
//-*****************************************************************************
// DO IT.
//-*****************************************************************************
//-*****************************************************************************
int main( int argc, char *argv[] )
{
    if ( argc != 2 )
    {
        std::cerr << "USAGE: " << argv[0] << " <AlembicArchive.abc>"
                  << std::endl;
        exit( -1 );
    }

    // Scoped.
    g_bounds.makeEmpty();
    {
        IArchive archive( Alembic::AbcCoreHDF5::ReadArchive(),
                          argv[1], ErrorHandler::kQuietNoopPolicy );
        visitObject( archive.getTop() );
    }

    std::cout << "/" << " " << g_bounds.min << " " << g_bounds.max << std::endl;

    return 0;
}
```

## Finding a Mesh By Name ##

In this example, we'll crawl through an IArchive, and find a mesh by a particular name.

This example is adapted from the first and second examples.

```
void visitObject( IObject iObj, const std::string &iName )
{
    const MetaData &md = iObj.getMetaData();

    // only bother with meshes
    if ( IPolyMeshSchema::matches( md ) || ISubDSchema::matches( md ) )
    {
        if ( iObj.getName() == iName )
        {
             std::cout << "Found it!  " << iObj.getFullName()
                       << std::endl;
             return;
        }
    }

    // now the child objects
    for ( size_t i = 0 ; i < iObj.getNumChildren() ; i++ )
    {
        visitObject( IObject( iObj, iObj.getChildHeader( i ).getName() ),
                     iName );
    }
}

void findMeshByName( const std::string &iName )
{
    IArchive archive( Alembic::AbcCoreHDF5::ReadArchive(), "wheresMeshy.abc" );

    visitObject( archive.getTop(), iName );
}
```

## Write a Set of Particles ##

This example will write out a set of particles; it's taken from lib/Alembic/AbcGeom/Tests/PointsTest.cpp.

```
void RunAndWriteParticles
(
    OObject &iParent,
    const ParticleSystem::Parameters &iParams,
    size_t iNumFrames,
    chrono_t iFps
)
{
    // Make the particle system.
    ParticleSystem parts( iParams );

    // Create the time sampling.
    TimeSampling ts(iFps, 0.0);
    uint32_t tsidx = iParent.getArchive().addTimeSampling(ts);

    // Create our object.
    OPoints partsOut( iParent, "simpleParticles", tsidx );
    std::cout << "Created Simple Particles" << std::endl;

    // Add attributes
    OPointsSchema &pSchema = partsOut.getSchema();
    MetaData mdata;
    SetGeometryScope( mdata, kVaryingScope );
    OV3fArrayProperty velOut( pSchema, "velocity", mdata, tsidx );
    OC3fArrayProperty rgbOut( pSchema, "Cs", tsidx );
    OFloatArrayProperty ageOut( pSchema, "age", tsidx );

    // Get seconds per frame.
    chrono_t iSpf = 1.0 / iFps;

    // Loop over the frames.
    for ( index_t sampIndex = 0;
          sampIndex < ( index_t )iNumFrames; ++sampIndex )
    {
        chrono_t sampTime = (( chrono_t )sampIndex) * iSpf;

        // First, write the sample.
        OPointsSchema::Sample psamp(
            P3fArraySample( parts.positionVec() ),
            UInt64ArraySample( parts.idVec() ) );
        pSchema.set( psamp );
        velOut.set( V3fArraySample( parts.velocityVec() ) );
        rgbOut.set( C3fArraySample( parts.colorVec() ) );
        ageOut.set( FloatArraySample( parts.ageVec() ) );

        // Now time step.
        parts.timeStep( iSpf );

        // Print!
        std::cout << "Wrote " << parts.numParticles()
                  << " particles to frame: " << sampIndex << std::endl;
    }

    // End it.
    std::cout << "Finished Sim, About to finish writing" << std::endl;
}
```

## Write Non-Standard Data for a PolyMesh ##

In this example, we'll store some non-standard data in a PolyMesh.  This would only be supported by custom workflows.

```
void crazyMeshOut()
{
    OArchive archive( Alembic::AbcCoreHDF5::WriteArchive(),
                      "crazyPolyMesh1.abc" );

    OPolyMesh meshyObj( OObject( archive, kTop ), "crazy" );
    OPolyMeshSchema &mesh = meshyObj.getSchema();

    // we need an OCompoundProperty to use as the parent of our user
    // Property
    OCompoundProperty myCrazyDataContainer = mesh.getUserProperties();

    ODoubleProperty mass( myCrazyDataContainer, "mass" );

    OPolyMeshSchema::Sample mesh_samp(
        P3fArraySample( ( const V3f * )g_verts, g_numVerts ),
        Int32ArraySample( g_indices, g_numIndices ),
        Int32ArraySample( g_counts, g_numCounts ) );

    mesh.set( mesh_samp );

    // this mesh weighs 2.0 mass units.
    mass.set( 2.0 );
}
```