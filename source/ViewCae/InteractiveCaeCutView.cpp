#include <ViewCae/InteractiveCaeCutView.h>
#include <ViewCae/CaeMeshTools.h>

#include <Graphic3d_Group.hxx>
#include <Graphic3d_ShaderObject.hxx>
#include <Graphic3d_ShaderProgram.hxx>
#include <RWCae/MeshAdaptor.h>
#include <Image_SupportedFormats.hxx>
#include <stdexcept>
#include <cmath>
#include <algorithm>

IMPLEMENT_STANDARD_RTTIEXT(ViewCae::InteractiveCaeCutView, AIS_InteractiveObject)

namespace {
Handle(Graphic3d_ShaderProgram) cutLineShader()
{
    // OCCT's default line program does not sample surface-result textures.
    Handle(Graphic3d_ShaderProgram) aProgram = new Graphic3d_ShaderProgram();
    aProgram->SetId("OCCTViewer_cut_line_contour");
    Graphic3d_ShaderObject::ShaderVariableList aUniforms, anInOut;
    anInOut.Append(Graphic3d_ShaderObject::ShaderVariable("vec2 CutTexCoord",
        Graphic3d_TOS_VERTEX | Graphic3d_TOS_FRAGMENT));
    TCollection_AsciiString aVertex =
        "void main() {\n"
        "  gl_Position = occProjectionMatrix * occWorldViewMatrix * occModelWorldMatrix * occVertex;\n"
        "  CutTexCoord = occTexCoord.xy;\n"
        "}\n";
    TCollection_AsciiString aFragment =
        "void main() {\n"
        "  vec4 color = occTexture2D(occSamplerBaseColor, CutTexCoord);\n"
        "  color.a *= occColor.a;\n"
        "  occSetFragColor(color);\n"
        "}\n";
    aProgram->AttachShader(Graphic3d_ShaderObject::CreateFromSource(aVertex, Graphic3d_TOS_VERTEX, aUniforms, anInOut));
    aProgram->AttachShader(Graphic3d_ShaderObject::CreateFromSource(aFragment, Graphic3d_TOS_FRAGMENT, aUniforms, anInOut));
    return aProgram;
}
}

namespace ViewCae {
InteractiveCaeCutView::InteractiveCaeCutView(const Handle(RWCae::MeshData)& theMesh) : myCuts(theMesh)
{
    SetDisplayMode(0);
}

void InteractiveCaeCutView::SetField(const Handle(RWCae::MeshNVecProperty)& theField, int theRawComponent)
{
    if (!theField.IsNull() && (!theField->IsInitialized() || theRawComponent < 0
        || theRawComponent >= theField->NbRawComponents())) {
        throw std::invalid_argument("Invalid cut contour component");
    }
    myField = theField;
    myComponent = theRawComponent;
    myCuts.InvalidateFields();
}

void InteractiveCaeCutView::SetStyle(ViewCaeGeom::MeshCutPortion thePortion, const MeshCutDisplayStyle& theStyle)
{
    const int anIndex = (int)thePortion;
    if (anIndex < 0 || anIndex >= 3 || !std::isfinite(theStyle.Transparency)
        || theStyle.Transparency < 0 || theStyle.Transparency > 1 || !std::isfinite(theStyle.LineWidth)
        || theStyle.LineWidth <= 0) {
        throw std::invalid_argument("Invalid cut display style");
    }
    myStyles[anIndex] = theStyle;
}

void InteractiveCaeCutView::Synchronize()
{
    decltype(myPresentations) presentations;
    decltype(myLineArrays) lineArrays;
    decltype(myLineAspects) lineAspects;
    decltype(myPointArrays) pointArrays;
    decltype(myPointAspects) pointAspects;
    std::map<std::string, uint64_t> revisions;
    const auto definitions = myCuts.Definitions();
    const bool hasActiveCut = std::any_of(definitions.begin(), definitions.end(), [](const auto& cut) { return cut.Enabled; });
    auto results = hasActiveCut ? myCuts.EvaluateVisible(myField) : std::vector<ViewCaeGeom::MeshCutResult>();
    if (!hasActiveCut) {
        ViewCaeGeom::MeshCutResult result;
        result.Definition.Id = "uncut_source"; result.SourceField = myField;
        result.Roots[0] = myCuts.SourceGeometry(); result.Portions[0] = result.Roots[0]->GetLastChild();
        result.Portions[0]->GetMeshData();
        auto slice = Handle(ViewCaeGeom::MeshGeometrySlice)::DownCast(result.Portions[0]);
        if (!slice.IsNull()) { result.LineField = slice->CalculateLineProperty(myField); }
        results.push_back(result);
    }
    for (const auto& aResult : results) {
        if (hasActiveCut) { revisions[aResult.Definition.Id] = aResult.Revision; }
        Handle(MeshPropertyData) aProps = new MeshPropertyData();
        const int aPropIndex = aProps->AddProperty(aResult.SourceField);
        if (!aResult.SourceField.IsNull()) { aProps->SetPropertyComponentIndex(myComponent, aPropIndex); }
        for (size_t i = 0; i < aResult.Portions.size(); ++i) {
            if (aResult.Portions[i].IsNull()) { continue; }
            const auto& aStyle = myStyles[i];
            Handle(MeshPresentationScalar) aPrs = new MeshPresentationScalar(aProps, aPropIndex,
                aResult.Roots[i], TCollection_AsciiString(aResult.Definition.Id.c_str()) + "_" + (int)i);
            auto aDrawer = aPrs->HighlightDrawer(MeshHighlightingType::None);
            aDrawer->SetShadingColor(aStyle.Color);
            aDrawer->SetShadingTransparency(aStyle.Transparency);
            aDrawer->SetEdgesColor(aStyle.Color);
            aPrs->ScalarVisProps()->SetPresentationMode(aStyle.Mode);
            aPrs->UpdateAspects(Handle(InteractiveCaeMeshOwner)());
            const auto mesh = aResult.Portions[i]->GetMeshData();
            if (mesh->NbElems(RWCae::MeshElemType::Elem0d) > 0) {
                aPrs->GetParts()(MeshPartType::Elems0d)->SetShow(false);
                RWCae::MeshAdaptor0d points(*mesh);
                const auto values = aResult.Portions[i]->CalculateProperty(aResult.SourceField);
                const bool colored = !values.IsNull();
                Handle(Graphic3d_ArrayOfPoints) array = new Graphic3d_ArrayOfPoints(points.NbElements(),
                    colored ? Graphic3d_ArrayFlags_VertexColor : Graphic3d_ArrayFlags_None);
                Handle(Image_PixMap) colors;
                double minimum = 0, maximum = 1;
                if (colored) {
                    const auto mapper = aProps->ColorMapper(aProps->Property(aPropIndex));
                    mapper->ValuesRange().GetBounds(minimum,maximum);
                    colors = mapper->ColorTexture()->GetImage(new Image_SupportedFormats());
                }
                for (RWCae::MeshEntityId e = 0; e < points.NbElements(); ++e) {
                    const auto& element = points.GetElement(e);
                    const int vertex = array->AddVertex(mesh->Nodes()[element[0]]);
                    if (colored && !colors.IsNull()) {
                        const double value = values->GetRawComponent(myComponent)->Value(RWCae::MeshPropertyEntityInfo(
                            values->IsNodal() ? element[0] : element.GlobalIndex(),values->HasNodeVecComponents() ? (int)element[0] : -1));
                        const auto uv = CaeMeshTools::GetUVCoords(value,minimum,maximum,CaeMeshTools::NbColorscaleColors);
                        const int column = std::clamp((int)(uv.x()*colors->SizeX()),0,(int)colors->SizeX()-1);
                        array->SetVertexColor(vertex,colors->PixelColor(column,0).GetRGB());
                    }
                }
                pointArrays.push_back(array);
                pointAspects.push_back(new Graphic3d_AspectMarker3d(Aspect_TOM_O_POINT,aStyle.Color,2.0));
            }
            presentations.push_back(aPrs);
            auto aSlice = Handle(ViewCaeGeom::MeshGeometrySlice)::DownCast(aResult.Portions[i]);
            if (aSlice.IsNull()) { continue; }
            const auto& aLines = aSlice->Lines();
            if (aLines.empty()) { continue; }
            const auto lineField = aSlice->CalculateLineProperty(aResult.SourceField);
            const bool hasField = !lineField.IsNull();
            Handle(Graphic3d_ArrayOfSegments) anArray = new Graphic3d_ArrayOfSegments((int)aLines.size()*2, 0,
                hasField ? Graphic3d_ArrayFlags_VertexTexel : Graphic3d_ArrayFlags_None);
            Handle(Graphic3d_AspectLine3d) anAspect = new Graphic3d_AspectLine3d(aStyle.Color, Aspect_TOL_SOLID, aStyle.LineWidth);
            double aMin = 0.0, aMax = 1.0;
            if (hasField) {
                auto aMapper = aProps->ColorMapper(aProps->Property(aPropIndex));
                aMapper->ValuesRange().GetBounds(aMin, aMax);
                anAspect->SetTextureMap(aMapper->ColorTexture());
                anAspect->SetTextureMapOn(true);
                anAspect->SetShaderProgram(cutLineShader());
            }
            for (size_t l = 0; l < aLines.size(); ++l) {
                for (auto n : aLines[l].Nodes) {
                    const int v = anArray->AddVertex(aSlice->GetMeshData()->Nodes()[n]);
                    if (hasField) {
                        const auto aComponent = lineField->GetRawComponent(myComponent);
                        const double aValue = aComponent->Value(RWCae::MeshPropertyEntityInfo(
                            lineField->IsNodal() ? (int)n : (int)l,lineField->HasNodeVecComponents() ? (int)n : -1));
                        const auto uv = CaeMeshTools::GetUVCoords(aValue, aMin, aMax, CaeMeshTools::NbColorscaleColors);
                        anArray->SetVertexTexel(v, uv.x(), uv.y());
                    }
                }
            }
            lineArrays.push_back(anArray);
            lineAspects.push_back(anAspect);
        }
    }
    myPresentations = std::move(presentations);
    myLineArrays = std::move(lineArrays); myLineAspects = std::move(lineAspects);
    myPointArrays = std::move(pointArrays); myPointAspects = std::move(pointAspects);
    mySynchronizedRevisions = std::move(revisions);
    ++mySynchronizationCount;
    SetToUpdate();
}

void InteractiveCaeCutView::Compute(const Handle(PrsMgr_PresentationManager)&,
    const Handle(Prs3d_Presentation)& thePrs, Standard_Integer theMode)
{
    if (theMode != 0) { return; }
    for (const auto& aPresentation : myPresentations) {
        const auto& aParts = aPresentation->GetParts();
        for (int i = 0; i < MeshNbPartTypes; ++i) {
            aParts((MeshPartType)i)->Compute(InteractiveContext(), aPresentation->Drawer(), thePrs, thePrs, 1.0f);
        }
    }
    for (size_t i = 0; i < myLineArrays.size(); ++i) {
        auto aGroup = thePrs->NewGroup();
        aGroup->SetGroupPrimitivesAspect(myLineAspects[i]);
        aGroup->AddPrimitiveArray(myLineArrays[i]);
    }
    for (size_t i = 0; i < myPointArrays.size(); ++i) {
        auto group = thePrs->NewGroup(); group->SetGroupPrimitivesAspect(myPointAspects[i]);
        group->AddPrimitiveArray(myPointArrays[i]);
    }
}
}
