#include <RWVtk/VtkNativeReader.h>
#include <RWVtk/VtkValidationReader.h>
#include <RWVtk/VtkLegacyReader.h>
#include <RWCae/MeshScalarProperty.h>

#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkGenericCell.h>
#include <vtkGenericDataObjectReader.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkXMLGenericDataObjectReader.h>
#include <vtkCommand.h>
#include <vtkExecutive.h>
#include <vtkUnstructuredGrid.h>
#include <Standard_Failure.hxx>

#include <cmath>
#include <algorithm>
#include <cctype>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace {
class ImportErrors : public vtkCommand {
public:
    static ImportErrors* New() { return new ImportErrors(); }
    std::string Message;
    void Execute(vtkObject*, unsigned long, void* data) override
    {
        Message += data == nullptr ? "VTK import error" : static_cast<const char*>(data);
    }
};

bool validHigherOrderCount(int type, vtkIdType count, const std::vector<int>& degrees)
{
    const int family = type >= 75 ? type-75 : type-68;
    auto cardinality = [family](int p, int q, int r) -> vtkIdType {
        switch (family) {
        case 0: return p+1;
        case 1: return (vtkIdType)(p+1)*(p+2)/2;
        case 2: return (vtkIdType)(p+1)*(q+1);
        case 3: return (vtkIdType)(p+1)*(p+2)*(p+3)/6;
        case 4: return (vtkIdType)(p+1)*(q+1)*(r+1);
        case 5: return (vtkIdType)(p+1)*(p+2)*(r+1)/2;
        default: return -1;
        }
    };
    auto matches = [&](int p, int q, int r) {
        if (p < 1 || (family == 2 || family == 4 || family == 5 ? q < 1 : false)
            || (family == 4 || family == 5 ? r < 1 : false) || (family == 5 && p != q)) { return false; }
        return cardinality(p,q,r) == count || (p == 2 && ((family == 1 && count == 7) || (family == 3 && count == 15)))
            || (family == 5 && p == 2 && r == 2 && count == 21);
    };
    if (!degrees.empty()) { return matches(degrees[0],degrees[1],degrees[2]); }
    for (int order = 1; order <= 1024; ++order) {
        if (matches(order,order,order)) { return true; }
        if (cardinality(order,order,order) > count) { break; }
    }
    return false;
}
}

namespace RWVtk {
bool VtkNativeReader::Perform(const TCollection_AsciiString& filename)
{
    myMesh.Nullify(); myReport = VtkImportReport();
    try {
        vtkSmartPointer<vtkDataSet> data;
        vtkSmartPointer<ImportErrors> errors = vtkSmartPointer<ImportErrors>::New();
        vtkSmartPointer<ImportErrors> warnings = vtkSmartPointer<ImportErrors>::New();
        const std::string path(filename.ToCString());
        std::string lowerPath = path;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        const auto budget = myBudget ? myBudget : std::make_shared<VtkResourceBudget>(myLimits);
        if (lowerPath.size() >= 4 && lowerPath.substr(lowerPath.size()-4) == ".vtk") {
            {
                VtkLegacyReader preflight;
                preflight.SetValidationOnly(true); preflight.SetResourceBudget(budget); preflight.SetToReportMessages(false);
                if (!preflight.Perform(filename)) { myReport = preflight.Report(); return false; }
            }
            vtkNew<vtkGenericDataObjectReader> reader;
            reader->AddObserver(vtkCommand::ErrorEvent,errors);
            reader->AddObserver(vtkCommand::WarningEvent,warnings);
            reader->GetExecutive()->AddObserver(vtkCommand::ErrorEvent,errors);
            reader->SetFileName(path.c_str()); reader->ReadAllScalarsOn(); reader->ReadAllVectorsOn();
            reader->ReadAllTensorsOn(); reader->ReadAllFieldsOn();
            reader->ReadAllNormalsOn(); reader->ReadAllColorScalarsOn(); reader->ReadAllTCoordsOn();
            if (!reader->GetExecutive()->Update()) { throw std::runtime_error("Native VTK legacy pipeline failed"); }
            data = vtkDataSet::SafeDownCast(reader->GetOutput());
        } else {
            {
                VtkValidationReader preflight(lowerPath.size() >= 4 && lowerPath.substr(lowerPath.size()-4) == ".vtp");
                preflight.SetResourceBudget(budget);
                preflight.SetToReportMessages(false);
                if (!preflight.Perform(filename)) {
                    myReport = preflight.Report();
                    return false;
                }
            }
            vtkNew<vtkXMLGenericDataObjectReader> reader;
            reader->AddObserver(vtkCommand::ErrorEvent,errors);
            reader->AddObserver(vtkCommand::WarningEvent,warnings);
            reader->GetExecutive()->AddObserver(vtkCommand::ErrorEvent,errors);
            reader->SetFileName(path.c_str());
            if (!reader->GetExecutive()->Update()) { throw std::runtime_error("Native VTK XML pipeline failed"); }
            data = vtkDataSet::SafeDownCast(reader->GetOutput());
        }
        if (!warnings->Message.empty()) { myReport.warnings.push_back(warnings->Message); }
        if (!errors->Message.empty()) { throw std::runtime_error(errors->Message); }
        if (!data || (data->GetNumberOfPoints() == 0 && data->GetNumberOfCells() == 0)
            || data->GetNumberOfPoints() > std::numeric_limits<int>::max()
            || data->GetNumberOfCells() > std::numeric_limits<int>::max()) {
            throw std::runtime_error("Native VTK requires a dataset within native entity limits");
        }
        Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
        double time = 0.0;
        if (const auto array = data->GetFieldData()->GetArray("TimeValue")) {
            if (array->GetNumberOfComponents() != 1 || array->GetNumberOfTuples() != 1
                || !std::isfinite(array->GetComponent(0,0))) { throw std::runtime_error("TimeValue requires one finite scalar tuple"); }
            time = array->GetComponent(0,0);
        }
        for (vtkIdType n = 0; n < data->GetNumberOfPoints(); ++n) {
            double p[3]; data->GetPoint(n,p);
            for (double value : p) {
                if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max()) {
                    throw std::runtime_error("Nonfinite or unrepresentable VTK coordinates");
                }
            }
            mesh->AddNodePosition(Graphic3d_Vec3((float)p[0],(float)p[1],(float)p[2]));
        }
        const std::map<int,int> fixed = {{0,0},{1,1},{3,2},{5,3},{8,4},{9,4},{10,4},{11,8},{12,8},{13,6},{14,5},
            {15,10},{16,12},{21,3},{22,6},{23,8},{24,10},{25,20},{26,15},{27,13},{28,9},{29,27},{30,6},
            {31,12},{32,18},{33,24},{34,7},{35,4},{37,19}};
        vtkNew<vtkIdList> ids;
        vtkNew<vtkGenericCell> cell;
        for (vtkIdType e = 0; e < data->GetNumberOfCells(); ++e) {
            const int type = data->GetCellType(e);
            auto concrete = vtkSmartPointer<vtkCell>::Take(vtkGenericCell::InstantiateCell(type));
            if (!concrete) { throw std::runtime_error("VTK cell type "+std::to_string(type)+" is abstract, reserved or unimplemented in this VTK SDK"); }
            data->GetCellPoints(e,ids);
            const auto expected = fixed.find(type);
            if ((expected != fixed.end() && ids->GetNumberOfIds() != expected->second)
                || (expected == fixed.end() && ids->GetNumberOfIds() < 1)) {
                throw std::runtime_error("Incorrect node count for VTK cell type "+std::to_string(type));
            }
            RWCae::MeshData::GenericCell topology;
            topology.Type = type;
            for (vtkIdType n = 0; n < ids->GetNumberOfIds(); ++n) {
                const auto id = ids->GetId(n);
                if (id < 0 || id >= data->GetNumberOfPoints()) { throw std::runtime_error("VTK cell point index out of range"); }
                topology.Nodes.push_back((RWCae::MeshEntityId)id);
            }
            if (auto degrees = data->GetCellData()->GetHigherOrderDegrees()) {
                if (degrees->GetNumberOfComponents() != 3 || degrees->GetNumberOfTuples() != data->GetNumberOfCells()) {
                    throw std::runtime_error("Invalid HigherOrderDegrees array");
                }
                for (int a = 0; a < 3; ++a) {
                    const double degree = degrees->GetComponent(e,a);
                    if (!std::isfinite(degree) || degree < 0 || std::trunc(degree) != degree || degree > 1024) {
                        throw std::runtime_error("Invalid higher-order degree");
                    }
                    topology.Degrees.push_back((int)degree);
                }
            }
            const vtkIdType count = ids->GetNumberOfIds();
            if ((type == 4 && count < 2) || ((type == 6 || type == 7) && count < 3)
                || (type == 36 && (count < 6 || count%2)) || ((type == 41 || type == 42) && count < 4)
                || (type >= 68 && !validHigherOrderCount(type,count,topology.Degrees))) {
                throw std::runtime_error("VTK node count is inconsistent with cell type/degrees");
            }
            if (auto weights = data->GetPointData()->GetRationalWeights()) {
                if (weights->GetNumberOfComponents() != 1 || weights->GetNumberOfTuples() != data->GetNumberOfPoints()) {
                    throw std::runtime_error("Invalid RationalWeights array");
                }
                for (auto id : topology.Nodes) {
                    const double weight = weights->GetComponent(id,0);
                    if (!std::isfinite(weight) || weight <= 0) { throw std::runtime_error("Rational weights must be finite and positive"); }
                    topology.RationalWeights.push_back(weight);
                }
            }
            if (type == 42) {
                const auto grid = vtkUnstructuredGrid::SafeDownCast(data);
                if (!grid) { throw std::runtime_error("Polyhedron requires an unstructured grid"); }
                vtkNew<vtkIdList> faces; grid->GetFaceStream(e,faces);
                if (faces->GetNumberOfIds() < 1 || faces->GetId(0) < 4) { throw std::runtime_error("Invalid polyhedron face stream"); }
                const std::set<RWCae::MeshEntityId> members(topology.Nodes.begin(),topology.Nodes.end());
                vtkIdType offset = 1;
                for (vtkIdType f = 0; f < faces->GetId(0); ++f) {
                    if (offset >= faces->GetNumberOfIds()) { throw std::runtime_error("Truncated polyhedron faces"); }
                    const auto size = faces->GetId(offset++);
                    if (size < 3 || size > faces->GetNumberOfIds()-offset) { throw std::runtime_error("Invalid polyhedron face size"); }
                    std::vector<RWCae::MeshEntityId> nodes;
                    for (vtkIdType n = 0; n < size; ++n) {
                        const auto id = faces->GetId(offset++);
                        if (id < 0 || id >= data->GetNumberOfPoints() || !members.count((RWCae::MeshEntityId)id)) {
                            throw std::runtime_error("Polyhedron face point is not in cell connectivity");
                        }
                        nodes.push_back((RWCae::MeshEntityId)id);
                    }
                    topology.Faces.push_back(std::move(nodes));
                }
                if (offset != faces->GetNumberOfIds()) { throw std::runtime_error("Extra polyhedron face data"); }
            }
            mesh->AddGenericCell(topology);
        }
        auto fields = [&](vtkDataSetAttributes* attributes, bool nodal) {
            const auto count = nodal ? data->GetNumberOfPoints() : data->GetNumberOfCells();
            for (int a = 0; a < attributes->GetNumberOfArrays(); ++a) {
                auto array = attributes->GetArray(a);
                if (!array) {
                    myReport.warnings.push_back("Non-numeric VTK attribute array is not exposed as a numeric field");
                    continue;
                }
                if (!array->GetName() || !array->GetName()[0]) { throw std::runtime_error("VTK numeric field has no name"); }
                if (array->GetNumberOfTuples() != count) { throw std::runtime_error("VTK field tuple count mismatch"); }
                const auto type = array == attributes->GetVectors() || array == attributes->GetNormals() ? RWCae::MeshNVecPropertyType::Vector
                    : array == attributes->GetTensors() ? RWCae::MeshNVecPropertyType::Tensor : RWCae::MeshNVecPropertyType::ScalarSet;
                const int rawComponents = array->GetNumberOfComponents() + (type == RWCae::MeshNVecPropertyType::Vector ? 1 : 0);
                for (int c = 0; c < rawComponents; ++c) { budget->Array((uint64_t)count*sizeof(double)); }
                Handle(RWCae::MeshNVecProperty) field = new RWCae::MeshNVecProperty(array->GetName(),array->GetNumberOfComponents(),
                    nodal ? RWCae::MeshPropertyBase::MappingType::Nodal : RWCae::MeshPropertyBase::MappingType::Elemental,type);
                field->Init<RWCae::MeshScalarProperty>();
                field->SetTime(time, 0);
                for (int c = 0; c < array->GetNumberOfComponents(); ++c) {
                    auto component = Handle(RWCae::MeshScalarProperty)::DownCast(field->GetComponent(c));
                    const char* name = array->GetComponentName(c);
                    component->SetName(name ? name : array->GetNumberOfComponents() == 3 ? (c == 0 ? "X" : c == 1 ? "Y" : "Z") : "");
                    component->Init((int)count);
                    for (vtkIdType n = 0; n < count; ++n) { component->SetValue(RWCae::MeshPropertyEntityInfo((int)n),array->GetComponent(n,c)); }
                }
                field->UpdateComponents(); mesh->ChangeProperties().Append(field);
            }
        };
        fields(data->GetPointData(),true); fields(data->GetCellData(),false);
        for (int a = 0; a < data->GetFieldData()->GetNumberOfArrays(); ++a) {
            const auto array = data->GetFieldData()->GetAbstractArray(a);
            if (array->GetName() && std::string(array->GetName()) == "TimeValue") { continue; }
            myReport.warnings.push_back(std::string("Dataset FieldData '")
                + (array->GetName() ? array->GetName() : "unnamed") + "' is not exposed as a nodal/elemental field");
        }
        myReport.points = mesh->NbNodes(); myReport.cells = mesh->NbIndexedElems();
        myReport.fields = mesh->Properties().Size(); myMesh = mesh;
        return true;
    } catch (const Standard_Failure& error) { myReport.errors.push_back(error.GetMessageString()); }
    catch (const std::exception& error) { myReport.errors.push_back(error.what()); }
    return false;
}
}
