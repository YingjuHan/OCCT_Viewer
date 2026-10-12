#include <ViewCae/InteractiveCaeCutView.h>
#include <ViewCaeGeom/MeshCutUnion.h>
#include <ViewCaeGeom/MeshCutTrace.h>
#include <ViewCae/CaeMeshTools.h>
#include <ViewCae/InteractiveCaeMeshOwner.h>

#include <Graphic3d_Group.hxx>
#include <Graphic3d_ShaderObject.hxx>
#include <Graphic3d_ShaderProgram.hxx>
#include <RWCae/MeshAdaptor.h>
#include <Image_SupportedFormats.hxx>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <Select3D_SensitivePoint.hxx>
#include <Select3D_SensitiveTriangle.hxx>
#include <Select3D_SensitiveSegment.hxx>
#include <SelectMgr_Selection.hxx>
#include <RWCae/MeshAdaptorsIterator.h>
#include <RWCae/MeshScalarProperty.h>
#include <Graphic3d_TransformPers.hxx>

IMPLEMENT_STANDARD_RTTIEXT(ViewCae::InteractiveCaeCutView, AIS_InteractiveObject)
IMPLEMENT_STANDARD_RTTIEXT(ViewCae::MeshCutPickOwner, SelectMgr_EntityOwner)

namespace {
class CutRangeMapper : public ViewCae::MeshColorMapper {
public:
    explicit CutRangeMapper(double minimum, double maximum) : myRange(minimum,maximum) {}
    Bnd_Range ValuesRange() const override { return myRange; }
private:
    Bnd_Range myRange;
};
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
InteractiveCaeCutView::InteractiveCaeCutView(const Handle(RWCae::MeshData)& theMesh) : myCuts(theMesh), myMesh(theMesh)
{
    SetDisplayMode(0);
    myLegend = new AIS_ColorScale();
    myLegend->SetSize(160,320); myLegend->SetTextHeight(14); myLegend->SetColor(Quantity_Color(Quantity_NOC_BLACK));
    myLegend->SetTransformPersistence(new Graphic3d_TransformPers(Graphic3d_TMF_2d,Aspect_TOTP_LEFT_LOWER,Graphic3d_Vec2i(16,16)));
    Handle(Graphic3d_SequenceOfHClipPlane) hudPlanes = new Graphic3d_SequenceOfHClipPlane();
    hudPlanes->SetOverrideGlobal(true); myLegend->SetClipPlanes(hudPlanes);
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
        || theStyle.LineWidth <= 0 || (int)theStyle.Mode < 0 || (int)theStyle.Mode >= InteractiveCaeMeshNbPrsModes) {
        throw std::invalid_argument("Invalid cut display style");
    }
    myStyles[anIndex] = theStyle;
}

void InteractiveCaeCutView::SetStyle(const std::string& cut, ViewCaeGeom::MeshCutPortion portion, const MeshCutDisplayStyle& style)
{
    const auto definitions = myCuts.Definitions();
    if (std::none_of(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == cut; })) {
        throw std::invalid_argument("Unknown cut style owner");
    }
    const auto previous = myStyles;
    SetStyle(portion,style); // Shared validation, preserve the public defaults.
    myStyles = previous;
    myStyleOverrides[{cut,(int)portion}] = style;
}
void InteractiveCaeCutView::ClearStyle(const std::string& cut, ViewCaeGeom::MeshCutPortion portion)
{
    if ((int)portion < 0 || (int)portion > 2) { throw std::invalid_argument("Invalid style portion"); }
    myStyleOverrides.erase({cut,(int)portion});
}
MeshCutDisplayStyle InteractiveCaeCutView::EffectiveStyle(const std::string& cut, ViewCaeGeom::MeshCutPortion portion) const
{
    const auto found = myStyleOverrides.find({cut,(int)portion});
    return found == myStyleOverrides.end() ? Style(portion) : found->second;
}
void InteractiveCaeCutView::SetContourOptions(const MeshCutContourOptions& options)
{
    if (!std::isfinite(options.Minimum) || !std::isfinite(options.Maximum) || options.Minimum >= options.Maximum
        || options.Intervals < 2 || options.Intervals > 256) { throw std::invalid_argument("Invalid cut contour range or intervals"); }
    myContourOptions = options;
}
void InteractiveCaeCutView::SetReferenceFrame(const ViewCaeGeom::MeshCutFrame& frame, bool show, double length)
{
    if (!frame.IsValid() || !std::isfinite(length) || length <= 0) { throw std::invalid_argument("Invalid reference frame"); }
    myReferenceFrame = frame; myShowReferenceFrame = show; myReferenceLength = length;
    SetToUpdate();
}
void InteractiveCaeCutView::SetVectorField(const Handle(RWCae::MeshNVecProperty)& field, double scale, double sampling, bool orientation)
{
    if (!std::isfinite(scale) || scale < 0 || !std::isfinite(sampling) || sampling <= 0 || sampling > 1
        || (!field.IsNull() && (!field->IsInitialized() || !field->IsVector3() || !field->HasScalarComponents()
            || field->NbValues() != (int)(field->IsNodal() ? myMesh->NbNodes() : myMesh->NbIndexedElems())))) {
        throw std::invalid_argument("Vectors require matching nodal/elemental 3D vectors, finite scale and sampling in (0,1]");
    }
    auto displayField = field;
    if (!field.IsNull()) {
        for (int i = 0; i < field->NbValues(); ++i) {
            Graphic3d_Vec3d value; field->GetVector(RWCae::MeshPropertyEntityInfo(i),value);
            if (!std::isfinite(std::hypot(value.x(),value.y(),value.z()))) { throw std::invalid_argument("Nonfinite vector result"); }
        }
    }
    if (orientation && !field.IsNull()) {
        displayField = new RWCae::MeshNVecProperty(field->Name(),3,field->MapType(),RWCae::MeshNVecPropertyType::Vector);
        displayField->SetTime(field->Time(),field->TimeStep());
        displayField->Init<RWCae::MeshScalarProperty>();
        for (int c = 0; c < displayField->NbRawComponents(); ++c) {
            Handle(RWCae::MeshScalarProperty)::DownCast(displayField->GetRawComponent(c))->Init(field->NbValues());
        }
        for (int i = 0; i < field->NbValues(); ++i) {
            Graphic3d_Vec3d value;
            field->GetVector(RWCae::MeshPropertyEntityInfo(i),value);
            const double length = std::hypot(value.x(),value.y(),value.z());
            if (!std::isfinite(length)) { throw std::invalid_argument("Nonfinite orientation vector"); }
            if (length > 0) { value /= length; }
            displayField->SetVector(RWCae::MeshPropertyEntityInfo(i),value);
        }
        displayField->UpdateComponents();
    }
    myVectorField = displayField; myVectorSource = field; myVectorScale = scale; myVectorSampling = sampling;
    myVectorOrientation = orientation;
}

std::vector<double> InteractiveCaeCutView::ProbeSource(RWCae::MeshEntityId entity, bool node, RWCae::MeshEntityId elementNode) const
{
    if (myField.IsNull() || myField->IsNodal() != node || entity >= (node ? myMesh->NbNodes() : myMesh->NbIndexedElems())) {
        throw std::invalid_argument("Source probe requires matching original field/entity mapping");
    }
    if (myField->HasNodeVecComponents()) {
        std::vector<RWCae::MeshEntityId> nodes; myMesh->ElementNodes(entity,nodes);
        if (std::find(nodes.begin(),nodes.end(),elementNode) == nodes.end()) { throw std::invalid_argument("Node does not belong to the original element"); }
    }
    std::vector<double> values;
    for (int c = 0; c < myField->NbRawComponents(); ++c) {
        values.push_back(myField->GetRawComponent(c)->Value(RWCae::MeshPropertyEntityInfo(entity,
            myField->HasNodeVecComponents() ? (int)elementNode : -1)));
    }
    return values;
}

MeshCutProbe InteractiveCaeCutView::Probe(const std::string& id, ViewCaeGeom::MeshCutPortion portion, RWCae::MeshEntityId entity, bool node)
{
    if ((int)portion < 0 || (int)portion > 2 || myField.IsNull()) { throw std::invalid_argument("Invalid section probe"); }
    const auto cut = myCuts.EvaluatePhysical(id,myField);
    return probeResult(cut,(int)portion,entity,node);
}
MeshCutProbe InteractiveCaeCutView::probeResult(const ViewCaeGeom::MeshCutResult& cut, int portion, RWCae::MeshEntityId entity, bool node)
{
    const auto geometry = cut.Portions[portion];
    if (geometry.IsNull()) { throw std::invalid_argument("Probe portion is hidden"); }
    const auto mesh = geometry->GetMeshData();
    const auto section = Handle(ViewCaeGeom::MeshGeometrySlice)::DownCast(geometry);
    if (!node && mesh->NbIndexedElems() == 0 && !section.IsNull() && entity < section->Lines().size()) {
        return probeLineResult(cut,entity,-1);
    }
    if (entity >= (node ? mesh->NbNodes() : mesh->NbIndexedElems())) { throw std::invalid_argument("Probe index outside section"); }
    const auto field = !cut.PhysicalFields[portion].IsNull() ? cut.PhysicalFields[portion] : geometry->CalculateProperty(myField);
    if (field.IsNull()) { throw std::invalid_argument("Probe has no physical field"); }
    MeshCutProbe result;
    if (node) {
        ViewCaeGeom::MeshInterpolatedNode origin;
        if (!geometry->OriginNode(entity,origin)) { throw std::runtime_error("Missing original node provenance"); }
        result.SourceNodes = origin.BaseNodes(); result.Position = Graphic3d_Vec3d(mesh->Nodes()[entity]);
        // A discontinuous node needs an owning element; choose lowest section element deterministically.
        bool found = false;
        auto process = [&](const auto& adaptor) {
            for (RWCae::MeshEntityId e = 0; e < adaptor.NbElements() && !found; ++e) {
                const auto& element = adaptor.GetElement(e);
                for (int n = 0; n < element.NbNodes(); ++n) { if (element[n] == entity) {
                    found = geometry->OriginElement(ViewCaeGeom::MeshGeomElem(e,adaptor.GetElementType(),false),result.SourceElement);
                    if (found) {
                        if (cut.PhysicalProvenance) {
                            const auto& trace = cut.PhysicalProvenance(element.GlobalIndex(),false);
                            result.SourceElement = trace.Element;
                            result.SourceNodes = trace.Weights.at(entity).BaseNodes();
                        }
                        for (int c = 0; c < field->NbRawComponents(); ++c) {
                            result.Values.push_back(field->GetRawComponent(c)->Value(RWCae::MeshPropertyEntityInfo(
                                field->IsNodal() ? entity : element.GlobalIndex(),field->HasNodeVecComponents() ? (int)entity : -1)));
                        }
                    }
                    break;
                } }
            }
        };
        RWCae::MeshAdaptorsIteratorAllElems::For(process,mesh);
        if (!found) {
            const auto slice = Handle(ViewCaeGeom::MeshGeometrySlice)::DownCast(geometry);
            if (!slice.IsNull()) {
                for (size_t l = 0; l < slice->Lines().size(); ++l) {
                    for (int endpoint = 0; endpoint < 2; ++endpoint) {
                        if (slice->Lines()[l].Nodes[endpoint] == entity) { return probeLineResult(cut,l,endpoint); }
                    }
                }
            }
            throw std::runtime_error("Missing section owner for probe");
        }
    } else {
        const auto& indexed = mesh->IndexedElements()[entity];
        if (!geometry->OriginElement(ViewCaeGeom::MeshGeomElem(indexed.Index,indexed.Type,false),result.SourceElement)) {
            throw std::runtime_error("Missing original element provenance");
        }
        if (cut.PhysicalProvenance) { result.SourceElement = cut.PhysicalProvenance(entity,false).Element; }
        // Element probes of nodal data return the mean of original-FE section-node values.
        auto process = [&](const auto& adaptor) {
            for (RWCae::MeshEntityId e = 0; e < adaptor.NbElements(); ++e) {
                const auto& element = adaptor.GetElement(e);
                if ((RWCae::MeshEntityId)element.GlobalIndex() != entity) { continue; }
                result.Values.resize(field->NbRawComponents(),0.0);
                for (int n = 0; n < element.NbNodes(); ++n) {
                    result.Position += Graphic3d_Vec3d(mesh->Nodes()[element[n]])/element.NbNodes();
                    for (int c = 0; c < field->NbRawComponents(); ++c) {
                        result.Values[c] += field->GetRawComponent(c)->Value(RWCae::MeshPropertyEntityInfo(
                            field->IsNodal() ? element[n] : entity,field->HasNodeVecComponents() ? (int)element[n] : -1))/element.NbNodes();
                    }
                }
            }
        };
        RWCae::MeshAdaptorsIteratorAllElems::For(process,mesh);
    }
    return result;
}
MeshCutProbe InteractiveCaeCutView::ProbeLine(const std::string& id, size_t line, int endpoint)
{
    if (endpoint < -1 || endpoint > 1 || myField.IsNull()) { throw std::invalid_argument("Invalid shell line probe"); }
    const auto cut = myCuts.EvaluatePhysical(id,myField);
    return probeLineResult(cut,line,endpoint);
}
MeshCutProbe InteractiveCaeCutView::probeLineResult(const ViewCaeGeom::MeshCutResult& cut, size_t line, int endpoint)
{
    const auto geometry = Handle(ViewCaeGeom::MeshGeometrySlice)::DownCast(cut.Portions[1]);
    if (geometry.IsNull() || line >= geometry->Lines().size()) { throw std::invalid_argument("Shell section line is unavailable"); }
    const auto& sectionLine = geometry->Lines()[line]; const auto mesh = geometry->GetMeshData();
    const auto field = !cut.PhysicalLineField.IsNull() ? cut.PhysicalLineField : geometry->CalculateLineProperty(myField);
    if (field.IsNull()) { throw std::invalid_argument("Shell probe requires a field"); }
    MeshCutProbe result; result.SourceElement = sectionLine.SourceElement;
    const ViewCaeGeom::MeshCutPrimitiveTrace* trace = cut.PhysicalProvenance ? &cut.PhysicalProvenance(line,true) : nullptr;
    if (trace) { result.SourceElement = trace->Element; }
    const int first = endpoint == -1 ? 0 : endpoint, last = endpoint == -1 ? 1 : endpoint;
    result.Values.resize(field->NbRawComponents(),0);
    std::map<RWCae::MeshEntityId,double> weights;
    for (int e = first; e <= last; ++e) {
        const auto node = sectionLine.Nodes[e]; const double ratio = endpoint == -1 ? 0.5 : 1;
        result.Position += Graphic3d_Vec3d(mesh->Nodes()[node])*ratio;
        ViewCaeGeom::MeshInterpolatedNode origin;
        if (trace) { origin = trace->Weights.at(node); }
        else if (!geometry->OriginNode(node,origin)) { throw std::runtime_error("Missing line FE provenance"); }
        for (const auto& weight : origin.BaseNodes()) { weights[weight.first] += weight.second*ratio; }
        for (int c = 0; c < field->NbRawComponents(); ++c) {
            result.Values[c] += field->GetRawComponent(c)->Value(RWCae::MeshPropertyEntityInfo(
                field->IsNodal() ? node : (int)line,field->HasNodeVecComponents() ? (int)node : -1))*ratio;
        }
    }
    result.SourceNodes.assign(weights.begin(),weights.end()); return result;
}
MeshCutProbe InteractiveCaeCutView::Probe(const Handle(MeshCutPickOwner)& owner)
{
    if (owner.IsNull() || owner->SelectablePointer() != this || owner->Generation != mySynchronizationCount) {
        throw std::invalid_argument("Stale cut pick owner");
    }
    if (owner->Branch >= myPickResults.size()) { throw std::invalid_argument("Invalid pick branch"); }
    if (owner->Line) { return probeLineResult(myPickResults[owner->Branch],owner->Entity,-1); }
    return probeResult(myPickResults[owner->Branch],owner->Portion,owner->Entity,owner->Node);
}

void InteractiveCaeCutView::Synchronize()
{
    decltype(myPresentations) presentations;
    decltype(myVectorPresentations) vectorPresentations;
    decltype(myLineArrays) lineArrays;
    decltype(myLineAspects) lineAspects;
    decltype(myPointArrays) pointArrays;
    decltype(myPointAspects) pointAspects;
    std::map<std::string, uint64_t> revisions;
    const auto definitions = myCuts.Definitions();
    for (auto it = myStyleOverrides.begin(); it != myStyleOverrides.end();) {
        if (std::none_of(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == it->first.first; })) {
            it = myStyleOverrides.erase(it);
        } else { ++it; }
    }
    const bool hasActiveCut = std::any_of(definitions.begin(), definitions.end(), [](const auto& cut) { return cut.Enabled; });
    auto cutPresentation = hasActiveCut ? myCuts.EvaluatePresentation(myField) : nullptr;
    auto results = cutPresentation ? cutPresentation->Branches : std::vector<ViewCaeGeom::MeshCutResult>();
    if (!hasActiveCut) {
        ViewCaeGeom::MeshCutResult result;
        result.Definition.Id = "uncut_source"; result.SourceField = myField;
        result.TensorDerived = !myField.IsNull() && myField->Type() == RWCae::MeshNVecPropertyType::Tensor
            && myCuts.RecoveryOptions().Tensor.Measure != ViewCaeGeom::MeshCutTensorMeasure::Components;
        if (result.TensorDerived) { result.SourceField = ViewCaeGeom::DeriveCutTensorField(myField,myCuts.RecoveryOptions().Tensor); }
        result.Roots[0] = myCuts.SourceGeometry(); result.Portions[0] = result.Roots[0]->GetLastChild();
        result.Portions[0]->GetMeshData();
        auto slice = Handle(ViewCaeGeom::MeshGeometrySlice)::DownCast(result.Portions[0]);
        if (!slice.IsNull()) { result.LineField = slice->CalculateLineProperty(result.SourceField); }
        results.push_back(result);
    }
    MeshCutContourState contourState = myField.IsNull() ? MeshCutContourState::NoField : MeshCutContourState::Empty;
    double minimum = INFINITY, maximum = -INFINITY;
    // Range is shared by all portions, derived from original/display field units.
    for (const auto& result : results) {
        if (result.SourceField.IsNull()) { continue; }
        const auto range = result.SourceField->GetRawComponent(result.TensorDerived ? 0 : myComponent)->ValueRange();
        double lo, hi;
        if (range.GetBounds(lo,hi) && std::isfinite(lo) && std::isfinite(hi)) {
            minimum = std::min(minimum,lo); maximum = std::max(maximum,hi);
        }
    }
    const bool empty = std::none_of(results.begin(),results.end(),[](const auto& result) {
        return std::any_of(result.Portions.begin(),result.Portions.end(),[](const auto& geometry) {
            return !geometry.IsNull() && geometry->GetMeshData()->NbNodes() > 0;
        });
    });
    if (std::isfinite(minimum)) {
        contourState = empty ? MeshCutContourState::Empty : minimum == maximum ? MeshCutContourState::Constant : MeshCutContourState::Variable;
    } else { minimum = 0; maximum = 1; }
    const double rangeMinimum = myContourOptions.AutomaticRange ? minimum : myContourOptions.Minimum;
    const double rangeMaximum = myContourOptions.AutomaticRange ? maximum : myContourOptions.Maximum;
    for (const auto& aResult : results) {
        const int component = aResult.TensorDerived ? 0 : myComponent;
        if (hasActiveCut) { revisions[aResult.Definition.Id] = aResult.Revision; }
        Handle(MeshPropertyData) aProps = new MeshPropertyData();
        const int aPropIndex = aProps->AddProperty(aResult.SourceField);
        if (!aResult.SourceField.IsNull()) {
            aProps->SetPropertyComponentIndex(component, aPropIndex);
            Handle(MeshColorMapper) mapper = new CutRangeMapper(rangeMinimum,rangeMaximum);
            mapper->SetMeshProperty(aResult.SourceField,component);
            aProps->SetColorMapper(mapper,aProps->Property(aPropIndex));
        }
        for (size_t i = 0; i < aResult.Portions.size(); ++i) {
            if (aResult.Portions[i].IsNull()) { continue; }
            const auto aStyle = EffectiveStyle(aResult.Definition.Id,(ViewCaeGeom::MeshCutPortion)i);
            Handle(MeshPresentationScalar) aPrs = new MeshPresentationScalar(aProps, aPropIndex,
                aResult.Roots[i], TCollection_AsciiString(aResult.Definition.Id.c_str()) + "_" + (int)i);
            auto aDrawer = aPrs->HighlightDrawer(MeshHighlightingType::None);
            aDrawer->SetShadingColor(aStyle.Color);
            aDrawer->SetShadingTransparency(aStyle.Transparency);
            aDrawer->SetEdgesColor(aStyle.Color);
            aDrawer->SetEdgesWidth(aStyle.LineWidth);
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
                        const double value = values->GetRawComponent(component)->Value(RWCae::MeshPropertyEntityInfo(
                            values->IsNodal() ? element[0] : element.GlobalIndex(),values->HasNodeVecComponents() ? (int)element[0] : -1));
                        const auto uv = CaeMeshTools::GetUVCoords(value,minimum,maximum,CaeMeshTools::NbColorscaleColors);
                        const int column = std::clamp((int)(uv.x()*colors->SizeX()),0,(int)colors->SizeX()-1);
                        array->SetVertexColor(vertex,colors->PixelColor(column,0).GetRGB());
                    }
                }
                pointArrays.push_back(array);
                Handle(Graphic3d_AspectMarker3d) marker = new Graphic3d_AspectMarker3d(Aspect_TOM_O_POINT,aStyle.Color,2.0);
                marker->SetInteriorColor(Quantity_ColorRGBA(aStyle.Color,(float)(1.0-aStyle.Transparency)));
                pointAspects.push_back(marker);
            }
            presentations.push_back(aPrs);
            if (!myVectorField.IsNull()) {
                Handle(MeshPresentationVector) vector = new MeshPresentationVector(aProps,aPropIndex,aResult.Roots[i],
                    TCollection_AsciiString(aResult.Definition.Id.c_str())+"_vector_"+(int)i);
                vector->VectorVisProps()->SetVectorScaleCoeff(myVectorScale);
                vector->VectorPartParams()->SetVectorProperty(myVectorField);
                vector->VectorPartParams()->SetSamplingRatio(myVectorSampling);
                vector->UpdateAspects(new InteractiveCaeMeshOwner(myMesh));
                vectorPresentations.push_back(vector);
            }
            auto aSlice = Handle(ViewCaeGeom::MeshGeometrySlice)::DownCast(aResult.Portions[i]);
            if (aSlice.IsNull()) { continue; }
            const auto& aLines = aSlice->Lines();
            if (aLines.empty()) { continue; }
            const auto lineField = aSlice->CalculateLineProperty(aResult.SourceField);
            const bool hasField = !lineField.IsNull();
            Handle(Graphic3d_ArrayOfSegments) anArray = new Graphic3d_ArrayOfSegments((int)aLines.size()*2, 0,
                hasField ? Graphic3d_ArrayFlags_VertexTexel : Graphic3d_ArrayFlags_None);
            Handle(Graphic3d_AspectLine3d) anAspect = new Graphic3d_AspectLine3d(aStyle.Color, Aspect_TOL_SOLID, aStyle.LineWidth);
            anAspect->SetInteriorColor(Quantity_ColorRGBA(aStyle.Color,(float)(1.0-aStyle.Transparency)));
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
                        const auto aComponent = lineField->GetRawComponent(component);
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
    myVectorPresentations = std::move(vectorPresentations);
    myLineArrays = std::move(lineArrays); myLineAspects = std::move(lineAspects);
    myPointArrays = std::move(pointArrays); myPointAspects = std::move(pointAspects);
    mySynchronizedRevisions = std::move(revisions);
    myCutPresentation = std::move(cutPresentation);
    myPickResults = std::move(results);
    myContourState = contourState;
    myLegend->SetRange(rangeMinimum,rangeMaximum);
    myLegend->SetNumberOfIntervals(myContourOptions.Intervals);
    myLegend->SetTitle(TCollection_ExtendedString(contourState == MeshCutContourState::NoField ? "No result"
        : contourState == MeshCutContourState::Empty ? "No finite values"
        : contourState == MeshCutContourState::Constant ? "Constant result" : myField->Name().ToCString()));
    myLegend->SetColorType(Aspect_TOCSD_USER);
    for (int i = 1; i <= myContourOptions.Intervals; ++i) {
        const double value = (i-0.5)/myContourOptions.Intervals;
        myLegend->SetIntervalColor(Quantity_Color(value < 0.5 ? 0 : 2*value-1,
            value < 0.5 ? 2*value : 2-2*value,value < 0.5 ? 1-2*value : 0,Quantity_TOC_RGB),i);
    }
    ++mySynchronizationCount;
    SetToUpdate();
    UpdateSelection();
    if (myOnSynchronize) { myOnSynchronize(); }
}
void InteractiveCaeCutView::NotifyDrag(AIS_DragAction action)
{
    if (myOnDrag) { try { myOnDrag(action); } catch (...) {} }
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
    for (const auto& vector : myVectorPresentations) {
        vector->GetParts()(MeshPartType::Vectors)->Compute(InteractiveContext(),vector->Drawer(),thePrs,thePrs,1.0f);
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
    if (myShowReferenceFrame) {
        const Graphic3d_Vec3d axes[] = {myReferenceFrame.X,myReferenceFrame.Y,myReferenceFrame.Z};
        const Quantity_NameOfColor colors[] = {Quantity_NOC_RED,Quantity_NOC_GREEN,Quantity_NOC_BLUE};
        for (int i = 0; i < 3; ++i) {
            const auto a = myReferenceFrame.Origin, b = a+axes[i]*myReferenceLength;
            Handle(Graphic3d_ArrayOfSegments) array = new Graphic3d_ArrayOfSegments(2);
            array->AddVertex(gp_Pnt(a.x(),a.y(),a.z())); array->AddVertex(gp_Pnt(b.x(),b.y(),b.z()));
            const auto group = thePrs->NewGroup();
            group->SetGroupPrimitivesAspect(new Graphic3d_AspectLine3d(Quantity_Color(colors[i]),Aspect_TOL_SOLID,2));
            group->AddPrimitiveArray(array);
        }
    }
}

void InteractiveCaeCutView::ComputeSelection(const Handle(SelectMgr_Selection)& selection, Standard_Integer mode)
{
    if (mode != 1 && mode != 2) { return; }
    for (size_t branch = 0; branch < myPickResults.size(); ++branch) {
        const auto& cut = myPickResults[branch];
        for (int portion = 0; portion < 3; ++portion) {
            const auto geometry = cut.Portions[portion];
            if (geometry.IsNull()) { continue; }
            const auto mesh = geometry->GetMeshData();
            if (mode == 1) {
                for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
                    Handle(MeshCutPickOwner) owner = new MeshCutPickOwner(this,cut.Definition.Id,portion,n,true,mySynchronizationCount);
                    owner->Branch = branch;
                    const auto p = mesh->Nodes()[n];
                    selection->Add(new Select3D_SensitivePoint(owner,gp_Pnt(p.x(),p.y(),p.z())));
                }
            } else {
                auto process = [&](const auto& adaptor) {
                    for (RWCae::MeshEntityId e = 0; e < adaptor.NbElements(); ++e) {
                        const auto& elem = adaptor.GetElement(e);
                        Handle(MeshCutPickOwner) owner = new MeshCutPickOwner(this,cut.Definition.Id,portion,elem.GlobalIndex(),false,mySynchronizationCount);
                        owner->Branch = branch;
                        auto point = [&](int n) { const auto p = mesh->Nodes()[elem[n]]; return gp_Pnt(p.x(),p.y(),p.z()); };
                        for (int n = 1; n+1 < elem.NbNodes(); ++n) {
                            selection->Add(new Select3D_SensitiveTriangle(owner,point(0),point(n),point(n+1)));
                        }
                    }
                };
                RWCae::MeshAdaptorsIterator2dElems::For(process,mesh);
                auto volumes = [&](const auto& adaptor) {
                    for (RWCae::MeshEntityId e = 0; e < adaptor.NbElements(); ++e) {
                        const auto& elem = adaptor.GetElement(e);
                        Handle(MeshCutPickOwner) owner = new MeshCutPickOwner(this,cut.Definition.Id,portion,elem.GlobalIndex(),false,mySynchronizationCount);
                        owner->Branch = branch;
                        for (RWCae::MeshElemEntityId face = 0; face < adaptor.NbFaces(elem); ++face) {
                            std::vector<RWCae::MeshEntityId> nodes; RWCae::MeshElemType type;
                            adaptor.FaceNodes(elem,face,nodes,type);
                            auto point = [&](size_t n) { const auto p = mesh->Nodes()[nodes[n]]; return gp_Pnt(p.x(),p.y(),p.z()); };
                            for (size_t n = 1; n+1 < nodes.size(); ++n) {
                                selection->Add(new Select3D_SensitiveTriangle(owner,point(0),point(n),point(n+1)));
                            }
                        }
                    }
                };
                RWCae::MeshAdaptorsIterator3dElems::For(volumes,mesh);
                const auto slice = Handle(ViewCaeGeom::MeshGeometrySlice)::DownCast(geometry);
                if (!slice.IsNull()) {
                    for (size_t line = 0; line < slice->Lines().size(); ++line) {
                        Handle(MeshCutPickOwner) owner = new MeshCutPickOwner(this,cut.Definition.Id,portion,(RWCae::MeshEntityId)line,false,mySynchronizationCount);
                        owner->Branch = branch; owner->Line = true;
                        const auto& nodes = slice->Lines()[line].Nodes;
                        const auto a = mesh->Nodes()[nodes[0]], b = mesh->Nodes()[nodes[1]];
                        selection->Add(new Select3D_SensitiveSegment(owner,gp_Pnt(a.x(),a.y(),a.z()),gp_Pnt(b.x(),b.y(),b.z())));
                    }
                }
            }
        }
    }
}
}
