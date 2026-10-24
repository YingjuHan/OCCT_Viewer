#include <RWVtk/VtkNativeReader.h>
#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCaeGeom/MeshCutUnion.h>
#include <RWCae/MeshScalarProperty.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ViewCaeGeom;
namespace {
void require(bool condition, const char* message)
{
    if (!condition) { throw std::runtime_error(message); }
}
void closeTo(double actual, double expected, const char* message, double tolerance = 2e-6)
{
    if (!std::isfinite(actual) || std::abs(actual-expected) > tolerance) {
        std::cerr << message << ": " << actual << " != " << expected << '\n';
        throw std::runtime_error(message);
    }
}
Handle(RWCae::MeshData) plate(double cycles, double phase)
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    const auto id = [](int x, int y, int k) { return RWCae::MeshEntityId(x+7*(y+7*k)); };
    for (int k = 0; k < 2; ++k) { for (int j = 0; j <= 6; ++j) { for (int i = 0; i <= 6; ++i) {
        const double x = i/6.0, top = .25+.15*std::sin(2*std::acos(-1.0)*cycles*x+phase);
        mesh->AddNodePosition(Graphic3d_Vec3((float)x,j/6.0f,(float)(top-(1-k)*.05)));
    } } }
    for (int j = 0; j < 6; ++j) { for (int i = 0; i < 6; ++i) {
        const RWCae::MeshEntityId nodes[] = {id(i,j,0),id(i+1,j,0),id(i+1,j+1,0),id(i,j+1,0),
            id(i,j,1),id(i+1,j,1),id(i+1,j+1,1),id(i,j+1,1)};
        mesh->AddElement(RWCae::MeshElementHexahedron(nodes));
    } }
    return mesh;
}
Handle(RWCae::MeshNVecProperty) temperature(const Handle(RWCae::MeshData)& mesh)
{
    Handle(RWCae::MeshNVecProperty) field = new RWCae::MeshNVecProperty("temperature",1,RWCae::MeshPropertyBase::MappingType::Nodal);
    field->Init<RWCae::MeshScalarProperty>();
    const auto scalar = Handle(RWCae::MeshScalarProperty)::DownCast(field->GetComponent(0));
    scalar->Init(mesh->NbNodes());
    for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
        scalar->SetValue(RWCae::MeshPropertyEntityInfo(n),mesh->Nodes()[n].z());
    }
    field->UpdateComponents(); return field;
}
// Independent 2D oracle: intersect the extruded XZ quadrilateral with a line.
// All tested planes are invariant along Y, so area = segment length * row width.
std::map<RWCae::MeshEntityId,double> expectedAreas(const Handle(RWCae::MeshData)& source, const Graphic3d_Vec4d& plane)
{
    std::map<RWCae::MeshEntityId,double> areas;
    for (RWCae::MeshEntityId e = 0; e < source->NbIndexedElems(); ++e) {
        std::vector<RWCae::MeshEntityId> nodes; source->ElementNodes(e,nodes);
        std::vector<Graphic3d_Vec3d> intersections;
        const int ring[] = {0,1,5,4,0};
        for (int edge = 0; edge < 4; ++edge) {
            const Graphic3d_Vec3d a(source->Nodes()[nodes[ring[edge]]]), b(source->Nodes()[nodes[ring[edge+1]]]);
            const double va = plane.x()*a.x()+plane.z()*a.z()+plane.w();
            const double vb = plane.x()*b.x()+plane.z()*b.z()+plane.w();
            if (std::abs(va) < 1e-8) { intersections.push_back(a); }
            if (va*vb < 0) { intersections.push_back(a+(b-a)*(va/(va-vb))); }
        }
        double length = 0;
        for (const auto& a : intersections) { for (const auto& b : intersections) { length = std::max(length,(b-a).Modulus()); } }
        const double width = source->Nodes()[nodes[3]].y()-source->Nodes()[nodes[0]].y();
        if (length*width > 1e-9) { areas[e] = length*width; }
    }
    return areas;
}
double negativeVolume(const Handle(RWCae::MeshData)& source, const Graphic3d_Vec4d& plane)
{
    double volume = 0;
    for (RWCae::MeshEntityId e = 0; e < source->NbIndexedElems(); ++e) {
        std::vector<RWCae::MeshEntityId> nodes; source->ElementNodes(e,nodes);
        std::vector<Graphic3d_Vec3d> polygon;
        for (int n : {0,1,5,4}) { polygon.emplace_back(source->Nodes()[nodes[n]]); }
        std::vector<Graphic3d_Vec3d> clipped;
        for (size_t i = 0; i < polygon.size(); ++i) {
            const auto a = polygon[i], b = polygon[(i+1)%polygon.size()];
            const double va = plane.x()*a.x()+plane.z()*a.z()+plane.w();
            const double vb = plane.x()*b.x()+plane.z()*b.z()+plane.w();
            if (va <= 0) { clipped.push_back(a); }
            if ((va < 0 && vb > 0) || (va > 0 && vb < 0)) { clipped.push_back(a+(b-a)*(va/(va-vb))); }
        }
        double twiceArea = 0;
        for (size_t i = 0; i < clipped.size(); ++i) {
            const auto a = clipped[i], b = clipped[(i+1)%clipped.size()]; twiceArea += a.x()*b.z()-b.x()*a.z();
        }
        volume += std::abs(twiceArea)*.5*(source->Nodes()[nodes[3]].y()-source->Nodes()[nodes[0]].y());
    }
    return volume;
}
void check(const Handle(RWCae::MeshData)& source, const Handle(RWCae::MeshNVecProperty)& field, const std::string& name)
{
    require(source->NbNodes() == 98 && source->NbIndexedElems() == 36,"Expected 98 nodes / 36 hexes");
    MeshCutViewManager manager(source); require(manager.AddPlane("p",Graphic3d_Vec4d(1,0,0,-.37)),"Plane creation");
    const std::vector<Graphic3d_Vec4d> planes = {
        Graphic3d_Vec4d(1,0,0,-.37),Graphic3d_Vec4d(1,0,0,-.5),Graphic3d_Vec4d(0,0,1,-.225),Graphic3d_Vec4d(0,0,1,-.25),
        Graphic3d_Vec4d(0,0,1,-.12),Graphic3d_Vec4d(0,0,1,-.33),Graphic3d_Vec4d(-.2,0,1,-.12),Graphic3d_Vec4d(.17,0,1,-.31),
        Graphic3d_Vec4d(0,0,1,-.01),Graphic3d_Vec4d(0,0,1,-.5)};
    for (size_t p = 0; p < planes.size(); ++p) {
        auto plane = planes[p];
        const auto expected = expectedAreas(source,plane);
        // Reversing the normal changes the retained side, never the section.
        for (int sign : {1,-1}) {
            manager.SetPlane("p",plane*(double)sign);
            for (unsigned mask : {2u,3u,6u,7u}) {
                manager.SetVisibility("p",mask);
                const auto cut = manager.Evaluate("p",field);
                const auto section = cut.Portions[1]; const auto mesh = section->GetMeshData();
                std::map<RWCae::MeshEntityId,double> actual;
                for (RWCae::MeshEntityId e = 0; e < mesh->NbIndexedElems(); ++e) {
                    const auto& indexed = mesh->IndexedElements()[e]; RWCae::MeshEntityId original;
                    require(section->OriginElement(MeshGeomElem(indexed.Index,indexed.Type,false),original),"Missing source element");
                    std::vector<RWCae::MeshEntityId> nodes; mesh->ElementNodes(e,nodes);
                    double area = 0;
                    for (size_t i = 1; i+1 < nodes.size(); ++i) {
                        const Graphic3d_Vec3d a(mesh->Nodes()[nodes[0]]), b(mesh->Nodes()[nodes[i]]), c(mesh->Nodes()[nodes[i+1]]);
                        area += Graphic3d_Vec3d::Cross(b-a,c-a).Modulus()*.5;
                    }
                    actual[original] += area;
                }
                // x=.5 coincides with a shared face. Trace selection emits it
                // once and may choose either incident cell as its owner.
                const bool sharedFace = p == 1;
                if (actual.size() != expected.size()/(sharedFace ? 2 : 1)) {
                    std::cerr << name << " plane " << p << " sign " << sign << " mask " << mask
                              << " expected " << expected.size() << " actual " << actual.size() << '\n';
                    throw std::runtime_error("Missing or duplicate section source cells");
                }
                double total = 0, reference = 0;
                for (const auto& item : expected) { reference += item.second; }
                for (const auto& item : actual) {
                    require(expected.count(item.first) != 0,"Section belongs to a nonintersecting cell");
                    closeTo(item.second,expected.at(item.first),"Per-cell section area"); total += item.second;
                }
                closeTo(total,reference/(sharedFace ? 2 : 1),"Total section area");
                for (RWCae::MeshEntityId node = 0; node < mesh->NbNodes(); ++node) {
                    const Graphic3d_Vec3d position(mesh->Nodes()[node]);
                    closeTo(plane.x()*position.x()+plane.z()*position.z()+plane.w(),0,"Section off plane");
                    MeshInterpolatedNode origin; require(section->OriginNode(node,origin),"Missing interpolation weights");
                    Graphic3d_Vec3d reconstructed(0); double sum = 0;
                    for (const auto& w : origin.BaseNodes()) { sum += w.second; reconstructed += Graphic3d_Vec3d(source->Nodes()[w.first])*w.second; }
                    closeTo(sum,1,"Interpolation partition of unity",1e-9);
                    closeTo((position-reconstructed).Modulus(),0,"Original FE position reconstruction");
                    closeTo(cut.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(node)),position.z(),"temperature != z");
                }
                // Exercise the section-only request used by the GPU backend too.
                const auto gpuSection = manager.EvaluateSectionForDisplay("p",field);
                require(gpuSection.Portions[1]->GetMeshData()->NbIndexedElems() == mesh->NbIndexedElems(),"GPU cap lost section cells");
            }
            // Check the actual CPU display union as well as numerical slices.
            // A correct cap alone cannot detect a missing retained body lobe.
            // The full union is substantially more expensive than section
            // evaluation. Exercise both lobes on the imported VTU baseline;
            // the period/phase matrix above checks each section separately.
            if (name == "vtu" && (p == 0 || p == 2)) {
                const double negative = negativeVolume(source,plane*(double)sign);
                double sides = 0;
                for (unsigned mask : {3u,6u,7u}) {
                    std::cout << "SINE_PLATE_BODY plane " << p << " sign " << sign << " mask " << mask << std::endl;
                    manager.SetVisibility("p",mask);
                    const auto presentation = manager.EvaluatePresentation();
                    const double reference = mask == 3 ? negative : mask == 6 ? .05-negative : .05;
                    closeTo(presentation->Volume,reference,"Retained sine body volume");
                    if (mask != 7) { sides += presentation->Volume; }
                }
                closeTo(sides,.05,"Negative + positive volume conservation");
            }
        }
        std::cout << "SINE_PLATE_SECTION " << name << " plane " << p << " source_cells " << expected.size() << std::endl;
    }
}
void samplingAlias()
{
    // At i/6, three cycles sample sin(pi*i)=0. The discrete source is flat;
    // the slicer must use that source instead of inventing the missing waves.
    const auto mesh = plate(3,0);
    for (int n = 0; n < 98; ++n) { closeTo(mesh->Nodes()[n].z(),n < 49 ? .20 : .25,"Three-cycle nodal alias",1e-7); }
    MeshCutViewManager manager(mesh); manager.AddPlane("alias",Graphic3d_Vec4d(0,0,1,-.225));
    const auto result = manager.EvaluateSectionForDisplay("alias",temperature(mesh));
    require(result.Portions[1]->GetMeshData()->NbIndexedElems() == 36,"Flat aliased source lost section cells");
}
}
int main()
{
    try {
        samplingAlias();
        for (const char* suffix : {"vtk","vtu"}) {
            RWVtk::VtkNativeReader reader;
            require(reader.Perform((std::string(SINE_PLATE_FIXTURE_DIR)+"/sine_plate_6x6."+suffix).c_str()),"Sine fixture import");
            const auto mesh = reader.GetMeshData();
            require(mesh->Properties().Size() == 1,"Imported point temperature missing");
            const auto field = mesh->Properties()[0];
            require(field->IsNodal() && field->NbComponents() == 1 && field->Name() == "temperature","Incorrect temperature mapping");
            for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
                closeTo(field->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),mesh->Nodes()[n].z(),"Source temperature != z");
            }
            check(mesh,field,suffix);
        }
        for (double cycles : {.5,1.0,1.5,2.0}) { for (double phase : {0.0,.31}) {
            const auto mesh = plate(cycles,phase);
            check(mesh,temperature(mesh),std::to_string(cycles)+"/"+std::to_string(phase));
        } }
        std::cout << "CUT_SINE_PLATE_PASS\n"; return 0;
    } catch (const std::exception& error) { std::cerr << "CUT_SINE_PLATE_FAIL " << error.what() << '\n'; return 1; }
}
