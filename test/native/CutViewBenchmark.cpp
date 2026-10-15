#include <ViewCaeGeom/MeshCutViewManager.h>
#include <RWCae/MeshScalarProperty.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv)
{
    try {
        const int n = argc > 1 ? std::stoi(argv[1]) : 100;
        const unsigned mask = argc > 2 ? (unsigned)std::stoul(argv[2]) : 2;
        const int frames = argc > 3 ? std::stoi(argv[3]) : 20;
        const double span = argc > 4 ? std::stod(argv[4]) : 0.7;
        if (n < 2 || n > 200 || mask > 7 || frames < 2 || frames > 1000) {
            throw std::invalid_argument("Usage: cut_view_benchmark [grid_size=100] [visibility=2] [frames=20] [movement_span=0.7]");
        }
        if (!std::isfinite(span) || span <= 0 || span >= n*0.5) { throw std::invalid_argument("Invalid movement span"); }
        Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
        Handle(RWCae::MeshNVecProperty) field = new RWCae::MeshNVecProperty("temperature", 1,
            RWCae::MeshPropertyBase::MappingType::Nodal);
        field->Init<RWCae::MeshScalarProperty>();
        auto component = Handle(RWCae::MeshScalarProperty)::DownCast(field->GetComponent(0));
        component->Init((n+1)*(n+1)*(n+1));
        auto node = [n](int x, int y, int z) -> RWCae::MeshEntityId { return x + (n+1)*(y + (n+1)*z); };
        for (int z = 0; z <= n; ++z) {
            for (int y = 0; y <= n; ++y) {
                for (int x = 0; x <= n; ++x) {
                    mesh->AddNodePosition(Graphic3d_Vec3((float)x, (float)y, (float)z));
                    component->SetValue(RWCae::MeshPropertyEntityInfo(node(x,y,z)), x + 2*y + 3*z, false);
                }
            }
        }
        field->UpdateComponents();
        for (int z = 0; z < n; ++z) {
            for (int y = 0; y < n; ++y) {
                for (int x = 0; x < n; ++x) {
                    const RWCae::MeshEntityId nodes[] = {
                        node(x,y,z), node(x+1,y,z), node(x+1,y+1,z), node(x,y+1,z),
                        node(x,y,z+1), node(x+1,y,z+1), node(x+1,y+1,z+1), node(x,y+1,z+1)};
                    mesh->AddElement(RWCae::MeshElementHexahedron(nodes));
                }
            }
        }
        ViewCaeGeom::MeshCutViewManager manager(mesh);
        manager.AddPlane("benchmark", Graphic3d_Vec4d(1,0,0,-n*0.5-0.125));
        manager.SetVisibility("benchmark", mask);
        using Clock = std::chrono::steady_clock;
        auto begin = Clock::now();
        manager.Evaluate("benchmark", field);
        const double cold = std::chrono::duration<double, std::milli>(Clock::now()-begin).count();
        std::vector<double> timings;
        for (int i = 0; i < frames; ++i) {
            begin = Clock::now();
            manager.SetPlane("benchmark", Graphic3d_Vec4d(1,0,0,-n*0.5-0.15-span*(i+1)/(frames+1)));
            auto result = manager.Evaluate("benchmark", field);
            timings.push_back(std::chrono::duration<double, std::milli>(Clock::now()-begin).count());
            if ((mask & 2) && result.Portions[1]->GetMeshData()->NbIndexedElems() != (unsigned)n*n) {
                throw std::runtime_error("Benchmark section count is incorrect");
            }
        }
        std::sort(timings.begin(), timings.end());
        std::cout << "cells=" << mesh->NbIndexedElems() << " visibility=" << mask << " frames=" << frames << " span=" << span
                  << " cold_ms=" << cold << " p50_ms=" << timings[timings.size()/2]
                  << " p95_ms=" << timings[(size_t)std::ceil(0.95*timings.size())-1]
                  << " measurement=geometry_and_fields_only rendering=excluded\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
