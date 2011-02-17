//-*****************************************************************************
//
// Copyright (c) 2009-2010,
//  Sony Pictures Imageworks Inc. and
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
// Industrial Light & Magic, nor the names of their contributors may be used
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

#ifndef _Alembic_AbcCoreHDF5_WrittenSampleMap_h_
#define _Alembic_AbcCoreHDF5_WrittenSampleMap_h_

#include <Alembic/AbcCoreAbstract/DataSampleKey.h>
#include <Alembic/AbcCoreHDF5/Foundation.h>
#include <Alembic/AbcCoreHDF5/HDF5Util.h>


namespace Alembic {
namespace AbcCoreHDF5 {

//-*****************************************************************************
// A Written Sample ID is a receipt that contains information that
// refers to the exact location in an HDF5 file that a data sample was written
// to. It also contains the Key of the data sample, so it may be verified.
//
// This object is used to "reuse" an already written data sample by linking
// it from the previous usage.
//-*****************************************************************************
class WrittenSampleID
{
public:
    WrittenSampleID()
      : m_sampleKey() {}

    WrittenSampleID( const AbcA::DataSample::Key &iKey, hid_t iObjLocID )
      : m_sampleKey( iKey )
    {
        int strLen =  H5Iget_name( iObjLocID, NULL, 0  );
        ABCA_ASSERT( strLen > 0, "WrittenSampleID() passed in bad iObjLocID" );

        // add 1 to account for the NULL seperator
        strLen ++;

        m_objectLocation.resize( strLen );
        H5Iget_name( iObjLocID, &(m_objectLocation[0]), strLen );
    }

    const AbcA::DataSample::Key &getKey() const { return m_sampleKey; }

    std::string getObjectLocation() const { return m_objectLocation; }

private:
    AbcA::DataSample::Key m_sampleKey;
    std::string m_objectLocation;
};

//-*****************************************************************************
typedef boost::shared_ptr<WrittenSampleID> WrittenSampleIDPtr;

//-*****************************************************************************
// This class handles the mapping.
class WrittenSampleMap
{
protected:
    friend class AwImpl;

    WrittenSampleMap() {}

public:

    // Returns 0 if it can't find it
    WrittenSampleIDPtr find( const AbcA::DataSample::Key &key ) const
    {
        Map::const_iterator miter = m_map.find( key );
        if ( miter != m_map.end() )
        {
            return (*miter).second;
        }
        else
        {
            return WrittenSampleIDPtr();
        }
    }

    // Store. Will clobber if you've already stored it.
    void store( WrittenSampleIDPtr r )
    {
        if ( !r )
        {
            ABCA_THROW( "Invalid WrittenSampleIDPtr" );
        }

        m_map[r->getKey()] = r;
    }

protected:
    typedef AbcA::UnorderedMapUtil<WrittenSampleIDPtr>::umap_type Map;
    Map m_map;
};

} // End namespace AbcCoreHDF5
} // End namespace Alembic

#endif
