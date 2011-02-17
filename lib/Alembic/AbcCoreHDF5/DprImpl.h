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

#ifndef _Alembic_AbcCoreHDF5_AprImpl_h_
#define _Alembic_AbcCoreHDF5_AprImpl_h_

#include <Alembic/AbcCoreHDF5/Foundation.h>

namespace Alembic {
namespace AbcCoreHDF5 {

//-*****************************************************************************
class AprImpl : public AbcA::DataPropertyReader,
    public boost::enable_shared_from_this<AprImpl>
{
public:
    AprImpl( AbcA::CompoundPropertyReaderPtr iParent, hid_t iParentGroup,
        PropertyHeaderPtr iHeader );

    virtual AbcA::DataPropertyReaderPtr asDataPtr();

    virtual const AbcA::PropertyHeader &getHeader() const;

    virtual AbcA::ObjectReaderPtr getObject();

    virtual AbcA::CompoundPropertyReaderPtr getParent();

    virtual size_t getNumSamples();

    virtual bool isConstant();

    virtual bool isScalar();

    virtual AbcA::TimeSampling getTimeSampling();

    virtual void getSample( index_t iSampleIndex,
                            AbcA::DataSamplePtr & oSample );

    virtual bool getKey( index_t iSampleIndex, AbcA::DataSampleKey & oKey );

protected:

    //-*************************************************************************
    // This function is called by SimplePrImpl to provide the actual
    // property reading.
    void readSample( hid_t iGroup,
                     const std::string &iSampleName,
                     index_t iSampleIndex,
                     AbcA::DataSamplePtr& oSamplePtr );

    //-*************************************************************************
    // This function is called by SimplePrImpl to provide the actual key reading
    bool readKey( hid_t iGroup,
                     const std::string &iSampleName,
                     AbcA::DataSampleKey & oSamplePtr );

    // Parent compound property writer. It must exist.
    AbcA::CompoundPropertyReaderPtr m_parent;

    // The HDF5 Group associated with the parent property reader.
    hid_t m_parentGroup;

    // We don't hold a pointer to the object, but instead
    // get it from the compound property reader.

    // The Header
    PropertyHeaderPtr m_header;

    // Data Types.
    hid_t m_fileDataType;
    bool m_cleanFileDataType;
    hid_t m_nativeDataType;
    bool m_cleanNativeDataType;

    bool m_isScalar;

    // The number of samples that were written. This may be greater
    // than the number of samples that were stored, because on the tail
    // of the property we don't write the same sample out over and over
    // until it changes.
    uint32_t m_numSamples;

    // The number of unique samples
    // This may be the same as the number of samples,
    // or it may be less. This corresponds to the number of samples
    // that are actually stored. In the case of a "constant" property,
    // this will be 1.
    uint32_t m_numUniqueSamples;

    // Value of the single time sample, if there's only one unique
    // sample.
    chrono_t m_timeSample0;

    // The time sampling ptr.
    // Contains Array Sample corresponding to the time samples
    AbcA::TimeSamplingPtr m_timeSamplingPtr;

    // The simple properties only store samples after the first
    // sample in a sub group. Therefore, there may not actually be
    // a group associated with this property.
    hid_t m_samplesIGroup;
};

} // End namespace AbcCoreHDF5
} // End namespace Alembic

#endif
