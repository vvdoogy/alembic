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

#include <Alembic/AbcGeom/OXform.h>
#include <Alembic/AbcGeom/GeometryScope.h>

namespace Alembic {
namespace AbcGeom {

//-*****************************************************************************
void OXformSchema::setXform( const XformOpVec & iOp,
    const Abc::DoubleArraySamplePtr iStatic )
{
    ALEMBIC_ABC_SAFE_CALL_BEGIN( "OXformSchema::setXform()" );

    ABCA_ASSERT( !m_xform,
        "Xform operations have already been written." );

    XformOpVec::const_iterator it = iOp.begin();
    XformOpVec::const_iterator itEnd = iOp.end();

    std::vector < uint32_t > data(iOp.size());
    size_t i = 0;
    size_t numStatic = 0;
    for ( ; it != itEnd; ++it, ++i )
    {
        size_t numChannels = it->getNumIndices();
        for ( size_t j = 0; j < numChannels; ++j )
        {
            if ( it->isIndexAnimated(j) )
            {
                m_numAnimated ++;
            }
            else
            {
                numStatic ++;
            }
        }

        data[i] = it->getEncodedValue();
    }

    ABCA_ASSERT( numStatic == iStatic->size(),
        "Not enough static data provided in OXformSchema::setXform");

    if ( !data.empty() )
    {
        OUInt32ArrayProperty ops( *this, ".ops" );
        ops.set(data);

        Abc::ODoubleArrayProperty staticData( *this, ".static" );
        staticData.set(*iStatic);
    }

    m_xform = true;
    ALEMBIC_ABC_SAFE_CALL_END();
}

//-*****************************************************************************
void OXformSchema::set( const Sample &iSamp, const Abc::OSampleSelector &iSS  )
{
    ALEMBIC_ABC_SAFE_CALL_BEGIN( "OXformSchema::set()" );

    ABCA_ASSERT( m_xform,
        "Must write xform operations before writing animated samples." );

    size_t animSize = iSamp.getChannels().size();
    ABCA_ASSERT( m_numAnimated == animSize, "Sample doesn't have enough data.");

    if ( iSS.getIndex() == 0 )
    {
        m_anim.set( iSamp.getChannels(), iSS );
        m_inherits.set( iSamp.inheritsTransform(), iSS );
    }
    else
    {
        SetPropUsePrevIfNull( m_anim, iSamp.getChannels(), iSS );
        SetPropUsePrevIfNull( m_inherits, iSamp.inheritsTransform(), iSS );
    }

    ALEMBIC_ABC_SAFE_CALL_END();
}

//-*****************************************************************************
void OXformSchema::setFromPrevious( const Abc::OSampleSelector &iSS )
{
    ALEMBIC_ABC_SAFE_CALL_BEGIN( "OXformSchema::setFromPrevious" );

    m_anim.setFromPrevious( iSS );
    m_inherits.setFromPrevious( iSS );

    ALEMBIC_ABC_SAFE_CALL_END();
}

//-*****************************************************************************
void OXformSchema::init( const AbcA::TimeSamplingType &iTst )
{
    ALEMBIC_ABC_SAFE_CALL_BEGIN( "OXformSchema::init()" );

    m_anim = Abc::ODoubleArrayProperty( *this, ".anim", iTst );
    m_inherits = Abc::OBoolProperty( *this, ".inherits", iTst );

    ALEMBIC_ABC_SAFE_CALL_END_RESET();
}

} // End namespace AbcGeom
} // End namespace Alembic
