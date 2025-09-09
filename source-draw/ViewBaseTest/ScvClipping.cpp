#include <ViewBaseTest/ScvClipping.h>

#include <ViewerTest.hxx>
#include <ViewerTest_DoubleMapOfInteractiveAndName.hxx>

Standard_IMPORT ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS();

namespace ViewBaseTest {

// =======================================================================
// function : ScvClipping
// =======================================================================
ScvClipping::ScvClipping()
{
    AddOnOffNoKey(this, "-clipping", "-clip")  = &m_isClipping;
    AddOnOffNoKey(this, "-reverse", "-flip")   = &m_toFlip;
    AddOnOffNoKey(this, "-capping", "-cap")    = &m_isCapping;
    AddOnOffNoKey(this, "-shaded", "-shading") = &m_isShaded;

    AddOnOffNoKey(this, "-objectmaterial", "-objmaterial", "objmat") = &m_isObjMat;

    AddKey(this, bool(), "-wireframe") = [](ScvClipping* theCmd, int&, bool theVal) -> bool {
        theCmd->m_isShaded = theVal ? 0 : 1;
        return true;
    };

    AddKey(this, "", "-config", "-cfg") = &ScvClipping::ParseConfig;
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvClipping::ParseAllArguments()
{
    if (!ViewerCommand::ParseAllArguments()) {
        return false;
    }
    if (m_name.IsEmpty()) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvClipping::ParseArgument(int& theArgIter)
{
    TCollection_AsciiString anArgCase(Argument(theArgIter));
    anArgCase.LowerCase();
    if (ViewerCommand::ParseArgument(theArgIter)) {
        //
    } else if (ParseCornerArguments(anArgCase, NbArguments(), AllArguments(), theArgIter)) {
        //
    } else if (ParseExtentArguments(anArgCase, NbArguments(), AllArguments(), theArgIter)) {
        //
    } else if (ParseMinMaxArguments(anArgCase, NbArguments(), AllArguments(), theArgIter)) {
        //
    } else if (m_name.IsEmpty()) {
        m_name = Argument(theArgIter);
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseConfig
// =======================================================================
bool ScvClipping::ParseConfig(const char* theCfg)
{
    TCollection_AsciiString aCfg(theCfg);
    aCfg.LowerCase();

    m_config = Graphic3d_Vec3i(0);
    if (aCfg == "box") {
        m_config.x() = nbClipPlanes2;
        m_config.y() = -nbClipPlanes2;
        m_config.z() = nbClipPlanes2;
        return true;
    }

    int aPos = 0;

    const char* aNames[3] = {"x", "y", "z"};
    for (int aCompIter = 0; aCompIter < Graphic3d_Vec3d::Length(); ++aCompIter) {
        TCollection_AsciiString aDName = TCollection_AsciiString(aNames[aCompIter]) + aNames[aCompIter];
        TCollection_AsciiString aNName = TCollection_AsciiString("-") + aNames[aCompIter];
        TCollection_AsciiString aPName = TCollection_AsciiString("+") + aNames[aCompIter];

        // xx, yy, zz
        aPos = aCfg.Search(aDName);
        if (aPos != -1) {
            aCfg.Remove(aPos, aDName.Length());
            m_config[aCompIter] = nbClipPlanes2;
            continue;
        }

        // -x+x, -y+y, -z+z
        TCollection_AsciiString aDSName = aNName + aPName;
        aPos                            = aCfg.Search(aDSName);
        if (aPos != -1) {
            aCfg.Remove(aPos, aDSName.Length());
            m_config[aCompIter] = -nbClipPlanes2;
            continue;
        }

        // -x, -y, -z
        aPos = aCfg.Search(aNName);
        if (aPos != -1) {
            aCfg.Remove(aPos, aNName.Length());
            m_config[aCompIter] = -1;
        }

        // +x, +y, +z
        aPos = aCfg.Search(aPName);
        if (aPos != -1) {
            aCfg.Remove(aPos, aPName.Length());
            m_config[aCompIter] = Abs(m_config[aCompIter]) + 1;
            continue;
        }

        // x, y, z
        aPos = aCfg.Search(aNames[aCompIter]);
        if (aPos != -1) {
            aCfg.Remove(aPos);
            m_config[aCompIter] = 1;
        }
    }

    if (!aCfg.IsEmpty()) {
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseComp
// =======================================================================
bool ScvClipping::ParseComp(const char* theStr, const int theComp, const bool theIsRange, double& theReal)
{
    TCollection_AsciiString aLower(theStr);
    aLower.LowerCase();
    if (aLower == "+inf" || aLower == "inf") {
        theReal = RealLast();
        return true;
    } else if (aLower == "-inf") {
        theReal = -RealLast();
        return true;
    } else if (aLower.Length() <= 1 || !aLower.EndsWith("%")) {
        return Draw::ParseReal(theStr, theReal);
    }

    aLower = aLower.SubString(1, aLower.Length() - 1);
    if (!Draw::ParseReal(aLower.ToCString(), theReal)) {
        return false;
    }

    Bnd_Box aViewBox = ViewerTest::CurrentView()->View()->MinMaxValues();
    if (aViewBox.IsVoid()) {
        aViewBox.Add(gp::Origin());
    }
    const double aMin = aViewBox.CornerMin().Coord(theComp + 1);
    const double aMax = aViewBox.CornerMax().Coord(theComp + 1);
    if (theIsRange) {
        theReal = theReal * percents001 * (aMax - aMin);
    } else {
        theReal = aMin + theReal * percents001 * (aMax - aMin);
    }
    return true;
}

// =======================================================================
// function : ParsePosXYZ
// =======================================================================
bool ScvClipping::ParsePosXYZ(const char** theArgVec, Graphic3d_Vec3d& theVec)
{
    Graphic3d_Vec3d aVec;
    if (ParsePosX(theArgVec[iX0], aVec) && ParsePosY(theArgVec[iY1], aVec) && ParsePosZ(theArgVec[iZ2], aVec)) {
        theVec = aVec;
        return true;
    }
    return false;
}

// =======================================================================
// function : ParseCornerArguments
// =======================================================================
bool ScvClipping::ParseCornerArguments(const TCollection_AsciiString& theArgCase,
                                       int                            theNbArgs,
                                       const char**                   theArgVec,
                                       int&                           theArgIter)
{
    if (CheckArgName(theArgCase, "-xyz", "-pos", "-corner", "-position")
        && theArgIter + Graphic3d_Vec3d::Length() < theNbArgs && ParsePosXYZ(theArgVec + theArgIter + 1, m_corner)) {
        theArgIter += Graphic3d_Vec3d::Length();
    } else if (CheckArgName(theArgCase, "-x", "-posx") && theArgIter + 1 < theNbArgs
               && ParsePosX(theArgVec[theArgIter + 1], m_corner)) {
        ++theArgIter;
    } else if (CheckArgName(theArgCase, "-y", "-posy") && theArgIter + 1 < theNbArgs
               && ParsePosY(theArgVec[theArgIter + 1], m_corner)) {
        ++theArgIter;
    } else if (CheckArgName(theArgCase, "-z", "-posz") && theArgIter + 1 < theNbArgs
               && ParsePosZ(theArgVec[theArgIter + 1], m_corner)) {
        ++theArgIter;
    } else if (theArgCase == "-scenecenter" || theArgCase == "-center") {
        const Bnd_Box aViewBox = ViewerTest::CurrentView()->View()->MinMaxValues();
        const gp_XYZ  aCenter  = !aViewBox.IsVoid() ? (aViewBox.CornerMin().XYZ() + aViewBox.CornerMax().XYZ()) * 0.5
                                                    : gp::Origin().XYZ();
        m_corner.SetValues(aCenter.X(), aCenter.Y(), aCenter.Z());
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseVecXYZ
// =======================================================================
bool ScvClipping::ParseVecXYZ(const char** theArgVec, Graphic3d_Vec3d& theVec)
{
    Graphic3d_Vec3d aVec;
    if (ParseVecX(theArgVec[iX0], aVec) && ParseVecY(theArgVec[iY1], aVec) && ParseVecZ(theArgVec[iZ2], aVec)) {
        theVec = aVec;
        return true;
    }
    return false;
}

// =======================================================================
// function : ParseExtentArguments
// =======================================================================
bool ScvClipping::ParseExtentArguments(const TCollection_AsciiString& theArgCase,
                                       int                            theNbArgs,
                                       const char**                   theArgVec,
                                       int&                           theArgIter)
{
    if (CheckArgName(theArgCase, "-extent", "-ext") && theArgIter + Graphic3d_Vec3d::Length() < theNbArgs
        && ParseVecXYZ(theArgVec + theArgIter + 1, m_extent)) {
        theArgIter += Graphic3d_Vec3d::Length();
        m_hasExt = NCollection_Vec3<bool>(true);
    } else if (CheckArgName(theArgCase, "-dx", "-extx", "-extentx") && theArgIter + 1 < theNbArgs
               && ParseVecX(theArgVec[theArgIter + 1], m_extent)) {
        ++theArgIter;
        m_hasExt.x() = true;
    } else if (CheckArgName(theArgCase, "-dy", "-exty", "-extenty") && theArgIter + 1 < theNbArgs
               && ParseVecY(theArgVec[theArgIter + 1], m_extent)) {
        ++theArgIter;
        m_hasExt.y() = true;
    } else if (CheckArgName(theArgCase, "-dz", "-extz", "-extentz") && theArgIter + 1 < theNbArgs
               && ParseVecZ(theArgVec[theArgIter + 1], m_extent)) {
        ++theArgIter;
        m_hasExt.z() = true;
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseMinMaxArguments
// =======================================================================
bool ScvClipping::ParseMinMaxArguments(const TCollection_AsciiString& theArgCase,
                                       int                            theNbArgs,
                                       const char**                   theArgVec,
                                       int&                           theArgIter)
{
    if (theArgCase == "-min" && theArgIter + Graphic3d_Vec3d::Length() < theNbArgs
        && ParsePosXYZ(theArgVec + theArgIter + 1, m_boxMin)) {
        theArgIter += Graphic3d_Vec3d::Length();
        m_hasBoxMin = NCollection_Vec3<bool>(true);
    } else if (theArgCase == "-max" && theArgIter + Graphic3d_Vec3d::Length() < theNbArgs
               && ParsePosXYZ(theArgVec + theArgIter + 1, m_boxMax)) {
        theArgIter += Graphic3d_Vec3d::Length();
        m_hasBoxMax = NCollection_Vec3<bool>(true);
    } else if (theArgCase == "-minx" && theArgIter + 1 < theNbArgs && ParsePosX(theArgVec[theArgIter + 1], m_boxMin)) {
        ++theArgIter;
        m_hasBoxMin.x() = true;
    } else if (theArgCase == "-miny" && theArgIter + 1 < theNbArgs && ParsePosY(theArgVec[theArgIter + 1], m_boxMin)) {
        ++theArgIter;
        m_hasBoxMin.y() = true;
    } else if (theArgCase == "-minz" && theArgIter + 1 < theNbArgs && ParsePosZ(theArgVec[theArgIter + 1], m_boxMin)) {
        ++theArgIter;
        m_hasBoxMin.z() = true;
    } else if (theArgCase == "-maxx" && theArgIter + 1 < theNbArgs && ParsePosX(theArgVec[theArgIter + 1], m_boxMax)) {
        ++theArgIter;
        m_hasBoxMax.x() = true;
    } else if (theArgCase == "-maxy" && theArgIter + 1 < theNbArgs && ParsePosY(theArgVec[theArgIter + 1], m_boxMax)) {
        ++theArgIter;
        m_hasBoxMax.y() = true;
    } else if (theArgCase == "-maxz" && theArgIter + 1 < theNbArgs && ParsePosZ(theArgVec[theArgIter + 1], m_boxMax)) {
        ++theArgIter;
        m_hasBoxMax.z() = true;
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvClipping::Perform()
{
    Handle(ViewBase::InteractiveClipping) aClipObj;
    if (const Handle(AIS_InteractiveObject)* anAISObject = GetMapOfAIS().Seek2(m_name)) {
        aClipObj = Handle(ViewBase::InteractiveClipping)::DownCast(*anAISObject);
    }
    if (aClipObj.IsNull()) {
        aClipObj = new ViewBase::InteractiveClipping(new ViewBase::InteractiveClippingOwner(m_name));
        aClipObj->ViewAffinity()->SetVisible(false);
        aClipObj->ViewAffinity()->SetVisible(ViewerTest::CurrentView()->View()->Identification(), true);
        ViewerTest::CurrentView()->SetClipPlanes(aClipObj->ManagedPlanes());
    }

    SetClippingCorner(aClipObj);
    SetClippingConfig(aClipObj);
    SetClippingExtent(aClipObj);
    SetClippingBox(aClipObj);
    SetClippingFlags(aClipObj);

    ViewerTest::Display(m_name, aClipObj, false);
    PrintInfo(*myDI, aClipObj);
    return true;
}

// =======================================================================
// function : PrintRealXYZRel
// =======================================================================
TCollection_AsciiString ScvClipping::PrintRealXYZRel(const double* theVec) const
{
    Bnd_Box aViewBox = ViewerTest::CurrentView()->View()->MinMaxValues();
    if (aViewBox.IsVoid()) {
        aViewBox.Add(gp::Origin());
    }
    const gp_XYZ aMin   = aViewBox.CornerMin().XYZ();
    const gp_XYZ aMax   = aViewBox.CornerMax().XYZ();
    const gp_XYZ aRange = aMax - aMin;

    gp_XYZ aRes = gp_XYZ(theVec[iX0], theVec[iY1], theVec[iZ2]) - aMin;
    if (Abs(aRange.X()) > Precision::Confusion()) {
        aRes.SetX(aRes.X() / aRange.X());
    }
    if (Abs(aRange.Y()) > Precision::Confusion()) {
        aRes.SetY(aRes.Y() / aRange.Y());
    }
    if (Abs(aRange.Z()) > Precision::Confusion()) {
        aRes.SetZ(aRes.Z() / aRange.Z());
    }
    aRes *= percents100;

    return PrintReal(aRes.Coord(iX0 + 1)) + "% " + PrintReal(aRes.Coord(iY1 + 1)) + "% "
         + PrintReal(aRes.Coord(iZ2 + 1)) + "%";
}

// =======================================================================
// function : PrintInfo
// =======================================================================
void ScvClipping::PrintInfo(Draw_Interpretor& theDI, const Handle(ViewBase::InteractiveClipping)& theClipObj)
{
    const gp_Pnt             aCorner  = theClipObj->GetClippingCorner();
    const gp_Pnt             aCorRel  = theClipObj->GetClippingCorner(true).XYZ() * percents100;
    const gp_Vec             anExt    = theClipObj->GetClippingExtent();
    const gp_Vec             anExtRel = theClipObj->GetClippingExtent(true) * percents100;
    const Graphic3d_BndBox3d aBox     = theClipObj->GetClippingBox();
    theDI << "Corner: " << PrintRealXYZ(aCorner.XYZ().GetData()) << "\t(" << PrintRealXYZ(aCorRel.XYZ().GetData(), "%")
          << ")\n";
    theDI << "Extent: " << PrintRealXYZ(anExt.XYZ().GetData()) << "\t(" << PrintRealXYZ(anExtRel.XYZ().GetData(), "%")
          << ")\n";
    ;
    theDI << "Min:    " << PrintRealXYZ(aBox.CornerMin()) << "\t(" << PrintRealXYZRel(aBox.CornerMin()) << ")\n";
    theDI << "Max:    " << PrintRealXYZ(aBox.CornerMax()) << "\t(" << PrintRealXYZRel(aBox.CornerMax()) << ")\n";
}

// =======================================================================
// function : SetClippingCorner
// =======================================================================
void ScvClipping::SetClippingCorner(const Handle(ViewBase::InteractiveClipping)& theClipObj)
{
    if (!Precision::IsInfinite(m_corner.x()) || !Precision::IsInfinite(m_corner.y())
        || !Precision::IsInfinite(m_corner.z())) {
        theClipObj->SetClippingCorner(gp_XYZ(m_corner.x(), m_corner.y(), m_corner.z()));
    }
}

// =======================================================================
// function : SetClippingConfig
// =======================================================================
void ScvClipping::SetClippingConfig(const Handle(ViewBase::InteractiveClipping)& theClipObj)
{
    if (m_config.minComp() != 0 || m_config.maxComp() != 0) {
        theClipObj->SetNbPlanesXYZ(m_config);
    }
}

// =======================================================================
// function : SetClippingExtent
// =======================================================================
void ScvClipping::SetClippingExtent(const Handle(ViewBase::InteractiveClipping)& theClipObj)
{
    gp_XYZ anExt = theClipObj->GetClippingExtent();
    for (int aCompIter = 0; aCompIter < Graphic3d_Vec3d::Length(); ++aCompIter) {
        if (m_hasExt[aCompIter]) {
            anExt.SetCoord(aCompIter + 1, m_extent[aCompIter]);
        }
    }
    if (m_hasExt.x() || m_hasExt.y() || m_hasExt.z()) {
        theClipObj->SetClippingExtent(anExt);
    }
}

// =======================================================================
// function : SetClippingBox
// =======================================================================
void ScvClipping::SetClippingBox(const Handle(ViewBase::InteractiveClipping)& theClipObj)
{
    bool               toSetBox = false;
    Graphic3d_BndBox3d aBox     = theClipObj->GetClippingBox();
    for (int aCompIter = 0; aCompIter < Graphic3d_Vec3d::Length(); ++aCompIter) {
        if (m_hasBoxMin[aCompIter]) {
            aBox.CornerMin()[aCompIter] = m_boxMin[aCompIter];
            toSetBox                    = true;
        }
        if (m_hasBoxMax[aCompIter]) {
            aBox.CornerMax()[aCompIter] = m_boxMax[aCompIter];
            toSetBox                    = true;
        }
    }
    if (toSetBox) {
        theClipObj->SetClippingBox(aBox);
    }
}

// =======================================================================
// function : SetClippingFlags
// =======================================================================
void ScvClipping::SetClippingFlags(const Handle(ViewBase::InteractiveClipping)& theClipObj)
{
    if (m_toFlip != -1) {
        theClipObj->SetFlipClipping(m_toFlip == 1);
    }
    if (m_isClipping != -1) {
        theClipObj->SetClippingOn(m_isClipping == 1);
    }
    if (m_isCapping != -1) {
        theClipObj->SetCappingOn(m_isCapping == 1);
    }
    if (m_isObjMat != -1) {
        theClipObj->SetUseObjectMaterial(m_isObjMat == 1);
    }
    if (m_isShaded != -1) {
        theClipObj->SetDatumDisplayMode(m_isShaded == 1 ? Prs3d_DM_Shaded : Prs3d_DM_WireFrame);
    }
}

}