
#include <ViewShapeTest/ScvSelmode.h>

#include <DBRep.hxx>
#include <TopoDS.hxx>

namespace ViewShapeTest {

// =======================================================================
// function : ScvSelmode
// =======================================================================
ScvSelmode::ScvSelmode()
{
    AddVoidKey(this, "-global", "global") = [](ScvSelmode* theCmd) -> bool {
        theCmd->m_shapeSelMode = (int)ViewShape::InteractiveShapeSelMode::Global;
        return true;
    };
    AddVoidKey(this, "-vertex", "vertex", "vertices") = [](ScvSelmode* theCmd) -> bool {
        theCmd->m_shapeSelMode |= (int)ViewShape::InteractiveShapeSelMode::Vertex;
        return true;
    };
    AddVoidKey(this, "-edge", "-edges", "edge", "edges") = [](ScvSelmode* theCmd) -> bool {
        theCmd->m_shapeSelMode |= (int)ViewShape::InteractiveShapeSelMode::Edge;
        return true;
    };
    AddVoidKey(this, "-wire", "-wires", "wire", "wires") = [](ScvSelmode* theCmd) -> bool {
        theCmd->m_shapeSelMode |= (int)ViewShape::InteractiveShapeSelMode::Wire;
        return true;
    };
    AddVoidKey(this, "-face", "-faces", "face", "faces") = [](ScvSelmode* theCmd) -> bool {
        theCmd->m_shapeSelMode |= (int)ViewShape::InteractiveShapeSelMode::Face;
        return true;
    };
    AddVoidKey(this, "-shell", "-shells", "shell", "shells") = [](ScvSelmode* theCmd) -> bool {
        theCmd->m_shapeSelMode |= (int)ViewShape::InteractiveShapeSelMode::Shell;
        return true;
    };
    AddVoidKey(this, "-solid", "-solids", "solid", "solids") = [](ScvSelmode* theCmd) -> bool {
        theCmd->m_shapeSelMode |= (int)ViewShape::InteractiveShapeSelMode::Solid;
        return true;
    };
    AddVoidKey(this, "-compsolid", "-compsolids", "compsolid", "compsolids") = [](ScvSelmode* theCmd) -> bool {
        theCmd->m_shapeSelMode |= (int)ViewShape::InteractiveShapeSelMode::CompSolid;
        return true;
    };

    AddVoidKey(this, "-group", "-groups", "group", "groups") = [](ScvSelmode* theCmd, int& theArgIter) -> bool {
        return theCmd->ParseGroups(theArgIter);
    };
}

// =======================================================================
// function : ParseGroups
// =======================================================================
bool ScvSelmode::ParseGroups(int& theArgIter)
{
    m_shapeSelMode = (int)ViewShape::InteractiveShapeSelMode::Group;

    while (theArgIter + 1 < NbArguments()) {
        Standard_CString aName  = Argument(theArgIter + 1);
        TopoDS_Shape     aShape = DBRep::Get(aName, TopAbs_COMPOUND);
        if (aShape.IsNull()) {
            break;
        }

        ++theArgIter;
        m_groups.push_back(TopoDS::Compound(aShape));
    }

    return true;
};

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvSelmode::ParseAllArguments()
{
    if (!ViewerCommand::ParseAllArguments()) {
        return false;
    }

    if (m_shapeList.IsEmpty()) {
        AIS_ListOfInteractive aPrsList;
        m_context->ObjectsInside(aPrsList);

        for (const Handle(AIS_InteractiveObject)& aPrsIter : aPrsList) {
            Handle(ViewShape::InteractiveShape) anObj = Handle(ViewShape::InteractiveShape)::DownCast(aPrsIter);
            if (!anObj.IsNull() && !anObj->MeshOwner().IsNull()) {
                m_shapeList.Append(anObj->MeshOwner());
            }
        }
    }

    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvSelmode::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        //
    } else {
        Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
        if (!ViewBase::InteractiveOwner::FindPrsOwnerById(aShapeOwner, *m_context, Argument(theArgIter))) {
            *myDI << "Syntax error: object '" << Argument(theArgIter) << "' is not InteractiveShape\n";
            return false;
        }

        m_shapeList.Append(aShapeOwner);
    }
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvSelmode::Perform()
{
    for (const Handle(ViewShape::InteractiveShapeOwner)& aPrsIter : m_shapeList) {
        if (!m_groups.empty()) {
            aPrsIter->SetShapeGroups(m_groups);
        }
        aPrsIter->SetSelectionMode(m_shapeSelMode);
    }
    return true;
}

}
