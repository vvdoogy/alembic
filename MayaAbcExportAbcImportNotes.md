# Introduction #

### AbcExport ###
This plugin contains one command named AbcExport.  It can be used to write out one or more Alembic files.  It isn't a file  translator so that it can write out multiple files with different settings while only stepping through the frame range once.

To get more info about this command in Mel:
```
AbcExport -h;
```

### AbcImport ###
This plugin is used to load an Alembic file and contains one command named AbcImport and a node named AlembicNode.  Creation and management of AlembicNode is done via the AbcImport command, and is only necessary when the scene contains animated data.  Creating and managing an AlembicNode without the AbcImport command is not recommended.

To get more info about this command in Mel:
```
AbcImport -h;
```

# Common Exporting Examples #

Let's say you have a Maya hierarchy that looks like this:

solar\_system - transform

|
`- sun - transform
> |
> `- sunShape - polyMesh
|
`- inner\_planets - transform
> |
> `- mercury - transform
> > |
> > `- mercuryShape - polyMesh

> |
> `- venus - transform
> > |
> > `- venusShape - polyMesh

> |
> `- earth - transform
> > |
> > `- earthShape - polyMesh

> |
> `- mars - transform
> > |
> > `- marsShape - polyMesh
|
`- outer\_planets - transform

> |
> `- jupiter - transform
> > |
> > `- jupiterShape - polyMesh

> |
> `- saturn - transform
> > |
> > `- saturnShape - polyMesh

> |
> `- uranus - transform
> > |
> > `- uranusShape - polyMesh

> |
> `- neptune - transform
> > |
> > `- neptuneShape - polyMesh

If you wanted to export inner\_planets and below while not including solar\_system or it's transform data:

```
AbcExport -j "-root inner_planets -file inner.abc";
```

If you would like to export inner\_planets and below, but exclude the mars hierarchy you can use selection:

```
select mercuryShape venusShape earthShape;
AbcExport -j "-sl -root inner_planets -file inner_no_mars.abc";
```

If you wanted to write out the entire solar\_system every 0.5 frame from 1 to 5 you can do:

```
AbcExport -j "-root solar_system -fr 1 5 -s 0.5 -file solar_system.abc";
```

If you wanted to write out the entire solar\_system but sample it with frame centered motion blur of -0.25 and 0.25 from frame 1 to 5 you can do:

```
AbcExport -j "-root solar_system -fr 1 5 -frs -0.25 -frs 0.0 -frs 0.25 -file solar_system.abc";
```

If you wanted to write out the sun, inner\_system, and outer\_system from frame 1 to 5 to different files you can do:

```
AbcExport -j "-root sun -fr 1 5 -file sun.abc" -j "-root inner_system -fr 1 5 -file inner.abc" -j "-root outer_system -fr 1 5 -file outer.abc";
```

# Per Shape Notes #

## ISubD/OSubD ##
Writing and creating an MFnSubd are not supported, instead an MFnMesh with a boolean attribute called SubDivisionMesh which is set to true is written as an Alembic OSubD and restored when reading ISubD.  Normals on the mesh are not written on OSubD.

If the MFnMesh has optional integer attributes faceVaryingInterpolateBoundary, interpolateBoundary, and faceVaryingPropagateCorners, then these values are written to the OSubD, and restored when reading ISubD.

## IPolyMesh/OPolyMesh ##
An MFnMesh that is not written as an OSubD is written as an OPolyMesh. Normals for the OPolyMesh are written if -noNormals isn't specified, the MFnMesh doesn't have a noNormals boolean attribute and at least one of the normals have actually been changed by the user.  Normals will also be written if the MFnMesh has a noNormals boolean attribute and it is set to false.

Normals can be automatically inverted when writing by adding a flipNormals boolean attribute to the MFnMesh and setting it to true.

For both OSubD and OPolyMesh, if -uvWrite has been specified the UVs from only the current UV set are written.

Polygon winding order in Alembic is clockwise, MFnMesh uses a counter-clockwise winding order so the face indices and normals when appropriate, are moved around to compensate.

## IFaceSet/OFaceSet ##
MFnSets with face component on an MFnMesh are written as an OFaceSet.  IFaceSets that are children of an IPolyMesh or ISubD are recreated on the MFnMesh as an MFnSet.

## IPoints/OPoints ##
Currently only MFnParticleSystems are written as OPoints.  IPoints create an MFnParticleSystem, unfortunately animated IPoints are not currently supported.

## ICurves/OCurves ##
If an MFnTransform has a boolean attribute named riCurves, and it is set to true, all MFnNurbsCurves in the hierarchy below it are written as a single OCurves.  Otherwise one MFnNurbsCurve is written to an OCurves.  If the curve has a degree other than 3 it is written as a 1 degree curve.  Only vertex varying or constant scoped width is supported.  Vertex varying width is handled by a double array attribute created on the MFnNurbsCurve called "width".  Constant width is handled by creating a float attribute on either the curve, or for curve groups, the parent MFnTransform that has the riCurves boolean attribute. When writing OCurves, if there is no width, or the width attribute is not the correct size a default constant width of 0.1 is used.  Knot data is not written out on export, during import the knot values are evenly distributed between 0 and 1.  When writing to OCurves the basis type kBezierBasis is always used.

When reading, the basis function is not currently considered.  The wrap type is currently ignored when reading ICurves, an open MFnNurbsCurve is always created.

## INuPatch/ONuPatch ##
MFnNurbsSurface stores the points so that v varies fastest (v, u) order. Alembic expects the data to be packed such that u varies fastest (u, v) order.  The v order is also reversed most likely to accommodate the winding order.  Two extra knots are added to each knot array to accommodate Alembic and several other packages.  The extra knots are stripped and the points are reordered when loading the Alembic data back into Maya.  The MFnNurbsSurface is currently created as open in U and open in V.

## ICamera/OCamera ##
If the motionBlurUseShutter is set to true on defaultRenderGlobals then
motionBlurShutterOpen and motionBlurShutterClose on defaultRenderGlobals are converted to seconds and used as the shutter open and shutter close on the OCamera.  If motionBlurUseShutter is set to false, shutter open is set to 0 and the shutter angle on the MFnCamera is used to determine the shutter close.

When reading the ICamera, the shutterAngle on the MFnCamera is set based
on the shutter open and shutter close, the defaultRenderGlobals are not changed while reading Alembic archives.

Other attributes on MFnCamera that are taken into account when writing the OCamera or reading the ICamera include:

  * focal length
  * lens squeeze ratio
  * horizontal film aperture
  * vertical film aperture
  * horizontal film offset
  * vertical film offset
  * overscan
  * near clipping plane
  * far clipping plane
  * f-stop
  * focus distance

These attributes end up being placed  in the Filmback Xform stack:

  * The various film fit types. (fill, horizontal, vertical, overscan)
  * pre-scale
  * film translate (horizontal and vertical)
  * post-scale
  * camera scale

Other MFnCamera attributes are currently ignored.

## IXform/OXform ##
MFnTransforms, MFnIkJoint, and MFnLocator are all written as OXform.
MFnLocator's write out an empty (inherently identity) OXform but adds a special attribute on the OObject called locator.  locator contains the 6 values from locatorPosition and locatorScale.  When reading the data back into Maya, IXforms with the locator attribute will create an MFnLocator, otherwise an MFnTransform is created.