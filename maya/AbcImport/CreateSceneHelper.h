//-*****************************************************************************
//
// Copyright (c) 2009-2011,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************

#ifndef ABCIMPORT_CREATE_SCENE_H_
#define ABCIMPORT_CREATE_SCENE_H_

#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MDagPath.h>
#include <maya/MDataHandle.h>
#include <maya/MSelectionList.h>
#include <maya/MFnMesh.h>

#include <set>
#include <map>
#include <vector>

#include <Alembic/AbcGeom/IPolyMesh.h>
#include <Alembic/AbcGeom/ISubD.h>
#include <Alembic/AbcGeom/IPoints.h>
#include <Alembic/AbcGeom/IXform.h>

#include "NodeIteratorVisitorHelper.h"

struct ltMObj
{
  bool operator()(const MObject & s1, const MObject & s2) const
  {
    return (&s1) < (&s2);
  }
};

/// static visitor that create the scene hierarchy
class CreateSceneVisitor
{
public:

    // iFrame:  the frame to be read in order to fill the newly created object
    // iFlag:   true - create new maya object as part of the process of setting
    //          up the maya hierarchy; also push the animated objects into the
    //          corresponding list used to set up the connections;
    //          the visitor with iFlag set to true is used in createScene()
    //          false - the maya hierarchy is already set up, but this time
    //          traverse the hdf file to initialize all the reader pointers;
    //          the visitor with iFlag set to false is used in AlembicNode
    //
    // **
    // Using the flag means we are going through the hierarchy twice.
    // Doing this is because otherwise we will need to pass the IObject
    // as an input to the AlembicNode.
    // Also it will make reloading maya scene file containing AlembicNode
    // easier.
    //
    CreateSceneVisitor(double iFrame = 0,
        const MObject & iParent = MObject::kNullObj, bool iNotCreate = 0);

    ~CreateSceneVisitor();

    // gets the ball rolling starting the hierarchy walk
    MStatus walk(Alembic::Abc::IArchive & iRoot);

    void visit(Alembic::Abc::IObject & iObj);

    MStatus operator()(Alembic::AbcGeom::IPoints & iNode);
    MStatus operator()(Alembic::AbcGeom::IPolyMesh & iNode);
    MStatus operator()(Alembic::AbcGeom::ISubD & iNode);
    MStatus operator()(Alembic::AbcGeom::IXform & iNode);

    void setConnectArgs(bool iConnect, MString iConnectRootNodes,
        bool iCreateIfNotFound = false, bool iRemoveIfNoUpdate = false);

    bool hasSampledData();

    void getData(WriterData & oData);

    // re-add the shader selection back to the sets for meshes
    void applyShaderSelection();

private:

    // helper function for building mShaderMeshMap
    // Remembers what sets a mesh was part of, gets those sets as a selection
    // and then clears the sets for reassignment later
    // This is only used when -connect flag is set to hook up a AlembicNode
    // to a previous hierarchy
    void checkShaderSelection(MFnMesh & iMesh, unsigned int iInst);

    double mFrame;
    MObject mParent;
    bool mNotCreate;

    // member variables related to flags -connect,
    // -createIfNotFound and -removeIfNoUpdate
    std::set<std::string> mConnectRootNodes;
    std::set<MDagPath> mConnectCurNodesInBoth;
    MDagPath mCurrentDagNode;
    enum { NONE, CONNECT, CREATE } mCurrentConnectAction;
    bool mConnect;
    bool mCreateIfNotFound;
    bool mRemoveIfNoUpdate;
    std::set<std::string>   mConnectUpdateNodes;

    WriterData  mData;

    // special map of shaders to selection lists that have selections
    // of parts of meshes.  They are to get around a problem where a shape
    // wont shade correctly after a swap if it is shaded per face
    std::map < MObject, MSelectionList, ltMObj > mShaderMeshMap;
};  // class CreateSceneVisitor


#endif  // ABCIMPORT_CREATE_SCENE_H_
