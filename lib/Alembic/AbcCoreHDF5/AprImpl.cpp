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

#include <Alembic/AbcCoreHDF5/AprImpl.h>

namespace Alembic {
namespace AbcCoreHDF5 {

//-*****************************************************************************
AprImpl( AbcA::CompoundPropertyReaderPtr iParent, hid_t iParentGroup,
    PropertyHeaderPtr iHeader )
    : m_parent( iParent )
    , m_parentGroup( iParentGroup )
    , m_header( iHeader )
    , m_fileDataType( -1 )
    , m_cleanFileDataType( false )
    , m_nativeDataType( -1 )
    , m_cleanNativeDataType( false )
    , m_timeSample0( 0.0 )
    , m_samplesIGroup( -1 )
{
    // Validate all inputs.
    ABCA_ASSERT( m_parent, "Invalid parent" );
    ABCA_ASSERT( m_parentGroup >= 0, "Invalid parent group" );
    ABCA_ASSERT( m_header, "Invalid header" );
    ABCA_ASSERT( m_header->getPropertyType() != AbcA::kCompoundProperty,
        "Tried to create a simple property with a compound header" );

    if ( m_header->getPropertyType() != AbcA::kDataProperty )
    {
        ABCA_THROW( "Attempted to create a DataPropertyReader from a non-array"
            " property type" );
    }

    // Get data types
    PlainOldDataType POD = m_header->getDataType().getPod();
    if ( POD != kStringPOD && POD != kWstringPOD )
    {
        m_fileDataType = GetFileH5T( m_header->getDataType(),
                                     m_cleanFileDataType );
        m_nativeDataType = GetNativeH5T( m_header->getDataType(),
                                         m_cleanNativeDataType );
    }

    // Get our name.
    const std::string &myName = m_header->getName();

    // Read the num samples.
    m_numSamples = 0;
    m_numUniqueSamples = 0;
    uint32_t numSamples32 = 0;
    uint32_t numUniqueSamples32 = 0;
    m_isScalar = false;  // look for the isScalar attr
    ReadNumSamples( m_parentGroup,
                    myName,
                    REMOVE_SCALAR,
                    numSamples32,
                    numUniqueSamples32 );
    m_numSamples = numSamples32;
    m_numUniqueSamples = numUniqueSamples32;

    // Validate num unique samples.
    ABCA_ASSERT( m_numUniqueSamples <= m_numSamples,
                 "Corrupt numUniqueSamples: " << m_numUniqueSamples
                 << "in property: " << myName
                 << " which has numSamples: " << m_numSamples );

}

//-*****************************************************************************
AbcA::DataPropertyReaderPtr AprImpl::asDataPtr()
{
    return shared_from_this();
}


//-*****************************************************************************
const AbcA::PropertyHeader & AprImpl::getHeader() const
{
    ABCA_ASSERT( m_header, "Invalid header" );
    return *m_header;
}

//-*****************************************************************************
AbcA::ObjectReaderPtr AprImpl::getObject()
{
    ABCA_ASSERT( m_parent, "Invalid parent" );
    return m_parent->getObject();
}

//-*****************************************************************************
AbcA::CompoundPropertyReaderPtr AprImpl::getParent()
{
    ABCA_ASSERT( m_parent, "Invalid parent" );
    return m_parent;
}

//-*****************************************************************************
size_t AprImpl::getNumSamples()
{
    return ( size_t )m_numSamples;
}

//-*****************************************************************************
bool AprImpl::isConstant()
{
    return ( m_numUniqueSamples < 2 );
}

//-*****************************************************************************
bool AprImpl::isScalar()
{
    return m_isScalar;
}

//-*****************************************************************************
// This class reads the time sampling on demand.
AbcA::TimeSampling AprImpl::getTimeSampling()
{
    //-*************************************************************************
    // Read the time samples as an array ptr
    // check their sizes, convert to times, create time sampling ptr.
    // whew.
    const std::string &myName = m_header->getName();

    // Read the array, possibly from the cache.
    // We are either a brand new shared_ptr <DataSample>
    // that owns the memory (and is now in the cache)
    // OR we are a ref_count ++ of the shared_ptr <DataSample>
    // found from the cache.
    // We'll create a TimeSamplingPtr and it will keep this
    // reference for us.
    AbcA::ArraySamplePtr timeSamples =
        AbcCoreHDF5::ReadTimeSamples( this->getObject()->getArchive()->
                         getReadArraySampleCachePtr(),
                         m_parentGroup, myName + ".time" );
    ABCA_ASSERT( timeSamples,
                 "Couldn't read time samples for attr: " << myName );

    // Check the byte sizes.
    const AbcA::TimeSamplingType &tst = m_header->getTimeSamplingType();
    uint32_t numExpectedTimeSamples =
        std::min( tst.getNumSamplesPerCycle(), m_numSamples );

    size_t gotNumTimes =
        timeSamples->getDimensions().numPoints();

    ABCA_ASSERT( numExpectedTimeSamples == 0 ||
                 numExpectedTimeSamples == gotNumTimes,
                 "Expected: " << numExpectedTimeSamples
                 << " time samples, but got: "
                 << gotNumTimes << " instead." );

    // Build a time sampling ptr.
    // m_timeSamplingPtr is shared_ptr to TimeSampling.
    // TimeSampling contains numSamples and handy accessors AND
    // the ArraySamplePtr of sampleTimes
    m_timeSamplingPtr.reset( new AbcA::TimeSampling( tst, m_numSamples,
                                                     timeSamples ) );

    AbcA::TimeSampling ret = *m_timeSamplingPtr;
    // And return it.
    return ret;
}

//-*****************************************************************************
void AprImpl::getSample( index_t iSampleIndex, AbcA::DataSamplePtr& oSample )
{
    // Verify sample index
    ABCA_ASSERT( iSampleIndex >= 0 &&
                 iSampleIndex < m_numSamples,
                 "Invalid sample index: " << iSampleIndex
                 << ", should be between 0 and " << m_numSamples-1 );

    if ( iSampleIndex >= m_numUniqueSamples )
    {
        iSampleIndex = m_numUniqueSamples-1;
    }

    // Get our name.
    const std::string &myName = m_header->getName();

    if ( iSampleIndex == 0 )
    {
        // Read the sample from the parent group.
        // Sample 0 is always on the parent group, with
        // our name + ".sample_0" as the name of it.
        std::string sample0Name = getSampleName( myName, 0 );
        if ( m_header->getPropertyType() == AbcA::kScalarProperty )
        {
            ABCA_ASSERT( H5Aexists( m_parentGroup, sample0Name.c_str() ),
                         "Invalid property: " << myName
                         << ", missing smp0" );
        }
        else
        {
            ABCA_ASSERT( DatasetExists( m_parentGroup, sample0Name ),
                         "Invalid property: " << myName
                         << ", missing smp1" );
        }

        this->readSample( m_parentGroup, sample0Name, iSampleIndex, oSample );
    }
    else
    {
        // Create the subsequent samples group.
        if ( m_samplesIGroup < 0 )
        {
            std::string samplesIName = myName + ".smpi";
            ABCA_ASSERT( GroupExists( m_parentGroup, samplesIName ),
                         "Invalid property: " << myName
                         << ", missing smpi" );

            m_samplesIGroup = H5Gopen2( m_parentGroup,
                                        samplesIName.c_str(),
                                        H5P_DEFAULT );
            ABCA_ASSERT( m_samplesIGroup >= 0,
                         "Invalid property: " << myName
                         << ", invalid smpi group" );
        }

        // Read the sample.
        std::string sampleName = getSampleName( myName, iSampleIndex );
        this->readSample( m_samplesIGroup, sampleName, iSampleIndex, oSample );
    }
}

//-*****************************************************************************
bool AprImpl::getKey( index_t iSampleIndex, AbcA::DataSampleKey & oKey )
{
    // Verify sample index
    ABCA_ASSERT( iSampleIndex >= 0 &&
                 iSampleIndex < m_numSamples,
                 "Invalid sample index: " << iSampleIndex
                 << ", should be between 0 and " << m_numSamples-1 );

    if ( iSampleIndex >= m_numUniqueSamples )
    {
        iSampleIndex = m_numUniqueSamples-1;
    }

    // Get our name.
    const std::string &myName = m_header->getName();

    if ( iSampleIndex == 0 )
    {
        // Read the sample from the parent group.
        // Sample 0 is always on the parent group, with
        // our name + ".sample_0" as the name of it.
        std::string sample0Name = getSampleName( myName, 0 );
        if ( m_header->getPropertyType() == AbcA::kScalarProperty )
        {
            ABCA_ASSERT( H5Aexists( m_parentGroup, sample0Name.c_str() ),
                         "Invalid property: " << myName
                         << ", missing smp0" );
        }
        else
        {
            ABCA_ASSERT( DatasetExists( m_parentGroup, sample0Name ),
                         "Invalid property: " << myName
                         << ", missing smp1" );
        }

        this->readKey( m_parentGroup, sample0Name, oKey );
    }
    else
    {
        // Create the subsequent samples group.
        if ( m_samplesIGroup < 0 )
        {
            std::string samplesIName = myName + ".smpi";
            ABCA_ASSERT( GroupExists( m_parentGroup,
                                      samplesIName ),
                         "Invalid property: " << myName
                         << ", missing smpi" );

            m_samplesIGroup = H5Gopen2( m_parentGroup,
                                        samplesIName.c_str(),
                                        H5P_DEFAULT );
            ABCA_ASSERT( m_samplesIGroup >= 0,
                         "Invalid property: " << myName
                         << ", invalid smpi group" );
        }

        // Read the sample.
        std::string sampleName = getSampleName( myName, iSampleIndex );
        this->readKey( m_samplesIGroup, sampleName, oKey );
    }
}

//-*****************************************************************************
AprImpl::~AprImpl()
{
    // Clean up our samples group, if necessary.
    if ( m_samplesIGroup >= 0 )
    {
        H5Gclose( m_samplesIGroup );
        m_samplesIGroup = -1;
    }

    if ( m_fileDataType >= 0 && m_cleanFileDataType )
    {
        H5Tclose( m_fileDataType );
        m_fileDataType = -1;
    }

    if ( m_nativeDataType >= 0 && m_cleanNativeDataType )
    {
        H5Tclose( m_nativeDataType );
        m_nativeDataType = -1;
    }
}

//-*****************************************************************************
void AprImpl::readSample( hid_t iGroup,
                          const std::string &iSampleName,
                          index_t iSampleIndex,
                          AbcA::ArraySamplePtr& oSamplePtr )
{
    assert( iGroup >= 0 );

    // Check index integrity.
    assert( iSampleIndex >= 0 && iSampleIndex < m_numUniqueSamples );

    // Read the array sample, possibly from the cache.
    const AbcA::DataType &dataType = m_header->getDataType();
    AbcA::ReadArraySampleCachePtr cachePtr =
        this->getObject()->getArchive()->getReadArraySampleCachePtr();
    oSamplePtr = ReadArray( cachePtr, iGroup, iSampleName, dataType,
                            m_fileDataType,
                            m_nativeDataType );
}

//-*****************************************************************************
bool AprImpl::readKey( hid_t iGroup,
                          const std::string &iSampleName,
                          AbcA::ArraySampleKey& oKey )
{
    assert( iGroup >= 0 );

    // Open the data set.
    hid_t dsetId = H5Dopen( iGroup, iSampleName.c_str(), H5P_DEFAULT );
    ABCA_ASSERT( dsetId >= 0, "Cannot open dataset: " << iSampleName );
    DsetCloser dsetCloser( dsetId );

    const AbcA::DataType &dataType = m_header->getDataType();
    if (ReadKey( dsetId, "key", oKey ))
    {
        hid_t dspaceId = H5Dget_space( dsetId );
        ABCA_ASSERT( dspaceId >= 0, "Could not get dataspace for dataSet: "
            << iSampleName );
        DspaceCloser dspaceCloser( dspaceId );

        oKey.readPOD = m_header->getDataType().getPod();
        oKey.origPOD = oKey.readPOD;

        oKey.numBytes = H5Sget_simple_extent_npoints( dspaceId );
        if (oKey.origPOD == kStringPOD || oKey.origPOD == kWstringPOD)
        {

            hid_t dsetFtype = H5Dget_type( dsetId );
            DtypeCloser dtypeCloser( dsetFtype );

            // string arrays get packed together
            oKey.numBytes *= H5Tget_size( dsetFtype );
        }
        else
        {
            oKey.numBytes *= m_header->getDataType().getNumBytes();
        }

        return true;
    }

    return false;
}

} // End namespace AbcCoreHDF5
} // End namespace Alembic
