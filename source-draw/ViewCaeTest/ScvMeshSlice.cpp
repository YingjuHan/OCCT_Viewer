
#include <ViewCaeTest/ScvMeshSlice.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshSlice
// =======================================================================
ScvMeshSlice::ScvMeshSlice()
{
    AddOnOffNoKey(this, "-clear") = &m_toClear;

    AddKey(this, Graphic3d_Vec4d(), "-add") = [](ScvMeshSlice* theCmd, int&, Graphic3d_Vec4d theVal) -> bool {
        theCmd->m_slices.Append(new Graphic3d_ClipPlane(theVal));
        return true;
    };
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshSlice::Perform()
{
    if (m_slices.IsEmpty() && !m_toClear) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }

    const TCollection_AsciiString& aPrsId = m_mesh->PresentationByIndex(m_prsIndex)->Id();

    if (m_toClear) {
        m_mesh->SetSlicePlanes(aPrsId, Graphic3d_SequenceOfHClipPlane());
    }

    if (!m_slices.IsEmpty()) {
        Graphic3d_SequenceOfHClipPlane aSlices = m_mesh->SlicePlanes(aPrsId);
        for (Graphic3d_SequenceOfHClipPlane::Iterator anIter(m_slices); anIter.More(); anIter.Next()) {
            aSlices.Append(anIter.Value());
        }
        m_mesh->SetSlicePlanes(aPrsId, aSlices);
    }

    UpdateMesh();
    return true;
}

}
