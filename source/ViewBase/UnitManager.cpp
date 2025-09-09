#include <ViewBase/UnitManager.h>

#include <gp.hxx>
#include <Message.hxx>
#include <Units.hxx>
#include <Units_NoSuchUnit.hxx>
#include <Units_NoSuchType.hxx>
#include <Units_UnitsDictionary.hxx>
#include <Units_UnitSentence.hxx>

namespace ViewBase
{

    // ================================================================
    // Function : UnitManager
    // ================================================================
    const Handle(UnitManager) & UnitManager::Instance()
    {
        static Handle(UnitManager) aMgr = new UnitManager();
        return aMgr;
    }

    // ================================================================
    // Function : UnitManager
    // ================================================================
    UnitManager::UnitManager()
    {
        // basic SI units
        myQuantities[(int)QuantityType::Time].Name = "Time";
        myQuantities[(int)QuantityType::Time].SI = "s";

        myQuantities[(int)QuantityType::Length].Name = "Length";
        myQuantities[(int)QuantityType::Length].SI = "m";

        myQuantities[(int)QuantityType::Mass].Name = "Mass";
        myQuantities[(int)QuantityType::Mass].SI = "kg";

        myQuantities[(int)QuantityType::ElectricCurrent].Name = "Electric Current";
        myQuantities[(int)QuantityType::ElectricCurrent].SI = "A";

        myQuantities[(int)QuantityType::ThermodynamicTemperature].Name = "Thermodynamic Temperature";
        myQuantities[(int)QuantityType::ThermodynamicTemperature].SI = "K";

        myQuantities[(int)QuantityType::AmountOfSubstance].Name = "Amount of Substance";
        myQuantities[(int)QuantityType::AmountOfSubstance].SI = "mol";

        myQuantities[(int)QuantityType::LuminousIntensity].Name = "Luminous Intensity";
        myQuantities[(int)QuantityType::LuminousIntensity].SI = "cd";

        // derivatives
        myQuantities[(int)QuantityType::PlaneAngle].Name = "Plane Angle";
        myQuantities[(int)QuantityType::PlaneAngle].SI = "rad";
        myQuantities[(int)QuantityType::PlaneAngle].Display = "\xC2\xB0";

        myQuantities[(int)QuantityType::Area].Name = "Area";
        myQuantities[(int)QuantityType::Area].SI = "m\xC2\xB2";

        myQuantities[(int)QuantityType::Volume].Name = "Volume";
        myQuantities[(int)QuantityType::Volume].SI = "m\xC2\xB3";

        myQuantities[(int)QuantityType::Speed].Name = "Speed";
        myQuantities[(int)QuantityType::Speed].SI = "m/s";

        myQuantities[(int)QuantityType::Force].Name = "Force";
        myQuantities[(int)QuantityType::Force].SI = "N";

        myQuantities[(int)QuantityType::Pressure].Name = "Pressure";
        myQuantities[(int)QuantityType::Pressure].SI = "Pa";
    }

    //! Parse 'Exponent' and '(Exponent)' syntax.
    //! @param[out] theNumber parsed exponent number
    //! @param[in]  theStr    input string
    //! @param[in]  theChar   start position in input string
    //! @return length of exponent string, including optional braces
    static int ParseExponent(double &theNumber, const TCollection_AsciiString &theStr, int theChar)
    {
        static constexpr int nbPairBracesSyms2 = 2;

        int aLen = 0;
        if (theStr.Value(theChar) == '(')
        {
            const char *aPos = theStr.ToCString() + theChar;
            Standard_Character *aNext = nullptr;
            theNumber = Strtod(aPos, &aNext);
            if (aPos == aNext || *aNext != ')')
            {
                return 0;
            }

            aLen = static_cast<int>(aNext - aPos);
            return aLen + nbPairBracesSyms2;
        }

        const char *aPos = theStr.ToCString() + theChar - 1;
        Standard_Character *aNext = nullptr;
        theNumber = Strtod(aPos, &aNext);
        if (aPos == aNext)
        {
            return 0;
        }

        aLen = static_cast<int>(aNext - aPos);
        return aLen;
    }

    //! Return UNICODE superscript versions of a symbol.
    static const char *GetSuperscriptChar(const int theChar)
    {
        switch (theChar)
        {
        case int('0'):
            return "\xE2\x81\xB0";
        case int('1'):
            return "\xC2\xB9";
        case int('2'):
            return "\xC2\xB2";
        case int('3'):
            return "\xC2\xB3";
        case int('4'):
            return "\xE2\x81\xB4";
        case int('5'):
            return "\xE2\x81\xB5";
        case int('6'):
            return "\xE2\x81\xB6";
        case int('7'):
            return "\xE2\x81\xB7";
        case int('8'):
            return "\xE2\x81\xB8";
        case int('9'):
            return "\xE2\x81\xB9";
        // undefined in used fonts
        // @c case int('+'): return "\xE2\x81\xBA";
        // @c case int('-'): return "\xE2\x81\xBB";
        case int('('):
            return "\xE2\x81\xBD";
        case int(')'):
            return "\xE2\x81\xBE";
        case int('n'):
            return "\xE2\x81\xBF";
        case int('i'):
            return "\xE2\x81\xB1";
        default:
            return nullptr;
        }
    }

    //! Format exponent number as a UNICODE superscript.
    static TCollection_AsciiString FormatExponent(double theNumber)
    {
        const int anIntExp = (int)theNumber;
        if (double(anIntExp) != theNumber)
        {
            return TCollection_AsciiString();
        }

        const TCollection_AsciiString aStr(anIntExp);

        TCollection_AsciiString aRes;
        for (int aCharIter = 1; aCharIter <= aStr.Length(); ++aCharIter)
        {
            const char *aSymb = GetSuperscriptChar((int)aStr.Value(aCharIter));
            if (aSymb == nullptr)
            {
                return TCollection_AsciiString();
            }

            aRes.AssignCat(aSymb);
        }
        return aRes;
    }

    // ================================================================
    // Function : FormatUnitName
    // ================================================================
    TCollection_AsciiString UnitManager::FormatUnitName(const TCollection_AsciiString &theUnit) const
    {
        static constexpr int expSignLen2 = 2;

        TCollection_AsciiString aRes = theUnit;
        for (int aCharIter = 2; aCharIter <= aRes.Length(); ++aCharIter)
        {
            if (aCharIter + expSignLen2 <= aRes.Length() && aRes.Value(aCharIter + 0) == '*' && aRes.Value(aCharIter + 1) == '*' && aRes.Value(aCharIter + expSignLen2) != '*')
            {
                double anExp = 0.0;
                int aLen = ParseExponent(anExp, aRes, aCharIter + expSignLen2);

                TCollection_AsciiString anExpStr = aLen != 0 ? FormatExponent(anExp) : TCollection_AsciiString();
                if (!anExpStr.IsEmpty())
                {
                    aRes = aRes.SubString(1, aCharIter - 1) + anExpStr + (aCharIter + expSignLen2 + aLen <= aRes.Length() ? aRes.SubString(aCharIter + expSignLen2 + aLen, aRes.Length()) : TCollection_AsciiString());
                }
                else if (strcmp(aRes.ToCString() + aCharIter + expSignLen2 - 1, "(1/2)") == 0)
                {
                    aLen = (int)strlen("(1/2)");
                    aRes = aRes.SubString(1, aCharIter - 1) + "^\xC2\xBD" + (aCharIter + expSignLen2 + aLen <= aRes.Length() ? aRes.SubString(aCharIter + expSignLen2 + aLen, aRes.Length()) : TCollection_AsciiString());
                }
                else
                {
                    aRes = aRes.SubString(1, aCharIter - 1) + "^" + aRes.SubString(aCharIter + expSignLen2, aRes.Length());
                }
            }
            else if (aCharIter + 1 <= aRes.Length() && aRes.Value(aCharIter + 0) == '*' && aRes.Value(aCharIter + 1) != '*')
            {
                aRes = aRes.SubString(1, aCharIter - 1) + "\xC2\xB7" + aRes.SubString(aCharIter + 1, aRes.Length());
            }
        }
        return aRes;
    }

    // ================================================================
    // Function : FindQuantity
    // ================================================================
    QuantityType UnitManager::FindQuantity(const TCollection_AsciiString &theUnit) const
    {
        // check quantity name or it's SI units
        for (int aQIter = 0; aQIter < (int)myQuantities.size(); ++aQIter)
        {
            const QuantityUnits &aQuantity = myQuantities[aQIter];
            if (TCollection_AsciiString::IsSameString(aQuantity.Name, theUnit, false) || TCollection_AsciiString::IsSameString(aQuantity.SI, theUnit, false))
            {
                return (QuantityType)aQIter;
            }
        }

        // aliases
        if (TCollection_AsciiString::IsSameString("temperature", theUnit, false))
        {
            return QuantityType::ThermodynamicTemperature;
        }

        // try converting units into SI
        for (int aQIter = 0; aQIter < (int)myQuantities.size(); ++aQIter)
        {
            const QuantityUnits &aQuantity = myQuantities[aQIter];

            Units_UnitSentence aUnitSent(theUnit.ToCString());
            if (!aUnitSent.IsDone())
            {
                continue;
            }

            if (TryConvert(theUnit, aQuantity.SI))
            {
                return (QuantityType)aQIter;
            }
        }

        return QuantityType::UNKNOWN;
    }

    // ================================================================
    // Function : SetSystemUnit
    // ================================================================
    void UnitManager::SetSystemUnit(QuantityType theQuantity, const TCollection_AsciiString &theUnit)
    {
        if (theQuantity == QuantityType::UNKNOWN)
        {
            throw Units_NoSuchType("UnitManager::SetSystemUnit() for UNKNOWN quantity");
        }

        QuantityUnits &aQuantity = myQuantities[(int)theQuantity];
        if (!theUnit.IsEmpty())
        {
            Units_UnitSentence aUnitSent(theUnit.ToCString());
            if (!aUnitSent.IsDone())
            {
                TCollection_AsciiString aMsg = TCollection_AsciiString("UnitManager::SetSystemUnit() with INVALID unit '") + theUnit + "'";
                throw Units_NoSuchUnit(aMsg.ToCString());
            }

            double aRes = Convert(1.0, aQuantity.SI, theUnit, true);
            if (Abs(aRes) <= gp::Resolution())
            {
                TCollection_AsciiString aMsg = TCollection_AsciiString() + "UnitManager::SetSystemUnit() with INCOMPATIBLE unit '" + theUnit + "'";
                throw Units_NoSuchUnit(aMsg.ToCString());
            }
            else if (aRes == 1.0 && !theUnit.IsEqual(aQuantity.SI))
            {
                TCollection_AsciiString aMsg = TCollection_AsciiString() + "UnitManager::SetSystemUnit() with INCOMPATIBLE unit '" + theUnit + "'";
                throw Units_NoSuchUnit(aMsg.ToCString());
            }
        }
        aQuantity.System = theUnit;

        // derivatives
        if (theQuantity == QuantityType::Length)
        {
            SetSystemUnit(QuantityType::Area, !theUnit.IsEmpty() ? theUnit + "\xC2\xB2" : theUnit);
            SetSystemUnit(QuantityType::Volume, !theUnit.IsEmpty() ? theUnit + "\xC2\xB3" : theUnit);
        }
    }

    // ================================================================
    // Function : SetDisplayUnit
    // ================================================================
    void UnitManager::SetDisplayUnit(QuantityType theQuantity, const TCollection_AsciiString &theUnit)
    {
        if (theQuantity == QuantityType::UNKNOWN)
        {
            throw Units_NoSuchType("UnitManager::SetSystemUnit() for UNKNOWN quantity");
        }

        QuantityUnits &aQuantity = myQuantities[(int)theQuantity];
        if (!theUnit.IsEmpty())
        {
            Units_UnitSentence aUnitSent(theUnit.ToCString());
            if (!aUnitSent.IsDone())
            {
                TCollection_AsciiString aMsg = TCollection_AsciiString("UnitManager::SetDisplayUnit() with INVALID unit '") + theUnit + "'";
                throw Units_NoSuchUnit(aMsg.ToCString());
            }

            double aRes = Convert(1.0, aQuantity.SI, theUnit, true);
            if (Abs(aRes) <= gp::Resolution())
            {
                TCollection_AsciiString aMsg = TCollection_AsciiString() + "UnitManager::SetDisplayUnit() with INCOMPATIBLE unit '" + theUnit + "'";
                throw Units_NoSuchUnit(aMsg.ToCString());
            }
            else if (aRes == 1.0 && !theUnit.IsEqual(aQuantity.SI))
            {
                TCollection_AsciiString aMsg = TCollection_AsciiString() + "UnitManager::SetDisplayUnit() with INCOMPATIBLE unit '" + theUnit + "'";
                throw Units_NoSuchUnit(aMsg.ToCString());
            }
        }
        aQuantity.Display = theUnit;

        // derivatives
        if (theQuantity == QuantityType::Length)
        {
            SetDisplayUnit(QuantityType::Area, !theUnit.IsEmpty() ? theUnit + "\xC2\xB2" : theUnit);
            SetDisplayUnit(QuantityType::Volume, !theUnit.IsEmpty() ? theUnit + "\xC2\xB3" : theUnit);
        }
    }

    // ================================================================
    // Function : Convert
    // ================================================================
    double UnitManager::Convert(double theVal,
                                Standard_CString theFromUnit,
                                Standard_CString theToUnit,
                                bool theToSilent) const
    {
        try
        {
            if (strcmp(theFromUnit, theToUnit) == 0)
            {
                return theVal;
            }
            // Don't try to convert values from/to units starting with "dB", treat them as dimensionless.
            if (strstr(theFromUnit, "dB") == theFromUnit || strstr(theToUnit, "dB") == theToUnit)
            {
                return theVal;
            }
            return Units::Convert(theVal, theFromUnit, theToUnit);
        }
        catch (const Standard_Failure &theErr)
        {
            if (theToSilent)
            {
                return theVal;
            }
            Message::SendFail() << "Units::Convert(" << theVal << ", " << theFromUnit << ", " << theToUnit
                                << ") - conversion FAILED:\n"
                                << theErr;
            return theVal;
        }
    }

    // ================================================================
    // Function : TryConvert
    // ================================================================
    bool UnitManager::TryConvert(const TCollection_AsciiString &theFromUnit, const TCollection_AsciiString &theToUnit) const
    {
        if (theFromUnit.IsEqual(theToUnit))
        {
            return true;
        }

        // Don't try to convert values in units starting with "dB"
        if (theFromUnit.StartsWith("dB") || theToUnit.StartsWith("dB"))
        {
            return false;
        }

        try
        {
            double aRes = Units::Convert(1.0, theFromUnit.ToCString(), theToUnit.ToCString());
            return aRes != 1.0;
        }
        catch (const Standard_Failure &)
        {
            return false;
        }
    }

    // ================================================================
    // Function : ConvertAnyFromSI
    // ================================================================
    double UnitManager::ConvertAnyFromSI(double theVal, const TCollection_AsciiString &theToUnit) const
    {
        // Treat values in units starting with "dB" as dimensionless quantity.
        if (theToUnit.StartsWith("dB"))
        {
            return theVal;
        }

        try
        {
            return Units::FromSI(theVal, theToUnit.ToCString());
        }
        catch (const Standard_Failure &theErr)
        {
            Message::SendFail() << "Units::FromSI(" << theVal << ", " << theToUnit << ") - conversion FAILED:\n"
                                << theErr;
            return theVal;
        }
    }

    // ================================================================
    // Function : ConvertAnyToSI
    // ================================================================
    double UnitManager::ConvertAnyToSI(double theVal, const TCollection_AsciiString &theFromUnit) const
    {
        // Treat values in units starting with "dB" as dimensionless quantity.
        if (theFromUnit.StartsWith("dB"))
        {
            return theVal;
        }

        try
        {
            return Units::ToSI(theVal, theFromUnit.ToCString());
        }
        catch (const Standard_Failure &theErr)
        {
            Message::SendFail() << "Units::ToSI(" << theVal << ", " << theFromUnit << ") - conversion FAILED:\n"
                                << theErr;
            return theVal;
        }
    }

} // namespace