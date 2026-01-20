#ifndef ViewBase_UNITMANAGER_HEADERFILE
#define ViewBase_UNITMANAGER_HEADERFILE

#include <NCollection_DataMap.hxx>
#include <Standard_Type.hxx>
#include <TCollection_AsciiString.hxx>

#include <array>

namespace ViewBase
{

    //! Quantity type.
    enum class QuantityType
    {
        UNKNOWN = -1, // unknown / dimensionless unit
        // 7 base units in SI
        Time = 0,                 // second,   s
        Length = 1,               // meter,    m
        Mass,                     // kilogram, kg
        ElectricCurrent,          // ampere,   A
        ThermodynamicTemperature, // kelvin,   K
        AmountOfSubstance,        // mole,     mol
        LuminousIntensity,        // candela,  cd
        //
        PlaneAngle, // radian, rad
        Area,       // squared meter, m^2
        Volume,     // cubic   meter, m^3
        Speed,      // m/s
        Force,      // newton, N
        Pressure,   // pascal, Pa
    };
    enum
    {
        QuantityType_NB = (int)QuantityType::Pressure + 1
    };

    //! Unit conversion section.
    enum class UnitSection
    {
        SI,
        System,
        Display
    };

    //! Quantity definition.
    struct QuantityUnits
    {
        TCollection_AsciiString Name;
        TCollection_AsciiString SI;
        TCollection_AsciiString System;
        TCollection_AsciiString Display;
    };

    //! Tool for managing units.
    class UnitManager : public Standard_Transient
    {
    public:
        //! Return global instance.
        Standard_EXPORT static const Handle(UnitManager) & Instance();

        //! Return TRUE if integer is within QuantityType enumeration range.
        static bool IsQuantityType(int theQuantity) { return theQuantity >= 0 && theQuantity < QuantityType_NB; }

        //! Return TRUE if integer is within UnitSection enumeration range.
        static bool IsSectionEnum(int theSec)
        {
            return theSec == (int)UnitSection::SI || theSec == (int)UnitSection::System || theSec == (int)UnitSection::Display;
        }

    public:
        //! Empty constructor.
        Standard_EXPORT UnitManager();

        //! Format unit name (e.g. replace '**2' with squared sign and similar).
        Standard_EXPORT TCollection_AsciiString FormatUnitName(const TCollection_AsciiString &theUnit) const;

        //! Find quantity for specified unit.
        Standard_EXPORT QuantityType FindQuantity(const TCollection_AsciiString &theUnit) const;

        //! Return quantity units.
        const QuantityUnits &GetQuantity(QuantityType theQuantity) const
        {
            if (theQuantity == QuantityType::UNKNOWN)
            {
                static const QuantityUnits anEmpty;
                return anEmpty;
            }

            return myQuantities[(int)theQuantity];
        }

        //! Return quantity name.
        const TCollection_AsciiString &GetQuantityName(QuantityType theQuantity) const
        {
            const QuantityUnits &aQuant = GetQuantity(theQuantity);
            return aQuant.Name;
        }

        //! Return quantity units for specific section.
        const TCollection_AsciiString &GetQuantityUnit(QuantityType theQuantity, UnitSection theSec) const
        {
            switch (theSec)
            {
            case UnitSection::SI:
                return GetSIUnit(theQuantity);
            case UnitSection::System:
                return GetSystemUnit(theQuantity);
            case UnitSection::Display:
                return GetDisplayUnit(theQuantity);
            }
            return GetQuantity(theQuantity).SI;
        }

        //! Return quantity SI units.
        const TCollection_AsciiString &GetSIUnit(QuantityType theQuantity) const
        {
            const QuantityUnits &aQuant = GetQuantity(theQuantity);
            return aQuant.SI;
        }

        //! Return quantity system units.
        const TCollection_AsciiString &GetSystemUnit(QuantityType theQuantity) const
        {
            const QuantityUnits &aQuant = GetQuantity(theQuantity);
            return !aQuant.System.IsEmpty() ? aQuant.System : aQuant.SI;
        }

        //! Set quantity system units.
        Standard_EXPORT void SetSystemUnit(QuantityType theQuantity, const TCollection_AsciiString &theUnit);

        //! Return quantity display units.
        const TCollection_AsciiString &GetDisplayUnit(QuantityType theQuantity) const
        {
            const QuantityUnits &aQuant = GetQuantity(theQuantity);
            return !aQuant.Display.IsEmpty() ? aQuant.Display : (!aQuant.System.IsEmpty() ? aQuant.System : aQuant.SI);
        }

        //! Set quantity display units.
        Standard_EXPORT void SetDisplayUnit(QuantityType theQuantity, const TCollection_AsciiString &theUnit);

        //! Convert value from one unit to another unit.
        double Convert(double theVal,
                       const TCollection_AsciiString &theFromUnit,
                       const TCollection_AsciiString &theToUnit,
                       bool theToSilent = false) const
        {
            return Convert(theVal, theFromUnit.ToCString(), theToUnit.ToCString(), theToSilent);
        }

        //! Convert value from one unit to another unit.
        Standard_EXPORT double Convert(double theVal,
                                       Standard_CString theFromUnit,
                                       Standard_CString theToUnit,
                                       bool theToSilent = false) const;

        //! Convert value from SI unit to another unit.
        Standard_EXPORT double ConvertAnyFromSI(double theVal, const TCollection_AsciiString &theToUnit) const;

        //! Convert value to SI unit from another unit.
        Standard_EXPORT double ConvertAnyToSI(double theVal, const TCollection_AsciiString &theFromUnit) const;

        //! Convert value from system units into display units.
        double SystemToDisplay(QuantityType theQuantity, double theSysVal) const
        {
            return theQuantity != QuantityType::UNKNOWN
                       ? Convert(theSysVal, GetSystemUnit(theQuantity), GetDisplayUnit(theQuantity))
                       : theSysVal;
        }

        //! Convert value from display units into system units.
        double SystemFromDisplay(QuantityType theQuantity, double theDispVal) const
        {
            return theQuantity != QuantityType::UNKNOWN
                       ? Convert(theDispVal, GetDisplayUnit(theQuantity), GetSystemUnit(theQuantity))
                       : theDispVal;
        }

        //! Convert value from system units into SI units.
        double SystemToSI(QuantityType theQuantity, double theSysVal) const
        {
            return theQuantity != QuantityType::UNKNOWN
                       ? Convert(theSysVal, GetSystemUnit(theQuantity), GetSIUnit(theQuantity))
                       : theSysVal;
        }

        //! Convert value from SI units into system units.
        double SystemFromSI(QuantityType theQuantity, double theSIVal) const
        {
            return theQuantity != QuantityType::UNKNOWN
                       ? Convert(theSIVal, GetSIUnit(theQuantity), GetSystemUnit(theQuantity))
                       : theSIVal;
        }

    private:
        //! Try converting value from one unit to another unit.
        Standard_EXPORT bool TryConvert(const TCollection_AsciiString &theFromUnit,
                                        const TCollection_AsciiString &theToUnit) const;

    private:
        std::array<QuantityUnits, QuantityType_NB> myQuantities;
    };

} // namespace

#endif // ViewBase_UNITMANAGER_HEADERFILE
