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

#include <Alembic/AbcCoreHDF5/ApwImpl.h>
#include <Alembic/AbcCoreHDF5/WriteUtil.h>
#include <Alembic/AbcCoreHDF5/StringWriteUtil.h>

namespace Alembic {
namespace AbcCoreHDF5 {

//-*****************************************************************************
ApwImpl::ApwImpl( AbcA::CompoundPropertyWriterPtr iParent,
                  hid_t iParentGroup,
                  PropertyHeaderPtr iHeader )
    : m_parent( iParent )
    , m_parentGroup( iParentGrp )
    , m_header( iHeader )
    , m_fileDataType( -1 )
    , m_cleanFileDataType( false )
    , m_nativeDataType( -1 )
    , m_cleanNativeDataType( false )
    , m_sampleIGroup( -1 )
    , m_nextSampleIndex( 0 )
    , m_numUniqueSamples( 0 )
{
    // Check the validity of all inputs.
    ABCA_ASSERT( m_parent, "Invalid parent" );
    ABCA_ASSERT( m_header, "Invalid property header" );
    ABCA_ASSERT( m_parentGroup >= 0, "Invalid parent group" );


    if ( m_header->getPropertyType() != AbcA::kDataProperty )
    {
        ABCA_THROW( "Attempted to create a DataPropertyWriter from a "
                    "non-data property type" );
    }

    // Get data types
    PlainOldDataType POD = m_header->getDataType().getPod();
    if ( POD != kStringPOD && POD != kWstringPOD )
    {
        m_fileDataType = GetFileH5T( m_header->getDataType(),
                                     m_cleanFileDataType );
        m_nativeDataType = GetNativeH5T( m_header->getDataType(),
                                         m_cleanNativeDataType );
        
        ABCA_ASSERT( m_fileDataType >= 0, "Couldn't get file datatype" );
        ABCA_ASSERT( m_nativeDataType >= 0, "Couldn't get native datatype" );
    }

    // The WrittenSampleID is invalid by default.
    assert( !m_previousWrittenSampleID );

    // Write the property header.
    // We don't write the time info yet, because there's
    // an optimization that the abstract API allows us to make which has a
    // surprisingly significant effect on file size. This optimization
    // is that when a "constant" property is written - that is, when
    // all the samples are identical, we can skip the writing of the
    // time sampling type and time samples and treat the sample as though
    // it were written with identity time sampling.
    // Since we can't know this until the end, we don't write time
    // sampling yet.
    WritePropertyHeaderExceptTime( m_parentGroup,
                                   m_header->getName(), *m_header );
}


//-*****************************************************************************
ApwImpl::~ApwImpl()
{
    // Wrap the whole thing in a try block, so as to prevent
    // exceptions from being thrown out of a destructor.
    try
    {
        if ( m_fileDataType >= 0 && m_cleanFileDataType )
        {
            H5Tclose( m_fileDataType );
        }

        if ( m_nativeDataType >= 0 && m_cleanNativeDataType )
        {
            H5Tclose( m_nativeDataType );
        }

        const std::string &myName = m_header->getName();

        // Check validity of the group.
        ABCA_ASSERT( m_parentGroup >= 0, "Invalid parent group" );

        uint32_t numSamples = m_nextSampleIndex;

        // Write all the sampling information.
        // This function, which lives in WriteUtil.cpp, contains all of the
        // logic regarding which information needs to be written.
        WriteSampling(
            GetWrittenSampleMap(this->getObject()->getArchive() ),
            m_parentGroup, myName, m_header->getTimeSamplingType(),
            numSamples, m_numUniqueSamples,
            m_timeSamples.empty() ? NULL : &m_timeSamples.front() );

        // Close the sampleIGroup if it was open
        if ( m_sampleIGroup >= 0 )
        {
            ABCA_ASSERT( m_numUniqueSamples > 1, "Corrupt SimplePwImpl" );
            H5Gclose( m_sampleIGroup );
            m_sampleIGroup = -1;
        }
    }
    catch ( std::exception & exc )
    {
        std::cerr << "AbcCoreHDF5::SimplePwImpl<A,I,K>::"
                  << "~SimplePwImpl(): EXCEPTION: "
                  << exc.what() << std::endl;
    }
    catch ( ... )
    {
        std::cerr << "AbcCoreHDF5::SimplePwImpl<A,I,K>::~SimplePwImpl(): "
                  << "UNKNOWN EXCEPTION: " << std::endl;
    }

    m_parentGroup = -1;
    m_sampleIGroup = -1;
    m_fileDataType = -1;
    m_nativeDataType = -1;
}

//-*****************************************************************************
const AbcA::PropertyHeader & ApwImpl::getHeader() const
{
    ABCA_ASSERT( m_header, "Invalid header" );
    return *m_header;
}

//-*****************************************************************************
AbcA::ObjectWriterPtr ApwImpl::getObject()
{
    ABCA_ASSERT( m_parent, "Invalid parent" );
    return m_parent->getObject();
}

//-*****************************************************************************
AbcA::DataPropertyWriterPtr ApwImpl::asDataPtr()
{
    return shared_from_this();
}

//-*****************************************************************************
AbcA::CompoundPropertyWriterPtr ApwImpl::getParent()
{
    ABCA_ASSERT( m_parent, "Invalid parent" );
    return m_parent;
}

//-*****************************************************************************
hid_t ApwImpl::getSampleIGroup()
{
    if ( m_sampleIGroup >= 0 )
    {
        return m_sampleIGroup;
    }

    ABCA_ASSERT( m_parentGroup >= 0, "invalid parent group" );
    ABCA_ASSERT( m_numUniqueSamples > 0,
                 "can't create sampleI group before numSamples > 1" );

    const std::string groupName = m_header->getName() + ".smpi";
    
    hid_t copl = CreationOrderPlist();
    PlistCloser plistCloser( copl );
    
    m_sampleIGroup = H5Gcreate2( m_parentGroup,
                                 groupName.c_str(),
                                 H5P_DEFAULT,
                                 copl,
                                 H5P_DEFAULT );
    ABCA_ASSERT( m_sampleIGroup >= 0,
                 "Could not create simple samples group named: "
                 << groupName );
    
    return m_sampleIGroup;
}

//-*****************************************************************************
void ApwImpl::writeSample( hid_t iGroup,
                           const std::string &iSampleName,
                           index_t iSampleIndex,
                           const AbcA::DataSample & iSamp,
                           const AbcA::DataSample::Key &iKey )
{
    AbcA::ArchiveWriterPtr awp =
        this->getObject()->getArchive();

    ABCA_ASSERT(iSamp.getDataType() == m_header->getDataType(),
        "DataType on DataSample iSamp: " << iSamp.getDataType() <<
        ", does not match the DataType of the Data property: " <<
        m_header->getDataType());

    // Write the sample.
    m_previousWrittenSampleID =
        WriteArray( GetWrittenSampleMap( awp ),
                    iGroup, iSampleName,
                    iSamp, iKey,
                    m_fileDataType,
                    m_nativeDataType,
                    awp->getCompressionHint() );
}

//-*****************************************************************************
void ApwImpl::setSample (index_t iSampleIndex, chrono_t iSampleTime,
    const DataSample & iSamp )
{
    // Check errors.
    ABCA_ASSERT( iSampleIndex == m_nextSampleIndex,
                 "Out-of-order sample writing. Expecting: "
                 << m_nextSampleIndex
                 << ", but got: " << iSampleIndex );

    bool pushTimeSamples = false;

    // decide whether we should push back the sample time later on
    // don't do it now because there are a few more checks that need to pass
    if ( m_timeSamples.size() <
         m_header->getTimeSamplingType().getNumSamplesPerCycle() )
    {
        ABCA_ASSERT(m_timeSamples.empty() || iSampleTime > m_timeSamples.back(),
            "Out-of-order time writing. Last time: " << m_timeSamples.back() <<
            " is greater than or equal to: " << iSampleTime);

        // if we are cyclic sampling make sure the difference between the our
        // first sample time and the one we want to push does not exceed the
        // time per cycle.  Doing so would violate the strictly increasing
        // rule we have for our samples
        ABCA_ASSERT(!m_header->getTimeSamplingType().isCyclic() ||
            m_timeSamples.empty() || iSampleTime - m_timeSamples.front() <
            m_header->getTimeSamplingType().getTimePerCycle(),
            "Out-of-order cyclic time writing.  This time: " << iSampleTime <<
            " minus first time: " << m_timeSamples.front() <<
            " is greater than or equal to: " <<
            m_header->getTimeSamplingType().getTimePerCycle());

        pushTimeSamples = true;
    }

    // Get my name
    const std::string &myName = m_header->getName();
    
    // Figure out if we need to write the sample. At first, no.
    bool needToWriteSample = false;

    // The Key helps us analyze the sample.
    AbcA::DataSample & key = this->computeSampleKey( iSamp );
    if ( m_numUniqueSamples == 0 )
    {
        // This means we are now writing the very first sample.
        assert( iSampleIndex == 0 );
        needToWriteSample = true;
    }
    else
    {
        // Check to see if there have been any changes.
        // Only if they're different do we bother.
        // We use the Key to check.
        assert( m_numUniqueSamples > 0 );
        needToWriteSample = !this->sameAsPreviousSample( iSamp, key ) );
    }

    // If we need to write sample, write sample!
    if ( needToWriteSample )
    {   
        // Write all the samples, starting from the last unique sample,
        // up to this sample.

        // This is tricky. If we get in here, and we have to write,
        // we want to write all the previously-thought-to-be-constant
        // samples. If we've already written them, then m_numUniqueSamples
        // will equal iSampleIndex-1, and this loop will skip.
        // If we haven't written them, this loop will do the right thing.
        // If this is the FIRST sample, iSampleIndex is zero, and this
        // will fail.
        // It's just tricky.
        for ( index_t smpI = m_numUniqueSamples;
              smpI < iSampleIndex; ++smpI )
        {
            assert( smpI > 0 );
            this->copyPreviousSample(getSampleIGroup(),
                getSampleName( myName, smpI ), smpI );
        }

        // Write this sample, which will update its internal
        // cache of what the previously written sample was.
        this->writeSample(iSampleIndex == 0 ? m_parentGroup : getSampleIGroup(),
            getSampleName( myName, iSampleIndex ), iSampleIndex, iSamp, key );

        // Time sample written, all is well.
        m_numUniqueSamples = iSampleIndex+1;
    }

    if (pushTimeSamples)
    {
        m_timeSamples.push_back( iSampleTime );
    }

    // Set the previous sample index.
    m_nextSampleIndex = iSampleIndex+1;
}

//-*****************************************************************************
void ApwImpl::setFromPreviousSample (index_t iSampleIndex, chrono_t iSampleTime)
{
    // Various programmer error checks
    // Check errors.
    ABCA_ASSERT( iSampleIndex == m_nextSampleIndex,
                 "Out-of-order sample writing. Expecting: "
                 << m_nextSampleIndex
                 << ", but got: " << iSampleIndex );

    // Verify indices
    if ( m_nextSampleIndex < 1 || m_numUniqueSamples < 1 )
    {
        ABCA_THROW( "Cannot set from previous sample before any "
                     "samples have been written" );
    }

    // Push back the sample time.
    if ( m_timeSamples.size() <
         m_header->getTimeSamplingType().getNumSamplesPerCycle() )
    {
        ABCA_ASSERT(m_timeSamples.empty() || iSampleTime > m_timeSamples.back(),
            "Out-of-order time writing. Last time: " << m_timeSamples.back() <<
            " is greater than or equal to: " << iSampleTime);

        // if we are cyclic sampling make sure the difference between the our
        // first sample time and the one we want to push does not exceed the
        // time per cycle.  Doing so would violate the strictly increasing
        // rule we have for our samples
        ABCA_ASSERT(!m_header->getTimeSamplingType().isCyclic() ||
            m_timeSamples.empty() || iSampleTime - m_timeSamples.front() <
            m_header->getTimeSamplingType().getTimePerCycle(),
            "Out-of-order cyclic time writing.  This time: " << iSampleTime <<
            " minus first time: " << m_timeSamples.front() <<
            " is greater than or equal to: " <<
            m_header->getTimeSamplingType().getTimePerCycle());

        m_timeSamples.push_back( iSampleTime );
    }

    // Just increase the previous sample index without increasing
    // the number of unique samples.
    m_nextSampleIndex = iSampleIndex + 1;
}

//-*****************************************************************************
size_t ApwImpl::getNumSamples()
{
    return ( size_t )m_nextSampleIndex;
}

//-*****************************************************************************

} // End namespace AbcCoreHDF5
} // End namespace Alembic
