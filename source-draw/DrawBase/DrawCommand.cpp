
#include <DrawBase/DrawCommand.h>

namespace DrawBase {

IMPLEMENT_STANDARD_RTTIEXT(DrawCommand, Standard_Transient)

// =======================================================================
// function : CheckArgName
// =======================================================================
bool DrawCommand::CheckArgName(const TCollection_AsciiString& theArg,
                               const char*                    theArg1,
                               const char*                    theArg2,
                               const char*                    theArg3,
                               const char*                    theArg4)
{
    if (theArg == theArg1) {
        return true;
    }
    if (theArg2 != nullptr && theArg == theArg2) {
        return true;
    }
    if (theArg3 != nullptr && theArg == theArg3) {
        return true;
    }
    if (theArg4 != nullptr && theArg == theArg4) {
        return true;
    }
    return false;
}

// ================================================================
// Function : ParseVec3
// ================================================================
bool DrawCommand::ParseVec3(const char** theArgVec, Graphic3d_Vec3d& thePnt)
{
    static constexpr int iX0 = 0;
    static constexpr int iY1 = 1;
    static constexpr int iZ2 = 2;

    const NCollection_Vec3<TCollection_AsciiString> anXYZ(theArgVec[iX0], theArgVec[iY1], theArgVec[iZ2]);
    if (!anXYZ.x().IsRealValue(true) || !anXYZ.y().IsRealValue(true) || !anXYZ.z().IsRealValue(true)) {
        return Standard_False;
    }

    thePnt.SetValues(anXYZ.x().RealValue(), anXYZ.y().RealValue(), anXYZ.z().RealValue());
    return true;
}

// ================================================================
// Function : ParseXYZ
// ================================================================
bool DrawCommand::ParseXYZ(const char** theArgVec, gp_XYZ& thePnt)
{
    Graphic3d_Vec3d aVec;
    if (ParseVec3(theArgVec, aVec)) {
        thePnt.SetCoord(aVec.x(), aVec.y(), aVec.z());
        return true;
    }
    return false;
}

// ================================================================
// Function : ParseRgbColorIterator
// ================================================================
bool DrawCommand::ParseRgbColorIterator(int             theNbArgs,
                                        const char**    theArgVec,
                                        int&            theArgIter,
                                        Quantity_Color& theColor)
{
    int aNbParsed = Draw::ParseColor(theNbArgs - (theArgIter + 1), theArgVec + (theArgIter + 1), theColor);
    if (aNbParsed == 0) {
        return false;
    }
    theArgIter += aNbParsed;
    return true;
}

// ================================================================
// Function : ParseRgbaColorIterator
// ================================================================
bool DrawCommand::ParseRgbaColorIterator(int                 theNbArgs,
                                         const char**        theArgVec,
                                         int&                theArgIter,
                                         Quantity_ColorRGBA& theColor)
{
    Quantity_Color aColorRgb;
    int            aNbParsed = Draw::ParseColor(theNbArgs - (theArgIter + 1), theArgVec + (theArgIter + 1), aColorRgb);
    if (aNbParsed != 0) {
        theColor.SetRGB(aColorRgb);
        theColor.SetAlpha(1.0f);
    } else {
        aNbParsed = Draw::ParseColor(theNbArgs - (theArgIter + 1), theArgVec + (theArgIter + 1), theColor);
        if (aNbParsed == 0) {
            return false;
        }
    }
    theArgIter += aNbParsed;
    return true;
}

// =======================================================================
// function : DrawCommand
// =======================================================================
DrawCommand::DrawCommand()
{
    //
}

// =======================================================================
// function : ~DrawCommand
// =======================================================================
DrawCommand::~DrawCommand()
{
    //
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool DrawCommand::ParseAllArguments()
{
    for (int anArgIter = 1; anArgIter < myNbArgs; ++anArgIter) {
        const std::shared_ptr<IBinding>* aBind = nullptr;
        if (!myKeyMap.IsEmpty()) {
            TCollection_AsciiString anArgCase(Argument(anArgIter));
            anArgCase.LowerCase();
            aBind = myKeyMap.Seek(anArgCase);
        }
        if (aBind != nullptr && (*aBind)->Parse(this, anArgIter)) {
            continue;
        } else if (aBind != nullptr) {
            *myDI << "Syntax error at '" << Argument(anArgIter) << "'";
            return false;
        }

        if (!ParseArgument(anArgIter)) {
            *myDI << "Syntax error at '" << Argument(anArgIter) << "'";
            return false;
        }
    }
    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool DrawCommand::ParseArgument(int& theArgIter)
{
    const std::shared_ptr<IBinding>* aBind = nullptr;
    if (!myKeyMap.IsEmpty()) {
        TCollection_AsciiString anArgCase(Argument(theArgIter));
        anArgCase.LowerCase();
        aBind = myKeyMap.Seek(anArgCase);
    }
    if (aBind != nullptr && (*aBind)->Parse(this, theArgIter)) {
        return true;
    }
    return false;
}

}