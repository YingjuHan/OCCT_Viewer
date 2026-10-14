#include <ViewCae/MeshCutGpuPresentation.h>
#include <ViewCae/CaeMeshTools.h>
#include <RWCae/MeshAdaptorsIterator.h>
#include <Graphic3d_AttribBuffer.hxx>
#include <Graphic3d_MutableIndexBuffer.hxx>
#include <Graphic3d_ShaderObject.hxx>
#include <Graphic3d_TextureSetBits.hxx>
#include <limits>
#include <stdexcept>

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
    varying.Append({"vec4 CutWorld",Graphic3d_TOS_VERTEX | Graphic3d_TOS_FRAGMENT});
    varying.Append({"vec3 CutNormal",Graphic3d_TOS_VERTEX | Graphic3d_TOS_FRAGMENT});
    varying.Append({"vec2 CutUV",Graphic3d_TOS_VERTEX | Graphic3d_TOS_FRAGMENT});
    TCollection_AsciiString vertexSource =
        "void main() {\n"
        " CutWorld = occModelWorldMatrix * occVertex;\n"
        " gl_Position = occProjectionMatrix * occWorldViewMatrix * CutWorld;\n"
        " CutNormal = mat3(occWorldViewMatrix * occModelWorldMatrix) * occNormal;\n"
        " CutUV = occTexCoord.xy;\n"
        "}\n";
    shader->AttachShader(Graphic3d_ShaderObject::CreateFromSource(vertexSource,Graphic3d_TOS_VERTEX,uniforms,varying));
    TCollection_AsciiString fragmentSource =
        "void main() {\n"
        " vec3 p = CutWorld.xyz / CutWorld.w;\n"
        " for (int i = 0; i < occClipPlaneCount;) {\n"
        "   if (dot(occClipPlaneEquations[i],vec4(p,1.0)) < 0.0) {\n"
        "     if (occClipPlaneChains[i] == 1) { discard; } ++i;\n"
        "   } else { i += occClipPlaneChains[i]; }\n"
        " }\n"
        " vec4 style = dot(CutPlane,vec4(p,1.0)) <= 0.0 ? CutNegative : CutPositive;\n"
        " vec4 color = style;\n"
        " if (CutHasField != 0) { color.rgb = occTexture2D(occSamplerBaseColor,CutUV).rgb; }\n"
        " else { color.rgb *= 0.35 + 0.65 * abs(normalize(CutNormal).z); }\n"
        " occSetFragColor(color);\n"
        "}\n";
    shader->AttachShader(Graphic3d_ShaderObject::CreateFromSource(fragmentSource,Graphic3d_TOS_FRAGMENT,uniforms,varying));
    return shader;
}
void setUniforms(const Handle(Graphic3d_ShaderProgram)& shader,const Graphic3d_Vec4d& plane,
    const ViewCae::MeshCutDisplayStyle& negative,const ViewCae::MeshCutDisplayStyle& positive,bool field)
{
    auto color = [](const ViewCae::MeshCutDisplayStyle& style) {
        return Graphic3d_Vec4((float)style.Color.Red(),(float)style.Color.Green(),(float)style.Color.Blue(),1-style.Transparency);
    };
    shader->ClearVariables();
    shader->PushVariableVec4("CutPlane",Graphic3d_Vec4(plane));
    shader->PushVariableVec4("CutNegative",color(negative));
    shader->PushVariableVec4("CutPositive",color(positive));
    shader->PushVariableInt("CutHasField",field ? 1 : 0);
}
struct SliceVertex { Graphic3d_Vec3 Position, Normal; Graphic3d_Vec2 UV; };
}

namespace ViewCae {
MeshCutGpuPresentation::~MeshCutGpuPresentation() { Reset(); }
void MeshCutGpuPresentation::Reset()
{
    if (!myParent.IsNull() && !myParent->IsDeleted() && !myBodyStructure.IsNull()) { myParent->Disconnect(myBodyStructure.get()); }
    if (!myBodyStructure.IsNull()) { myBodyStructure->Remove(); }
    myBodyStructure.Nullify(); myParent.Nullify(); mySliceGroup.Nullify();
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
    auto bodyLease = myBodyLease;
    uint64_t bodyBytes = 0;
    if (bodyChanged) {
        const auto& parts = body->GetParts();
        for (int i = 0; i < MeshNbPartTypes; ++i) {
            const auto array = parts((MeshPartType)i)->PrimitiveArray();
            if (!array.IsNull()) {
                bodyBytes += array->Attributes()->Size();
                if (!array->Indices().IsNull()) { bodyBytes += array->Indices()->Size(); }
            }
        }
        bodyLease = cuts.ReserveDisplayBytes(bodyBytes*2+65536);
    }
    std::vector<SliceVertex> vertices;
    if (showSlice && !section.Portions[1].IsNull()) {
        const auto mesh = section.Portions[1]->GetMeshData();
        const auto field = section.Fields[1];
        double minimum = 0, maximum = 1;
        if (!section.SourceField.IsNull()) { section.SourceField->GetRawComponent(component)->ValueRange().GetBounds(minimum,maximum); }
        Graphic3d_Vec3 normal((float)definition.Plane.x(),(float)definition.Plane.y(),(float)definition.Plane.z());
        if ((mask & 5) == 4) { normal = -normal; }
        auto process = [&](const auto& adaptor) {
            for (RWCae::MeshEntityId e = 0; e < adaptor.NbElements(); ++e) {
                const auto& element = adaptor.GetElement(e);
                auto add = [&](int index) {
                    const auto node = element[index];
                    SliceVertex vertex{mesh->Nodes()[node],normal,Graphic3d_Vec2(0)};
                    if (!field.IsNull()) {
                        const double value = field->GetRawComponent(component)->Value(RWCae::MeshPropertyEntityInfo(
                            field->IsNodal() ? node : element.GlobalIndex(),field->HasNodeVecComponents() ? (int)node : -1));
                        vertex.UV = Graphic3d_Vec2(CaeMeshTools::GetUVCoords(value,minimum,maximum,CaeMeshTools::NbColorscaleColors));
                    }
                    vertices.push_back(vertex);
                };
                for (int n = 1; n+1 < element.NbNodes(); ++n) { add(0); add(n); add(n+1); }
            }
        };
        RWCae::MeshAdaptorsIterator2dElems::For(process,mesh);
    }
    if (vertices.size() > (size_t)std::numeric_limits<int>::max()/2) { throw std::length_error("Display slice exceeds buffer capacity"); }
    auto slice = mySlice;
    auto sliceLease = mySliceLease;
    const bool realloc = !vertices.empty() && (slice.IsNull() || (size_t)slice->VertexNumberAllocated() < vertices.size());
    if (realloc) {
        const int capacity = (int)std::max<size_t>(256,vertices.size()*2);
        // Reserve old + new during staging. No publication on budget failure.
        sliceLease = cuts.ReserveDisplayBytes((uint64_t)capacity*(sizeof(SliceVertex)+sizeof(unsigned))*2);
        slice = new Graphic3d_ArrayOfTriangles(capacity,capacity,
            Graphic3d_ArrayFlags_VertexNormal | Graphic3d_ArrayFlags_VertexTexel
            | Graphic3d_ArrayFlags_AttribsMutable | Graphic3d_ArrayFlags_IndexesMutable);
    }
    auto bodyShader = myBodyShader.IsNull() ? planeShader() : myBodyShader;
    auto sliceShader = mySliceShader.IsNull() ? planeShader() : mySliceShader;
    Handle(Graphic3d_AspectFillArea3d) bodyAspect = new Graphic3d_AspectFillArea3d();
    Handle(Graphic3d_AspectFillArea3d) sliceAspect = new Graphic3d_AspectFillArea3d();
    const int cap = (mask & 2) != 0 ? 1 : (mask & 1) != 0 ? 0 : 2;
    const auto& capStyle = styles[cap];
    auto configure = [&](const Handle(Graphic3d_AspectFillArea3d)& aspect,float opacity) {
        aspect->SetInteriorStyle(Aspect_IS_SOLID);
        aspect->SetInteriorColor(Quantity_ColorRGBA(Quantity_Color(Quantity_NOC_WHITE),opacity));
        aspect->SetBackInteriorColor(aspect->InteriorColor());
        aspect->SetFaceCulling(Graphic3d_TypeOfBackfacingModel_DoubleSided);
        if (!texture.IsNull()) { aspect->SetTextureMap(texture); aspect->SetTextureMapOn(true); }
    };
    configure(bodyAspect,1-std::max(styles[0].Transparency,styles[2].Transparency));
    configure(sliceAspect,1-capStyle.Transparency);
    bodyAspect->SetShaderProgram(bodyShader); sliceAspect->SetShaderProgram(sliceShader);
    // All potentially failing allocation/FE work precedes the paired publication.
    if (!cuts.IsCurrentRequest(cuts.RequestGeneration())) { throw std::runtime_error("Display frame cancelled"); }
    if (!slice.IsNull()) {
        slice->Attributes()->NbElements = (int)vertices.size();
        slice->Indices()->NbElements = (int)vertices.size();
        for (size_t i = 0; i < vertices.size(); ++i) {
            slice->SetVertice((int)i+1,vertices[i].Position.x(),vertices[i].Position.y(),vertices[i].Position.z());
            slice->SetVertexNormal((int)i+1,vertices[i].Normal.x(),vertices[i].Normal.y(),vertices[i].Normal.z());
            slice->SetVertexTexel((int)i+1,vertices[i].UV.x(),vertices[i].UV.y());
            slice->Indices()->SetIndex((int)i,(int)i);
        }
        // Invalidate one reserved entry for an empty frame too: OCCT updates
        // draw counts only when consuming an invalidated mutable buffer range.
        const int last = (int)std::max<size_t>(1,vertices.size())-1;
        Handle(Graphic3d_AttribBuffer)::DownCast(slice->Attributes())->Invalidate(0,last);
        Handle(Graphic3d_MutableIndexBuffer)::DownCast(slice->Indices())->Invalidate(0,last);
    }
    setUniforms(bodyShader,definition.Plane,styles[0],styles[2],!texture.IsNull());
    setUniforms(sliceShader,definition.Plane,capStyle,capStyle,!texture.IsNull());
    myBodyChanged |= bodyChanged; mySliceChanged |= realloc;
    if (!body.IsNull()) { myBody = body; }
    myBodyAspect = bodyAspect; mySliceAspect = sliceAspect;
    myBodyShader = bodyShader; mySliceShader = sliceShader;
    mySlice = slice; myBodyLease = bodyLease; mySliceLease = sliceLease;
    myShowBody = showBody; myShowSlice = showSlice && !vertices.empty();
    myPlane = definition.Plane; myMask = mask;
    statistics.BodyBuilds += bodyChanged ? 1 : 0;
    statistics.SliceReallocations += realloc ? 1 : 0;
    statistics.SubmittedBodyBytes = bodyBytes;
    statistics.SubmittedSliceBytes = slice.IsNull() ? 0 : std::max<size_t>(1,vertices.size())*(sizeof(SliceVertex)+sizeof(unsigned));
    updateStructures();
}

void MeshCutGpuPresentation::updateStructures()
{
    if (myBodyStructure.IsNull()) { return; }
    if (myBodyChanged) {
        myBodyStructure->Clear();
        const auto& parts = myBody->GetParts();
        for (int i = 0; i < MeshNbPartTypes; ++i) {
            const auto array = parts((MeshPartType)i)->PrimitiveArray();
            if (!parts((MeshPartType)i)->ToShow() || array.IsNull()) { continue; }
            auto group = myBodyStructure->NewGroup();
            group->SetGroupPrimitivesAspect(myBodyAspect);
            group->AddPrimitiveArray(array);
        }
        myBodyChanged = false;
    }
    for (int i = 1; i <= myBodyStructure->NumberOfGroups(); ++i) {
        myBodyStructure->Groups().Value(i)->SetGroupPrimitivesAspect(myBodyAspect);
    }
    myClip->SetEquation((myMask & 5) == 1 ? -myPlane : myPlane);
    myClip->SetCapping(false);
    myClip->SetOn((myMask & 5) == 1 || (myMask & 5) == 4);
    myBodyStructure->SetVisible(myShowBody);
    if (mySliceChanged || (mySliceGroup.IsNull() && !mySlice.IsNull())) {
        if (!mySliceGroup.IsNull()) { mySliceGroup->Clear(); }
        if (!mySlice.IsNull()) {
            if (mySliceGroup.IsNull()) { mySliceGroup = myParent->NewGroup(); }
            mySliceGroup->SetGroupPrimitivesAspect(mySliceAspect);
            mySliceGroup->AddPrimitiveArray(mySlice);
        }
        mySliceChanged = false;
    }
    if (!mySliceGroup.IsNull()) {
        mySliceGroup->SetGroupPrimitivesAspect(mySliceAspect);
        // Mutating vertices must refresh bounds as well as invalidate VBO data.
        if (mySlice->VertexNumber() > 0) {
            Bnd_Box bounds;
            for (int i = 1; i <= mySlice->VertexNumber(); ++i) { bounds.Add(mySlice->Vertice(i)); }
            const auto minimum = bounds.CornerMin(), maximum = bounds.CornerMax();
            mySliceGroup->SetMinMaxValues(minimum.X(),minimum.Y(),minimum.Z(),maximum.X(),maximum.Y(),maximum.Z());
        }
    }
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
        myBodyChanged = !myBody.IsNull(); mySliceChanged = !mySlice.IsNull();
    }
    myParent = parent;
    // Compute runs after the parent was cleared. Reattach the slice group;
    // warm plane edits do not request a parent recomputation.
    mySliceGroup.Nullify(); mySliceChanged = !mySlice.IsNull();
    updateStructures();
    parent->Connect(myBodyStructure);
}

bool MeshCutGpuPresentation::AcceptBodyPoint(const gp_Pnt& point) const
{
    if (!myShowBody) { return false; }
    const double value = myPlane.x()*point.X()+myPlane.y()*point.Y()+myPlane.z()*point.Z()+myPlane.w();
    return (myMask & 5) == 5 || ((myMask & 5) == 1 ? value <= 0 : value >= 0);
}
}
