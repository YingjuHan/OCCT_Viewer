
#ifndef _DIMENSIONPARAMS_HEADERFILE
#define _DIMENSIONPARAMS_HEADERFILE

#include <PrsDim_Dimension.hxx>

#include "VecN.h"

#include <vector>

namespace Viewer {
//! Parameters for dimension object.
struct DimensionParams {
    //! Plane position.
    Ax2 plane = Ax2::InvalidAx2();

    //! Real computed value.
    double realValue = 0.0;

    //! Custom text value overriding real value, when non-empty.
    std::string textValue;

    //! Special symbol (single character!)
    std::string specialSymbol;

    //! Special symbol position.
    bool isSpecialSymbolAfter = false;

public:
    //! Copy parameters from presentation.
    void InitFromPresentation(const Handle(PrsDim_Dimension)& thePrs);

    //! Update interactive object.
    void UpdatePresentation(const Handle(PrsDim_Dimension)& thePrs) const;
};

} // namespace

#endif // _DIMENSIONPARAMS_HEADERFILE