#ifndef _UNITMANAGER_HEADERFILE
#define _UNITMANAGER_HEADERFILE

#include "IManager.h"
#include "Utils/VecN.h"

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

    //! Multiply two transformations in specified order.
    Standard_EXPORT Location TrsfMultiply(const Location& theLoc1, const Location& theLoc2) const;

    //! Compute inverted transformation.
    Standard_EXPORT Location TrsfInvert(const Location& theLoc) const;

    //! Return 4x4 matrix from transformation.
    Standard_EXPORT Mat4 TrsfGetMat4(const Location& theLoc) const;

    //! Return rotation part as 3 Euler angles.
    Standard_EXPORT Vec3 TrsfGetEulerAngles(const Location& theLoc, int theSeq) const;

    //! Set rotation part from 3 Euler angles.
    Standard_EXPORT Location TrsfSetEulerAngles(const Location& theLoc, const Vec3& theAngles, int theSeq) const;

    //! Setup rotation transformation around specified axis and angle in radians.
    Standard_EXPORT Location TrsfRotateAroundAxis(const Vec3& theDir, double theAngle, const Vec3& theOrigin) const;

    //! Return transformed 3D point.
    Standard_EXPORT Vec3 TrsfTransformPnt(const Location& theLoc, const Vec3& thePnt) const;

    //! Return transformed 3D direction.
    Standard_EXPORT Vec3 TrsfTransformDir(const Location& theLoc, const Vec3& theDir) const;
};

} // namespace

#endif // _UNITMANAGER_HEADERFILE

