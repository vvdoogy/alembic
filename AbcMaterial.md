# AbcMaterial #

## What is AbcMaterial? ##

AbcMaterial is meant to help formalize material definitions and bindings within Alembic. It’s important to understand it is not intended as a material interchange format between renderers: AbcMaterial does not define shading code, conventions or behavior. It’s designed for transfer of target-specific material information between applications, NOT for translation of shaders between targets.

AbcMaterial is a peer of AbcGeom, and the way it works is by defining a schema for a material definition which can act as a standalone object or as supplemental data attached to another object. AbcMaterial also defines conventions and utilities for binding and resolving material assignments and overrides. It does not define what a material schema should actually contain (the way AbcGeom does with well-defined geometry types such as cameras or meshes) since it would be impossible to accomodate all of the many variations in material standards that exist today. Instead, a material schema contains the shader names, parameters and connections for one or more named targets (such as "maya", "mentalray", "arnold", "prman", "glsl", etc.). Shaders may be specified in both monolithic and networked form.

In general terms, the AbcMaterial library allows you to:
  * describe target-specific materials as sets of shaders, with their list of named and typed parameters, as well as their parameter values
  * describe inheritance relationships between materials
  * describe networked materials (the nodes that they're composed of, their public interface, and any parameters set on said interface)
  * assign materials to locations in an Alembic file - this is done either by:
    * describing the material directly as a child compound on that location
    * specifying an assignment with a full path to an alternate location in the alembic file where the material is described
  * override material parameter values at each geometry location the material is assigned to


For a quick start, the simplest is probably to look at the AbcMaterial/Tests/MaterialAssignAndFlattenTest.cpp example.