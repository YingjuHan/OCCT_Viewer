
#include <ViewCaeTest/ScvMeshTestGroup.h>

#include <RWCae/MeshAdaptorsIterator.h>
#include <RWCae/MeshGroupProperty.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshTestGroup
// =======================================================================
ScvMeshTestGroup::ScvMeshTestGroup()
{
    AddKey(this, "", "-id") = [](ScvMeshTestGroup* theCmd, const char* theVal) -> bool {
        theCmd->m_id = theVal;
        return true;
    };
    AddVoidKey(this, "-nodal") = [](ScvMeshTestGroup* theCmd) -> bool {
        theCmd->m_isElemental = false;
        return true;
    };
    AddVoidKey(this, "-elemental") = [](ScvMeshTestGroup* theCmd) -> bool {
        theCmd->m_isElemental = true;
        return true;
    };

    AddRangeKey(this, 0, RWCae::MeshGroupProperty::MaxNbGroups - 1, "-nbgroups") = &m_nbGroups;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshTestGroup::Perform()
{
    RWCae::MeshPropertyBase::MappingType aType = m_isElemental ? RWCae::MeshPropertyBase::MappingType::Elemental
                                                                 : RWCae::MeshPropertyBase::MappingType::Nodal;

    const Handle(RWCae::MeshData)&  aMeshData = m_mesh->GetMeshData();
    const int                         aSize     = m_isElemental ? aMeshData->NbIndexedElems() : aMeshData->NbNodes();
    Handle(RWCae::MeshNVecProperty) aProp =
        new RWCae::MeshNVecProperty(m_isElemental ? "Elem. groups" : "Nodal groups", 1, aType);
    aProp->Init<RWCae::MeshGroupProperty>();
    if (!m_id.IsEmpty()) {
        aProp->SetId(m_id);
    }
    Handle(RWCae::MeshGroupProperty) aComp = Handle(RWCae::MeshGroupProperty)::DownCast(aProp->GetRawComponent(0));
    aComp->Init(aSize);
    Bnd_Box aBox = aMeshData->BoundingBox();
    aBox.Enlarge(gp::Resolution());
    if (m_isElemental) {
        for (int i = 0; i < aSize; i++) {
            Graphic3d_Vec3                           anElemCenter;
            const RWCae::MeshData::IndexedElement& anElem = aMeshData->IndexedElements()[i];
            if (!ElementCenter(aMeshData, anElem, anElemCenter)) {
                continue;
            }
            aComp->SetGroup(i, GenerateGroup(anElemCenter, aBox));
        }
    } else {
        for (int i = 0; i < aSize; i++) {
            const Graphic3d_Vec3& aNode = aMeshData->Nodes()[i];
            aComp->SetGroup(i, GenerateGroup(aNode, aBox));
        }
    }
    for (int i = 0; i < RWCae::MeshGroupProperty::MaxNbGroups; i++) {
        aComp->SetGroupName(i, TCollection_AsciiString("") + i);
    }

    aProp->UpdateComponents(false);

    aMeshData->ChangeProperties().Append(aProp);
    return true;
}

// =======================================================================
// function : ElementCenter
// =======================================================================
bool ScvMeshTestGroup::ElementCenter(const Handle(RWCae::MeshData)&         theMeshData,
                                     const RWCae::MeshData::IndexedElement& theElem,
                                     Graphic3d_Vec3&                          theCenter)
{
    auto compute = [&theCenter, &theElem](const auto& theAdaptor) {
        theCenter = theAdaptor.ElementCenter(theElem.Index);
        return true;
    };

    return RWCae::MeshAdaptorsIteratorAllElems::Switch(theElem.Type, compute, theMeshData);
}

}

