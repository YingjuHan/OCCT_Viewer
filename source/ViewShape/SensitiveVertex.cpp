#include <ViewShape/SensitiveVertex.h>

#include <ViewShape/InteractiveShapeOwner.h>

#include <BRep_Tool.hxx>

namespace ViewShape
{

    // ================================================================
    // Function : SensitiveVertex
    // ================================================================
    SensitiveVertex::SensitiveVertex(const Handle(SelectMgr_EntityOwner) & theOwnerId, const TopoDS_Vertex &theVertex)
        : Select3D_SensitivePoint(theOwnerId, BRep_Tool::Pnt(theVertex)),
          myVertex(theVertex)
    {
        //
    }

    // ================================================================
    // Function : ~SensitiveVertex
    // ================================================================
    SensitiveVertex::~SensitiveVertex()
    {
        //
    }

    // ================================================================
    // Function : Matches
    // ================================================================
    Standard_Boolean SensitiveVertex::Matches(SelectBasics_SelectingVolumeManager &theMgr,
                                              SelectBasics_PickResult &thePickResult)
    {
        const InteractiveShapeOwner *anOwner = dynamic_cast<InteractiveShapeOwner *>(myOwnerId.get());
        if (anOwner != nullptr && anOwner->GetSelectionMode() != (int)InteractiveShapeSelMode::Global)
        {
            if (mySubshapeIndex != -1 && anOwner->GetDisabledElems().Contains(ShapeSelElemType::Vertex, mySubshapeIndex))
            {
                return false;
            }
        }

        return Select3D_SensitivePoint::Matches(theMgr, thePickResult);
    }

} // namespace ViewShape