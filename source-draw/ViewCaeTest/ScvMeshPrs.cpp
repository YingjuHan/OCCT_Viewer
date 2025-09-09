#include <ViewCaeTest/ScvMeshPrs.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshPrs
// =======================================================================
ScvMeshPrs::ScvMeshPrs()
{
    AddKey(this, "", "-create") = [](ScvMeshPrs* theCmd, const char* theVal) -> bool {
        theCmd->m_toCreate = true;
        TCollection_AsciiString aVal(theVal);
        aVal.LowerCase();
        if (aVal == "scalar") {
            theCmd->m_type = ViewCae::MeshPresentationType::Scalar;
        } else if (aVal == "vector") {
            theCmd->m_type = ViewCae::MeshPresentationType::Vector;
        } else if (aVal == "streamlines") {
            theCmd->m_type = ViewCae::MeshPresentationType::Streamlines;
        } else {
            return false;
        }
        return true;
    };

    AddKey(this, int(), "-remove") = &m_indexToRemove;
    AddOnOffNoKey(this, "-info")   = &m_toShowInfo;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshPrs::Perform()
{
    if (m_toShowInfo) {
        std::cout << "Presentations:\n";
        for (const Handle(ViewCae::MeshPresentationBase)& aPrsBase : m_mesh->Presentations()) {
            const ViewCae::MeshPresentation* aPrs = dynamic_cast<const ViewCae::MeshPresentation*>(aPrsBase.get());
            std::cout << "Id = '" << aPrs->Id() << "', ";
            std::cout << "Type = '" << (aPrs->Type() == ViewCae::MeshPresentationType::Scalar ? "Scalar" : "Vector")
                      << "'\n";
        }
    }

    if (m_toCreate) {
        m_mesh->CreatePresentation(m_type);
    }

    if (m_indexToRemove >= 0) {
        m_mesh->RemovePresentation(m_mesh->PresentationByIndex(m_indexToRemove)->Id());
    }

    UpdateMesh();
    return true;
}

}
