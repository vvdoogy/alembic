# Alembic 0.9 #

### Alembic Today ###

This 0.9 release of Alembic on November 2nd, 2010 has a dual purpose: it is a beta release, but it is also the beginning of a conversation.  We are fairly
confident that the Alembic library, as it exists now, performs well
and represents a solid beta release ready for deeper evaluation. We are now at the beginning of building the tools that will leverage the features that Alembic provides.
By making our beta version of this library public, we hope that the attention of the community will ferret out subtle problems or insufficiently supported use-cases that are meaningful to the industry as a whole.

However, in addition to bug-hunting and performance tuning we are also seeking feedback from the visual effects community on the usability and extensibility of the user-facing API layers. Specifically:

  * Design and API of [AbcGeom](http://code.google.com/p/alembic/source/browse/#hg/lib/Alembic/AbcGeom): We have worked to make [AbcGeom](http://code.google.com/p/alembic/source/browse/#hg/lib/Alembic/AbcGeom) both a statement of industry consensus of what certain geometric primitives are (polygon meshes, for example), but to also be extensible, so advanced users can quickly create their own high-level schemas to support other types of data, such as lights, bezier curves and so forth. Have we struck the right balance between extensibility and standardization?

  * Design and API of [Abc](http://code.google.com/p/alembic/source/browse/#hg/lib/Alembic/Abc): The [Abc](http://code.google.com/p/alembic/source/browse/#hg/lib/Alembic/Abc) API layer is what we consider the lowest-level of the Alembic library that authors of plug-ins or utilities will use.  It has grown from a set of "thin" wrapper classes into a full-fledged API for reading and writing arbitrary data, and this ancestry shows in its design.  To use the _Abc_ API, you will need to use classes and types from the libraries [Abc](http://code.google.com/p/alembic/source/browse/#hg/lib/Alembic/Abc),  [AbcCoreAbstract](http://code.google.com/p/alembic/source/browse/#hg/lib/Alembic/AbcCoreAbstract), and [Util](http://code.google.com/p/alembic/source/browse/#hg/lib/Alembic/Util). We already have some plans in mind for 1.0, but we want to know if there are parts of this API that you find hard to understand or use. Alembic has a carefully layered design philosophy which means it is easy for one to implement a "nicer" alternative to _Abc_ that can remain fully binary compatible with Alembic.

### What's Next ###
There are several items that we know aren't in version 0.9. This list is the starting point for our plans as we work toward 1.0:
  * Python bindings for the public-facing API.
  * Missing AbcGeom schemas:
    * NURBs surfaces
    * Cameras
    * Maya-like transform stack
    * Curves
    * Optional validation of data on write
  * A full-featured Maya importer and exporter
  * More complete documentation of the Abc and AbcGeom layers
  * An Alembic file stitcher
  * Renderman plugin
  * Katana/Arnold plugin
  * 32-bit support (currently only 64-bit architectures are supported)
  * Exploration of Windows support (currently, we believe Alembic **should** work under Windows without unreasonable effort to anneal the codebase, but we are seeking assistance with this task)

### Your Participation ###
These topics, and the questions that descend from them, aren't meant
to be prescriptive but rather representative of the feedback we'd like
to get from our users.  Alembic is stable and usable, but far from
finished, and any comments, criticisms or insights that you can
provide are more than welcome. We hope you'll share your thoughts
and feedback with us on alembic-discussion@googlegroups.com.
To join the alembic-discussion mailing list, please visit [http://groups.google.com/group/alembic-discussion](http://groups.google.com/group/alembic-discussion).

Thanks,
The Alembic Team