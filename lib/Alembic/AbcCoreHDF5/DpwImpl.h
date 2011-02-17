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

#ifndef _Alembic_AbcCoreHDF5_DpwImpl_h_
#define _Alembic_AbcCoreHDF5_DpwImpl_h_

#include <Alembic/AbcCoreHDF5/Foundation.h>
#include <Alembic/AbcCoreHDF5/SimplePwImpl.h>
#include <Alembic/AbcCoreHDF5/WrittenSampleMap.h>

namespace Alembic {
namespace AbcCoreHDF5 {

//-*****************************************************************************
class DpwImpl : public AbcA::DataPropertyWriter,
    public boost::enable_shared_from_this<DpwImpl>
{
protected:
    friend class BaseCpwImpl;

    //-*************************************************************************
    DpwImpl( AbcA::CompoundPropertyWriterPtr iParent,
             hid_t iParentGroup,
             PropertyHeaderPtr iHeader );

    virtual AbcA::DataPropertyWriterPtr asDataPtr();
    
public:
    virtual ~DpwImpl();

    virtual const AbcA::PropertyHeader & getHeader() const;

    virtual AbcA::ObjectWriterPtr getObject();

    virtual AbcA::CompoundPropertyWriterPtr getParent();

    virtual void setSample( index_t iSampleIndex,
                            chrono_t iSampleTime,
                            Abc iSamp );

    virtual void setFromPreviousSample( index_t iSampleIndex,
                                        chrono_t iSampleTime );

    virtual size_t getNumSamples();

    //-*************************************************************************
    static AbcA::DataSample::Key
    computeSampleKey( const AbcA::DataSample &iSamp )
    {
        return iSamp.computeKey();
    }

    //-*************************************************************************
    bool sameAsPreviousSample( const AbcA::DataSample &iSamp,
                               const AbcA::DataSample::Key &iKey ) const
    {
        return ( m_previousWrittenSampleID &&
                 iKey == m_previousWrittenSampleID->computeKey() );
    }

    //-*************************************************************************
    void copyPreviousSample( hid_t iGroup,
                             const std::string &iSampleName,
                             index_t iSampleIndex )
    {
        // Copy the sample.
        CopyWrittenArray( iGroup, iSampleName,
                          m_previousWrittenSampleID );
    }

    //-*************************************************************************
    void writeSample( hid_t iGroup,
                      const std::string &iSampleName,
                      index_t iSampleIndex,
                      const AbcA::DataSample & iSamp,
                      const AbcA::DataSample::Key &iKey );

private:
    hid_t getSampleIGroup();

protected:
    // The parent compound property writer.
    AbcA::CompoundPropertyWriterPtr m_parent;
    
    // The parent group. We need to keep this around because we
    // don't create our group until we need to. This is guaranteed to
    // exist because our parent (or object) is guaranteed to exist.
    hid_t m_parentGroup;

    // The header which defines this property.
    PropertyHeaderPtr m_header;

    // The DataTypes for this property.
    hid_t m_fileDataType;
    bool m_cleanFileDataType;
    hid_t m_nativeDataType;
    bool m_cleanNativeDataType;

    // The group corresponding to this property.
    // It may never be created or written.
    hid_t m_sampleIGroup;
    
    // The time samples. The number of these will be determined by
    // the TimeSamplingType
    std::vector<chrono_t> m_timeSamples;

    // Index of the next sample to write
    uint32_t m_nextSampleIndex;

    // Number of unique samples.
    // If this is zero, it means we haven't written a sample yet.
    // Otherwise, it is the number of samples we've actually written.
    // It is common for the tail end of sampling blocks to be repeated
    // values, so we don't bother writing them out if the tail is
    // non-varying.
    uint32_t m_numUniqueSamples;

    // Previous written array sample identifier!
    WrittenSampleIDPtr m_previousWrittenSampleID;
};

} // End namespace AbcCoreHDF5
} // End namespace Alembic

#endif
