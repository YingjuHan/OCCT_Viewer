#include <LodCaeTest/ScvMeshSimplify.h>

#include <RWCaeTest/DrawableMeshData.h>

namespace LodCaeTest {

// =======================================================================
// function : ScvMeshSimplify
// =======================================================================
ScvMeshSimplify::ScvMeshSimplify() : DrawBase::DrawCommand()
{
    AddRangeKey(this, 0., 1., "-targetratio", "-ratio") = &m_targetRatio;
    AddKey(this, 0., "-maxerror", "-error")             = &m_maxError;
    AddOnOffNoKey(this, "-aggregation")                 = &m_aggregation;
    AddOnOffNoKey(this, "-allowinverse")                = &m_allowInverse;
    AddKey(this, "", "-elemtype")                       = &ScvMeshSimplify::ParseElemType;
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvMeshSimplify::ParseAllArguments()
{
    if (!DrawBase::DrawCommand::ParseAllArguments()) {
        return false;
    }
    if (m_inputName.IsEmpty() || m_outputName.IsEmpty()) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvMeshSimplify::ParseArgument(int& theArgIter)
{
    if (DrawBase::DrawCommand::ParseArgument(theArgIter)) {
        return true;
    }

    const char* anArg = Argument(theArgIter);
    if (m_inputName.IsEmpty()) {
        Handle(Draw_Drawable3D) aDrawable = Draw::GetExisting(anArg);

        RWCaeTest::DrawableMeshData* aDMesh = dynamic_cast<RWCaeTest::DrawableMeshData*>(aDrawable.get());
        if (aDMesh == nullptr) {
            *myDI << "Syntax error: '" << anArg << "' is not a mesh data\n";
            return false;
        }
        m_inputName = anArg;
    } else if (m_outputName.IsEmpty() && anArg[0] != '\0') {
        m_outputName = anArg;
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseElemType
// =======================================================================
bool ScvMeshSimplify::ParseElemType(const char* theStr)
{
    TCollection_AsciiString anArg(theStr);
    anArg.LowerCase();
    if (CheckArgName(anArg, "2d")) {
        m_elemType = LodCae::SourceElemType::Elem2d;
    } else if (CheckArgName(anArg, "surf")) {
        m_elemType = LodCae::SourceElemType::ElemSurface;
    } else if (CheckArgName(anArg, "prefersurf")) {
        m_elemType = LodCae::SourceElemType::PreferSurface;
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshSimplify::Perform()
{
    Handle(Draw_Drawable3D)        aDrawable   = Draw::GetExisting(m_inputName.ToCString());
    RWCaeTest::DrawableMeshData* aDInputMesh = dynamic_cast<RWCaeTest::DrawableMeshData*>(aDrawable.get());

    LodCae::SimplParameters aParams{m_targetRatio, m_maxError, m_aggregation, m_allowInverse};

    LodCae::Simplifier aSimplifier;
    aSimplifier.AddMesh(aDInputMesh->MeshData());
    aSimplifier.SetId(m_outputName);
    aSimplifier.SetSourceElemType(m_elemType);

    if (!aSimplifier.Perform(aParams)) {
        *myDI << "Error: simplification failure";
        return false;
    }

    *myDI << "Done, max deviation is " << aSimplifier.GetMaxError();
    Handle(RWCaeTest::DrawableMeshData) aDOutMesh = new RWCaeTest::DrawableMeshData(aSimplifier.GetResult());
    Draw::Set(m_outputName.ToCString(), aDOutMesh);
    return true;
}

}

