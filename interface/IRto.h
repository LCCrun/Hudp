#ifndef HEADER_INTERFACE_RTO
#define HEADER_INTERFACE_RTO

#include "CommonType.h"

namespace hudp {
    // rto calc base class.
    class CRto
    {
    public:
        CRto() {}
        virtual ~CRto() {}
        // get rto
        virtual uint32_t GetRto() = 0;

    };
}
#endif