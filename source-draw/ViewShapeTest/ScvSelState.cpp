
#include <ViewShapeTest/ScvSelState.h>

#include <ViewerTest.hxx>
#include <ViewerTest_DoubleMapOfInteractiveAndName.hxx>

Standard_IMPORT ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS();

namespace ViewShapeTest {

// =======================================================================
// function : ScvSelState
// =======================================================================
ScvSelState::ScvSelState()
{
    m_autoUpdater.Invalidate();

    AddVoidKey(this, "-vertex", "-verts", "vertex", "vertices") = [](ScvSelState* theCmd) -> bool {
        theCmd->m_toPrintSelModes |= (unsigned int)ViewShape::InteractiveShapeSelMode::Vertex;
        return true;
    };
    AddVoidKey(this, "-edge", "-edges", "edge", "edges") = [](ScvSelState* theCmd) -> bool {
        theCmd->m_toPrintSelModes |= (unsigned int)ViewShape::InteractiveShapeSelMode::Edge;
        return true;
    };
    AddVoidKey(this, "-wire", "-wires", "wire", "wires") = [](ScvSelState* theCmd) -> bool {
        theCmd->m_toPrintSelModes |= (unsigned int)ViewShape::InteractiveShapeSelMode::Wire;
        return true;
    };
    AddVoidKey(this, "-face", "-faces", "face", "faces") = [](ScvSelState* theCmd) -> bool {
        theCmd->m_toPrintSelModes |= (unsigned int)ViewShape::InteractiveShapeSelMode::Face;
        return true;
    };
    AddVoidKey(this, "-shell", "-shells", "shell", "shells") = [](ScvSelState* theCmd) -> bool {
        theCmd->m_toPrintSelModes |= (unsigned int)ViewShape::InteractiveShapeSelMode::Shell;
        return true;
    };
    AddVoidKey(this, "-solid", "-solids", "solid", "solids") = [](ScvSelState* theCmd) -> bool {
        theCmd->m_toPrintSelModes |= (unsigned int)ViewShape::InteractiveShapeSelMode::Solid;
        return true;
    };
    AddVoidKey(this, "-compsolid", "-compsolids", "compsolid", "compsolids") = [](ScvSelState* theCmd) -> bool {
        theCmd->m_toPrintSelModes |= (unsigned int)ViewShape::InteractiveShapeSelMode::CompSolid;
        return true;
    };

    AddOnOffNoKey(this, "-detected") = &m_toPrintDetected;
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvSelState::ParseAllArguments()
{
    if (!ViewerCommand::ParseAllArguments()) {
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvSelState::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        //
    } else if (m_prsName.IsEmpty()) {
        m_prsName = Argument(theArgIter);
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvSelState::Perform()
{
    if (m_prsName.IsEmpty() && (m_toPrintSelModes != 0)) {
        return false;
    }

    if (m_toPrintDetected) {
        PrintDetected(*myDI);
        return true;
    }

    PrintSelected(*myDI);
    return true;
}

// =======================================================================
// function : PrintDetected
// =======================================================================
void ScvSelState::PrintDetected(Draw_Interpretor& theDI) const
{
    if (m_toPrintSelModes == 0) {
        theDI << "Detected entities:\n";
    }

    Handle(StdSelect_ViewerSelector3d) aSelector = m_context->MainSelector();
    for (int aPickIter = 1; aPickIter <= aSelector->NbPicked(); ++aPickIter) {
        const Handle(Select3D_SensitiveEntity)& anEntity = aSelector->PickedEntity(aPickIter);
        const Handle(SelectMgr_EntityOwner)&    anOwner  = anEntity->OwnerId();
        Handle(AIS_InteractiveObject)           anObj = Handle(AIS_InteractiveObject)::DownCast(anOwner->Selectable());

        TCollection_AsciiString aName;
        GetMapOfAIS().Find1(anObj, aName);
        if (!m_prsName.IsEmpty() && m_prsName != aName) {
            continue;
        }

        if (m_toPrintSelModes == 0) {
            theDI << "  " << aName << " (" << anEntity->DynamicType()->Name() << ")"
                  << "\n";
        }

        PrintDetectedShapeOwner(theDI, Handle(ViewShape::InteractiveShapeOwner)::DownCast(anOwner));
    }
}

// =======================================================================
// function : PrintDetectedShapeOwner
// =======================================================================
void ScvSelState::PrintDetectedShapeOwner(Draw_Interpretor&                                theDI,
                                          const Handle(ViewShape::InteractiveShapeOwner)& theShapeOwner) const
{
    if (theShapeOwner.IsNull()) {
        return;
    }

    const ViewShape::InteractiveShapeOwner::DetectedShapeElem& anElem = theShapeOwner->GetLastDetectedElem();
    if (!anElem.IsValid()) {
        return;
    }

    const ViewShape::InteractiveShapeSelMode anElemSelMode = ViewShape::ShapeElemTypeToSelMode(anElem.Type);
    if (m_toPrintSelModes != 0) {
        if (((unsigned int)anElemSelMode & m_toPrintSelModes) != 0) {
            theDI << anElem.Index;
        }
    } else {
        theDI << "  Detected Shape Elem:\n"
              << "    " << ViewShape::ShapeSelElemTypeToString(anElem.Type) << " " << anElem.Index << "\n";
    }
}

// =======================================================================
// function : PrintSelected
// =======================================================================
void ScvSelState::PrintSelected(Draw_Interpretor& theDI) const
{
    if (m_context->NbSelected() <= 0) {
        return;
    }

    if (m_toPrintSelModes == 0) {
        theDI << "Selected entities:\n";
    }
    for (m_context->InitSelected(); m_context->MoreSelected(); m_context->NextSelected()) {
        const Handle(SelectMgr_EntityOwner) anOwner = m_context->SelectedOwner();
        const Handle(AIS_InteractiveObject) anObj   = Handle(AIS_InteractiveObject)::DownCast(anOwner->Selectable());

        TCollection_AsciiString aName;
        GetMapOfAIS().Find1(anObj, aName);
        if (!m_prsName.IsEmpty() && m_prsName != aName) {
            continue;
        }

        if (m_toPrintSelModes == 0) {
            theDI << "  " << aName << "\n";
        }
        PrintSelectedShapeOwner(theDI, Handle(ViewShape::InteractiveShapeOwner)::DownCast(anOwner));
    }
}

// =======================================================================
// function : PrintSelectedShapeOwner
// =======================================================================
void ScvSelState::PrintSelectedShapeOwner(Draw_Interpretor&                                theDI,
                                          const Handle(ViewShape::InteractiveShapeOwner)& theShapeOwner) const
{
    using ShapeSelElemType = ViewShape::ShapeSelElemType;
    if (theShapeOwner.IsNull()) {
        return;
    }

    const ViewShape::ShapeSelElems& anElems = theShapeOwner->GetSelectedElems();
    if (!anElems.IsValid()) {
        return;
    }

    const ViewShape::InteractiveShapeSelMode aSelModes[8] = {ViewShape::InteractiveShapeSelMode::Vertex,
                                                              ViewShape::InteractiveShapeSelMode::Edge,
                                                              ViewShape::InteractiveShapeSelMode::Wire,
                                                              ViewShape::InteractiveShapeSelMode::Face,
                                                              ViewShape::InteractiveShapeSelMode::Shell,
                                                              ViewShape::InteractiveShapeSelMode::Solid,
                                                              ViewShape::InteractiveShapeSelMode::CompSolid,
                                                              ViewShape::InteractiveShapeSelMode::Group};

    bool isFirst = true;
    for (ViewShape::InteractiveShapeSelMode aSelIter : aSelModes) {
        if (((unsigned int)aSelIter & m_toPrintSelModes) == 0) {
            continue;
        }

        ShapeSelElemType aShapeType = ShapeSelElemType::CompSolid;
        ViewShape::ShapeElemTypeFromSelMode(aShapeType, (int)aSelIter);
        if (!anElems.IsValid(aShapeType)) {
            continue;
        }

        if (!isFirst) {
            theDI << "\n";
        }
        PrintIndexList(theDI, anElems.Indices(aShapeType));
        isFirst = false;
    }

    if (m_toPrintSelModes == 0) {
        theDI << "  Selected Shape Elem(s):\n";
        for (int aTypeIter = 0; aTypeIter < ViewShape::ShapeSelElemType_NB; ++aTypeIter) {
            if (!anElems.IsValid(ShapeSelElemType(aTypeIter))) {
                continue;
            }

            const Handle(TColStd_HPackedMapOfInteger)& aSelIds = anElems.Indices(ShapeSelElemType(aTypeIter));
            theDI << "    " << ViewShape::ShapeSelElemTypeToString((ShapeSelElemType)aTypeIter);
            for (TColStd_PackedMapOfInteger::Iterator anIndexIter(aSelIds->Map()); anIndexIter.More();
                 anIndexIter.Next()) {
                theDI << " " << anIndexIter.Key();
            }
            theDI << "\n";
        }
    }
}

// =======================================================================
// function : PrintIndexList
// =======================================================================
void ScvSelState::PrintIndexList(Draw_Interpretor& theDI, const Handle(TColStd_HPackedMapOfInteger)& theIds) const
{
    int aCounter = 0;
    for (TColStd_PackedMapOfInteger::Iterator anIndexIter(theIds->Map()); anIndexIter.More();
         anIndexIter.Next(), ++aCounter) {
        theDI << (aCounter > 0 ? " " : "") << anIndexIter.Key();
    }
}

}
