# Geometric types proposed in Alembic 1.0: #

## Transform ##
The AbcGeom Transform class has these attributes:
  * ops - One or more operations of translation, scale, rotation, or 4x4 Rotation is angle in degrees and axis of rotation.  Operations can optionally indicate a hint (e.g. for Maya to record hinting for pivot behavior).
  * inherits - Does this transform concatenate or ignore the transforms of its parents
  * childBounds - (optional) The bounding volume, including all subobjects
Alembic uses a right handed coordinate system.

## Points ##
The AbcGeom Points class has these attributes:
  * P - the positions of each point
  * ids - for each P a 64bit unsigned integer that can be used to identify a Point
  * selfBounds - The bounding volume for just this object
  * childBounds - The bounding volume, including all subobjects
  * velocities - (optional) per point value is in units/second

## Curves ##
The AbcGeom Curves class has these attributes:
  * type - linear or cubic (for all curves in object)
  * wrap - periodic or non-periodic (for all curves in object)
  * P - vertices
  * nVertices - for each curve the number of vertics in the curve
  * width (optional) - can be constant or varying, expressed as a diameter
  * N - (optional) Normals
  * uv - (optional) u-v coordinates
  * velocities - (optional) per point value is in units/second
  * selfBounds - The bounding volume for just this object
  * childBounds - (optional) The bounding volume, including all subobjects


## PolyMesh ##
The Abc PolyMesh class has these attributes:
  * faceCounts - for each face the count of vertices that make up the face
  * faceIndices - for each vertex of each face the slot in P for the vertex
  * P - vertices
  * N - (optional) Normals
  * uv - (optional) u-v coordinates
  * selfBounds - The bounding volume for just this object
  * childBounds - (optional) The bounding volume, including all subobjects
  * facesets - (optional) named sets of faces from this mesh
  * velocities - (optional) per point value is in units/second

## SubD ##
The Abc SubD class has these attributes:
  * faceCounts - for each face, the count of vertices that make up the face
  * faceIndices - for each vertex the slot in P for the vertex's coordinates
  * P - vertex coordinates
  * creases - (optional) creased edges
  * corners - (optional) vertexes that are corners
  * holes - (optional) regions to remove from the surface
  * uv - (optional) u-v coordinates
  * selfBounds - The bounding volume for just this object
  * childBounds - (optional) The bounding volume, including all subobjects
  * facesets - (optional) named sets of faces from this subd
  * velocities - (optional) value is in units/second

## FaceSet ##
The Abc FaceSet class can be created as part of a SubD or PolyMesh and has these attributes:
  * faceNumbers - the faces from the SubD or PolyMesh that are in this faceset

## NuPatch ##
The Abc NuPatch class has these attributes:
  * nucvs - the number of CVs in the u direction
  * nvcvs - the number of CVs in the v direction
  * uorder - the order of the patch in the u direction
  * vorder - the order of the patch in the v direction
  * uknot - floating point array containing the number of knots in the u direction
  * vknot - floating point array containing the number of knots in the v direction
  * Parameter lists (N, P, uv, arbparams)
  * selfBounds - The bounding volume for just this object
  * childBounds - (optional) The bounding volume, including all subobjects
  * velocities - (optional) per point value is in units/second
> Additionally, the NURBS objects will contain TrimCurves which may contain zero or more trim curves.  The path is composed of segments in the u direction and then the v direction.

## Camera ##
The Abc Camera class has these attributes:
  * focalLength (mm)
  * horizontalAperture/verticalAperture dimensions (cm)
  * horizontalFilmOffset/verticalFilmOffset (cm)
  * lenseSqueezeRatio: to handle anamorphic lenses, among other things. (width / height)
  * 2D transform stack (translate/scale only) for expressing additional 2D translate/scale filmback operations
  * overscan stored as 4 percentage values for independent left/right/top/bottom values (to support fractional extension of screen window)
  * non-symmetrical overscan, which we've come across)
  * fStop (focal length divided by "effective" lens diameter)
  * focusDistance
  * shutterOpen/shutterClose: these would be frame-relative values, in seconds (secs.)
  * near/far clipping planes

## Light ##
The attributes of the Abc Light class are to be expressed via an attached AbcMaterial.  The other optional attributes on the light include:
  * childBounds - (optional) The bounding volume, including all subobjects
  * camera - (optional) An Abc Camera.