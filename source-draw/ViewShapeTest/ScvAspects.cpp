#include <ViewShapeTest/ScvAspects.h>

#include <ViewerTest.hxx>
#include <ViewerTest_DoubleMapOfInteractiveAndName.hxx>
extern ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS();

namespace ViewShapeTest {

// =======================================================================
// function : ScvAspects
// =======================================================================
ScvAspects::ScvAspects()
{
    AddKey(this, "", "-prsmode", "-dispmode") = &ScvAspects::ParsePrsMode;

    AddVoidKey(this, "-nosubcolors", "-unsetsubcolors", "-resetsubcolors") = [](ScvAspects* theCmd, int&) -> bool {
        theCmd->m_unsetSubcolors = true;
        ++theCmd->m_nbModif;
        return true;
    };
    AddVoidKey(this, "-nomaterial", "-unsetmaterial", "-resetmaterial") = [](ScvAspects* theCmd, int&) -> bool {
        theCmd->m_resetMaterial = true;
        ++theCmd->m_nbModif;
        return true;
    };

    AddOnOffNoKey(this, "-infinite", "-inf") = [](ScvAspects* theCmd, int&, bool theVal) -> bool {
        if (theCmd->m_isInfinite == -1) {
            theCmd->m_isInfinite = theVal ? 1 : 0;
            ++theCmd->m_nbModif;
            return true;
        }
        return false;
    };

    AddPbrKeys();
    AddForcedKeys();
}

// =======================================================================
// function : AddPbrKeys
// =======================================================================
void ScvAspects::AddPbrKeys()
{
    AddKey(this, Quantity_ColorRGBA(), "-basecolor", "-color") =
        [](ScvAspects* theCmd, int&, Quantity_ColorRGBA theVal) -> bool {
        if (theCmd->m_baseColor.r() == ShortRealLast()) {
            theCmd->m_baseColor = theVal;
            ++theCmd->m_nbModif;
            return true;
        }
        return false;
    };
    AddKey(this, Quantity_Color(), "-emissivecolor", "-emission") =
        [](ScvAspects* theCmd, int&, Quantity_Color theVal) -> bool {
        if (theCmd->m_emissiveColor.r() == ShortRealLast()) {
            theCmd->m_emissiveColor = theVal;
            ++theCmd->m_nbModif;
            return true;
        }
        return false;
    };
    AddKey(this, 1.0, "-metallic", "-metalness") = [](ScvAspects* theCmd, int&, double theVal) -> bool {
        if (theCmd->m_metallic == RealLast() && theVal >= 0.0 && theVal <= 1.0) {
            theCmd->m_metallic = theVal;
            ++theCmd->m_nbModif;
            return true;
        }
        return false;
    };
    AddKey(this, 1.0, "-roughness") = [](ScvAspects* theCmd, int&, double theVal) -> bool {
        if (theCmd->m_roughness == RealLast() && theVal >= 0.0 && theVal <= 1.0) {
            theCmd->m_roughness = theVal;
            ++theCmd->m_nbModif;
            return true;
        }
        return false;
    };
    AddKey(this, 1.0, "-ior", "-refractionindex") = [](ScvAspects* theCmd, int&, double theVal) -> bool {
        if (theCmd->m_refractionIndex == RealLast() && theVal >= 0.0) {
            theCmd->m_refractionIndex = theVal;
            ++theCmd->m_nbModif;
            return true;
        }
        return false;
    };
}

// =======================================================================
// function : AddForcedKeys
// =======================================================================
void ScvAspects::AddForcedKeys()
{
    AddKey(this, Quantity_Color(), "-forcedcolor", "-overcolor") =
        [](ScvAspects* theCmd, int&, Quantity_Color theVal) -> bool {
        if (theCmd->m_forcedColor.r() == ShortRealLast()) {
            theCmd->m_forcedColor = theVal;
            ++theCmd->m_nbModif;
            return true;
        }
        return false;
    };
    AddVoidKey(this, "-noforcedcolor", "-unsetforcedcolor", "-noovercolor", "unsetovercolor") = [](ScvAspects* theCmd,
                                                                                                   int&) -> bool {
        if (theCmd->m_forcedColor.r() == ShortRealLast()) {
            theCmd->m_forcedColor = Graphic3d_Vec3(-1.0f);
            ++theCmd->m_nbModif;
            return true;
        }
        return false;
    };
    AddKey(this, 1.0, "-forcedtransparency", "forcedtransp", "-overtransparency") =
        [](ScvAspects* theCmd, int&, double theVal) -> bool {
        if (theCmd->m_forcedTransp == RealLast() && theVal >= 0.0 && theVal <= 1.0) {
            theCmd->m_forcedTransp = theVal;
            ++theCmd->m_nbModif;
            return true;
        }
        return false;
    };
    AddVoidKey(this, "-noforcedtransparency", "-noforcedtransp", "-unsetforcedtransparency", "noovertransparency") =
        [](ScvAspects* theCmd, int&) -> bool {
        if (theCmd->m_forcedTransp == RealLast()) {
            theCmd->m_forcedTransp = -1.0;
            ++theCmd->m_nbModif;
            return true;
        }
        return false;
    };
    AddVoidKey(this, "-noovertransp", "-unsetovertransparency") = [](ScvAspects* theCmd, int&) -> bool {
        if (theCmd->m_forcedTransp == RealLast()) {
            theCmd->m_forcedTransp = -1.0;
            ++theCmd->m_nbModif;
            return true;
        }
        return false;
    };
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvAspects::ParseAllArguments()
{
    if (!ViewerCommand::ParseAllArguments()) {
        return false;
    }
    if (m_nbModif == 0) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }

    if (m_prsList.IsEmpty()) {
        m_context->ObjectsInside(m_prsList);
    }

    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvAspects::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        //
    } else if (ParsePrsMode(Argument(theArgIter))) { // svcdispmode alias
        //
    } else {
        Handle(ViewShape::InteractiveShape) aShapeObj;
        if (!ViewBase::IInteractiveObject::FindObjectById(aShapeObj, *m_context, Argument(theArgIter))) {
            Handle(AIS_InteractiveObject) anAisObj;
            GetMapOfAIS().Find2(Argument(theArgIter), anAisObj);
            aShapeObj = Handle(ViewShape::InteractiveShape)::DownCast(anAisObj);
        }

        if (aShapeObj.IsNull()) {
            *myDI << "Syntax error: object '" << Argument(theArgIter) << "' is not InteractiveShape\n";
            return false;
        }

        m_prsList.Append(aShapeObj);
    }
    return true;
}

// =======================================================================
// function : ParsePrsMode
// =======================================================================
bool ScvAspects::ParsePrsMode(const char* theName)
{
    if (m_shapeMode != -1) {
        return false;
    }

    TCollection_AsciiString aName(theName);
    aName.LowerCase();
    if (CheckArgName(aName, "flatshading", "flatshaded", "shadedflat", "flat")) {
        m_shapeMode = (int)ViewShape::InteractiveShapePrsMode::ShadedFlat;
    } else if (CheckArgName(aName, "hiddenline", "hlr")) {
        m_shapeMode = (int)ViewShape::InteractiveShapePrsMode::HiddenLine;
    } else if (aName == "wireframe") {
        m_shapeMode = (int)ViewShape::InteractiveShapePrsMode::Wireframe;
    } else if (CheckArgName(aName, "smoothshading", "smoothshaded", "shading", "shaded") || aName == "smooth") {
        m_shapeMode = (int)ViewShape::InteractiveShapePrsMode::Shaded;
    } else if (CheckArgName(aName, "edgeshading", "edgeshaded", "shadedboundaries", "shadedfaceboundaries")) {
        m_shapeMode = (int)ViewShape::InteractiveShapePrsMode::ShadedFaceBoundaries;
    } else if (CheckArgName(aName, "shadedtessellation", "tessellation", "shadedtess", "triangulation")) {
        m_shapeMode = (int)ViewShape::InteractiveShapePrsMode::ShadedTessellation;
    } else {
        return false;
    }

    ++m_nbModif;
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvAspects::Perform()
{
    for (const Handle(AIS_InteractiveObject)& aPrsIter : m_prsList) {
        Handle(ViewShape::InteractiveShape) anObj = Handle(ViewShape::InteractiveShape)::DownCast(aPrsIter);
        if (anObj.IsNull() || !anObj->IsModel()) {
            continue;
        }

        ProcessShape(anObj);
    }

    return true;
}

// =======================================================================
// function : ProcessShape
// =======================================================================
void ScvAspects::ProcessShape(const Handle(ViewShape::InteractiveShape)& theObj)
{
    theObj->SetPresentationMode((ViewShape::InteractiveShapePrsMode)m_shapeMode);

    const Handle(ViewShape::InteractiveShapeOwner)& anOwner = theObj->MeshOwner();
    if (m_forcedColor.r() != ShortRealLast()) {
        if (m_forcedColor.r() >= 0.0f) {
            anOwner->SetForcedColor(Quantity_Color(m_forcedColor));
        } else {
            anOwner->UnsetForcedColor();
        }
    }
    if (m_forcedTransp != RealLast()) {
        if (m_forcedTransp >= 0.0) {
            anOwner->SetForcedTransparency((float)m_forcedTransp);
        } else {
            anOwner->UnsetForcedTransparency();
        }
    }

    if (m_isInfinite != -1) {
        anOwner->SetInfiniteState(m_isInfinite == 1);
        theObj->SetToUpdate();
    }

    if (m_unsetSubcolors) {
        anOwner->ShapeData()->ChangeAttributeMap().Clear();
        theObj->SetToUpdate();
    }
    ApplyPbrMaterial(anOwner);
    theObj->UpdateAspects();
    if (anOwner->IsLoaded()) {
        m_context->Display(theObj, false);
    }
}

// =======================================================================
// function : ApplyPbrMaterial
// =======================================================================
void ScvAspects::ApplyPbrMaterial(const Handle(ViewShape::InteractiveShapeOwner)& theOwner)
{
    if (m_resetMaterial) {
        theOwner->ChangeBaseStyle().SetMaterial(Handle(XCAFDoc_VisMaterial)());
    }
    if (m_baseColor.r() != ShortRealLast()) {
        theOwner->InitBaseMaterial();

        XCAFDoc_VisMaterialPBR aPbrMat = theOwner->BaseStyle().Material()->PbrMaterial();
        aPbrMat.BaseColor              = Quantity_ColorRGBA(m_baseColor);
        theOwner->BaseStyle().Material()->SetPbrMaterial(aPbrMat);
    }
    if (m_emissiveColor.r() != ShortRealLast()) {
        theOwner->InitBaseMaterial();

        XCAFDoc_VisMaterialPBR aPbrMat = theOwner->BaseStyle().Material()->PbrMaterial();
        aPbrMat.EmissiveFactor         = m_emissiveColor;
        theOwner->BaseStyle().Material()->SetPbrMaterial(aPbrMat);
    }
    if (m_metallic != RealLast()) {
        theOwner->InitBaseMaterial();

        XCAFDoc_VisMaterialPBR aPbrMat = theOwner->BaseStyle().Material()->PbrMaterial();
        aPbrMat.Metallic               = (float)m_metallic;
        theOwner->BaseStyle().Material()->SetPbrMaterial(aPbrMat);
    }
    if (m_roughness != RealLast()) {
        theOwner->InitBaseMaterial();

        XCAFDoc_VisMaterialPBR aPbrMat = theOwner->BaseStyle().Material()->PbrMaterial();
        aPbrMat.Roughness              = (float)m_roughness;
        theOwner->BaseStyle().Material()->SetPbrMaterial(aPbrMat);
    }
    if (m_refractionIndex != RealLast()) {
        theOwner->InitBaseMaterial();

        XCAFDoc_VisMaterialPBR aPbrMat = theOwner->BaseStyle().Material()->PbrMaterial();
        aPbrMat.RefractionIndex        = (float)m_refractionIndex;
        theOwner->BaseStyle().Material()->SetPbrMaterial(aPbrMat);
    }
}

}
