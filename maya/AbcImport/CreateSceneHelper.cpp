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

#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MIntArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MPlug.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTransform.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MFnSet.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "util.h"
#include "MeshHelper.h"
#include "PointHelper.h"
#include "XformHelper.h"
#include "CreateSceneHelper.h"

namespace
{
    void removeDagNode(bool removeIfNotUpdate, MDagPath & dagPath)
    {
        if (removeIfNotUpdate)
        {
            MStatus status = deleteDagNode(dagPath);
            if ( status != MS::kSuccess )
            {
                MString theError = dagPath.partialPathName();
                theError += MString(" removal not successful");
                printError(theError);
            }
        }
    }  // removeDagNode

}


CreateSceneVisitor::CreateSceneVisitor(double iFrame,
    const MObject & iParent, bool iNotCreate) :
    mFrame(iFrame), mParent(iParent), mNotCreate(iNotCreate)
{
}

CreateSceneVisitor::~CreateSceneVisitor()
{
}

void CreateSceneVisitor::setConnectArgs(
    bool iConnect, MString iConnectRootNodes,
    bool iCreateIfNotFound, bool iRemoveIfNoUpdate)
{
    mConnect = iConnect;
    mCreateIfNotFound = iCreateIfNotFound;
    mRemoveIfNoUpdate = iRemoveIfNoUpdate;
    mCurrentConnectAction = NONE;
    mConnectRootNodes.clear();
    mConnectUpdateNodes.clear();

    // parse the input string to extract the nodes that need (re)connection
    if ( iConnectRootNodes != MString("/") )
    {
        MStringArray theArray;
        if (iConnectRootNodes.split(' ', theArray) == MS::kSuccess)
        {
            unsigned int len = theArray.length();
            for (unsigned int i = 0; i < len; i++)
            {
                MString name = theArray[i];
                // the name could be either a partial path or a full path
                MDagPath dagPath;
                if ( getDagPathByName( name, dagPath ) == MS::kSuccess )
                {
                    name = dagPath.partialPathName();
                }

                mConnectRootNodes.insert(name.asChar());
            }
        }
    }
}

void CreateSceneVisitor::getData(WriterData & oData)
{
    oData = mData;
}

bool CreateSceneVisitor::hasSampledData()
{
    unsigned int subDSize = mData.mSubDList.size();
    unsigned int polySize = mData.mPolyMeshList.size();
    unsigned int cameraSize = 0; //mData.mCameraList.size();
    // Currently there's no support for bringing in particle system simulation
    // unsigned int particleSize = mData.mParticleList.size();
    unsigned int transopSize = mData.mXformList.size();
    unsigned int nSurfaceSize  = 0; //mData.mNurbsList.size();
    unsigned int nCurveSize  = 0; //mData.mCurveList.size();
    unsigned int propSize = 0; //mData.mPropList.size();

    return ( subDSize > 0 || polySize > 0 || nSurfaceSize > 0 || nCurveSize > 0
            || transopSize > 0 || cameraSize > 0  // || particleSize > 0
            || propSize > 0);
}

// re-add the selection back to the sets
void CreateSceneVisitor::applyShaderSelection()
{
    std::map < MObject, MSelectionList, ltMObj >::iterator i =
        mShaderMeshMap.begin();

    std::map < MObject, MSelectionList, ltMObj >::iterator end =
        mShaderMeshMap.end();

    for (; i != end; ++i)
    {
        MFnSet curSet(i->first);
        curSet.addMembers(i->second);
    }
    mShaderMeshMap.clear();
}

// remembers what sets a mesh was part of, gets those sets as a selection
// and then clears the sets for reassignment later  this is only used when
// hooking up an HDF node to a previous hierarchy (swapping)
void CreateSceneVisitor::checkShaderSelection(MFnMesh & iMesh,
    unsigned int iInst)
{
    MObjectArray sets;
    MObjectArray comps;

    iMesh.getConnectedSetsAndMembers(iInst, sets, comps, false);
    unsigned int setsLength = sets.length();
    for (unsigned int i = 0; i < setsLength; ++i)
    {
        MObject & curSetObj = sets[i];
        if (mShaderMeshMap.find(curSetObj) == mShaderMeshMap.end())
        {
            MFnSet curSet(curSetObj);
            MSelectionList & curSel = mShaderMeshMap[curSetObj];
            curSet.getMembers(curSel, true);

            // clear before hand so that when we add the selection to the
            // set later, it will take.  This is to get around a problem where
            // shaders are assigned per face dont shade correctly
            curSet.clear();
        }
    }
}

void CreateSceneVisitor::visit(Alembic::Abc::IObject & iObj)
{
    if ( Alembic::AbcGeom::IXform::matches(iObj.getHeader()) )
    {
        Alembic::AbcGeom::IXform xform(iObj, Alembic::Abc::kWrapExisting);
        (*this)(xform);
    }
    else if ( Alembic::AbcGeom::ISubD::matches(iObj.getHeader()) )
    {
        Alembic::AbcGeom::ISubD mesh(iObj, Alembic::Abc::kWrapExisting);
        (*this)(mesh);
    }
    else if ( Alembic::AbcGeom::IPolyMesh::matches(iObj.getHeader()) )
    {
        Alembic::AbcGeom::IPolyMesh mesh(iObj, Alembic::Abc::kWrapExisting);
        (*this)(mesh);
    }
    else if ( Alembic::AbcGeom::IPoints::matches(iObj.getHeader()) )
    {
        Alembic::AbcGeom::IPoints pts(iObj, Alembic::Abc::kWrapExisting);
        (*this)(pts);
    }
    else
    {
        MString theWarning(iObj.getName().c_str());
        theWarning += " is an unsupported schema, skipping: ";
        theWarning += iObj.getMetaData().get("schema").c_str();
        printWarning(theWarning);
    }
}

 // root of file, no creation of DG node
MStatus CreateSceneVisitor::walk(Alembic::Abc::IArchive & iRoot)
{
    MStatus status = MS::kSuccess;

    mData.mIsSampledXformOpAngle.clear();
    MObject saveParent = mParent;

    Alembic::Abc::IObject top = iRoot.getTop();
    size_t numChildren = top.getNumChildren();

    if (numChildren == 0) return status;

    if (!mConnect)  // simple scene creation mode
    {
        for (size_t i = 0; i < numChildren; i++)
        {
            Alembic::Abc::IObject child = top.getChild(i);
            this->visit(child);
            mParent = saveParent;
        }
    }
    else  // connect flag set
    {
        if (!mNotCreate)
        {
            std::set<std::string> connectCurNodesInFile;
            std::set<std::string> connectCurNodesInBoth;
            std::set<std::string> connectCurNodesToBeCreated;

            bool connectWorld = (mConnectRootNodes.size() == 0);
            std::set<std::string>::iterator fileEnd =
                connectCurNodesInFile.end();
            for (size_t i = 0; i < numChildren; i++)
            {
                Alembic::Abc::IObject obj = top.getChild(i);
                std::string name = obj.getName();
                connectCurNodesInFile.insert(name);

                // see if this name is part of the input to AlembicNode 
                if (connectWorld || (!connectWorld &&
                  mConnectRootNodes.find(name) != mConnectRootNodes.end()))
                {
                    // Find out if this node exists in the current scene
                    MDagPath dagPath;

                    if (getDagPathByName(MString(name.c_str()), dagPath) ==
                        MS::kSuccess)
                    {
                        connectCurNodesInBoth.insert(name);
                        mConnectUpdateNodes.insert(name);
                        mCurrentDagNode = dagPath;
                        mCurrentConnectAction = CONNECT;
                        name = dagPath.partialPathName().asChar();
                        this->visit(obj);
                        mParent = saveParent;
                    }
                    else
                    {
                        connectCurNodesToBeCreated.insert(name);
                        if ( mCreateIfNotFound )
                        {
                            mConnectUpdateNodes.insert(name);
                            mCurrentConnectAction = CREATE;
                            this->visit(obj);
                            mParent = saveParent;
                        }
                        else
                        {
                            MString warn(
                                "-createIfNotFound flag not set, skipping: ");
                            warn += name.c_str();
                            printWarning(warn);
                        }
                    }
                }
            }  // for-loop

            if ( connectWorld )
            {
                mConnectRootNodes = connectCurNodesInFile;
            }
            else if (mConnectRootNodes.size() >
                (connectCurNodesInBoth.size()
                + connectCurNodesToBeCreated.size()))
            {
                std::set<std::string>::iterator iter =
                    mConnectRootNodes.begin();
                const std::set<std::string>::iterator fileEndIter =
                    connectCurNodesInFile.end();
                MDagPath dagPath;
                for ( ; iter != mConnectRootNodes.end(); iter++)
                {
                    std::string name = *iter;
                    bool existInFile =
                        (connectCurNodesInFile.find(name) != fileEndIter);
                    bool existInScene =
                        (getDagPathByName(MString(name.c_str()), dagPath)
                            == MS::kSuccess);
                    if ( existInScene && !existInFile )
                        removeDagNode(mRemoveIfNoUpdate, dagPath);
                    else if (!existInScene && !existInFile)
                    {
                        MString theWarning(name.c_str());
                        theWarning += 
                            " exists neither in file nor in the scene";
                        printWarning(theWarning);
                    }
                }
            }
        }
        else  // mNotCreate == 1
        {
            for (size_t i = 0; i < numChildren; i++)
            {
                Alembic::Abc::IObject child = top.getChild(i);
                std::string name = child.getName();
                if ( mConnectUpdateNodes.find( name )
                    != mConnectUpdateNodes.end() )
                {
                    getDagPathByName(name.c_str(), mCurrentDagNode);
                    this->visit(child);
                }
            }
        }
    }

    return status;
}

MStatus CreateSceneVisitor::operator()(Alembic::AbcGeom::IPoints& iNode)
{
    MStatus status = MS::kSuccess;
    MObject particleObj = MObject::kNullObj;

    if (mNotCreate == 1)
    {
        if (iNode.getSchema().getNumSamples() > 1)
            mData.mPointsList.push_back(iNode);

        // review other arbitrary attributes and add it to the lists

        return status;
    }

    std::vector<std::string> propNameList;
    status = create(mFrame, iNode, mParent, particleObj, propNameList);
    if (iNode.getSchema().getNumSamples() > 1)
    {
        mData.mPointsObjList.push_back(particleObj);
        mData.mPointsList.push_back(iNode);
    }

    if (propNameList.size() > 0)
    {
        SampledPair mSampledPair(particleObj, propNameList);
        //mData.mPropList.push_back(mSampledPair);
        //mData.mPropNodePtrList.push_back(iNode);
    }

    return status;
}

MStatus CreateSceneVisitor::operator()(Alembic::AbcGeom::ISubD& iNode)
{
    MStatus status = MS::kSuccess;
    MObject subDObj = MObject::kNullObj;

    if (mNotCreate == 1)
    {
        if (iNode.getSchema().getNumSamples() > 1)
            mData.mSubDList.push_back(iNode);

        // review other arbitrary attributes and add it to the lists
        return status;
    }

    if ( mConnect == true && mCurrentConnectAction == CONNECT )
    {
        subDObj = mCurrentDagNode.node();
        MFnMesh mFn(subDObj, &status);

        // check that the data types are compatible
        if ( status == MS::kSuccess )
        {
            std::vector<std::string> propNameList;
            mParent = mCurrentDagNode.transform(&status);

            connectToSubD(mFrame, iNode, mParent, propNameList, subDObj);
            if (iNode.getSchema().getNumSamples() > 1)
            {
                mData.mSubDObjList.push_back(subDObj);
                mData.mSubDList.push_back(iNode);
            }
            else
            {
                createSubD(mFrame, iNode, mParent, propNameList);
            }

            /*
            if (iNode.hasPropertyFrames())
            {
                SampledPair mSampledPair(subDObj, propNameList);
                mData.mPropList.push_back(mSampledPair);
                mData.mPropNodePtrList.push_back(iNode);
            }
            */

            checkShaderSelection(mFn, mCurrentDagNode.instanceNumber());
        }
        else
        {
            MString theError("No connection done for node '");
            theError += MString(iNode.getName().c_str());
            theError += MString("' with ");
            theError += mCurrentDagNode.fullPathName();
            printError(theError);
            return status;
        }
        return status;
    }

    std::vector<std::string> propNameList;

    if (iNode.getSchema().getNumSamples() > 0)
    {
        subDObj = createSubD(mFrame, iNode, mParent, propNameList);
        mData.mSubDObjList.push_back(subDObj);
        mData.mSubDList.push_back(iNode);
    }
    else
    {
        subDObj = createSubD(mFrame, iNode, mParent, propNameList);
    }

    /*
    if (iNode.hasPropertyFrames())
    {
        SampledPair mSampledPair(subDObj, propNameList);
        mData.mPropList.push_back(mSampledPair);
        mData.mPropNodePtrList.push_back(iNode);
    }
    */

    return status;
}

MStatus CreateSceneVisitor::operator()(Alembic::AbcGeom::IPolyMesh & iNode)
{
    MStatus status = MS::kSuccess;
    MObject polyObj = MObject::kNullObj;

    if (mNotCreate == 1)
    {
        if (iNode.getSchema().getNumSamples() > 0)
            mData.mPolyMeshList.push_back(iNode);

        /*
        if (iNode.hasPropertyFrames())
            mData.mPropNodePtrList.push_back(iNode);
        */

        return status;
    }

    if ( mConnect == true && mCurrentConnectAction == CONNECT )
    {
        polyObj = mCurrentDagNode.node();
        MFnMesh mFn(polyObj, &status);

        // check that the data types are compatible
        if ( status == MS::kSuccess )
        {
            std::vector<std::string> propNameList;
            mParent = mCurrentDagNode.transform(&status);

            connectToPoly(mFrame, iNode, mParent, propNameList, polyObj);

            if (iNode.getSchema().getNumSamples() > 0)
            {
                mData.mPolyMeshObjList.push_back(polyObj);
                mData.mPolyMeshList.push_back(iNode);
            }

            /*
            if (iNode.hasPropertyFrames())
            {
                SampledPair mSampledPair(polyObj, propNameList);
                mData.mPropList.push_back(mSampledPair);
                mData.mPropNodePtrList.push_back(iNode);
            }
            */

            checkShaderSelection(mFn, mCurrentDagNode.instanceNumber());
        }
        else
        {
            MString theError("No connection done for node '");
            theError += MString(iNode.getName().c_str());
            theError += MString("' with ");
            theError += mCurrentDagNode.fullPathName();
            printError(theError);
            return status;
        }
        return status;
    }

    std::vector<std::string> propNameList;

    polyObj = createPoly(mFrame, iNode, mParent, propNameList);
    if (iNode.getSchema().getNumSamples() > 0)
    {
        mData.mPolyMeshObjList.push_back(polyObj);
        mData.mPolyMeshList.push_back(iNode);
    }

    /*
    if (iNode.hasPropertyFrames())
    {
        SampledPair mSampledPair(polyObj, propNameList);
        mData.mPropList.push_back(mSampledPair);
        mData.mPropNodePtrList.push_back(iNode);
    }
    */

    return status;
}

MStatus CreateSceneVisitor::operator()(Alembic::AbcGeom::IXform & iNode)
{
    MStatus status = MS::kSuccess;
    MObject transObj;

    if (mNotCreate == 1)
    {
        if (iNode.getSchema().getNumAnimSamples())
        {
            mData.mXformList.push_back(iNode);
            mData.mIsComplexXform.push_back(isComplex(iNode));
        }

        //if (iNode.hasPropertyFrames())
        //    mData.mPropNodePtrList.push_back(iNode);

        size_t numChildren = iNode.getNumChildren();
        if ( !mConnect )
        {
            for (size_t i = 0; i < numChildren; ++i)
            {
                Alembic::Abc::IObject child = iNode.getChild(i);
                this->visit(child);
            }
        }
        else
        // if mConnect is on, selectively traverse the hierarchy
        // (possibly for a second time)
        {
            std::set<std::string> childNodesInFile;
            MObject saveParent = transObj;
            MDagPath saveDag = mCurrentDagNode;
            for (size_t i = 0; i < numChildren; ++i)
            {
                Alembic::Abc::IObject child = iNode.getChild(i);
                mParent = saveParent;
                std::string childName = child.getName();
                MString name = saveDag.fullPathName();
                name += "|";
                name += childName.c_str();
                if (getDagPathByName(name, mCurrentDagNode) == MS::kSuccess)
                    this->visit(child);
            }
        }

        return status;
    }

    if ( mConnect == 1 && mCurrentConnectAction == CONNECT )
    {
        transObj = mCurrentDagNode.node();
        if (transObj.hasFn(MFn::kTransform))
        {
            std::vector<std::string> mTransopNameList;
            std::vector<std::string> propNameList;
            bool isComplex = false;
            create(mFrame, iNode, mParent, transObj, propNameList,
                mTransopNameList, isComplex, true);

            unsigned int size = mTransopNameList.size();
            for (unsigned int i = 0; i < size; i++)
            {
                if (mTransopNameList[i].find("rotate") != std::string::npos
                    && mTransopNameList[i].find("rotatePivot")
                        == std::string::npos)
                    mData.mIsSampledXformOpAngle.push_back(true);
                else
                    mData.mIsSampledXformOpAngle.push_back(false);
            }

            if (iNode.getSchema().getNumAnimSamples() > 0)
            {
                SampledPair mSampledPair(transObj, mTransopNameList);
                mData.mXformOpList.push_back(mSampledPair);
                mData.mXformList.push_back(iNode);
                mData.mIsComplexXform.push_back(isComplex);
            }

            /*
            if (iNode.hasPropertyFrames())
            {
                SampledPair mSampledPair(transObj, propNameList);
                mData.mPropList.push_back(mSampledPair);
                mData.mPropNodePtrList.push_back(iNode);
            }
            */

            // go down the current DAG's children and current
            // AlembicNode's  children
            size_t numChildren = iNode.getNumChildren();
            std::set<std::string> childNodesInFile;
            MObject saveParent = transObj;
            MDagPath saveDag = mCurrentDagNode;
            for (size_t childIndex = 0; childIndex < numChildren; ++childIndex)
            {
                Alembic::Abc::IObject child = iNode.getChild(childIndex);
                mParent = saveParent;
                std::string childName = child.getName();
                childNodesInFile.insert(childName);
                MString name = saveDag.fullPathName();
                name += "|";
                name += childName.c_str();
                MDagPath dagPath;
                if (getDagPathByName(name, dagPath) == MS::kSuccess)
                {
                    mCurrentDagNode = dagPath;
                    this->visit(child);
                    mCurrentDagNode = saveDag;
                }
                else if (mCreateIfNotFound)  // create if necessary
                {
                    mCurrentConnectAction = CREATE;
                    this->visit(child);
                    mCurrentConnectAction = CONNECT;
                }
            }

            // There might be children under the current DAG node that
            // don't exist in the file.
            // remove if the -removeIfNoUpdate flag is set
            if ( mRemoveIfNoUpdate )
            {
                unsigned int numChild = mCurrentDagNode.childCount();
                std::vector<MDagPath> dagToBeRemoved;
                for ( unsigned int i = 0; i < numChild; i++ )
                {
                    MObject child = mCurrentDagNode.child(i);
                    MFnTransform fn(child, &status);
                    if ( status == MS::kSuccess )
                    {
                        std::string childName = fn.fullPathName().asChar();
                        size_t found = childName.rfind("|");
                        if (found != std::string::npos)
                        {
                            childName = childName.substr(
                                found+1, childName.length() - found);
                            if (childNodesInFile.find(childName)
                                == childNodesInFile.end())
                            {
                                MDagPath dagPath;
                                getDagPathByName(
                                    fn.fullPathName(), dagPath);
                                dagToBeRemoved.push_back(dagPath);
                            }
                        }
                    }
                }
                if (dagToBeRemoved.size() > 0)
                {
                    unsigned int dagSize = dagToBeRemoved.size();
                    for ( unsigned int i = 0; i < dagSize; i++ )
                        removeDagNode(mRemoveIfNoUpdate, dagToBeRemoved[i]);
                }
            }
        }
        else
        {
            MString theError = mCurrentDagNode.partialPathName();
            theError += MString(" is not compatible as a transform node. ");
            theError += MString("Connection failed.");
            printError(theError);
        }
    }

    if ( !mConnect || mCurrentConnectAction == CREATE )
    {
        // create transform node
        std::vector<std::string> mTransopNameList;
        std::vector<std::string> propNameList;

        bool isComplex = false;
        status = create(mFrame, iNode, mParent, transObj,
              propNameList, mTransopNameList, isComplex);

        unsigned int size = mTransopNameList.size();
        for (unsigned int i = 0; i < size; i++)
        {
            if (mTransopNameList[i].find("rotate") != std::string::npos &&
               mTransopNameList[i].find("rotatePivot") == std::string::npos)
                mData.mIsSampledXformOpAngle.push_back(true);
            else
                mData.mIsSampledXformOpAngle.push_back(false);
        }

        if (iNode.getSchema().getNumAnimSamples() > 0)
        {
            SampledPair mSampledPair(transObj, mTransopNameList);
            mData.mXformOpList.push_back(mSampledPair);
            mData.mXformList.push_back(iNode);
            mData.mIsComplexXform.push_back(isComplex);
        }

        /*
        if (iNode->hasPropertyFrames())
        {
            SampledPair mSampledPair(transObj, propNameList);
            mData.mPropList.push_back(mSampledPair);
            mData.mPropNodePtrList.push_back(iNode);
        }
        */

        size_t numChildren = iNode.getNumChildren();
        MObject saveParent = transObj;

        for (size_t i = 0; i < numChildren; ++i)
        {
            Alembic::Abc::IObject child = iNode.getChild(i);
            mParent = saveParent;
            this->visit(child);
        }
   }

    return status;
}
