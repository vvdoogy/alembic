//-*****************************************************************************
//
// Copyright (c) 2009-2010,
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

#ifndef _Alembic_AbcGeom_XformOp_h_
#define _Alembic_AbcGeom_XformOp_h_

#include <Alembic/AbcGeom/Foundation.h>

namespace Alembic {
namespace AbcGeom {


//! \brief Enum that indicates the type of transformational operation.
//! This enum is used when encoding and decoding the transform operation data.
enum XformOperationType
{
    cScaleOperation = 0,
    cTranslateOperation = 1,
    cRotateOperation = 2,
    cMatrixOperation = 3
};


//! \brief The Matrix identifier hint.
//! Some 3d packages (like Maya) may have certain transformation operations
//! that aren't supported in other packages.  MatrixHint is meant to 
//! help with reading back into applications that natively support the type.
enum MatrixHint
{
    //! Regular Matrix
    cMatrix = 0,

    //! Matrix represents Maya's version of Shear 
    cMayaShear = 1
};

//! \brief The Rotate identifier hint.
//! Some 3d packages (like Maya) have multiple rotation operations
//! that are mathmatically of the same type.  RotateHint is meant to 
//! help disambiguate these similiar mathmatical types when reading back
//! into applications that natively support the type.
enum RotateHint
{
    //! Default regular rotation
    cRotate = 0,

    //! Rotation that goes along with the rotate pivot to help
    //! orient the local rotation space.
    cRotateOrientation = 1
};

//! \brief The Scale identifier hint.
//! Some 3d packages (like Maya) have multiple transformation operations
//! that are mathmatically of the same type.  ScaleHint is meant to 
//! help disambiguate these similiar mathmatical types when reading back
//! into applications that natively support that type.
enum ScaleHint
{
    //! Default, regular scale.
    cScale = 0
};


//! \brief The Translation identifier hint.
//! Some 3d packages (like Maya) have multiple transformation operations
//! that are mathmatically of the same type.  TranslateHint is meant to 
//! help disambiguate these similiar mathmatical types when reading back
//! into applications that natively support the type.
enum TranslateHint
{
    //! Default, regular translation.
    cTranslate = 0,

    //! Translation used for scaling around a pivot point.
    cScalePivotPoint = 1,

    //! Translation which is used to help preserve existing scale
    //! transformations when moving the pivot.
    cScalePivotTranslation = 2,

    //! Translation used for rotating around the pivot point.
    cRotatePivotPoint = 3,

    //! Translation which is used to help preserve existing rotate
    //! transformations when moving the pivot.
    cRotatePivotTranslation = 4
};

class XformOp
{
public:
    XformOp();
    XformOp(XformOperationType iType, Alembic::Util::uint8_t iHint);

    XformOperationType getType() const;
    void setType(XformOperationType iType);

    Alembic::Util::uint8_t getHint() const;
    void setHint(Alembic::Util::uint8_t iHint);

    bool isXAnimated() const;
    void setXAnimated(bool iAnim);

    bool isYAnimated() const;
    void setYAnimated(bool iAnim);

    bool isZAnimated() const;
    void setZAnimated(bool iAnim);

    bool isAngleAnimated() const;
    void setAngleAnimated(bool iAnim);

    bool isIndexAnimated(Alembic::Util::uint8_t iIndex) const;
    void setIndexAnimated(Alembic::Util::uint8_t iIndex, bool iAnim);

    Alembic::Util::uint8_t getNumIndices() const;
    Alembic::Util::uint32_t getEncodedValue() const;
    void setEncodedValue(Alembic::Util::uint32_t iVal);

private:
    XformOperationType m_type;
    Alembic::Util::uint16_t m_anim;
    Alembic::Util::uint8_t m_hint;
};

typedef std::vector < XformOp > XformOpVec;

} // End namespace AbcGeom
} // End namespace Alembic

#endif
