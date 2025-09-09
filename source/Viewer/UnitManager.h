#ifndef _UNITMANAGER_HEADERFILE
#define _UNITMANAGER_HEADERFILE

#include "IManager.h"

namespace Viewer {
//! Class provides public interface for managing units.
class UnitManager : public IManager {
public:
    //! Main constructor
    Standard_EXPORT explicit UnitManager(WasmViewer* theViewer);

    //! Format unit name (e.g. replace '**2' with squared sign and similar).
    Standard_EXPORT std::string FormatUnitName(const std::string& theUnit) const;

    //! Find quantity for specified unit.
    Standard_EXPORT int FindQuantity(const std::string& theUnit) const;

    //! Return quantity name.
    std::string GetQuantityName(int theQuantity) const;

    //! Return quantity units for specific section.
    std::string GetQuantityUnit(int theQuantity, int theSec) const;

    //! Set quantity units for specific section.
    Standard_EXPORT void SetQuantityUnit(int theQuantity, int theSec, const std::string& theUnit);

    //! Convert value from one unit to another unit.
    Standard_EXPORT double Convert(const double       theVal,
                                   const std::string& theFromUnit,
                                   const std::string& theToUnit) const;

    //! Convert value from SI unit to another unit.
    Standard_EXPORT double ConvertAnyFromSI(const double theVal, const std::string& theToUnit) const;

    //! Convert value to SI unit from another unit.
    Standard_EXPORT double ConvertAnyToSI(const double theVal, const std::string& theFromUnit) const;

    //! Convert value from system units into display units.
    Standard_EXPORT double SystemToDisplay(int theQuantity, double theSysVal) const;

    //! Convert value from display units into system units.
    Standard_EXPORT double SystemFromDisplay(int theQuantity, double theDispVal) const;

    //! Convert value from system units into SI units.
    Standard_EXPORT double SystemToSI(int theQuantity, double theSysVal) const;

    //! Convert value from SI units into system units.
    Standard_EXPORT double SystemFromSI(int theQuantity, double theSIVal) const;
};

} // namespace

#endif // _UNITMANAGER_HEADERFILE

