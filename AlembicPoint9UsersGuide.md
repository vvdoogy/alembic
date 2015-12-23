# The Purpose of This Document #
This document is intended to serve as a brief introduction and a programmer's guide to using Alembic v0.9.  This page is not complete documentation, but should give the reader enough understanding of the fundamentals of the public-facing APIs to begin to read and understand the code and its comments.  The code samples provided below should compile and run with no alteration (other than removing the illustrative line numbers).

# The Purpose of Alembic #

Alembic is an open computer graphics interchange framework, written in collaboration between Sony Picture Imageworks and Industrial Light & Magic.  The 0.9 release contains a library that implements an extensible, data representation scheme and some tools and example code.

Alembic distills complex, animated, scenes into non-procedural, application-independent, baked geometric results. This distillation of scenes into baked geometry is analogous to the distillation of lighting and rendering scenes into rendered image data.

Alembic is focused on efficiently storing the computed results of complex procedural geometric constructions. It is specifically NOT concerned with storing the complex dependency graph of procedural tools used to create the computed results. For example, Alembic will efficiently store the animated vertex positions and animated transforms that result from an arbitrarily complex animation and simulation process, but will not attempt to store a representation of the network of computations (rigs, basically) which were required to produce the final, animated vertex positions and animated transforms.


# Layers of the Library #

Alembic is composed of several different libraries which present increasingly high-level APIs either to other layers of the library or the user.  The lowest level library is an abstract interface layer, **AbcCoreAbstract**, and is implemented concretely by **AbcCoreHDF5**, implements the calls to the HDF5 library, and is used for storing and managing Alembic data.  The AbcCoreHDF5 layer is most properly considered an opaque implementation of the CoreAbstract layer, and should never be called directly, with one minor exception to be described later.  For now, it's best to think of AbcCoreAbstract as the lowest layer upon which all other user-facing layers are built.

**Abc** sits on top of AbcCoreAbstract and is intended to be easily understood by a non-beginner user.  In practice, AbcCoreAbstract would only be used by users who wish to implement their own data objects which are not part of the official Alembic release, such as an analog of the Abc layer with different types or ways of expressing data manipulation or object creation, yet is still compatible with the AbcCoreAbstract interface.

Both the AbcCoreAbstract layer and the Abc layer deal almost exclusively with data-management, in the form of [Objects](Objects.md) and [Properties](Properties.md), which are interpretation-free.

When you wish to begin dealing concretely with high-level, CG-specific datatypes, like poly meshes, subdivision meshes, transforms, etc., [\*AbcGeom\*](AlembicPoint9UsersGuide#AbcGeom.md) is the layer for that enables the storage and retrieval of geometric data in an intuitive way.  AbcGeom is implemented mainly atop Abc (and some simple AbcCoreAbstract types).

## Namespacing ##

To strike a balance between convenience of usage and making explicit the origins of classes, the Alembic libraries use namespace abbreviations in header files.  Specifically,

  * Alembic::AbcCoreAbstract::v1 is abbreviated in the source-code as "AbcA"
  * Alembic::Abc is abbreviated as "Abc"
  * and Alembic::AbcGeom is abbreviated as AbcG

AbcCoreHDF5 is not abbreviated by design. The AbcCoreHDF5 library is an almost entirely private, opaque implementation of the abstract, public AbcCoreAbstract API.  Other than "WriteArchive" and "ReadArchive" classes, users should not be using AlembicHDF5 classes.

# Major Concepts in Alembic #
An alembic [Archive](Archive.md) consists of multiple [Objects](Objects.md), which may or may not have [Properties](Properties.md), in a hierarchical parent/child tree that forms an acyclic directed graph.  Data are written as [Samples](Samples.md) at specific times, and are read back similarly.  Interpolation between these samples is not the function of Alembic and, if required, must be done by the plug-in that uses Alembic.  However, Alembic provides a rich interface for retrieving multiple Samples based on time through the TimeSampling class, defined in AbcCoreAbstract.  Using that, client code may get all the required data for whatever interpolation is desired.

## Properties ##
Data in Alembic is stored in **Samples**, which are then stored in **Properties**.

Properties come in three types: CompoundProperties, which do not contain data directly, but rather contain other Properties (including, potentially, other CompoundProperties), ScalarProperties which store simple scalar values of small and unchanging extent (fewer than 256 elements, i.e. a single integer, or three Imath::Vec3ds), and ArrayProperties, which store variable-length arrays of potentially "unlimited" extent at each Sample.

Examples of ScalarProperties:
  * The mass of a rigid body (extent=1, a single numerical value)
  * A color (for an RGB vector, extent=3, three floating point values)
  * A bounding box (extent=6, two three-vectors of floating point values)

Examples of Array Properties:
  * A list of the vertices of a polygonal mesh
  * A list of particles in a fluid simulation

The major semantic difference between scalar and array properties is that to say that a Property is Scalar is to say, "There is one single value per Sample for this Property".  Because of this, it is meaningless to talk about a Scalar Property changing topology (eg, mass will be one floating point value; it won't sometimes be two floating point values); array properties have no such restriction. Furthermore, array properties are cached and indexed in such a way that repeatedly storing identical samples will not store multiple copies of the data.  Scalar properties do not cache in this way.

### Compound Properties ###
Compound properties are a special type of property in that they are not properties _per se_ but rather containers for other properties (compound properties included).  If an object has any properties, it **must** have a compound property which contains all other properties.

## Parent/Child Hierarchies ##
Because of its intention to store samples of scene graphs, Alembic has a notion of parent/child hierarchy.  At the apex of this hierarchy is the Archive, which can be treated as an Object.  Beneath this top-level Archive can be many child objects, each of which can be the parent to any number of other objects, and so on.  Ancestry is acyclic -- a descendant of an object cannot be the ancestor of that same object.

As mentioned above, If an object contains any properties, it contains them within a top-level compound property.  A compound property can have any number of child properties of any time (scalar, array or compound). Again, this relationship is acyclic.

## Time Sampling ##
As mentioned in the introduction, Alembic files consist of a series of samples of properties at different times.  Every property, therefore, has a notion of when it has been 'sampled' (both during write and during read).  There are four types of time sampling that Alembic supports:
  * Uniform - One sample per _dt_ where the _dt_ is defined during construction. This is probably the most common time sampling type; specifically properties that are sampled every 24th of a second.
  * Identity - Identity time sampling is the default, and is merely a special case of uniform sampling where for each sample, _i_, the time of that sample is _i_.
  * Cyclic - Some finite number of samples per _dt_; for example, shutter open and shutter close.
  * Acyclic - Sample times are arbitrary and follow no particular cycle. This is the least common case and will cause the most amount of data to be stored in the Alembic file (explicit values for each sample time must be stored, for every acyclic property).

All samples must be written in strictly increasing temporal order.

## Metadata ##
Objects and properties have metadata: a collection of string key/value pairs that allow users to specify information about the properties or objects that doesn't fit within the existing framework. For example, a property named "mass" on an object representing a rigid body might have metadata with the key/value pair of "units/kilograms", indicating that the data stored is stored in terms of kilograms.

Note that metadata, while useful and even necessary for data portability, does not (as of Alembic 0.9) have any standardized entries. For example, using the example above, one facility may express the mass units in metadata keyed on "units" while another may use the key "kg/lb"; both are valid, but neither are portable.

# AbcGeom #

## AbcGeom Fundamental Concepts ##

First off, a brief overview of the library.  Consonant with the overall structure of the Alembic system, it's a higher-level tier that sits atop base mechanisms in lower levels.  In this case, Geom is based on Abc, where the high-level geometric types like AbcGeom::PolyMesh are publicly derived from Abc::Schema, and Schema is derived from Abc::CompoundProperty (I'm eliding the 'I' and 'O' qualifiers here for the sake of clarity; when it's important to consider I and O sides distinctly, I'll be explicit).

Semantically, it makes sense for Schema to live in Abc, rather than in AbcGeom, for the following reason:

> The basic concept of a Schema - an expected and minimal set of expected properties grouped together to represent a behavior - is not a concept that is specific to Geometry, nor any other specific usage of Abc or the core libraries. It is simply that - a collection of properties, which may be dynamic (as in the case of SimpleXform) that encapsulate some semantic understanding of an object.  The SchemaObject is simply "an object containing a single, specifically named schema", and nothing more.  There is no reason why an object could not contain multiple schemas, such as a rigid body system containing a points schema as well as a secondary compound property named ".bodies".  Therefore, the notion of Schemas, as a general mechanism, resides in Abc - whereas Geometry-specific instances of Schema appropriately belong in AbcGeom. If, perhaps, you were to use Alembic to store information such as is used at Palantir [note: a startup that sells data analysis software](ed.md), you might have an AbcPalantir library with whatever schemas are appropriate.

Abc::Schema is actually a templated class, templated on a "schema info" struct.  The schema info struct is a very simple struct that has two methods: _title()_ which returns what in GTO was called "protocol", and _defaultName()_, which returns the default name of the CompoundProperty that the schema **is**.  Here's a concrete example for PolyMeshSchemaInfo:

```
struct PolyMeshSchemaInfo
{
    static const char * title() { return "AbcGeom_PolyMesh_v1"; }
    static const char * defaultName() { return ".geom"; }
};
```

There's a convenience macro to create these schema trait classes in AbcGeom/SchemaInfoDeclarations.h called "ALEMBIC\_ABCGEOM\_DECLARE\_SCHEMA\_INFO", that looks like this:

```
#define ALEMBIC_ABCNICE_DECLARE_SCHEMA_TRAITS( STITLE, SDFLT, STDEF )   \
struct STDEF                                                            \
{                                                                       \
    static const char * title() { return ( STITLE ) ; }                 \
    static const char * defaultName() { return ( SDFLT ); }             \
}
```

Preceding that macro definition is a comment that explains the high-level concept behind a Schema:

> With properties, specific flavors of properties are expressed via the TypedScalarProperty and the TypedArrayProperty. Compound Properties are more complex, and the specific flavors require a more complex treatment - That's what Schemas are. The CompoundProperty equivalent of a TypedArrayProperty or a TypedScalarProperty.

> A Schema is a collection of grouped properties which implement some complex object, such as a poly mesh. In the simplest, standard case, there will be a compound property at the top with a certain name, and inside the compound property will be some number of additional properties that implement the object. In the case of a poly mesh, these properties would include a list of vertices (a V3fArray), a list of indices (an IntArray), and a list of "per-face counts" (also an IntArray).

> In somewhat more complex cases, such as a TransformStack, the set of properties that are added may vary based on configuration information provided by the user.

> Because a Schema is to a CompoundProperty what a TypedArrayProperty or TypedScalarProperty is to a regular property, it is directly derived from CompoundProperty. However... Whereas TypedProperties can be instanced as typedefs, Schemas will invariably require additional functionality, and thus the AbcNice::Schema class is intended for use as a base class.

## Extending AbcGeom/adding new high-level types ##

The basic structure of a geometric type class in Geom is as follows, in a C++-esque pseudo-code, and with some boilerplate removed:

```
class GeometricThingySchema : public Abc::Schema<GeometricThingySchemaInfo>
{
public:
    class Sample
    {
    public:
        Sample() {}

        V3dArraySamplePtr getPositions() const { return m_positions; }

    protected:
        friend class GeometricThingySchema;
        V3dArraySamplePtr m_positions;
    };


    // a bunch of constructors elided

    // fill the provided Sample ref with the data
    void get( Sample &iSample,
              const SampleSelector iSS = SampleSelector() );

    // return a Sample object filled with the data
    Sample getValue( const SampleSelector &iSS = SampleSelector() );

protected:
    void init( Argument ); // this is important!
    V3dArrayProperty m_positions;
};

typedef Abc::SchemaObject<GeometricThingySchema> GeometricThingy;
```

There's quite a bit of code left out there, but hopefully you get the gist:

1) create a schema info struct for Schema's template argument;

2) publicly derive your new geometric type from Abc::Schema, with your schema info struct as the template argument;

3) define an inner class called "Sample" that is used for all data access (reading and writing);

4) `typedef Abc::SchemaObject<YourGeometricThingSchema> YourGeometricThing`.

That last comment, after void init(), says, "This is important!"  This is because that method is how the class sets itself up and establishes the names for the properties whose presence embodies the schema.  Here's the init() method from OPolyMesh.cpp:

```
void OPolyMeshSchema::init( const TimeSamplingType &iTst )
{
    ALEMBIC_ABC_SAFE_CALL_BEGIN( "OPolyMeshSchema::init()" );

    MetaData mdata;
    SetGeometryScope( mdata, kVertexScope );
    m_positions = OV3fArrayProperty( *this, "P", mdata, iTst );

    m_indices = OIntArrayProperty( *this, ".faceIndices", iTst );

    m_counts = OIntArrayProperty( *this, ".faceCounts", iTst );

    ALEMBIC_ABC_SAFE_CALL_END_RESET();
}
```

The thing to notice is that this is the place where we implement design policies like, "Mesh positions are named 'P'."  You could also infer that a valid PolyMesh has three mandatory properties with particular types: an array of V3fs for positions, and two integer arrays for face indices and face counts.  You could also have additional properties on a PolyMesh object, such as normals or UVs, but since they are not included in the base declaration there, they are not mandatory (and if you look at, eg,
http://code.google.com/p/alembic/source/browse/lib/Alembic/AbcGeom/Tests/PolyMeshTest.cpp you can see how you'd add them).

As previously stated, all the geometric types publicly inherit from Abc::Schema.  As alluded to, the final step in creating a "thing" rather than a "schema of a thing" is a typedef:

`typedef Abc::SchemaObject<GeometricThingySchema> GeometricThingy;`

and a SchemaObject **is** an Object that contains a single Schema.  If you look at lib/Alembic/Abc/OSchemaObject.h, you'll see the following code:

```
//-*****************************************************************************
//! An OSchemaObject is an object with a single schema. This is just
//! a convenience class, really, but it also deals with setting up and
//! validating metadata
template <class SCHEMA>
class OSchemaObject : public OObject
```

The SchemaObject class contains the Schema as a member, and as it says there, ensures that the MetaData is correct and appropriate.  It also provides the mechanism by which the default name for the CompoundProperty that is the Schema is created or read, by getting it from the schema info template argument that was passed to the Schema.  Again looking at the PolyMeshTest.cpp in AbcGeom/Tests, you can see that ".geom" is never mentioned in the client code, yet h5ls -r shows that our PolyMeshSchemaInfo was in effect:

```
$ AbcGeom/Tests> h5ls -r polyMesh1.abc
/                        Group
/ABC                     Group
/ABC/meshy               Group
/ABC/meshy/.prop         Group
/ABC/meshy/.prop/.geom   Group
/ABC/meshy/.prop/.geom/.faceCounts.smp0 Dataset {6}
/ABC/meshy/.prop/.geom/.faceIndices.smp0 Dataset {24}
/ABC/meshy/.prop/.geom/N.smp0 Dataset {24}
/ABC/meshy/.prop/.geom/P.smp0 Dataset {8}
/ABC/meshy/.prop/.geom/st.smp0 Dataset {24}
```

The code that wrote that file is incredibly concise yet comprehensible; here it is, stripped of comments:

```
void Example1_MeshOut()
{
    OArchive archive(
        Alembic::AbcCoreHDF5::WriteArchive(),
        "polyMesh1.abc" );

    OPolyMesh meshyObj( archive.getTop(), "meshy" );
    OPolyMeshSchema &mesh = meshyObj.getSchema();

    MetaData normsUvsMeta;
    SetGeometryScope( normsUvsMeta, kFacevaryingScope );
    OV3fArrayProperty Nprop( mesh, "N", normsUvsMeta );
    OV2fArrayProperty STprop( mesh, "st", normsUvsMeta );

    OPolyMeshSchema::Sample mesh_samp(
        V3fArraySample( ( const V3f * )g_verts, g_numVerts ),
        Int32ArraySample( g_indices, g_numIndices ),
        Int32ArraySample( g_counts, g_numCounts ) );

    mesh.set( mesh_samp );
    Nprop.set( V3fArraySample( ( const V3f * )g_normals, g_numNormals ) );
    STprop.set( V2fArraySample( ( const V2f * )g_uvs, g_numUVs ) );
}
```

If you had multiple samples, you'd just loop through creating new OPolyMeshSchema::Samples and calling "mesh.set( mesh\_samp );", and "set( ArraySample )" on the N and ST Properties.

One thing previously unmentioned is the GeometryScope.  This is a comprehensive implementation of "detail type" as thought of in the context of Renderman; check out http://code.google.com/p/alembic/source/browse/lib/Alembic/AbcGeom/GeometryScope.h .

# Examples #
In this section, we will create some simple programs which use the Alembic library to read and write data.  These examples will be done in two libraries: first using the lower-level Abc library, which is suitable for writing and reading samples of arbitrary objects with any number of properties.  The second set of examples will concentrate on the AbcGeom layer, which implements standards for reading and writing high-level, commonly used geometric data such as polygon meshes, transforms and subdivision surfaces.

## Examples in the Abc Layer ##

As mentioned above, the fundamental data object in Alembic is the **archive**.  An archive must be created before any objects can be created (just as an object must be created before any of its properties can be instantiated).  To create an Alembic archive for writing, we simply do:

```
1  int
2  main( int argc, char *argv[] )
3  {
4     std::string archiveName("myFirstArchive.abc");
5
6     // Create an archive with the default writer
7     OArchive archive( Alembic::AbcCoreHDF5::WriteArchive(),
8                       archiveName, 
9                       ErrorHandler::kThrowPolicy );
10
11    // The archive is closed (and written to disk) when 'archive'
12    //  goes out of scope.
13 }
```

Details to note: the default **WriteArchive()** object  (derived in AbcCoreHDF5/ReadWrite.h from an abstract class in AbcCoreAbstract/ArchiveWriter.h, and implemented in AbcCoreHDF5/AwImpl.cpp) is the object that implements the writing of the archive itself to the HDF5 file.  In theory, if other fundamental file formats (other than HDF5) are implemented, different WriteArchive() objects could be used in this constructor and the code would require no other changes.  The third argument in the archive constructor (line 9) indicates how you wish to handle any internal errors in Alembic. Here we have chosen to throw exceptions, although you can choose for Alembic to silently swallow the errors instead.

Once an archive for writing has been created, objects can be added to it.  Because Alembic is scene-graph-based, all objects must be children (or grandchildren, or descendants in general) of the archive. So a simple object would be added like so:

### Example 1 ###
```
1  int
2  main( int argc, char *argv[] )
3  {
4     std::string archiveName("myFirstArchive.abc");
5
6     // Create an archive with the default writer
7     OArchive archive( Alembic::AbcCoreHDF5::WriteArchive(),
8                       archiveName, 
9                       ErrorHandler::kThrowPolicy );
10
11    OObject archiveTop = archive.getTop();
12
13    std::string name = "myFirstChild";
14    OObject child( archiveTop, name );
15
16    // The archive is closed (and written to disk) when 'archive'
17    //  goes out of scope.
18 }
```

We see that on line 11, we grab the top Object of the archive -- every archive will have a top object -- and parent our new object **child** underneath is in its constructor (line 14).  Alembic uses the idiom of constructors to specify names, parent-child relationships, and other important aspects of objects and properties.  Other objects may be parented under the archive's top object, or under the new child object:

### Example 2 ###
```
1  int
2  main( int argc, char *argv[] )
3  {
4     std::string archiveName("myFirstArchive.abc");
5
6     // Create an archive with the default writer
7     OArchive archive( Alembic::AbcCoreHDF5::WriteArchive(),
8                       archiveName, 
9                       ErrorHandler::kThrowPolicy );
10
11    OObject archiveTop = archive.getTop();
12
13    std::string name = "myFirstChild";
14    OObject child( archiveTop, name );
15
16    name = "grandChild01";
17    OObject gchild1( child, name );
18
19    name = "grandChild02";
20    OObject gchild2( child, name );
21
22    name = "greatGrandChild";
23    OObject ggchild( gchild1, name );
24
25    // The archive is closed (and written to disk) when 'archive'
26    //  goes out of scope.
27 }
```

In the previous two examples, we have created archives with empty objects: objects that contain no properties.  Creating properties is similar to creating objects: they are named and parented under something. In the case of objects, they are parented under other objects. In the case of properties, they are parented under compound properties (mentioned above).  Building on **Example 1** above:

### Example 3 ###
```
1  int
2  main( int argc, char *argv[] )
3  {
4     std::string archiveName("myFirstArchive.abc");
5
6     // Create an archive with the default writer
7     OArchive archive( Alembic::AbcCoreHDF5::WriteArchive(),
8                       archiveName, 
9                       ErrorHandler::kThrowPolicy );
10
11    OObject archiveTop = archive.getTop();
12
13    std::string name = "myFirstChild";
14    OObject child( archiveTop, name );
15
16    OCompoundProperty childProps = child.getProperties();
17
18    // Create a TimeSampling object: one sample every 24th of a second
19    const chrono_t dt = 1.0 / 24.0;
20
21    TimeSamplingType tst( dt );               // uniform with cycle=dt
22
23    // Create a scalar property on this child object named 'mass'
24    ODoubleProperty mass( childProps,  // owner
25                          "mass", // name
26                           tst );
27
28    // The archive is closed (and written to disk) when 'archive'
29    //  goes out of scope.
30 }
```

Similarly to how we first create an archive and then access its top object (line 11) in order to add new objects, we first create an object and then access its top **OCompoundProperty** (line 16) before adding new properties. Also in parallel with the archive/object idiom, we construct a new property by specifying a parent (in this case, the top **OCompoundProperty**, on line 24) and a name (line 25).  However, because properties hold time sampled data, we also must specify a **TimeSamplingType** object on construction.

TimeSamplingType is a class defined in AbcCoreAbstract, that controls how properties in Alembic relate time values to their sample indices.  That is: all samples of a property (for example mass, or position) are stored as indexed (from 0) data within the Alembic file.  However, these samples occur in time and thus we must have conventional time values expressed in the abstract 'chrono\_t' type which, in this case, is assumed to be seconds.  A property's TimeSamplingType class is responsible for storing this correspondence.  As mentioned above, there are three time sampling modes or behaviors; by specifying a _delta t_ in the TimeSamplingType constructor we are indicating that we are performing uniform time sampling with one sample per 24th of a second (lines 19 and 21). More information on the various modes of TimeSamplingType are documented in the comments in TimeSampleType.h.

A property must be typed when it is constructed: in this case we are storing a single double-precision value as our samples, so we create an 'ODoubleProperty'.   Once we have created our typed property, we can begin to write samples to it:

### Example 4 ###
```
1  int
2  main( int argc, char *argv[] )
3  {
4     std::string archiveName("myFirstArchive.abc");
5
6     // Create an archive with the default writer
7     OArchive archive( Alembic::AbcCoreHDF5::WriteArchive(),
8                       archiveName, 
9                       ErrorHandler::kThrowPolicy );
10
11    OObject archiveTop = archive.getTop();
12
13    std::string name = "myFirstChild";
14    OObject child( archiveTop, name );
15
16    OCompoundProperty childProps = child.getProperties();
17
18    // Create a TimeSampling object: one sample every 24th of a second
19    const chrono_t dt = 1.0 / 24.0;
20
21    TimeSamplingType tst( dt );               // uniform with cycle=dt
22
23    // Create a scalar property on this child object named 'mass'
24    ODoubleProperty mass( childProps,  // owner
25                          "mass", // name
26                           tst );
27
28    // Write out the samples
29    const unsigned int numSamples = 5;
30    const chrono_t startTime = 10.0;
31    for (int tt=0; tt<numSamples; tt++)
32    {
33        double mm = (1.0 + 0.1*tt); // vary the mass
34        mass.set( mm,  OSampleSelector(tt, startTime + tt*dt ) );
35    }
36
37    // The archive is closed (and written to disk) when 'archive'
38    //  goes out of scope.
39 }
```

Here we are writing out 5 samples of our time-varying double precision floating point property at the times: 5.0, 5.041666, 5.083333 and so on (specified as the second argument of the **OSampleSelector** constructor on line 34).  It is important to note that samples of properties must be written in increasing time order; that is you cannot write a sample at t=5.0, t=1.0 and then t=6.0.  Doing so will cause an exception to be thrown in the above example code, because of the error policy specified on line 9.

In the above example, we are specifying different data at each of the five sample times of our property.  In general, however, you will have properties whose values do not vary at every sample and may be constant for long periods of time.  Alembic allows you, via a **setFromPrevious()** function call, to specify that a sample is identical to the previous one, so you won't have to reassemble the (redundant) sample data.  Both scalar and array properties support this functionality.

### Reading ###

So far, we have emphasized writing Alembic data via the **Abc** API.  Reading data via the **Abc** layer is effectively symmetric: the classes mentioned and used above all have an 'I' counterpart (e.g. **IObject** and **IArchive**).

### Example 5 ###
```
1  int
2  main( int argc, char *argv[] )
3  {
4     std::string archiveName("myFirstArchive.abc");
5
6     // Read an archive with the default reader
7     IArchive archive( Alembic::AbcCoreHDF5::ReadArchive(),
8                       archiveName, 
9                       ErrorHandler::kThrowPolicy );
10
11    IObject archiveTop = archive.getTop();
12
13    // The archive is closed when 'archive' goes out of scope.
14  }
```

As with the writing examples above, we first open an archive with a given name, this time using the default archive reader defined in **AbcCoreHDF5**.  This input archive is guaranteed to have a top object (fetched on line 11) from which the rest of the scene graph will descend.

To navigate the sampled scene, we determine the number of children and then iterate through them, like so:

### Example 6 ###
```
1  int
2  main( int argc, char *argv[] )
3  {
4     std::string archiveName("myFirstArchive.abc");
5
6     // Read an archive with the default reader
7     IArchive archive( Alembic::AbcCoreHDF5::ReadArchive(),
8                       archiveName, 
9                       ErrorHandler::kThrowPolicy );
10
11    IObject archiveTop = archive.getTop();
12    unsigned int numChildren = archiveTop.getNumChildren();
13
14    for (int ii=0; ii<numChildren; ++ii)
15    {
16       std::cout << "Child #" << ii << " is named "
17                 << archiveTop.getChildHeader(ii).getName() << std::endl;
18    }
19
20    // The archive is closed when 'archive' goes out of scope.
21  }
```

Line 17 exhibits a useful feature of Alembic: some of the data about an object is available through its header (defined in AbcCoreAbstract/ObjectHeader.h), which is compact and quick to load. This means that, for example, if you wish to find an object's name you don't need to (and shouldn't) load the entire object into memory.  an object's header contains the object's name and its Metadata (discussed above).  This should allow a user of the library to quickly inventory an archive and extract only the data she is interested in.

Once you've decided that you'd like to examine the properties of a child object, you can access them through the object's top compound property (just like with writing.)  The crucial difference here is that, in general, you won't know the data type of the property but must query that property's **ProertyHeader**:

### Example 7 ###
```
1  int
2  main( int argc, char *argv[] )
3  {
4     std::string archiveName("myFirstArchive.abc");
5
6     // Read an archive with the default reader
7     IArchive archive( Alembic::AbcCoreHDF5::ReadArchive(),
8                       archiveName, 
9                       ErrorHandler::kThrowPolicy );
10
11    IObject archiveTop = archive.getTop();
12    size_t numChildren = archiveTop.getNumChildren();
13
14    for (int ii=0; ii<numChildren; ++ii)
15    {
16       IObject child( archiveTop, archiveTop.getChildHeader( ii ).getName() );
17       ICompoundProperty props = child.getProperties();
18       size_t numProperties = props.getNumProperties();
19
20       for (int pp=0; pp<numProperties; pp++)
21       {
22          PropertyType pType = props.getPropertyHeader(pp).getPropertyType();
23          if (pType == kCompoundProperty)
24          {
25              std::cout << "compound" << std::endl;
26          }
27          else if (pType == kScalarProperty)
28          {
29              std::cout << "scalar" << std::endl;
30          }
31          else if (pType == kArrayProperty)
32          {
33              std::cout << "array" << std::endl;
34          }
35    }
36
37    // The archive is closed when 'archive' goes out of scope.
38  }
```

On line 22 above, we query the property header of property number _pp_ (within the top level compound property accessed on line 17) and retrieve the property type: whether this property is a scalar, array, or compound property.  If the property contains data samples (i.e. is a scalar or array property), information about the data type (the fundamental, "plain old data" type, as well as the 8-bit extent) of its samples can also be queried:

### Example 8 ###
```
1   AbcCoreAbstract::DataType dType = props.getPropertyHeader(pp).getDataType();
2   Util::PlainOldDataType podType = dType.getPod();
3   uint8_t dType.getExtent();
```

The DataType class, defined in AbcCoreAbstract, allows access to the data types (line 2, above) defined in the 'PlainOldDataType' enum in Util/PlainOldDataType.h.  The extent, defined in terms of 8-bit length of a sample property, can also be queried (line 3).  The extent and "plain old datatype" should be enough for a program using the library to branch and specialize the data retrieval from this property.  Assuming for the moment that we have a scalar property:

### Example 9 ###
```
1   switch (dType.getPod())
2   {
3   // Switch on data type:
4   // Boolean
5   case  kBooleanPOD:
6   {
7       IBoolProperty prop( props,  propNames[jj] );
8       // Do stuff with 'prop'
9       break;
10   }
11
12   // Char/UChar
13   case kUint8POD:
14   {
15       IUcharProperty prop( props,  propNames[jj] );
16       // Do stuff with 'prop'
17       break;
18   }
19   case kInt8POD:
20   {
21       ICharProperty prop( props,  propNames[jj] );
22       // Do stuff with 'prop'
23       break;
24   }
25    // Short/UShort
26   case kUint16POD:
27   {
28       IUshortProperty prop( props,  propNames[jj] );
29       // Do stuff with 'prop'
30       break;
31   }
32   case kInt16POD:
33   {
34       IShortProperty prop( props,  propNames[jj] );
35       // Do stuff with 'prop'
36       break;
37   }
38    // Int/UInt
39   case kUint32POD:
40   {
41       IUintProperty prop( props,  propNames[jj] );
42       // Do stuff with 'prop'
43       break;
44   }
45   case kInt32POD:
46   {
47       IIntProperty prop( props,  propNames[jj] );
48       // Do stuff with 'prop'
49       break;
50   }
51    // Long/ULong
52   case kUint64POD:
53   {
54       IUlongProperty prop( props,  propNames[jj] );
55       // Do stuff with 'prop'
56       break;
57   }
58   case kInt64POD:
59   {
60       ILongProperty prop( props,  propNames[jj] );
61       // Do stuff with 'prop'
62       break;
63   }
64   // Half/Float/Double
65   case kFloat16POD:
66       IHalfProperty prop ( props,  propNames[jj] ),
67       // Do stuff with 'prop'
68       break;
69   case kFloat32POD:
70   {
71       IFloatProperty prop( props,  propNames[jj] );
72       // Do stuff with 'prop'
73       break;
74   }
75   case kFloat64POD:
76   {
77       IDoubleProperty prop( props,  propNames[jj] );
78       // Do stuff with 'prop'
79       break;
80   }

    ... and so on ...

81  };
```

Given a typed property, such as **IDoubleProperty**, we can query its sampling attrubutes:
```
    size_t numSamples = property.getNumSamples();
```

and iterate through the samples to get the sample values:

```
    for (int ss=0; ss<numSamples; ss++)
    {
        ISampleSelector iss( (index_t) ss);
        property.getValue( iss );
    }
```

Scalar properties will return their samples by value (either as a singleton or as a std::vector of the data type), while array properties will return their samples as a boost shared pointer (Alembic will manage this memory).

## Examples in the AbcGeom Layer ##
AbcGeom is a high-level API intended to be used to easily store and
retreive samples of common geometric data types.  Currently, we have
restricted the objects in this library to the fundamental Renderman
geometric types: point clouds, polygonal meshes, subdivision surfaces,
and simple homogenous transforms.  (There are plans to include a camera
defintion in the 1.0 version of Alembic.)

Using the AbcGeom layer is conceptually very much like using the Abc
layer but, since you are dealing in well-defined and specified
higher-level types, is far less verbose.  For example, to write
a polygonal mesh using AbcGeom functions:

### Example 10 ###
```
1  int
2  main( int argc, char *argv[] )
3  {
4     std::string archiveName("polyMesh1.abc");
5     OArchive archive( Alembic::AbcCoreHDF5::WriteArchive(),
6                       archiveName );
8     OObject archiveTop = archive.getTop();
9
10    // Create a PolyMesh class.
11    OPolyMesh meshObj( archiveTop, "myMesh" );
12    OPolyMeshSchema &mesh = meshObj.getSchema();
13
14    // Set a mesh sample.
15    // We're creating the sample inline here,
16    // but we could create a static sample and leave it around,
17    // only modifying the parts that have changed.
18    OPolyMeshSchema::Sample mesh_samp(
19        V3fArraySample( ( const V3f * )g_verts, g_numVerts ),
20        Int32ArraySample( g_indices, g_numIndices ),
21        Int32ArraySample( g_counts, g_numCounts ) );
22
23    // Set the sample.
24    mesh.set( mesh_samp );
25
26    // The archive is closed (and written to disk) when 'archive'
27    //  goes out of scope.
28 }
```

Here, the variables g\_verts, g\_numVerts, g\_indices, g\_numIndices,
g\_counts, and g\_numCounts are global variables that are assumed
to be defined elsewhere.  (See AbcGeom/Tests/MeshData.cpp for an
example).

As illustrated above, the process by which you write a sample of an
AbcGeom type is to:
  1. Create the object
  1. Get its "schema" (i.e. its definition of what properties this object possesses)
  1. Fill out a sample of that schema
  1. Set the sample on the object's schema.

Although the sample selector usage is elided above, setting samples in AbcGeom is identical to how it is done in Abc. So, more generally:

```
    for (int iSample=0; iSample<10; iSample++)
    {
        ...
        mesh.set( mesh_samp );
    }
```

Note that currently, the first sample of a schema must have all its required properties filled out the first time the schema is sampled.  (Optional properties such as UVs, or properties that may no exist such as holes in subdivision surfaces do not need to be filled out on this first sample).

In Alembic, reading is analogous to writing, and is in fact separated into two mirror APIs, similar to the way [std::iostream](http://www.cplusplus.com/reference/iostream/iostream/) is separated.

Reading data from an instance of an AbcGeom type is through an inner Sample class for each geometry schema that provides convenient schema-specific methods for dealing with the schema-specific data.  Example 11 below illustrates this:

### Example 11 ###
```
1  void Example1_MeshIn()
2  {
3      IArchive archive( Alembic::AbcCoreHDF5::ReadArchive(), "polyMesh1.abc" );
4      std::cout << "Reading: " << archive.getName() << std::endl;
5
6      IPolyMesh meshyObj( IObject( archive, kTop ), "myMesh" );
7      IPolyMeshSchema &mesh = meshyObj.getSchema();
8      IV3fArrayProperty N( mesh, "N" );
9      IV2fArrayProperty st( mesh, "st" );
13
11     IPolyMeshSchema::Sample mesh_samp;
12     mesh.get( mesh_samp );
13
14     V3fArraySamplePtr normalsSamp;
15     N.get( normalsSamp );
16
17     V2fArraySamplePtr stSamp;
18     st.get( stSamp );

19     std::cout << "Mesh num vertices: "
20               << mesh_samp.getPositions()->size() << std::endl;
21
22     std::cout << "0th vertex from the mesh sample: "
23               << (*(mesh_samp.getPositions()))[0] << std::endl;
24 
25     std::cout << "0th vertex from the mesh sample with get method: "
26               << mesh_samp.getPositions()->get()[0] << std::endl;
27 }
```

Line 6 shows the construction of an IPolyMesh.  Remember that an IPolyMesh _is an_ IObject, and an IPolyMeshSchema _is an_ ICompoundProperty (and an IPolyMesh _has an_ IPolyMeshSchema).  So, just like any other Object in Alembic, it can be constructed with its parent Object and its name.  Every Object in Alembic is the child of some other Object, EXCEPT for the TopObject, which is the lone direct child of the Archive.  To get the TopObject, you can do one of two things:

1) call getTop() on the Archive, or

2) construct the TopObject by passing the Archive as the parent, and giving it the **kTop** enum instead of a std::string name.

Anyway, what line 6 is doing is constructing the Archive's TopObject inline and using that as the parent of the IObject named "myMesh".  We happen to know that myMesh is an IPolyMesh that lives under the Archive's TopObject, because the Archive in which it lives was created by the code from Example 10.

Lines 7, 8, and 9 show two important things:

1) That an IPolyMeshSchema is an ICompoundProperty (as mentioned above), shown by the fact that the _mesh_ object is used as the parent for the _N_ and _st_ Properties, and;

2) _N_ and _st_ are **optional** Properties, and not part of the Schema of a PolyMesh, so they must be read on their own, rather than through an IPolyMeshSchema::Sample object (lines 14 and 17 demonstrate using appropriately typed Samples from the Abc layer to get the data from those Properties).

Lines 23 and 26 show equivalent ways of getting data out of the AbcGeom::IPolyMeshSchema::Sample.  That object has a method, _getPositions()_, that returns a shared pointer to an Abc::V3fArraySample (the return type is a typedef'd boost::shared\_ptr, and is an Abc::V3fArraySamplePtr; see http://code.google.com/p/alembic/source/browse/lib/Alembic/Abc/TypedArraySample.h).  Shared pointers to `Abc::TypedArraySample`s have overloaded `operator*()`, which is the same as calling the get() method (that is, both methods return an indexable reference to the underlying array).

To get the data out of the _N_ and _st_ Samples, you'd do the same thing as in lines 23 and 26, but using the V3fArraySamplePtr and V2fArraySamplePtr from lines 14-18:

```
     std::cout << "0th vertex from the normals sample with get method: "
               << normalsSamp->get()[0] << std::endl;

     std::cout << "0th vertex from the st sample with get method: "
               << stSamp->get()[0] << std::endl;
```

"Jeez," I hear you say.  "That seems easier than having to first call _getPositions()_, and **then** _get()_."  Except, the _getPositions()_ method on _mesh\_samp_ has companion methods for face counts and indices, so you could do:

```
     std::cout << "0th face index: "
               << mesh_samp.getIndices()->get()[0] << std::endl;

     std::cout << "0th face count with operator*: "
               << ( *(mesh_samp.getCounts()) )[0] << std::endl;
```

without having to create any explicit instances of Alembic data accessors like the _normalsSamp_.

AbcGeom objects can be parented under one another, just as with Abc Objects (because they **are** Objects), and their children, metadata, etc. are all queried identically.  One important difference between AbcGeom Objects and Abc Objects is that AbcGeom objects have an identifying string in their metadata which indicates which schema with which it was saved:

```
   // Get a generic object
   IObject obj( parentObj, parentObj.getChildHeader( cc ).getName() );

   // Fetch the schema name
   std::string schemaName = obj.getMetaData().get( "schema" );

   if (schemaName == ALEMBIC_ABCGEOM_POLYMESH_SCHEMA)
   {
      // This is a polymesh. 
      ...
   }
```

For pre-defined types in AbcGeom (eg, a polymesh), the name of the schema is set for you.