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

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/Abc/Tests/Assert.h>

using namespace Alembic::AbcGeom;

//-*****************************************************************************
void xformOut()
{
    OArchive archive( Alembic::AbcCoreHDF5::WriteArchive(), "Xform1.abc" );
    OXform a( OObject( archive, kTop ), "a" );
    OXform b( a, "b" );
    OXform c( b, "c" );
    OXform d( c, "d" );

    XformOpVec aVec;
    XformOp op(cTranslateOperation, cTranslate);

    std::vector <double> valVec;
    valVec.push_back(12.0);  // translate x
    valVec.push_back(20.0);  // translate z
    Abc::DoubleArraySample data(valVec);

    op.setYAnimated(true);
    aVec.push_back(op);
    a.getSchema().setXform( aVec, data);

    for (size_t i = 0; i < 20; ++i)
    {
        valVec.clear();
        valVec.push_back(i+42.0);
        data = Abc::DoubleArraySample(valVec);
        a.getSchema().set( data, OSampleSelector( i ) );
    }

    for (size_t i = 0; i < 20; ++i)
    {
        b.getSchema().setInherits( i&1, OSampleSelector( i ) );
    }

    // for c we write nothing

    XformOpVec dVec;
    op = XformOp(cScaleOperation, cScale);
    dVec.push_back(op);

    valVec.clear();
    valVec.push_back(3.0);
    valVec.push_back(6.0);
    valVec.push_back(9.0);
    data = Abc::DoubleArraySample(valVec);

    d.getSchema().setXform( dVec, data );
}

//-*****************************************************************************
void xformIn()
{
    IArchive archive( Alembic::AbcCoreHDF5::ReadArchive(),
                      "Xform1.abc" );
    IXform a( IObject( archive, kTop ), "a" );
    XformOpVec ops = a.getSchema().getOps();
    TESTING_ASSERT( ops.size() == 1 );
    TESTING_ASSERT( a.getSchema().getNumAnimSamples() == 20 );
    TESTING_ASSERT( a.getSchema().inherits() );
    for ( index_t i = 0; i < 20; ++i )
    {
        Abc::M44d mat = a.getSchema().getMatrix(Abc::ISampleSelector(i));
        TESTING_ASSERT( mat ==
            Abc::M44d().setTranslation( V3d(12.0, i+42.0, 20.0)) );
    }

    Abc::M44d identity;

    IXform b( a, "b" );
    TESTING_ASSERT( b.getSchema().getOps().size() == 0 );
    TESTING_ASSERT( b.getSchema().getMatrix() == identity );

    IXform c( b, "c" );
    TESTING_ASSERT( c.getSchema().getOps().size() == 0 );
    TESTING_ASSERT( c.getSchema().getMatrix() == identity );
    TESTING_ASSERT( c.getSchema().inherits() );

    IXform d( c, "d" );
    TESTING_ASSERT( d.getSchema().getOps().size() == 1 );
    TESTING_ASSERT( d.getSchema().inherits() );
}

//-*****************************************************************************
int main( int argc, char *argv[] )
{
    xformOut();
    xformIn();

    return 0;
}
