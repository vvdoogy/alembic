# CONCEPTS AND PHILOSOPHY IN ALEMBIC INTRO #
This document is intended for developers who wish to understand so of the original thinking and design philosophy in Alembic so that they can see why things are organized they way they are in the library. This is not an end-user’s guide for things like the reference Maya or PRMan plugins.

The Alembic system is intended to facilitate baked (cached) geometry workflows and platform-independent geometry interchange and sharing.  It accomplishes this by being, at its core, a library for efficiently storing samples of hierarchically related typed data.  Layered on top of that is a system for interpreting that data as geometry or geometric transforms, but it’s important to keep in mind the primacy of data management at its center.  This document aims to acquaint the reader with the core concepts in Alembic, and how they are used to create geometric representations.
# SECTION 1: Layers of the Library #

Alembic itself is composed of several layers of libraries, each of which builds extensively on the layer below it.  Starting with the lowest layer and moving up towards concrete implementation, there is:

## Alembic::AbcCoreAbstract (abbreviated namespace: “AbcA”) ##

The AbcCoreAbstract library is an almost-pure abstract library that describes the interfaces that the storage layer (e.g. AbcCoreHDF5) has to implement for Alembic. The AbcCoreAbstract layer specifies the interface for concepts like Objects or Properties, and the byte-size of data.  More on the Alembic concepts like Objects or Properties in Section 2.  Almost all argument and return types in this layer are boost::shared\_ptrs, and their names will have the suffix “Ptr” to reflect that. Boost shared pointers provides a powerful means for managing resources without a lot of manual work on your part and this makes the entire Alembic library easier to use and more consistent with respect to memory management.

One important piece of Alembic that is defined in this layer is Alembic’s notion of time, in the form of the TimeSampling and TimeSamplingType classes.  These are non-abstract classes, and provide a rich interface for working with the temporal component of data samples, and are some of the only classes from this layer that are directly and prominently exposed in higher layers.

AbcCoreAbstract is not intended to be the primary human-friendly data-manipulation library in Alembic; that distinction is held by the Abc library.

## Alembic::Abc (abbreviated namespace: “Abc”) ##

The Abc library, as mentioned above, is the human-friendly data-manipulation library in Alembic, and provides object-based wrappers around the shared pointers from the AbcCoreAbstract layer.  As well, and in contrast with AbcCoreAbstract, the Abc layer provides for interpretation of the data manipulated with it, eg, rather than a shared pointer to a contiguous block of 1024 bytes at the AbcCoreAbstract layer, at the Abc layer those bytes can be thought of as a value instance of an Imath::M44d.

## Alembic::AbcGeom (abbreviated namespace: “AbcGeom”) ##

The AbcGeom library builds atop Abc, and is intended to be the primary layer for dealing with data with a geometric interpretation, eg, PolyMesh or Xform.  Although Alembic is separated into layers as described here, all lower namespaces are lifted into each namespace above it, so that when client code has a statement like “using namespace AbcGeom;” all the namespaced identifiers from AbcA or Abc, as well as from AbcGeom, are available without qualification.
## A note about writing and reading data ##

Because Alembic is intended as a caching system, and not a live scenegraph, the API is split into two roughly symmetrical halves: one for writing, and one for reading.  At the Abc and AbcGeom layers, classes that start with ‘O’ are for writing (or “output”), and classes that start with ‘I’ are for reading (or “input”).  This is analogous to the C++ iostreams conceptual separation of istreams and ostreams.
# SECTION 2: Container Hierarchy in Alembic #
## As above, so below. ##

The notion of hierarchy is central to Alembic, and that is reflected in the structure of containers in Alembic.  For purposes of this section, all the classes here are from the Abc layer, unless otherwise noted with an abbreviated namespace qualifier, eg, “AbcA::chrono\_t”.  As well, the containers discussed here are given without their ‘O’ or ‘I’ prefixes, unless it’s important to distinguish between writing and reading (eg, rather than talking about an Abc::OCompoundProperty, I will say merely “CompoundProperty”).

That said, here is the high-level overview of the container hierarchy in Alembic.  Each container will be discussed in greater detail once their overall relationships are established.

## Archive ##

An Alembic Archive is the top-level container, and is in practice the C++ class representing the actual on-disk file.  Archives contain Objects.

## Object ##

An Alembic Object is the main unit of hierarchy in Alembic.  You can think of an Alembic Archive as a typical UNIX-style filesystem (eg, ext2fs), and of Alembic Objects as directories in that filesystem.  The analogy is not perfect, as will be seen shortly, but it’s not bad.  Objects don’t contain data directly, but instead provide structure for filling with more directly-data-containing entities.  Alembic Objects are the primary unit of encapsulation.  Objects contain Properties.

## Property ##

There are two types of Alembic properties, Simple and Compound. A CompoundProperty is used to hold properties. It is the main type of container you’ll be working with. Simple Properties hold samples and can be either Scalar or Array. These samples are what actually holds your data.

### Scalar Property ###

An Alembic ScalarProperty is a Simple Property that contains Samples whose type and number of elements (the extent) is fixed and known prior to writing.  Examples of ScalarProperties are FloatProperty (each Sample is a 32-bit floating point number; extent = 1), StringProperty (each sample is a single string, of whatever size; extent = 1), or an M44dProperty (each Sample is sixteen 64-bit floating point numbers; extent = 16).  The maximum extent for a ScalarProperty is 256.

### Array Property ###

An Alembic ArrayProperty is a Simple Property that contains Samples whose type is fixed and known prior to writing, but whose extent is variable.  Examples of ArrayProperties are DoubleArrayProperty (each Sample is an array of varying length, each array element being a single 64-bit floating point number), V3fArrayProperty (each Sample is an array of varying length, and each element in the array being an Imath::Vec3f, which is three 32-bit floating point numbers), or M44fArrayProperty (each Sample is an array of varying length, and each array element is an Imath::M44f, or sixteen 32-bit floating point numbers).

## Sample ##

An Alembic Sample is the container that composes raw data and a time into a single entity.  As with Properties, Samples are either Scalar or Array.

## To Recap: ##

Archives contain Objects; Objects contain Properties, which can be either Compound or Simple; Compound Properties contain Properties, which can be Simple or Compound; Simple Properties contain Samples, and can be Scalar or Array; Samples contain data.

There are some subtleties about the above statement, but those will be elucidated in code samples later on.
# SECTION 3: Time #

Because Alembic is a sampling system, and does not natively store things like animation curves or natively provide interpolated values, there is a rich system for recording and recovering the time information associated with the data stored in it.  In the classes AbcA::TimeSamplingType and AbcA::TimeSampling, you will find that interface.

## TimeSamplingType ##

The TimeSamplingType class controls how properties in Alembic relate time values to their sample indices.

The default behavior is where there is a time value associated with sample zero, and a uniform time amount between each subsequent sample.  This is Uniform time sampling, and would correspond to sampling
every frame at 1/24 per second, or similar.  Without an explicit interval between samples, it defaults to 1.0 units of time (time is specified in the Alembic C++ code with the chrono\_t type, which is typedefed in AbcCoreAbstract to a 64-bit floating point number).

The second behavior is where there is a period of time over which a fixed number of samples are distributed unevenly - imagine a render scene sampled across a shutter period at shutter-begin-open, shutter-full-open, shutter-begin-close, shutter-full-close. This is Cyclic time sampling.

The final behavior is where the time samples follow no repeating scheme, though time must progress in a strictly increasing manner.  This is called Acyclic time sampling.

In the most general case, instances of TimeSamplingType classes are constructed by supplying two numbers: an unsigned 32-bit integer (uint32\_t) that represents the number of samples per cycle, and a signed 64-bit floating point number (chrono\_t) that represents the time per cycle.  However, there are convenience constructors for Uniform time sampling:
```
    TimeSamplingType()
     : m_numSamplesPerCycle( 1 )
       , m_timePerCycle( 1.0 ) {}

    explicit TimeSamplingType( chrono_t iTimePerCycle )
     : m_numSamplesPerCycle( 1 )
     , m_timePerCycle( iTimePerCycle ) {}
```
## TimeSampling ##

The TimeSampling class is the main interface to time information.  It’s constructed in general with an instance of TimeSamplingType, and an array of chrono\_ts of length TimeSamplingType::m\_numSamplesPerCycle.  If the TimeSamplingType instance is not Acyclic, then all times beyond startTime + timePerCycle are calculated rather than stored, though this is transparent to any client code.  There is a convenience constructor in the case of Uniform TimeSamplingType:
```
    TimeSampling( chrono_t iTimePerCycle, chrono_t iStartTime );
```
When accessing time information, that information is returned as a `std::pair<index_t, chrono_t>.`  The typename “index\_t” is typedefed in AbcCoreAbstract to be a signed 64-bit integer, and is used for sample indices; see `lib/Alembic/AbcCoreAbstract/Foundation.h`  The TimeSampling class has four main methods for getting time information:
```
    chrono_t getSampleTime( index_t iIndex ) const;

    //! Find the largest valid index that has a time less than or equal
    //! to the given time. Invalid to call this with zero samples.
    //! If the minimum sample time is greater than iTime, index
    //! 0 will be returned.
    std::pair<index_t, chrono_t> getFloorIndex( chrono_t iTime,
                                                 index_t iNumSamples ) const;

    //! Find the smallest valid index that has a time greater
    //! than the given time. Invalid to call this with zero samples.
    //! If the maximum sample time is less than iTime, index
    //! numSamples-1 will be returned.
    std::pair<index_t, chrono_t> getCeilIndex( chrono_t iTime,
                                                index_t iNumSamples ) const;

    //! Find the valid index with the closest time to the given
    //! time. Invalid to call this with zero samples.
    std::pair<index_t, chrono_t> getNearIndex( chrono_t iTime,
                                                index_t iNumSamples ) const;
```
You’ll note that getSampleTime() takes only an index, from which an actual time value is computed using the scheme specified in the TimeSampling instance’s TimeSamplingType member.  The three getFooIndex() methods, though, also take a number of samples, and will use that to clamp the returned index to iNumSamples - 1.

The reason for the return type being the pair that it is is to allow you to check your requested time against the time associated with that sample index.  The following relationship will always be true:
```
getFloorIndex (someTime, numSamples).second ==
   getSampleTime (getFloorIndex(someTime).first, numSamples);
```
Not just for getFloorIndex(), but for getCeilIndex() and getNearIndex() as well.

With these four methods, I hope it is clear how client code can implement interpolators.

TimeSampling instances are stored in an array in the Archive.  When constructing Simple Properties for writing, an index into that array can be provided to the Property’s constructor, so that Samples given to the Property can have the appropriate time information associated.  If no such index is given, the default TimeSampling will be assumed, which is Uniform with start time of 0.0, and 1.0 chrono\_t per cycle.

# SECTION 4: Deep dive into Simple Properties #

Remember the container hierarchy: Simple Properties contain Samples; Samples contain data.  For most purposes, though, Samples and data can be considered as the same thing, and most client code is going to deal with creating and manipulating Properties in some way.  In this section, we’ll be dealing with the Abc level, which is data-centric, rather than geometric.  In a later section, we’ll show how the AbcGeom layer uses the Properties from Abc to articulate a geometric interpretation of data.

## OScalarProperties ##

First off, we’ll be focusing on the ‘O’ side of the API, for writing.  At the Abc layer, the Simple Properties are further broken into typed and untyped, as well as Scalar vs. Array.  The untyped OScalarProperties, as defined in `lib/Alembic/Abc/OScalarProperty.h`, differ mainly from the OTypedScalarProperty in that they accept data for sampling in the form of a `void*`, and they must be constructed with an instance of AbcA::DataType, a class that describes the size and type of the primitive data in a Sample, so that Alembic can accurately keep track of resources.  An OTypedScalarProperty, on the other hand, can infer the size and type of Samples given to it because the class itself knows what it’s supposed to hold.  Most client code will be dealing with TypedScalarProperties, but since an OTypedScalarProperty is-an OScalarProperty, some of the methods and members of OTypedScalarProperties will be coming from the parent class.  This section is going to focus on the OTypedScalarProperties.

Looking at the constructors for an OTypedScalarProperty, the main one looks like this:
```
    //! Create a new TypedScalarProperty
    //! as a child of the passed COMPOUND_PTR
    //! Arguments can specify metadata, timesampling, and error handling.
    template <class COMPOUND_PTR>
    OTypedScalarProperty(
         COMPOUND_PTR iParent,
         const std::string &iName,

         const Argument &iArg0 = Argument(),
         const Argument &iArg1 = Argument(),
         const Argument &iArg2 = Argument() );
```
The template argument, COMPOUND\_PTR, is any type that can be intrusively converted to an AbcA::CompoundPropertyWriterPtr (“intrusively converted” meaning the type either has-a or is-a CompoundPropertyWriterPtr that can be extracted from the concrete type, or cast to from a derived type, respectively) to be used as the parent in a parent-child hierarchical relationship.  The second argument is a string to be used as the name of the Property.  The final three optional arguments, of type Abc::Argument, are boost::variants, which is a union-esque multi-type class, and will be explained shortly, but the comment gives a clue about what you can pass in there.  Because Argument is a multi-type, you may pass in as many or as few optional arguments as you’d like, without worrying about their position in the ctor call.

Don’t be alarmed if the preceding paragraph didn’t make a lot of sense; there’s a lot of context specific to Alembic whose meaning is only implied.  Let’s break it down:
  * one of the obligatory arguments, iParent, is the parent (duh) of this Property: all entities constructed by Alembic client code must be constructed hierarchically, except for the Archive itself;
  * the parent of a Simple Property is always a Compound Property;
  * the name given in the second argument must be unique at that level of hierarchy;
  * Simple Properties are constructed with an optional reference to a TimeSampling instance;
  * the types in Abc are object-based wrappers around the more primitive types from AbcCoreAbstract; an OCompoundProperty has-an AbcA::CompoundPropertyWriterPtr.


Those are the Big Ideas for Simple Properties.  There are a couple Small Ideas, though, less specific to Simple Properties, and more generic to Alembic as a whole:

  * the suffix “Ptr”, as in AbcA::CompoundPropertyWriterPtr, means that the type is a boost::shared\_ptr to an instance of the suffix’s root (in that case, an AbcA::CompoundPropertyWriter);
  * entities in Alembic may be constructed with metadata; specifically, with an instance of AbcA::MetaData, which is a string to string key-value hashmap.

In practice, Simple Properties for writing, not just OScalarProperties, have as their parent an Abc::OCompoundProperty.

Anyway, this is beginning to digress from OScalarProperties!  Back on track!

If you look in the file lib/Alembic/Abc/OTypedScalarProperty.h, at the bottom of the file, there are a bunch of typedefs of templated OTypedScalarProperties to some meaningful name, eg:
```
typedef OTypedScalarProperty<BooleanTPTraits>         OBoolProperty;
typedef OTypedScalarProperty<Uint8TPTraits>           OUcharProperty;
typedef OTypedScalarProperty<Int8TPTraits>            OCharProperty;
typedef OTypedScalarProperty<Uint16TPTraits>          OUInt16Property;
typedef OTypedScalarProperty<Int16TPTraits>           OInt16Property;
typedef OTypedScalarProperty<Uint32TPTraits>          OUInt32Property;
typedef OTypedScalarProperty<Int32TPTraits>           OInt32Property;
typedef OTypedScalarProperty<Uint64TPTraits>          OUInt64Property;
typedef OTypedScalarProperty<Int64TPTraits>           OInt64Property;
typedef OTypedScalarProperty<Float16TPTraits>         OHalfProperty;
typedef OTypedScalarProperty<Float32TPTraits>         OFloatProperty;
typedef OTypedScalarProperty<Float64TPTraits>         ODoubleProperty;
typedef OTypedScalarProperty<StringTPTraits>          OStringProperty;
typedef OTypedScalarProperty<WstringTPTraits>         OWstringProperty;
```
etc.  The template argument, the various “TPTraits”, are exactly analogous to STL type traits, and specify things like how many bytes each datum in the Property will take, their interpretation, etc.  These type traits are defined in the file lib/Alembic/Abc/TypedPropertyTraits.h, if you want to take a look.  When you construct an OTypedScalarProperty, you’d never do this:

```
OTypedScalarProperty myProp( parent, “this_is_the_property_name” );
```
Instead, you’d invoke one of the typedefed names, like so:
```
ODoubleProperty myDoubleProp( parent, “this_is_the_property_name” );
```
and Alembic will know that it’s intended to hold 64-bit floating point numeric values.

Speaking of a Property holding values, the Property has a method, set(), with a signature like this:
```
void set( const value_type &iVal )
```
The type declaration of the argument, `value_type`, is typedefed at the top of OTypedScalarProperty.h, and is defined in the traits template argument for the Property.  For an ODoubleProperty, `value_type` is `Alembic::Util::float64_t`.  See `lib/Alembic/Util/PlainOldDataType.h` for a list of the most primitive types in Alembic.

Back to the matter at hand: storing values in a Property.  Let’s say you’ve constructed an ODoubleProperty, “myDoubleProp”, as shown above.  Because it was not constructed with a reference to a TimeSampling instance, it will use the default TimeSampling, and assume that data given to it starts with time 0.0, and each subsequent Sample is meant to be associated with 1.0 chrono\_ts after the previous Sample.  The following code will set ten Samples on myDoubleProp:
```
for ( size_t i = 0 ; i < 10 ; ++i )
{
   myDoubleProp.set( 2.0 * i );
}
```
Now, when you call `myDoubleProp.getNumSamples()`, it will return 10.  The 0th Sample has the value 0.0, the 1th has the value 2.0, the 2th 4.0, etc.  The following software listing, if compiled (and linked with the right libs) and run, will produce an Alembic Archive that contains a single Simple Property, with the values as described above:
```
#include <Alembic/Abc/All.h>
// the following include is for the concrete implementation of Alembic
#include <Alembic/AbcCoreHDF5/All.h>

using namespace Alembic::Abc;

int main( int, char** )
{
   OArchive archive( Alembic::AbcCoreHDF5::WriteArchive(), “myFirstArchive.abc” );
   OObject child( archive.getTop(), “childObject” );
   ODoubleProperty myDoubleProp( child.getProperties(), “doubleProp” );
   for ( size_t i = 0 ; i < 10 ; ++i )
   {
       myDoubleProp.set( 2.0 * i );
   }
   return 0;
}
```

Ta-da!  Pretty easy, right?  Of course, there’s that HDF5 stuff, and the matter of `archive.getTop().getProperties()`, but that will become clear later.  You might guess, though, that getProperties() is a method that returns an Abc::OCompoundProperty, and you’d be right.  The full name of that method is Abc::OObject::getProperties(), which might lead you to also guess, correctly, that Abc::OArchive::getTop() returns an Abc::OObject.  Once again, we are digressing from the discussion of OScalarProperties, but we’ll return to the matter of getTop() and getProperties() later.

As simple as this code is, it may raise more questions than it answers, aside from the questions hinted at in the preceding paragraph:

  * When does the data get written?
  * Also, what if instead of an ODoubleProperty, I had an OReallyHeavyValueTypeProperty, such that I was allocating values for passing to set() on the heap; when could I safely delete it?

Those are very good questions!

You may have noticed that there was no explicit resource management being done in this code.  That’s because Alembic is doing that for you, through the use of shared pointers.  In Alembic, data is written once all its enclosures have gone out of scope.  So, once main() is left, the Archive writes itself out to disk, the Archive’s top Object does the same, the top Object’s Compound Property follows, and finally that Compound Property’s Simple Property (written to disk as “doubleProp”).  Using a program like abcecho, from $ALEMBIC\_SRC\_ROOT/examples/bin/AbcEcho/AbcEcho.cpp, you can see that the resulting Archive has a single Simple Property, as we expect:

```
$ abcecho myFirstArchive.abc
Object name=/childObject
   ScalarProperty name=doubleProp;interpretation=;datatype=float64_t;numsamps=10
$
```

As for the second good question, “when can I safely free resources?” the answer is, “As soon as you call set(), Alembic owns that data, and you may safely free it.”  This is true for all Properties (scalar or array) in Alembic, as well as true for the high-level geometric types in AbcGeom.

At this point, it would be instructive to digress from the focus on writing, and expand the source listing above into a write/read example, showing how the scoping mechanism works, as well as illustrating the API symmetry.
```
#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <iostream>

using namespace Alembic::Abc;

int main( int, char** )
{
   std::string archiveName( “myFirstArchive.abc” );
   { // open new scope for writing
       OArchive archive( Alembic::AbcCoreHDF5::WriteArchive(),
                         archiveName );
       OObject child( archive.getTop(), “childObject” );
       ODoubleProperty myDoubleProp( child.getProperties(), “doubleProp” );
       for ( std::size_t i = 0 ; i < 10 ; ++i )
       {
           myDoubleProp.set( 2.0 * i );
       }
   } // the Archive is written here when it goes out of scope

   { // open new scope for reading
       IArchive archive( Alembic::AbcCoreHDF5::ReadArchive(),
                         archiveName );
       IObject child( archive.getTop(), “childObject” );
       IDoubleProperty myDoubleProp( child.getProperties(), “doubleProp” );
       for ( std::size_t i = 0 ; i < myDoubleProp.getNumSamples() ; ++i )
       {
           std::cout << i << “th sample is “ << myDoubleProp.getValue( i )
                     << std::endl;
       }
   } // end of scope for reading

   return 0;
}
```