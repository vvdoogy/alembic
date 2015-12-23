# 2011-08-08: Alembic 1.0 is released #

Welcome to the first official Alembic release. For more information on the meaning behind this 1.0 release, please read through the AlembicPromises document.

# 2011-06-29: Alembic 1.0, release candidate 1 #
Hello, Alembic users, welcome to the latest beta version of Alembic.  As we
close in on a 1.0 release, we hope the API and file format are now stable
and fully forwards-compatible.  We consider these to be at a first
"Release Candidate" stage. (note that this doesn't yet apply to the
application-specific reference implementations). As with all pre-release
software, we can't guarantee that changes won’t occur between now and a full
1.0 release.

This release is mostly a bugfix and cleanup release.  We've improved the build
support for Windows and OS X, as well as conformed the definitions of the Curves
and NuPatch types more closely to the RI spec.  Those changes, though, do mean
that Curves and NuPatch geometry written out with previous releases will not be
recognized with this release.

The complete code changes for this release can be examined here:

http://codereview.appspot.com/4662068

Some highlights:

> - The flags and arguments for the AbcExport Maya plugin have been updated
> > based on feedback from the community, and to be more Maya-idiomatic.
> > You can see all the arguments supported by entering "AbcExport -h"
> > in the script editor.


> - The Makefile setup for the AlembicIn Houdini SOP is greatly simplified,
> > though still not fully integrated with Alembic's CMake setup.


> - The PRMan procedural now supports the Points, NuPatch, Curves, and FaceSet
> > types from AbcGeom.

One other important thing to note: previous releases of Alembic did not
require any of the compiled Boost libraries, but going forward, Alembic
requires libboost\_thread.

Although this is our initial 1.0 release candidate, it should still be
considered pre-release software, and should not be used for critical, deadline-
driven production work.

Also as usual, feedback and questions are welcome!  The main discussion list
is low-traffic, but anyone can sign up and participate:

http://groups.google.com/group/alembic-discussion

As always, discussion is welcome on the discussion list.

# 2011-5-18: Alembic 0.9.3 Beta Update #

We're pleased to announce the 0.9.3 Beta release of Alembic. Highlights of this version include:

  * Definitions of geometric data types for Curves, Nurbs patches, Face sets, and Cameras. This represents the complete set of data types planned for Alembic 1.0.
  * The API for working with time values has been cleaned up and is a bit faster.
  * The Transform data type has been cleaned up and is a bit faster.
  * Many bug fixes and performance improvements.
  * Experimental support for building on Windows

In line with the previous Beta releases, 0.9.3:

  * Is not a 1.0, therefore commercial or date driven work should not depend on this version.
  * We are targeting a 1.0 release mid 2011 and really looking for your feedback on this version to round out our 1.0 plan.
  * We encourage you to post to the Alembic discussion list so that others can benefit from your feedback or join in on the conversation.

Additional documentation and the update itself can be found on this page (links to: http://code.google.com/p/alembic/ <http://code.google.com/p/alembic/> )


# 2011-02-23: Alembic 0.9.2 released #

We're pleased to announce the 0.9.2 Beta release of Alembic. Highlights of this version include:

  * Reference Maya plugins for import and export.
  * Limited Python bindings using Boost.Python.
  * Formal mechanism for specifying UV and Normals data as indexed or expanded arrays.
  * Many bug fixes and performance improvements.

In line with the previous Beta releases, 0.9.2:

  * Is not a 1.0, therefore commercial or date driven work should not depend on this version.
  * We welcome feedback and encourage you to post to the Alembic discussion list.
  * We are targeting a 1.0 release mid 2011, but that will depend on the reports we get on this version.

We continue to make great progress inside our two studios, but still need additional real-world testing outside of our doors. We ask that you please download and evaluate at your earliest convenience so there is an opportunity to contribute feedback to the 1.0 design.

Thank you for your continued support!

_The Alembic Team_