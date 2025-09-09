#include <ViewCaeTest/ScvMeshActiveProp.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshActiveProp
// =======================================================================
ScvMeshActiveProp::ScvMeshActiveProp()
{
    AddKey(this, int(), "-comp") = &m_comp;

    AddKey(this, int(), "-nodal") = [](ScvMeshActiveProp* theCmd, int&, int theVal) -> bool {
        theCmd->m_isElemental = false;
        if (theVal < 0) {
            return false;
        }
        theCmd->m_index = theVal;
        return true;
    };
    AddKey(this, int(), "-elem", "-elemental") = [](ScvMeshActiveProp* theCmd, int&, int theVal) -> bool {
        theCmd->m_isElemental = true;
        if (theVal < 0) {
            return false;
        }
        theCmd->m_index = theVal;
        return true;
    };

    AddVoidKey(this, "-unset") = [](ScvMeshActiveProp* theCmd, int&) -> bool {
        theCmd->m_index = -1;
        return true;
    };

    AddRangeKey(this, 0, IntegerLast(), "-timestep", "-time") = &m_timeStep;

    AddKey(this, true, "-normalize", "-norm") = &m_toNormalize;
}

// =======================================================================
// function : FindPropertyByIndex
// =======================================================================
Handle(RWCae::MeshNVecProperty) ScvMeshActiveProp::FindPropertyByIndex(int theIndex)
{
    const Handle(RWCae::MeshData)&   aMeshData   = m_mesh->GetMeshData();
    const RWCae::MeshNVecProperties& aProperties = aMeshData->Properties();

    int anIndex = -1;
    for (const Handle(RWCae::MeshNVecProperty)& aProp : aProperties) {
        if (((m_isElemental && aProp->IsElemental()) || (!m_isElemental && !aProp->IsElemental()))
            && ++anIndex == theIndex) {
            return aProp;
        }
    }
    return Handle(RWCae::MeshNVecProperty)();
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshActiveProp::Perform()
{
    if (m_index < -1 && m_propId.IsEmpty() && m_comp == -1) {
        PrintPropsInfo(*myDI);
        return true;
    }

    const Handle(RWCae::MeshData)&  aMeshData = m_mesh->GetMeshData();
    Handle(RWCae::MeshNVecProperty) aProperty;
    if (m_index >= 0) {
        aProperty = FindPropertyByIndex(m_index);
        if (aProperty.IsNull()) {
            *myDI << "Syntax error: cannot find a property with index " << m_index;
            return false;
        }
    } else if (!m_propId.IsEmpty()) {
        aProperty = aMeshData->GetPropertyById(m_propId, m_timeStep);
        if (aProperty.IsNull()) {
            if (m_timeStep < 0) {
                *myDI << "Syntax error: cannot find a property with id '" << m_propId << "'";
            } else {
                *myDI << "Syntax error: cannot find a property with id '" << m_propId << "' and time step '"
                      << m_timeStep << "'";
            }
            return false;
        }
    }

    if (m_toNormalize) {
        if (aProperty->IsVector3()) {
            aProperty->NormalizeVector();
        } else {
            *myDI << "Syntax error: cannot normalize a non-vector property";
            return false;
        }
    }

    const TCollection_AsciiString& aPrsId = m_mesh->PresentationByIndex(m_prsIndex)->Id();
    m_mesh->SetProperty(aPrsId, aProperty);
    if (!aProperty.IsNull() && m_comp != -1) {
        if (!aProperty->EnabledComponents().Contains(m_comp)) {
            *myDI << "Syntax error: cannot find a component '" << m_comp << "'";
            return false;
        }
        m_mesh->SetPropertyComponentIndex(aPrsId, m_comp);
    }

    UpdateMesh();

    return true;
}

// =======================================================================
// function : PrintPropsInfo
// =======================================================================
void ScvMeshActiveProp::PrintPropsInfo(Draw_Interpretor& theDI)
{
    const RWCae::MeshNVecProperties& aProperties = m_mesh->GetMeshData()->Properties();
    theDI << "Nodal properties:\n";
    for (const Handle(RWCae::MeshNVecProperty)& aProp : aProperties) {
        if (aProp->IsNodal()) {
            PrintPropInfo(theDI, aProp);
        }
    }
    theDI << "Elemental properties:\n";
    for (const Handle(RWCae::MeshNVecProperty)& aProp : aProperties) {
        if (aProp->IsElemental()) {
            PrintPropInfo(theDI, aProp);
        }
    }
}

// =======================================================================
// function : PrintPropInfo
// =======================================================================
void ScvMeshActiveProp::PrintPropInfo(Draw_Interpretor& theDI, const Handle(RWCae::MeshNVecProperty)& theProp)
{
    theDI << "\tId='" << theProp->Id() << "', Name='" << theProp->Name() << "', Description=" << theProp->Description()
          << "', NbValues=" << theProp->NbValues() << ", "
          << ", TimeStepDescription='" << theProp->TimeStepDescription() << "', TimeStep=" << theProp->TimeStep()
          << ", Time=" << theProp->Time() << ", TimeDisplayUnits='" << theProp->TimeDisplayUnits()
          << "', Loaded=" << (theProp->IsLoaded() ? "true" : "false") << "\n";
    int aNbComponents = theProp->NbRawComponents();
    for (int i = 0; i < aNbComponents; i++) {
        const Handle(RWCae::MeshScalarPropertyBase)& aComp = theProp->GetRawComponent(i);

        double aLowerRange = 0.0, anUpperRange = 0.0;
        aComp->ValueRange().GetBounds(aLowerRange, anUpperRange);
        theDI << "\t\tId='" << aComp->Id() << "', Name='" << aComp->Name() << "', Enabled=" << aComp->IsEnabled()
              << ", Range=(" << aLowerRange << ", " << anUpperRange << "), DisplayUnits='" << aComp->DisplayUnits()
              << "', Loaded=" << (aComp->IsLoaded() ? "true" : "false") << "\n";
    }
}

}

