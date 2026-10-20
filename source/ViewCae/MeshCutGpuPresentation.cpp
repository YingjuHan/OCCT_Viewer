#include <ViewCae/MeshCutGpuPresentation.h>
#include <ViewCae/CaeMeshTools.h>
#include <RWCae/MeshAdaptorsIterator.h>
#include <Graphic3d_AttribBuffer.hxx>
#include <Graphic3d_MutableIndexBuffer.hxx>
#include <Graphic3d_ShaderObject.hxx>
#include <Graphic3d_TextureSetBits.hxx>
#include <OSD_ThreadPool.hxx>
#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace {
Handle(Graphic3d_ShaderProgram) planeShader()
{
    Handle(Graphic3d_ShaderProgram) shader = new Graphic3d_ShaderProgram();
    // Each structure owns its uniforms; sharing an ID would share mutable state.
    shader->SetNbClipPlanesMax(8);
    shader->SetTextureSetBits(Graphic3d_TextureSetBits_BaseColor);
    Graphic3d_ShaderObject::ShaderVariableList uniforms, varying;
    uniforms.Append({"vec4 CutPlane",Graphic3d_TOS_FRAGMENT});
    uniforms.Append({"vec4 CutNegative",Graphic3d_TOS_FRAGMENT});
    uniforms.Append({"vec4 CutPositive",Graphic3d_TOS_FRAGMENT});
    uniforms.Append({"int CutHasField",Graphic3d_TOS_FRAGMENT});
    uniforms.Append({"int CutMode",Graphic3d_TOS_FRAGMENT});
    uniforms.Append({"int CutLines",Graphic3d_TOS_FRAGMENT});
    uniforms.Append({"vec3 CutOrigin",Graphic3d_TOS_VERTEX});
    varying.Append({"vec3 CutLocal",Graphic3d_TOS_VERTEX | Graphic3d_TOS_FRAGMENT});
    varying.Append({"vec4 CutWorld",Graphic3d_TOS_VERTEX | Graphic3d_TOS_FRAGMENT});
    varying.Append({"vec3 CutNormal",Graphic3d_TOS_VERTEX | Graphic3d_TOS_FRAGMENT});
    varying.Append({"vec2 CutUV",Graphic3d_TOS_VERTEX | Graphic3d_TOS_FRAGMENT});
    TCollection_AsciiString vertexSource =
        "void main() {\n"
        " CutLocal = occVertex.xyz;\n"
        " CutWorld = occModelWorldMatrix * (occVertex + vec4(CutOrigin,0.0));\n"
        " gl_Position = occProjectionMatrix * occWorldViewMatrix * occModelWorldMatrix * vec4(CutOrigin,1.0)\n"
        "             + occProjectionMatrix * occWorldViewMatrix * occModelWorldMatrix * vec4(occVertex.xyz,0.0);\n"
        " CutNormal = mat3(occWorldViewMatrix * occModelWorldMatrix) * occNormal;\n"
        " CutUV = occTexCoord.xy;\n"
        "}\n";
    shader->AttachShader(Graphic3d_ShaderObject::CreateFromSource(vertexSource,Graphic3d_TOS_VERTEX,uniforms,varying));
    TCollection_AsciiString fragmentSource =
        "void main() {\n"
        " vec3 p = CutWorld.xyz / CutWorld.w;\n"
        " float side = dot(CutPlane,vec4(CutLocal,1.0));\n"
        " if (CutMode == 0 || (CutMode == 1 && side > 0.0) || (CutMode == 2 && side < 0.0)) { discard; }\n"
        " for (int i = 0; i < occClipPlaneCount;) {\n"
        "   if (dot(occClipPlaneEquations[i],vec4(p,1.0)) < 0.0) {\n"
        "     if (occClipPlaneChains[i] == 1) { discard; } ++i;\n"
        "   } else { i += occClipPlaneChains[i]; }\n"
        " }\n"
        " vec4 style = side <= 0.0 ? CutNegative : CutPositive;\n"
        " vec4 color = style;\n"
        " if (CutHasField != 0) { color.rgb = occTexture2D(occSamplerBaseColor,CutUV).rgb; }\n"
        " else if (CutLines == 0) { color.rgb *= 0.35 + 0.65 * abs(CutNormal.z) / max(length(CutNormal),1.0e-20); }\n"
        " occSetFragColor(color);\n"
        "}\n";
    shader->AttachShader(Graphic3d_ShaderObject::CreateFromSource(fragmentSource,Graphic3d_TOS_FRAGMENT,uniforms,varying));
    return shader;
}
void setUniforms(const Handle(Graphic3d_ShaderProgram)& shader,const Graphic3d_Vec4d& plane,
    const ViewCae::MeshCutDisplayStyle& negative,const ViewCae::MeshCutDisplayStyle& positive,bool field,
    const Graphic3d_Vec3d& origin,int mode,bool lines = false)
{
    auto color = [](const ViewCae::MeshCutDisplayStyle& style) {
        return Graphic3d_Vec4((float)style.Color.Red(),(float)style.Color.Green(),(float)style.Color.Blue(),1-style.Transparency);
    };
    shader->ClearVariables();
    shader->PushVariableVec4("CutPlane",Graphic3d_Vec4(plane));
    shader->PushVariableVec4("CutNegative",color(negative));
    shader->PushVariableVec4("CutPositive",color(positive));
    shader->PushVariableInt("CutHasField",field ? 1 : 0);
    shader->PushVariableVec3("CutOrigin",Graphic3d_Vec3(origin));
    shader->PushVariableInt("CutMode",mode);
    shader->PushVariableInt("CutLines",lines ? 1 : 0);
}
struct SliceVertex { Graphic3d_Vec3 Position, Normal; Graphic3d_Vec2 UV; };
static_assert(std::is_trivially_copyable<SliceVertex>::value, "Slice vertices must be copyable as buffer bytes");
bool hasSliceLayout(const Handle(Graphic3d_Buffer)& buffer)
{
    return buffer->IsInterleaved() && buffer->NbAttributes == 3 && buffer->Stride == sizeof(SliceVertex)
        && buffer->Attribute(0).Id == Graphic3d_TOA_POS && buffer->Attribute(0).DataType == Graphic3d_TOD_VEC3
        && buffer->Attribute(1).Id == Graphic3d_TOA_NORM && buffer->Attribute(1).DataType == Graphic3d_TOD_VEC3
        && buffer->Attribute(2).Id == Graphic3d_TOA_UV && buffer->Attribute(2).DataType == Graphic3d_TOD_VEC2
        && buffer->AttributeOffset(0) == offsetof(SliceVertex,Position)
        && buffer->AttributeOffset(1) == offsetof(SliceVertex,Normal)
        && buffer->AttributeOffset(2) == offsetof(SliceVertex,UV);
}
template<class Array>
bool reserveSlice(Handle(Array)& array,std::shared_ptr<void>& lease,size_t count,ViewCaeGeom::MeshCutViewManager& cuts)
{
    if (count > (size_t)std::numeric_limits<int>::max()/2) { throw std::length_error("Display slice exceeds buffer capacity"); }
    if (count == 0 || (!array.IsNull() && (size_t)array->VertexNumberAllocated() >= count)) { return false; }
    const int capacity = (int)std::max<size_t>(256,count*2);
    // Reserve old + new during staging, before any frame publication.
    lease = cuts.ReserveDisplayBytes((uint64_t)capacity*(sizeof(SliceVertex)+sizeof(unsigned))*2);
    array = new Array(capacity,capacity,Graphic3d_ArrayFlags_VertexNormal | Graphic3d_ArrayFlags_VertexTexel
        | Graphic3d_ArrayFlags_AttribsMutable | Graphic3d_ArrayFlags_IndexesMutable);
    for (int i = 0; i < capacity; ++i) { array->Indices()->SetIndex(i,i); }
    return true;
}
uint64_t publishSlice(const Handle(Graphic3d_ArrayOfPrimitives)& array,const std::vector<SliceVertex>& vertices)
{
    if (array.IsNull()) { return 0; }
    const bool countChanged = array->VertexNumber() != (int)vertices.size();
    array->Attributes()->NbElements = (int)vertices.size(); array->Indices()->NbElements = (int)vertices.size();
    if (!vertices.empty()) { std::memcpy(array->Attributes()->ChangeData(),vertices.data(),vertices.size()*sizeof(SliceVertex)); }
    // OCCT updates draw counts only when consuming an invalidated range.
    Handle(Graphic3d_AttribBuffer)::DownCast(array->Attributes())->Invalidate(0,(int)std::max<size_t>(1,vertices.size())-1);
    if (countChanged) { Handle(Graphic3d_MutableIndexBuffer)::DownCast(array->Indices())->Invalidate(0,0); }
    return std::max<size_t>(1,vertices.size())*sizeof(SliceVertex)+(countChanged ? array->Indices()->Stride : 0);
}
}

namespace ViewCae {
MeshCutGpuPresentation::~MeshCutGpuPresentation() { Reset(); }
void MeshCutGpuPresentation::Reset()
{
    if (!myParent.IsNull() && !myParent->IsDeleted() && !myBodyStructure.IsNull()) { myParent->Disconnect(myBodyStructure.get()); }
    if (!myBodyStructure.IsNull()) { myBodyStructure->Remove(); }
    myBodyStructure.Nullify(); myParent.Nullify(); mySliceGroup.Nullify(); mySliceEdgeGroup.Nullify();
    myBodyConnected = false;
}

void MeshCutGpuPresentation::Update(const Handle(MeshPresentationScalar)& body,
    const ViewCaeGeom::MeshCutResult& section,const ViewCaeGeom::MeshCutDefinition& definition,
    const std::array<MeshCutDisplayStyle,3>& styles,const Handle(Graphic3d_Texture2D)& texture,int component,
    ViewCaeGeom::MeshCutViewManager& cuts,MeshCutRenderStatistics& statistics)
{
    const unsigned mask = definition.Visibility;
    const bool showBody = (mask & 5) != 0;
    const bool showSlice = mask != 0 && mask != 5;
    const bool bodyChanged = !body.IsNull() && body != myBody;
    const bool sourceChanged = mySourceEpoch != cuts.Statistics().SourceEpoch;
    auto origin = myOrigin;
    auto bounds = myBounds;
    if (sourceChanged) {
        bounds = cuts.SourceBounds();
        if (!bounds.IsVoid()) { const auto a = bounds.CornerMin(), b = bounds.CornerMax(); origin = Graphic3d_Vec3d((a.X()+b.X())/2,(a.Y()+b.Y())/2,(a.Z()+b.Z())/2); }
    }
    auto bodyLease = myBodyLease;
    auto bodyArrays = myBodyArrays;
    uint64_t bodyBytes = 0;
    std::vector<std::vector<Graphic3d_Vec2>> bodyUVs;
    if (bodyChanged) {
        const auto data = body->GetMeshGeometry()->GetLastNotModifierChild()->GetMeshData();
        // Preflight before primitive construction, including old/new coexistence.
        const uint64_t bound = (uint64_t)(data->NbSurfElems()+data->NbElems2d())*64*(sizeof(SliceVertex)+sizeof(unsigned))+65536;
        bodyLease = cuts.ReserveDisplayBytes(bound);
        const auto& parts = body->GetParts();
        std::vector<Handle(Graphic3d_ArrayOfPrimitives)> inputs;
        for (int i = 0; i < MeshNbPartTypes; ++i) {
            const auto array = parts((MeshPartType)i)->PrimitiveArray();
            if (!array.IsNull() && parts((MeshPartType)i)->ToShow()) { inputs.push_back(array); }
        }
        bool rebuild = myBodyEpoch != cuts.Statistics().SourceEpoch || inputs.size() != bodyArrays.size();
        for (size_t i = 0; !rebuild && i < inputs.size(); ++i) {
            rebuild |= inputs[i]->VertexNumber() != bodyArrays[i]->VertexNumber() || inputs[i]->Type() != bodyArrays[i]->Type();
        }
        if (rebuild) { bodyArrays.clear(); }
        for (size_t i = 0; i < inputs.size(); ++i) {
            const auto input = inputs[i];
            const int count = input->VertexNumber();
            const int indices = input->Indices().IsNull() ? count : input->Indices()->NbElements;
            if (rebuild) {
                const auto flags = Graphic3d_ArrayFlags_VertexNormal | Graphic3d_ArrayFlags_VertexTexel
                    | Graphic3d_ArrayFlags_AttribsMutable | Graphic3d_ArrayFlags_AttribsDeinterleaved;
                Handle(Graphic3d_ArrayOfPrimitives) array;
                if (input->Type() == Graphic3d_TOPA_SEGMENTS) { array = new Graphic3d_ArrayOfSegments(count,indices,flags); }
                else { array = new Graphic3d_ArrayOfTriangles(count,indices,flags); }
                for (int v = 1; v <= count; ++v) {
                    const auto p = input->Vertice(v);
                    array->SetVertice(v,(float)(p.X()-origin.x()),(float)(p.Y()-origin.y()),(float)(p.Z()-origin.z()));
                    if (input->HasVertexNormals()) {
                        // Collapsed high-order patches may have zero normals.
                        // Copy components without constructing a gp_Dir.
                        double x, y, z; input->VertexNormal(v,x,y,z);
                        array->SetVertexNormal(v,x,y,z);
                    }
                    const auto uv = input->HasVertexTexels() ? input->VertexTexel(v) : gp_Pnt2d(0,0);
                    array->SetVertexTexel(v,uv.X(),uv.Y());
                }
                for (int index = 0; index < indices; ++index) { array->AddEdge(input->Indices().IsNull() ? index+1 : input->Indices()->Index(index)+1); }
                bodyBytes += array->Attributes()->Size()+array->Indices()->Size();
                bodyArrays.push_back(array);
            } else {
                std::vector<Graphic3d_Vec2> uv; uv.reserve(count);
                for (int v = 1; v <= count; ++v) {
                    const auto value = input->HasVertexTexels() ? input->VertexTexel(v) : gp_Pnt2d(0,0);
                    uv.emplace_back((float)value.X(),(float)value.Y());
                }
                bodyBytes += (uint64_t)count*sizeof(Graphic3d_Vec2);
                bodyUVs.push_back(std::move(uv));
            }
        }
    }
    std::vector<SliceVertex> vertices;
    std::vector<SliceVertex> edgeVertices;
    const int cap = (mask & 2) != 0 ? 1 : (mask & 1) != 0 ? 0 : 2;
    const auto& capStyle = styles[cap];
    const bool sliceFaces = capStyle.Mode != InteractiveCaeMeshPrsMode::MeshEdges;
    const bool sliceEdges = capStyle.Mode != InteractiveCaeMeshPrsMode::FlatShaded;
    Bnd_Box sliceBounds;
    if (showSlice && !section.Portions[1].IsNull()) {
        const auto mesh = section.Portions[1]->GetMeshData();
        const auto field = section.Fields[1];
        const auto source = cuts.SourceGeometry();
        const auto sourceMesh = source->GetMeshData();
        const auto values = field.IsNull() ? Handle(RWCae::MeshScalarPropertyBase)() : field->GetRawComponent(component);
        const bool nodal = !field.IsNull() && field->IsNodal();
        const bool nodeVector = !field.IsNull() && field->HasNodeVecComponents();
        double minimum = 0, maximum = 1;
        if (!section.SourceField.IsNull()) { section.SourceField->GetRawComponent(component)->ValueRange().GetBounds(minimum,maximum); }
        std::vector<Graphic3d_Vec3> localPositions(mesh->NbNodes());
        std::vector<Graphic3d_Vec2> nodeUVs(nodal ? mesh->NbNodes() : 0);
        const auto prepareNode = [&](int, int node) {
            Graphic3d_Vec3d position(mesh->Nodes()[node]);
            ViewCaeGeom::MeshInterpolatedNode weights;
            if (section.Portions[1]->OriginNode(node,weights)) {
                position = Graphic3d_Vec3d(0);
                for (const auto& weight : weights.BaseNodes()) {
                    auto p = sourceMesh->Nodes()[weight.first]; source->ModifyNode(weight.first,p);
                    position += Graphic3d_Vec3d(p)*weight.second;
                }
            }
            localPositions[node] = Graphic3d_Vec3(position-origin);
            if (nodal) {
                const double value = values->Value(RWCae::MeshPropertyEntityInfo(node,nodeVector ? node : -1));
                nodeUVs[node] = CaeMeshTools::GetUVCoords(value,minimum,maximum,CaeMeshTools::NbColorscaleColors);
            }
        };
        // Geometry and field data are materialized above. Workers only read
        // provenance/modifiers and write their own positions and color entries.
        if (mesh->NbNodes() >= 4096) {
            OSD_ThreadPool::Launcher launcher(*OSD_ThreadPool::DefaultPool(),4);
            launcher.Perform(0,(int)mesh->NbNodes(),prepareNode);
        } else {
            for (RWCae::MeshEntityId node = 0; node < mesh->NbNodes(); ++node) { prepareNode(0,(int)node); }
        }
        if (sliceFaces) { vertices.reserve((size_t)mesh->NbElems2d()*6); }
        if (sliceEdges) { edgeVertices.reserve((size_t)mesh->NbElems2d()*8); }
        Graphic3d_Vec3 lower(std::numeric_limits<float>::max()), upper(std::numeric_limits<float>::lowest());
        Graphic3d_Vec3 normal((float)definition.Plane.x(),(float)definition.Plane.y(),(float)definition.Plane.z());
        if ((mask & 5) == 4) { normal = -normal; }
        auto process = [&](const auto& adaptor) {
            for (RWCae::MeshEntityId e = 0; e < adaptor.NbElements(); ++e) {
                const auto& element = adaptor.GetElement(e);
                Graphic3d_Vec2 elementUV(0);
                if (!values.IsNull() && !nodal && !nodeVector) {
                    elementUV = CaeMeshTools::GetUVCoords(values->Value(RWCae::MeshPropertyEntityInfo(element.GlobalIndex())),
                        minimum,maximum,CaeMeshTools::NbColorscaleColors);
                }
                auto add = [&](int index,std::vector<SliceVertex>& target) {
                    const auto node = element[index];
                    // Reconstruct original FE position in double before storing
                    // in the shared local GPU frame. Physical data stay unchanged.
                    SliceVertex vertex{localPositions[node],normal,Graphic3d_Vec2(0)};
                    if (nodal) { vertex.UV = nodeUVs[node]; }
                    else if (nodeVector) {
                        const double value = values->Value(RWCae::MeshPropertyEntityInfo(element.GlobalIndex(),(int)node));
                        vertex.UV = Graphic3d_Vec2(CaeMeshTools::GetUVCoords(value,minimum,maximum,CaeMeshTools::NbColorscaleColors));
                    } else { vertex.UV = elementUV; }
                    for (int axis = 0; axis < 3; ++axis) {
                        lower[axis] = std::min(lower[axis],vertex.Position[axis]);
                        upper[axis] = std::max(upper[axis],vertex.Position[axis]);
                    }
                    target.push_back(vertex);
                };
                if (sliceFaces) {
                    for (int n = 1; n+1 < element.NbNodes(); ++n) { add(0,vertices); add(n,vertices); add(n+1,vertices); }
                }
                // FE section polygon perimeter, never the triangulation fan's diagonals.
                if (sliceEdges) {
                    for (int n = 0; n < element.NbNodes(); ++n) { add(n,edgeVertices); add((n+1)%element.NbNodes(),edgeVertices); }
                }
            }
        };
        RWCae::MeshAdaptorsIterator2dElems::For(process,mesh);
        if (!vertices.empty() || !edgeVertices.empty()) {
            sliceBounds.Update(lower.x()+origin.x(),lower.y()+origin.y(),lower.z()+origin.z(),
                upper.x()+origin.x(),upper.y()+origin.y(),upper.z()+origin.z());
        }
    }
    auto slice = mySlice;
    auto sliceLease = mySliceLease;
    auto sliceEdge = mySliceEdges;
    auto sliceEdgeLease = mySliceEdgeLease;
    const bool realloc = reserveSlice(slice,sliceLease,vertices.size(),cuts);
    const bool edgeRealloc = reserveSlice(sliceEdge,sliceEdgeLease,edgeVertices.size(),cuts);
    if (!slice.IsNull() && !hasSliceLayout(slice->Attributes())) { throw std::runtime_error("Unexpected slice vertex buffer layout"); }
    if (!sliceEdge.IsNull() && !hasSliceLayout(sliceEdge->Attributes())) { throw std::runtime_error("Unexpected slice edge buffer layout"); }
    auto bodyShader = myBodyShader.IsNull() ? planeShader() : myBodyShader;
    auto sliceShader = mySliceShader.IsNull() ? planeShader() : mySliceShader;
    auto bodyEdgeShader = myBodyEdgeShader.IsNull() ? planeShader() : myBodyEdgeShader;
    auto sliceEdgeShader = mySliceEdgeShader.IsNull() ? planeShader() : mySliceEdgeShader;
    Handle(Graphic3d_AspectFillArea3d) bodyAspect = new Graphic3d_AspectFillArea3d();
    Handle(Graphic3d_AspectFillArea3d) sliceAspect = new Graphic3d_AspectFillArea3d();
    Handle(Graphic3d_AspectLine3d) bodyEdgeAspect = new Graphic3d_AspectLine3d(Quantity_Color(Quantity_NOC_GRAY20),Aspect_TOL_SOLID,styles[0].LineWidth);
    Handle(Graphic3d_AspectLine3d) sliceEdgeAspect = new Graphic3d_AspectLine3d(Quantity_Color(Quantity_NOC_GRAY20),Aspect_TOL_SOLID,capStyle.LineWidth);
    auto configure = [&](const Handle(Graphic3d_AspectFillArea3d)& aspect,float opacity) {
        aspect->SetInteriorStyle(Aspect_IS_SOLID);
        aspect->SetInteriorColor(Quantity_ColorRGBA(Quantity_Color(Quantity_NOC_WHITE),opacity));
        aspect->SetBackInteriorColor(aspect->InteriorColor());
        Graphic3d_MaterialAspect material;
        material.SetTransparency(1-opacity);
        aspect->SetFrontMaterial(material); aspect->SetBackMaterial(material);
        aspect->SetFaceCulling(Graphic3d_TypeOfBackfacingModel_DoubleSided);
        if (!texture.IsNull()) { aspect->SetTextureMap(texture); aspect->SetTextureMapOn(true); }
    };
    // A mixed opaque/transparent body is submitted to OCCT's transparent pass;
    // fragment alpha remains per side, including a fully invisible side.
    configure(bodyAspect,std::max(styles[0].Transparency,styles[2].Transparency) > 0 ? .5f : 1.0f);
    configure(sliceAspect,1-capStyle.Transparency);
    bodyAspect->SetShaderProgram(bodyShader); sliceAspect->SetShaderProgram(sliceShader);
    bodyEdgeAspect->SetShaderProgram(bodyEdgeShader); sliceEdgeAspect->SetShaderProgram(sliceEdgeShader);
    if (styles[0].Mode == InteractiveCaeMeshPrsMode::ShadedMeshEdges) { bodyAspect->SetPolygonOffsets(Aspect_POM_Fill,1,1); }
    if (capStyle.Mode == InteractiveCaeMeshPrsMode::ShadedMeshEdges) { sliceAspect->SetPolygonOffsets(Aspect_POM_Fill,1,1); }
    // All potentially failing allocation/FE work precedes the paired publication.
    if (!cuts.IsCurrentRequest(cuts.RequestGeneration())) { throw std::runtime_error("Display frame cancelled"); }
    for (size_t i = 0; i < bodyUVs.size(); ++i) {
        for (size_t v = 0; v < bodyUVs[i].size(); ++v) { bodyArrays[i]->SetVertexTexel((int)v+1,bodyUVs[i][v].x(),bodyUVs[i][v].y()); }
        const auto buffer = Handle(Graphic3d_AttribBuffer)::DownCast(bodyArrays[i]->Attributes());
        buffer->Invalidate(buffer->FindAttribute(Graphic3d_TOA_UV));
    }
    const auto sliceBytes = publishSlice(slice,vertices)+publishSlice(sliceEdge,edgeVertices);
    auto localPlane = definition.Plane;
    localPlane.w() += localPlane.x()*origin.x()+localPlane.y()*origin.y()+localPlane.z()*origin.z();
    setUniforms(bodyShader,localPlane,styles[0],styles[2],!texture.IsNull(),origin,
        !showBody ? 0 : (mask & 5) == 1 ? 1 : (mask & 5) == 4 ? 2 : 3);
    setUniforms(sliceShader,localPlane,capStyle,capStyle,!texture.IsNull(),origin,3);
    auto edgeStyle = capStyle; edgeStyle.Color = Quantity_Color(Quantity_NOC_GRAY20);
    setUniforms(bodyEdgeShader,localPlane,edgeStyle,edgeStyle,false,origin,
        !showBody ? 0 : (mask & 5) == 1 ? 1 : (mask & 5) == 4 ? 2 : 3,true);
    setUniforms(sliceEdgeShader,localPlane,edgeStyle,edgeStyle,false,origin,3,true);
    const bool geometryBuild = bodyChanged && bodyUVs.empty();
    myBodyChanged |= geometryBuild; mySliceChanged |= realloc || edgeRealloc;
    if (!body.IsNull()) { myBody = body; }
    myOrigin = origin; myBounds = bounds; mySourceEpoch = cuts.Statistics().SourceEpoch; myBodyArrays = std::move(bodyArrays);
    if (bodyChanged) { myBodyEpoch = mySourceEpoch; }
    myBodyAspect = bodyAspect; mySliceAspect = sliceAspect;
    myBodyEdgeAspect = bodyEdgeAspect; mySliceEdgeAspect = sliceEdgeAspect;
    myBodyShader = bodyShader; mySliceShader = sliceShader;
    myBodyEdgeShader = bodyEdgeShader; mySliceEdgeShader = sliceEdgeShader;
    mySlice = slice; mySliceBounds = sliceBounds; myBodyLease = bodyLease; mySliceLease = sliceLease;
    mySliceEdges = sliceEdge; mySliceEdgeLease = sliceEdgeLease;
    myShowBody = showBody; myShowSlice = showSlice && (!vertices.empty() || !edgeVertices.empty());
    myPlane = definition.Plane; myMask = mask;
    statistics.BodyBuilds += geometryBuild ? 1 : 0;
    statistics.SliceReallocations += (realloc ? 1 : 0)+(edgeRealloc ? 1 : 0);
    statistics.SubmittedBodyBytes = bodyBytes;
    statistics.SubmittedSliceBytes = sliceBytes;
    updateStructures();
}

void MeshCutGpuPresentation::updateStructures()
{
    if (myBodyStructure.IsNull()) { return; }
    if (myBodyChanged) {
        myBodyStructure->Clear();
        for (const auto& array : myBodyArrays) {
            auto group = myBodyStructure->NewGroup();
            if (array->Type() == Graphic3d_TOPA_SEGMENTS) { group->SetGroupPrimitivesAspect(myBodyEdgeAspect); }
            else { group->SetGroupPrimitivesAspect(myBodyAspect); }
            group->AddPrimitiveArray(array);
            if (!myBounds.IsVoid()) {
                const auto a = myBounds.CornerMin(), b = myBounds.CornerMax();
                group->SetMinMaxValues(a.X(),a.Y(),a.Z(),b.X(),b.Y(),b.Z());
            }
        }
        myBodyChanged = false;
        myBodyStructure->CalculateBoundBox();
    }
    for (int i = 1; i <= myBodyStructure->NumberOfGroups(); ++i) {
        const auto group = myBodyStructure->Groups().Value(i);
        if (myBodyArrays[i-1]->Type() == Graphic3d_TOPA_SEGMENTS) { group->SetGroupPrimitivesAspect(myBodyEdgeAspect); }
        else { group->SetGroupPrimitivesAspect(myBodyAspect); }
    }
    myClip->SetEquation((myMask & 5) == 1 ? -myPlane : myPlane);
    myClip->SetCapping(false);
    // OCCT instanced geometry ignores child clip state. CutMode clips the
    // body in its local GPU frame and leaves the slice independent.
    myClip->SetOn(false);
    myBodyStructure->SetVisible(myShowBody);
    // Instanced children bypass their visibility flag too. Disconnect hidden
    // bodies while retaining the structure and its VBOs for the next mask.
    if (!myParent.IsNull() && myShowBody != myBodyConnected) {
        if (myShowBody) { myParent->Connect(myBodyStructure); }
        else { myParent->Disconnect(myBodyStructure.get()); }
        myBodyConnected = myShowBody;
    }
    const auto updateSliceGroup = [&](Handle(Graphic3d_Group)& group,const Handle(Graphic3d_ArrayOfPrimitives)& array,
                                      const Handle(Graphic3d_Aspects)& aspect) {
        if (mySliceChanged || (group.IsNull() && !array.IsNull())) {
            if (!group.IsNull()) { group->Clear(); }
            if (!array.IsNull()) {
                if (group.IsNull()) { group = myParent->NewGroup(); }
                group->SetGroupPrimitivesAspect(aspect);
                group->AddPrimitiveArray(array);
            }
        }
        if (group.IsNull()) { return; }
        group->SetGroupPrimitivesAspect(aspect);
        // Mutating vertices must refresh bounds as well as invalidate VBO data.
        if (!mySliceBounds.IsVoid()) {
            const auto a = mySliceBounds.CornerMin(), b = mySliceBounds.CornerMax();
            group->SetMinMaxValues(a.X(),a.Y(),a.Z(),b.X(),b.Y(),b.Z());
        }
    };
    updateSliceGroup(mySliceGroup,mySlice,mySliceAspect);
    updateSliceGroup(mySliceEdgeGroup,mySliceEdges,mySliceEdgeAspect);
    mySliceChanged = false;
    // SetMinMaxValues changes only the group's box. Refresh the structure and
    // layer BVH too, otherwise FitAll/culling still use the previous section.
    if (!myParent.IsNull()) { myParent->CalculateBoundBox(); myManager->Update(myParent->GetZLayer()); }
}

void MeshCutGpuPresentation::Attach(const Handle(Prs3d_Presentation)& parent,const Handle(Graphic3d_StructureManager)& manager)
{
    if (myBodyStructure.IsNull() || myManager != manager) {
        Reset();
        myManager = manager;
        myBodyStructure = new Graphic3d_Structure(manager);
        myClip = new Graphic3d_ClipPlane(myPlane); myClip->SetCapping(false);
        Handle(Graphic3d_SequenceOfHClipPlane) planes = new Graphic3d_SequenceOfHClipPlane(); planes->Append(myClip);
        myBodyStructure->SetClipPlanes(planes);
        myBodyChanged = !myBody.IsNull(); mySliceChanged = !mySlice.IsNull() || !mySliceEdges.IsNull();
    }
    myParent = parent;
    myBodyConnected = false;
    // Compute runs after the parent was cleared. Reattach the slice group;
    // warm plane edits do not request a parent recomputation.
    mySliceGroup.Nullify(); mySliceEdgeGroup.Nullify(); mySliceChanged = !mySlice.IsNull() || !mySliceEdges.IsNull();
    updateStructures();
}

bool MeshCutGpuPresentation::AcceptBodyPoint(const gp_Pnt& point) const
{
    if (!myShowBody) { return false; }
    const double value = myPlane.x()*point.X()+myPlane.y()*point.Y()+myPlane.z()*point.Z()+myPlane.w();
    return (myMask & 5) == 5 || ((myMask & 5) == 1 ? value <= 0 : value >= 0);
}
}
