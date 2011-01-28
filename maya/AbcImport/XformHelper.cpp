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

#include <maya/MObject.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnTransform.h>
#include <maya/MMatrix.h>
#include <maya/MPoint.h>
#include <maya/MQuaternion.h>
#include <maya/MString.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>
#include <maya/MGlobal.h>
#include <maya/MDagModifier.h>

#include "util.h"
#include "XformHelper.h"
#include "NodeIteratorVisitorHelper.h"

#include <Alembic/AbcGeom/IXform.h>

namespace
{
    // I'm not sure why kPreTransform works and kTransform does not for all
    // cases except when the transform is complicated and we need to use the
    // matrix.
    MSpace::Space gSpace = MSpace::kPreTransform;
    const bool gBalance = false;
};

// complex matrix doesn't map to Maya directly, so get the matrix
// and then try to break it down into Maya parts
void readComplex(double iFrame, Alembic::AbcGeom::IXform & iNode,
    std::vector<double> & oSampleList)
{
    int64_t index, ceilIndex;
    double alpha = getWeightAndIndex(iFrame,
        iNode.getSchema().getTimeSampling(), index, ceilIndex);

    Alembic::Abc::M44d m; 

    if (alpha != 0.0 && index != ceilIndex)
    {
        Alembic::Abc::M44d mlo = iNode.getSchema().getMatrix(
            Alembic::Abc::ISampleSelector(index));

        Alembic::Abc::M44d mhi = iNode.getSchema().getMatrix(
            Alembic::Abc::ISampleSelector(ceilIndex));

        m = ((1 - alpha) * mlo) + (alpha * mhi);
    }
    else
    {
        m = iNode.getSchema().getMatrix(
            Alembic::Abc::ISampleSelector(index));
    }

    MTransformationMatrix mmat(MMatrix(m.x));

    // Everywhere else we use kPreTransform, but this doesn't work
    // when we pass in the matrix for some reason
    MSpace::Space tSpace = MSpace::kTransform;

    // push the results into sampleList
    MVector vec = mmat.getTranslation(tSpace);
    oSampleList.push_back(vec.x);
    oSampleList.push_back(vec.y);
    oSampleList.push_back(vec.z);

    vec = mmat.rotatePivotTranslation(tSpace);
    oSampleList.push_back(vec.x);
    oSampleList.push_back(vec.y);
    oSampleList.push_back(vec.z);

    MPoint pt = mmat.rotatePivot(tSpace);
    oSampleList.push_back(pt.x);
    oSampleList.push_back(pt.y);
    oSampleList.push_back(pt.z);

    double rot[3];
    MTransformationMatrix::RotationOrder order;
    mmat.getRotation(rot, order);
    oSampleList.push_back(rot[0]);
    oSampleList.push_back(rot[1]);
    oSampleList.push_back(rot[2]);

    MQuaternion quat = mmat.rotationOrientation();
    vec = quat.asEulerRotation().asVector();
    oSampleList.push_back(vec.x);
    oSampleList.push_back(vec.y);
    oSampleList.push_back(vec.z);

    pt = mmat.scalePivotTranslation(tSpace);
    oSampleList.push_back(vec.x);
    oSampleList.push_back(vec.y);
    oSampleList.push_back(vec.z);

    vec = mmat.scalePivot(tSpace);
    oSampleList.push_back(vec.x);
    oSampleList.push_back(vec.y);
    oSampleList.push_back(vec.z);

    mmat.scalePivot(tSpace);
    double shear[3];
    mmat.getShear(shear, tSpace);
    oSampleList.push_back(shear[0]);
    oSampleList.push_back(shear[1]);
    oSampleList.push_back(shear[2]);

    double scale[3];
    mmat.getScale(scale, tSpace);
    oSampleList.push_back(scale[0]);
    oSampleList.push_back(scale[1]);
    oSampleList.push_back(scale[2]);
}

void read(double iFrame, Alembic::AbcGeom::IXform & iNode,
    std::vector<double> & oSampleList)
{
    int64_t index, ceilIndex;
    double alpha = getWeightAndIndex(iFrame,
        iNode.getSchema().getTimeSampling(), index, ceilIndex);

    if (alpha != 0 && index != ceilIndex &&
        iNode.getSchema().getNumAnimSamples() > 0)
    {
        Alembic::Abc::DoubleArraySamplePtr loPtr =
            iNode.getSchema().getAnimData(
                Alembic::Abc::ISampleSelector(index));

        Alembic::Abc::DoubleArraySamplePtr hiPtr =
            iNode.getSchema().getAnimData(
                Alembic::Abc::ISampleSelector(ceilIndex));

        size_t numAnim = loPtr->size();
        oSampleList.resize(numAnim);
        for (size_t i = 0; i < numAnim; ++i)
        {
            oSampleList[i] = simpleLerp<double>(alpha,(*loPtr)[i], (*hiPtr)[i]);
        }
    }
    else if (iNode.getSchema().getNumAnimSamples() > 0)
    {
        Alembic::Abc::DoubleArraySamplePtr valPtr =
            iNode.getSchema().getAnimData(
                Alembic::Abc::ISampleSelector(index));

        size_t numAnim = valPtr->size();
        oSampleList.resize(numAnim);
        for (size_t i = 0; i < numAnim; ++i)
        {
            oSampleList[i] = (*valPtr)[i];
        }
    }
}

bool isComplex(Alembic::AbcGeom::IXform & iNode)
{
    Alembic::AbcGeom::XformOpVec opVec = iNode.getSchema().getOps();
    Alembic::AbcGeom::XformOpVec::iterator i = opVec.begin();
    Alembic::AbcGeom::XformOpVec::iterator iEnd = opVec.end();
    Alembic::Abc::DoubleArraySamplePtr staticPtr =
        iNode.getSchema().getStaticData();
    size_t statPos = 0;
    int state = 0;


    bool scPivot = false;
    bool roPivot = false;
    bool xAxis = false;
    bool yAxis = false;
    bool zAxis = false;

    for (; i != iEnd; ++i)
    {
        switch (i->getType())
        {
            case Alembic::AbcGeom::kScaleOperation:
            {
                if (state < 12)
                {
                    state = 12;
                }
                else
                    return true;
            }
            break;

            case Alembic::AbcGeom::kTranslateOperation:
            {
                switch (i->getHint())
                {
                    case Alembic::AbcGeom::kTranslateHint:
                    {
                        if (state < 1)
                            state = 1;
                        else
                            return true;
                    }
                    break;

                    case Alembic::AbcGeom::kScalePivotPointHint:
                    {
                        if (state < 10 && !scPivot)
                        {
                            scPivot = true;
                            state = 10;
                        }
                        // we have encounted this pivot before,
                        // this one undoes the first one
                        else if (state < 13 && scPivot)
                        {
                            scPivot = false;
                            state = 13;
                        }
                        else
                            return true;
                    }
                    break;

                    case Alembic::AbcGeom::kScalePivotTranslationHint:
                    {
                        if (state < 9)
                            state = 9;
                        else
                            return true;
                    }
                    break;

                    case Alembic::AbcGeom::kRotatePivotPointHint:
                    {
                        if (state < 3 && !roPivot)
                        {
                            roPivot = true;
                            state = 3;
                        }
                        // we have encounted this pivot before,
                        // this one undoes the first one
                        else if (state < 8 && roPivot)
                        {
                            roPivot = false;
                            state = 8;
                        }
                        else
                            return true;
                    }
                    break;

                    case Alembic::AbcGeom::kRotatePivotTranslationHint:
                    {
                        if (state < 2)
                            state = 2;
                        else
                            return true;
                    }
                    break;

                    default:
                        return true;
                    break;
                }

            }
            break;

            case Alembic::AbcGeom::kRotateOperation:
            {
                // if any axis is animated, we have a complex xform
                if (i->isXAnimated() || i->isYAnimated() || i->isZAnimated())
                    return true;

                double x = (*staticPtr)[statPos];
                double y = (*staticPtr)[statPos + 1];
                double z = (*staticPtr)[statPos + 2];

                switch (i->getHint())
                {
                    case Alembic::AbcGeom::kRotateHint:
                    {
                        if (x == 1 && y == 0 && z == 0 && !xAxis && state <= 4)
                        {
                            state = 4;
                            xAxis = true;
                        }
                        else if (x == 0 && y == 1 && z == 0 && !yAxis && 
                            state <= 4)
                        {
                            state = 4;
                            yAxis = true;
                        }
                        else if (x == 0 && y == 0 && z == 1 && !zAxis && 
                            state <= 4)
                        {
                            state = 4;
                            zAxis = true;
                        }
                        else
                            return true;
                    }
                    break;

                    case Alembic::AbcGeom::kRotateOrientationHint:
                    {
                        if (x == 1 && y == 0 && z == 0 && state < 7)
                        {
                            state = 7;
                        }
                        else if (x == 0 && y == 1 && z == 0 && state < 6)
                        {
                            state = 6;
                        }
                        else if (x == 0 && y == 0 && z == 1 && state < 5)
                        {
                            state = 5;
                        }
                        else
                            return true;
                    }
                    break;

                    default:
                        return true;
                    break;
                }
            }
            break;

            case Alembic::AbcGeom::kMatrixOperation:
            {
                if (i->getHint() == Alembic::AbcGeom::kMayaShearHint &&
                    state < 11)
                {
                    state = 11;
                }
                else
                    return true;
            }
            break;

            default:
            {
                return true;
            }
            break;
        } // switch

        // increment the static data position in case we come across any
        // rotations since we need to check the rotate axis
        Alembic::Util::uint8_t numIndices = i->getNumIndices();
        for (Alembic::Util::uint8_t j = 0; j < numIndices; ++j)
        {
            if (!i->isIndexAnimated(j))
                statPos++;
        }

    } // for i

    // if we encounter a scale or rotate pivot point,
    // it needs to be inverted again otherwise the matrix is complex
    return scPivot || roPivot;
}

MStatus create(double iFrame, Alembic::AbcGeom::IXform & iNode,
    MObject & iParent, MObject & ioObject,
    std::vector<std::string> & iSampledPropNameList,
    std::vector<std::string> & iSampledTransOpNameList,
    bool & oIsComplex, bool iSwap)
{
    MStatus status = MS::kSuccess;
    MString name(iNode.getName().c_str());

    MFnTransform trans;

    if (iSwap)
    {
        trans.setObject(ioObject);

        // disconnect and reset all the native attributes

        const MVector   vec(0, 0, 0);
        const MPoint    point(0, 0, 0);
        const double    zero[3] = {0, 0, 0};

        MPlug dstPlug;

        dstPlug = trans.findPlug("translateX");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("translateY");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("translateZ");
        disconnectAllPlugsTo(dstPlug);
        trans.setTranslation(vec, gSpace);

        dstPlug = trans.findPlug("scaleX");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("scaleY");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("scaleZ");
        disconnectAllPlugsTo(dstPlug);
        const double scale[3] = {1, 1, 1};
        trans.setScale(scale);

        dstPlug = trans.findPlug("shearXY");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("shearXZ");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("shearYZ");
        disconnectAllPlugsTo(dstPlug);
        trans.setShear(zero);

        dstPlug = trans.findPlug("rotateX");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("rotateY");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("rotateZ");
        disconnectAllPlugsTo(dstPlug);
        MTransformationMatrix::RotationOrder order =
            MTransformationMatrix::kXYZ;
        trans.setRotation(zero, order);

        dstPlug = trans.findPlug("scalePivotX");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("scalePivotY");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("scalePivotZ");
        disconnectAllPlugsTo(dstPlug);
        trans.setScalePivot(point, gSpace, gBalance);

        dstPlug = trans.findPlug("scalePivotTranslateX");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("scalePivotTranslateY");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("scalePivotTranslateZ");
        disconnectAllPlugsTo(dstPlug);
        trans.setScalePivotTranslation(vec, gSpace);

        dstPlug = trans.findPlug("rotatePivotX");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("rotatePivotY");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("rotatePivotZ");
        disconnectAllPlugsTo(dstPlug);
        trans.setScalePivot(point, gSpace, gBalance);

        dstPlug = trans.findPlug("rotatePivotTranslateX");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("rotatePivotTranslateY");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("rotatePivotTranslateZ");
        disconnectAllPlugsTo(dstPlug);
        trans.setScalePivotTranslation(vec, gSpace);

        dstPlug = trans.findPlug("rotateAxisX");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("rotateAxisY");
        disconnectAllPlugsTo(dstPlug);
        dstPlug = trans.findPlug("rotateAxisZ");
        disconnectAllPlugsTo(dstPlug);
        const MQuaternion quat;
        trans.setRotateOrientation(quat, gSpace, gBalance);

        dstPlug = trans.findPlug("rotateOrder");
        disconnectAllPlugsTo(dstPlug);
        bool reorder = true;
        trans.setRotationOrder(order, reorder);

        dstPlug = trans.findPlug("inheritsTransform");
        disconnectAllPlugsTo(dstPlug);
        dstPlug.setBool(true);

        // disconnect connections to animated props
        /*
        loop over properties
        {
            std::string propName = propIter->first;
            // SPT_xxColor is special
            if ( propName.find("SPT_") != std::string::npos
                && propName.find("Color") != std::string::npos )
            {
                std::string colorR = propName+std::string("R");
                dstPlug = trans.findPlug(colorR.c_str());
                disconnectAllPlugsTo(dstPlug);
                std::string colorG = propName+std::string("G");
                dstPlug = trans.findPlug(colorG.c_str());
                disconnectAllPlugsTo(dstPlug);
                std::string colorB = propName+std::string("B");
                dstPlug = trans.findPlug(colorB.c_str());
                disconnectAllPlugsTo(dstPlug);
            }
            else
            {
                dstPlug = trans.findPlug(propIter->first.c_str());
                disconnectAllPlugsTo(dstPlug);
            }
            propIter++;
        }
        addProperties(iFrame, *iNode, ioObject, iSampledPropNameList);
        */
    }
    else
    {
        ioObject = trans.create(iParent, &status);
        if (status != MS::kSuccess)
        {
            MString theError("Failed to create transform node ");
            theError += name;
            printError(theError);
            oIsComplex = false;
            ioObject = MObject::kNullObj;
            return status;
        }

        trans.setName(name);

        MPlug dstPlug;
        dstPlug = trans.findPlug("inheritsTransform");
        if (!dstPlug.isNull())
        {
            dstPlug.setBool( iNode.getSchema().inherits(
                Alembic::Abc::ISampleSelector(iFrame,
                    Alembic::Abc::ISampleSelector::kNearIndex)) );
        }

        //addProperties(iFrame, iNode, ioObject, iSampledPropNameList);
    }

    oIsComplex = isComplex(iNode);

    int64_t index, ceilIndex;
    double alpha = getWeightAndIndex(iFrame,
        iNode.getSchema().getTimeSampling(), index, ceilIndex);

    if (oIsComplex)
    {

        if (iNode.getSchema().getNumAnimSamples() > 0)
        {
            iSampledTransOpNameList.push_back("translateX");
            iSampledTransOpNameList.push_back("translateY");
            iSampledTransOpNameList.push_back("translateZ");
            iSampledTransOpNameList.push_back("rotatePivotTranslateX");
            iSampledTransOpNameList.push_back("rotatePivotTranslateY");
            iSampledTransOpNameList.push_back("rotatePivotTranslateZ");
            iSampledTransOpNameList.push_back("rotatePivotX");
            iSampledTransOpNameList.push_back("rotatePivotY");
            iSampledTransOpNameList.push_back("rotatePivotZ");
            iSampledTransOpNameList.push_back("rotateX");
            iSampledTransOpNameList.push_back("rotateY");
            iSampledTransOpNameList.push_back("rotateZ");
            iSampledTransOpNameList.push_back("rotateAxisX");
            iSampledTransOpNameList.push_back("rotateAxisY");
            iSampledTransOpNameList.push_back("rotateAxisZ");
            iSampledTransOpNameList.push_back("scalePivotTranslateX");
            iSampledTransOpNameList.push_back("scalePivotTranslateY");
            iSampledTransOpNameList.push_back("scalePivotTranslateZ");
            iSampledTransOpNameList.push_back("scalePivotX");
            iSampledTransOpNameList.push_back("scalePivotY");
            iSampledTransOpNameList.push_back("scalePivotZ");
            iSampledTransOpNameList.push_back("shearXY");
            iSampledTransOpNameList.push_back("shearXZ");
            iSampledTransOpNameList.push_back("shearYZ");
            iSampledTransOpNameList.push_back("scaleX");
            iSampledTransOpNameList.push_back("scaleY");
            iSampledTransOpNameList.push_back("scaleZ");
        }

        Alembic::Abc::M44d m; 

        if (alpha != 0.0 && index != ceilIndex)
        {
            Alembic::Abc::M44d mlo = iNode.getSchema().getMatrix(
                Alembic::Abc::ISampleSelector(index));

            Alembic::Abc::M44d mhi = iNode.getSchema().getMatrix(
                Alembic::Abc::ISampleSelector(ceilIndex));

            m = ((1 - alpha) * mlo) + (alpha * mhi);
        }
        else
        {
            m = iNode.getSchema().getMatrix(
                Alembic::Abc::ISampleSelector(index));
        }

        MTransformationMatrix mmat(MMatrix(m.x));

        // Everywhere else we use kPreTransform, but this doesn't work
        // when we pass in the matrix for some reason
        MSpace::Space tSpace = MSpace::kTransform;

        trans.setTranslation(mmat.getTranslation(tSpace), tSpace);

        trans.setRotatePivotTranslation(
            mmat.rotatePivotTranslation(tSpace), tSpace);

        trans.setRotatePivot(
            mmat.rotatePivot(tSpace), tSpace, gBalance);

        trans.setRotation(mmat.eulerRotation());

        trans.setRotateOrientation(
            mmat.rotationOrientation(), tSpace, gBalance);

        trans.setScalePivotTranslation(
            mmat.scalePivotTranslation(tSpace), tSpace);

        trans.setScalePivot(
            mmat.scalePivot(tSpace), tSpace, gBalance);

        double shear[3];
        mmat.getShear(shear, tSpace);
        trans.setShear(shear);

        double scale[3];
        mmat.getScale(scale, tSpace);
        trans.setScale(scale);
        return status;
    }

    std::vector<double> animValue;
    Alembic::Abc::DoubleArraySamplePtr staticPtr =
        iNode.getSchema().getStaticData();

    read(iFrame, iNode, animValue);

    bool scPivot = false;
    bool roPivot = false;
    size_t statPos = 0;
    size_t animPos = 0;

    MTransformationMatrix::RotationOrder rotOrder[2];
    rotOrder[0] = MTransformationMatrix::kInvalid;
    rotOrder[1] = MTransformationMatrix::kInvalid;

    Alembic::AbcGeom::XformOpVec opVec = iNode.getSchema().getOps();
    Alembic::AbcGeom::XformOpVec::iterator op = opVec.begin();
    Alembic::AbcGeom::XformOpVec::iterator opEnd = opVec.end();

    for (; op != opEnd; ++op)
    {
        switch (op->getType())
        {
            case Alembic::AbcGeom::kScaleOperation:
            {
                double scale[3];

                if (op->isXAnimated())
                {
                    iSampledTransOpNameList.push_back("scaleX");
                    scale[0] = animValue[animPos++];
                }
                else
                {
                    scale[0] = (*staticPtr)[statPos++];
                }

                if (op->isYAnimated())
                {
                    iSampledTransOpNameList.push_back("scaleY");
                    scale[1] = animValue[animPos++];
                }
                else
                {
                    scale[1] = (*staticPtr)[statPos++];
                }

                if (op->isZAnimated())
                {
                    iSampledTransOpNameList.push_back("scaleZ");
                    scale[2] = animValue[animPos++];
                }
                else
                {
                    scale[2] = (*staticPtr)[statPos++];
                }
                trans.setScale(scale);
            }
            break;

            case Alembic::AbcGeom::kRotateOperation:
            {
                double x = (*staticPtr)[statPos++];
                double y = (*staticPtr)[statPos++];
                double z = (*staticPtr)[statPos++];
                double angle = 0.0;

                if (op->getHint() == Alembic::AbcGeom::kRotateHint)
                {
                    if (x == 1 && y == 0 && z == 0)
                    {
                        if (op->isAngleAnimated())
                        {
                            iSampledTransOpNameList.push_back("rotateX");
                            angle = animValue[animPos++];
                        }
                        else
                        {
                            angle = (*staticPtr)[statPos++];
                        }

                        // we have encountered the first rotation, set it
                        // to the 2 X possibilities
                        if (rotOrder[0] == MTransformationMatrix::kInvalid)
                        {
                            rotOrder[0] = MTransformationMatrix::kYZX;
                            rotOrder[1] = MTransformationMatrix::kZYX;
                        }
                        // we have filled in the two possibilities, now choose
                        // which one we should use
                        else if (rotOrder[1] != MTransformationMatrix::kInvalid)
                        {
                            if (rotOrder[1] == MTransformationMatrix::kYXZ)
                            {
                                rotOrder[0] = rotOrder[1];
                            }

                            rotOrder[1] = MTransformationMatrix::kInvalid;
                        }
                    }
                    else if (x == 0 && y == 1 && z == 0)
                    {

                        if (op->isAngleAnimated())
                        {
                            iSampledTransOpNameList.push_back("rotateY");
                            angle = animValue[animPos++];
                        }
                        else
                        {
                            angle = (*staticPtr)[statPos++];
                        }

                        // we have encountered the first rotation, set it
                        // to the 2 X possibilities
                        if (rotOrder[0] == MTransformationMatrix::kInvalid)
                        {
                            rotOrder[0] = MTransformationMatrix::kZXY;
                            rotOrder[1] = MTransformationMatrix::kXZY;
                        }
                        // we have filled in the two possibilities, now choose
                        // which one we should use
                        else if (rotOrder[1] != MTransformationMatrix::kInvalid)
                        {
                            if (rotOrder[1] == MTransformationMatrix::kZYX)
                            {
                                rotOrder[0] = rotOrder[1];
                            }

                            rotOrder[1] = MTransformationMatrix::kInvalid;
                        }
                    }
                    else if (x == 0 && y == 0 && z == 1)
                    {
                        if (op->isAngleAnimated())
                        {
                            iSampledTransOpNameList.push_back("rotateZ");
                            angle = animValue[animPos++];
                        }
                        else
                        {
                            angle = (*staticPtr)[statPos++];
                        }

                        // we have encountered the first rotation, set it
                        // to the 2 X possibilities
                        if (rotOrder[0] == MTransformationMatrix::kInvalid)
                        {
                            rotOrder[0] = MTransformationMatrix::kXYZ;
                            rotOrder[1] = MTransformationMatrix::kYXZ;
                        }
                        // we have filled in the two possibilities, now choose
                        // which one we should use
                        else if (rotOrder[1] != MTransformationMatrix::kInvalid)
                        {
                            if (rotOrder[1] == MTransformationMatrix::kXZY)
                            {
                                rotOrder[0] = rotOrder[1];
                            }

                            rotOrder[1] = MTransformationMatrix::kInvalid;
                        }
                    }

                    MVector axis(x, y, z);
                    MQuaternion quat(angle, axis);
                    trans.rotateBy(quat, gSpace);
                }
                // kRotateOrientationHint
                else
                {
                    MQuaternion quat;

                    if (x == 1 && y == 0 && z == 0)
                    {
                        if (op->isAngleAnimated())
                        {
                            iSampledTransOpNameList.push_back("rotateAxisX");
                            angle = animValue[animPos++];
                        }
                        else
                        {
                            angle = (*staticPtr)[statPos++];
                        }

                        quat.setToXAxis(angle);

                    }
                    else if (x == 0 && y == 1 && z == 0)
                    {
                        if (op->isAngleAnimated())
                        {
                            iSampledTransOpNameList.push_back("rotateAxisY");
                            angle = animValue[animPos++];
                        }
                        else
                        {
                            angle = (*staticPtr)[statPos++];
                        }
                        quat.setToYAxis(angle);
                    }
                    else if (x == 0 && y == 0 && z == 1)
                    {
                        if (op->isAngleAnimated())
                        {
                            iSampledTransOpNameList.push_back("rotateAxisZ");
                            angle = animValue[animPos++];
                        }
                        else
                        {
                            angle = (*staticPtr)[statPos++];
                        }
                        quat.setToZAxis(angle);
                    }

                    MQuaternion curq = trans.rotateOrientation(gSpace);
                    trans.setRotateOrientation(curq*quat, gSpace, gBalance);
                }

            }
            break;

            // shear
            case Alembic::AbcGeom::kMatrixOperation:
            {
                double shear[3];

                // we only care about shearXY, shearXZ, and shearYZ here
                size_t otherVals[13] =
                    {0, 1, 2, 3, 5, 6, 7, 10, 11, 12, 13, 14, 15};

                for (size_t j = 0; j < 13; ++j)
                {
                    if (op->isIndexAnimated(otherVals[j]))
                        animPos ++;
                    else
                        statPos ++;
                }

                if (op->isIndexAnimated(4))
                {
                    iSampledTransOpNameList.push_back("shearXY");
                    shear[0] = animValue[animPos++];
                }
                else
                {
                    shear[0] = (*staticPtr)[statPos++];
                }

                if (op->isIndexAnimated(8))
                {
                    iSampledTransOpNameList.push_back("shearXZ");
                    shear[1] = animValue[animPos++];
                }
                else
                {
                    shear[1] = (*staticPtr)[statPos++];
                }

                if (op->isIndexAnimated(9))
                {
                    iSampledTransOpNameList.push_back("shearYZ");
                    shear[2] = animValue[animPos++];
                }
                else
                {
                    shear[2] = (*staticPtr)[statPos++];
                }

                trans.setShear(shear);
            }
            break;

            case Alembic::AbcGeom::kTranslateOperation:
            {
                Alembic::Util::uint8_t hint = op->getHint();
                switch (hint)
                {
                    case Alembic::AbcGeom::kTranslateHint:
                    {
                        MVector vec;

                        if (op->isXAnimated())
                        {
                            iSampledTransOpNameList.push_back("translateX");
                            vec.x = animValue[animPos++];
                        }
                        else
                        {
                            vec.x = (*staticPtr)[statPos++];
                        }

                        if (op->isYAnimated())
                        {
                            iSampledTransOpNameList.push_back("translateY");
                            vec.y = animValue[animPos++];
                        }
                        else
                        {
                            vec.y = (*staticPtr)[statPos++];
                        }

                        if (op->isZAnimated())
                        {
                            iSampledTransOpNameList.push_back("translateZ");
                            vec.z = animValue[animPos++];
                        }
                        else
                        {
                            vec.z = (*staticPtr)[statPos++];
                        }
                        trans.setTranslation(vec, gSpace);
                    }
                    break;

                    case Alembic::AbcGeom::kScalePivotPointHint:
                    {
                        MPoint point;
                        point.w = 1.0;

                        if (op->isXAnimated())
                        {
                            point.x = animValue[animPos++];
                            if (!scPivot)
                            {
                                iSampledTransOpNameList.push_back(
                                    "scalePivotX");
                            }
                            else
                            {
                                iSampledTransOpNameList.push_back(
                                    "scalePivotXInv");
                            }
                        }
                        else
                        {
                            point.x = (*staticPtr)[statPos++];
                        }

                        if (op->isYAnimated())
                        {
                            point.y = animValue[animPos++];
                            if (!scPivot)
                            {
                                iSampledTransOpNameList.push_back(
                                    "scalePivotY");
                            }
                            else
                            {
                                iSampledTransOpNameList.push_back(
                                    "scalePivotYInv");
                            }
                        }
                        else
                        {
                            point.y = (*staticPtr)[statPos++];
                        }

                        if (op->isZAnimated())
                        {
                            point.z = animValue[animPos++];
                            if (!scPivot)
                            {
                                iSampledTransOpNameList.push_back(
                                    "scalePivotZ");
                            }
                            else
                            {
                                iSampledTransOpNameList.push_back(
                                    "scalePivotZInv");
                            }
                        }
                        else
                        {
                            point.z = (*staticPtr)[statPos++];
                        }

                        // we only want to apply this to the first one
                        // the second one is the inverse
                        if (!scPivot)
                        {
                            trans.setScalePivot(point, gSpace, gBalance);
                        }

                        scPivot = !scPivot;
                    }
                    break;

                    case Alembic::AbcGeom::kScalePivotTranslationHint:
                    {
                        MVector vec;

                        if (op->isXAnimated())
                        {
                            iSampledTransOpNameList.push_back(
                                "scalePivotTranslateX");
                            vec.x = animValue[animPos++];
                        }
                        else
                        {
                            vec.x = (*staticPtr)[statPos++];
                        }

                        if (op->isYAnimated())
                        {
                            iSampledTransOpNameList.push_back(
                                "scalePivotTranslateY");
                            vec.y = animValue[animPos++];
                        }
                        else
                        {
                            vec.y = (*staticPtr)[statPos++];
                        }

                        if (op->isZAnimated())
                        {
                            iSampledTransOpNameList.push_back(
                                "scalePivotTranslateZ");
                            vec.z = animValue[animPos++];
                        }
                        else
                        {
                            vec.z = (*staticPtr)[statPos++];
                        }
                        trans.setScalePivotTranslation(vec, gSpace);
                    }
                    break;

                    case Alembic::AbcGeom::kRotatePivotPointHint:
                    {
                        MPoint point;
                        point.w = 1.0;

                        if (op->isXAnimated())
                        {
                            point.x = animValue[animPos++];
                            if (!roPivot)
                            {
                                iSampledTransOpNameList.push_back(
                                    "rotatePivotX");
                            }
                            else
                            {
                                iSampledTransOpNameList.push_back(
                                    "rotatePivotXInv");
                            }
                        }
                        else
                        {
                            point.x = (*staticPtr)[statPos++];
                        }

                        if (op->isYAnimated())
                        {
                            point.y = animValue[animPos++];
                            if (!roPivot)
                            {
                                iSampledTransOpNameList.push_back(
                                    "rotatePivotY");
                            }
                            else
                            {
                                iSampledTransOpNameList.push_back(
                                    "rotatePivotYInv");
                            }
                        }
                        else
                        {
                            point.y = (*staticPtr)[statPos++];
                        }

                        if (op->isZAnimated())
                        {
                            point.z = animValue[animPos++];
                            if (!roPivot)
                            {
                                iSampledTransOpNameList.push_back(
                                    "rotatePivotZ");
                            }
                            else
                            {
                                iSampledTransOpNameList.push_back(
                                    "rotatePivotZInv");
                            }
                        }
                        else
                        {
                            point.z = (*staticPtr)[statPos++];
                        }

                        // only set rotate pivot on the first one, the second
                        // one is just the inverse
                        if (!roPivot)
                            trans.setRotatePivot(point, gSpace, gBalance);

                        roPivot = !roPivot;
                    }
                    break;

                    case Alembic::AbcGeom::kRotatePivotTranslationHint:
                    {
                        MVector vec;

                        if (op->isXAnimated())
                        {
                            iSampledTransOpNameList.push_back(
                                "rotatePivotTranslateX");
                            vec.x = animValue[animPos++];
                        }
                        else
                        {
                            vec.x = (*staticPtr)[statPos++];
                        }

                        if (op->isYAnimated())
                        {
                            iSampledTransOpNameList.push_back(
                                "rotatePivotTranslateY");
                            vec.y = animValue[animPos++];
                        }
                        else
                        {
                            vec.y = (*staticPtr)[statPos++];
                        }

                        if (op->isZAnimated())
                        {
                            iSampledTransOpNameList.push_back(
                                "rotatePivotTranslateZ");
                            vec.z = animValue[animPos++];
                        }
                        else
                        {
                            vec.z = (*staticPtr)[statPos++];
                        }
                        trans.setRotatePivotTranslation(vec, gSpace);
                    }
                    break;

                    default:
                    break;
                }
            }
            break;

            default:
            break;
        }

    }

    // no rotates were found so default to XYZ
    if (rotOrder[0] == MTransformationMatrix::kInvalid)
    {
        rotOrder[0] =  MTransformationMatrix::kXYZ;
    }

    trans.setRotationOrder(rotOrder[0], true);

    return status;
}
