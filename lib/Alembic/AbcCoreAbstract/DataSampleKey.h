//-*****************************************************************************
//
// Copyright (c) 2009-2011,
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

#ifndef _Alembic_AbcCoreAbstract_DataSampleKey_h_
#define _Alembic_AbcCoreAbstract_DataSampleKey_h_

#include <Alembic/AbcCoreAbstract/Foundation.h>
#include <Alembic/AbcCoreAbstract/DataType.h>

namespace Alembic {
namespace AbcCoreAbstract {
namespace v1 {

struct DataSampleKey : public boost::totally_ordered<DataSampleKey>
{
    //! total number of bytes of the sample as originally stored
    uint64_t numBytes;

    //! Original POD as stored
    PlainOldDataType origPOD;

    //! POD used at read time
    PlainOldDataType readPOD;

    MD5Digest digest;

    bool operator==( const DataSampleKey &iRhs ) const
    {
        return ( ( numBytes == iRhs.numBytes ) &&
                 ( origPOD  == iRhs.origPOD  ) &&
                 ( readPOD  == iRhs.readPOD  ) &&
                 ( digest ==   iRhs.digest ) );
    };

    bool operator<( const DataSampleKey &iRhs ) const
    {
        return ( numBytes < iRhs.numBytes ? true :
                 ( numBytes > iRhs.numBytes ? false :
                   
                   ( origPOD < iRhs.origPOD ? true :
                     ( origPOD > iRhs.origPOD ? false :
                       
                       ( readPOD < iRhs.readPOD ? true :
                         ( readPOD > iRhs.readPOD ? false :
                           
                           ( digest < iRhs.digest ) ) ) ) ) ) );
    };

};

//-*****************************************************************************
// Equality operator.
struct DataSampleKeyEqualTo :
        public std::binary_function<DataSampleKey,DataSampleKey,bool>
{
    bool operator()( DataSampleKey const &a,
                     DataSampleKey const &b ) const
    {
        return a == b;
    }
};

//-*****************************************************************************
// Hash function
inline uint64_t StdHash( DataSampleKey const &a )
{
    // Theoretically, the bits of an MD5 Hash are uniformly
    // randomly distributed, so it doesn't matter which of the 128
    // bits we use to generate the 64 bits that we return as the hash
    // key. So, I'll just do the simple thing.
    return *(( const uint64_t * )&a.digest);
}

//-*****************************************************************************
struct DataSampleKeyStdHash :
        public std::unary_function<DataSampleKey,uint64_t>
{
    uint64_t operator()( DataSampleKey const &a ) const
    {
        return StdHash( a );
    }
};

//-*****************************************************************************
template <class MAPPED>
struct UnorderedMapUtil
{
    typedef boost::unordered_map<DataSampleKey,
                                 MAPPED,
                                 DataSampleKeyStdHash,
                                 DataSampleKeyEqualTo> umap_type;
    typedef boost::unordered_multimap<DataSampleKey,
                                      MAPPED,
                                      DataSampleKeyStdHash,
                                      DataSampleKeyEqualTo> umultimap_type;
};

//-*****************************************************************************
// Unordered sets don't need a wrapping template.
// This isn't a terribly useful type. And it's meaningless to have
// multisets in this context.
typedef boost::unordered_set<DataSampleKey,
                             DataSampleKeyStdHash,
                             DataSampleKeyEqualTo> UnorderedDataSampleKeySet;

} // End namespace v1
} // End namespace AbcCoreAbstract
} // End namespace Alembic

#endif
