#include "MeshExchanger.h"

#include "MeshExchangerUtils.h"
#include "Smds2MeshDataConvert.h"

#include <DriverBDF/DriverBDF_Read.h>
#include <DriverINP/DriverINP_Read.h>

#include <SMESHDS_Mesh.hxx>
#include <SMESH_TypeDefs.hxx>
#include <SMESHDS_Group.hxx>

#include <RWCae/MeshBinWriter.h>
#include <RWCae/MeshBinReader.h>
#include <RWMeshConv/GmshLoader.h>

#include <DriverMED_R_SMESHDS_Mesh.h>
#include <DriverMED_W_SMESHDS_Mesh.h>
#include <DriverSTL_R_SMDS_Mesh.h>
#include <DriverCGNS_Read.hxx>

#include <string>
#include <cstdio>
#include <filesystem>

#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>

const int MAX_MED_GROUP_NAME_LENGTH = 80;

namespace {
const size_t EXT_LEN = 3;

bool IsMedFileName(const std::string& file)
{
    size_t i = file.size() - EXT_LEN;
    return (strncasecmp(file.data() + i, "med", EXT_LEN) == 0);
}

bool IsStlFileName(const std::string& file)
{
    size_t i = file.size() - EXT_LEN;
    return (strncasecmp(file.data() + i, "stl", EXT_LEN) == 0);
}

bool IsMshFileName(const std::string& file)
{
    size_t i = file.size() - EXT_LEN;
    return (strncasecmp(file.data() + i, "msh", EXT_LEN) == 0);
}

bool IsMdbFileName(const std::string& file)
{
    size_t i = file.size() - EXT_LEN;
    return (strncasecmp(file.data() + i, "mdb", EXT_LEN) == 0);
}

bool IsCgnsFileName(const std::string& file)
{
    size_t i = file.size() - EXT_LEN - 1;
    return (strncasecmp(file.data() + i, "cgns", EXT_LEN + 1) == 0);
}

bool IsBdfFileName(const std::string& file)
{
    size_t i = file.size() - EXT_LEN;
    return (strncasecmp(file.data() + i, "bdf", EXT_LEN) == 0) || (strncasecmp(file.data() + i, "blk", EXT_LEN) == 0)
        || (strncasecmp(file.data() + i, "dat", EXT_LEN) == 0);
}

bool IsInpFileName(const std::string& file)
{
    size_t i = file.size() - EXT_LEN;
    return (strncasecmp(file.data() + i, "inp", EXT_LEN) == 0);
}

void WriteGroup(const SMESHDS_GroupBase* group, rapidjson::PrettyWriter<rapidjson::OStreamWrapper>& writer)
{
    writer.StartObject();

    std::string groupName = group->GetStoreName();
    MeshExchangerUtils::TrimString(groupName);
    writer.Key("id");
    writer.String(groupName.c_str());
    writer.Key("name");
    writer.String(groupName.c_str());

    SMDSAbs_ElementType groupType = group->GetType();
    writer.Key("type");
    writer.String(groupType == SMDSAbs_Node ? "NODE" : "ELEMENT");

    std::string elemType;
    switch (groupType) {
        case SMDSAbs_Node: elemType = "NODE"; break;
        case SMDSAbs_Edge: elemType = "EDGE"; break;
        case SMDSAbs_Face: elemType = "FACE"; break;
        case SMDSAbs_Volume: elemType = "VOLUME"; break;
        case SMDSAbs_0DElement: elemType = "0D"; break;
        default: break;
    }
    writer.Key("element-type");
    writer.String(elemType.c_str());
    writer.Key("ids");
    writer.StartArray();
    for (SMDS_ElemIteratorPtr elemIt = group->GetElements(); elemIt->more();) {
        const SMDS_MeshElement* elem = elemIt->next();
        writer.Int(elem->GetID());
    }
    writer.EndArray();
    writer.EndObject();
}
}

bool MeshExchanger::ReadMesh(const std::string& file, SMDS_Mesh& mesh)
{
    if (IsMedFileName(file)) {
        return ReadMed(file, mesh);
    } else if (IsStlFileName(file)) {
        return ReadStl(file, mesh);
    } else if (IsMshFileName(file)) {
        return ReadMsh2(file, mesh);
    } else if (IsCgnsFileName(file)) {
        return ReadCgns(file, mesh);
    } else if (IsBdfFileName(file)) {
        return ReadBdf(file, mesh);
    } else if (IsInpFileName(file)) {
        return ReadInp(file, mesh);
    } else {
        std::cerr << "File format is not supported \"" << file.c_str() << "\"!" << std::endl;
    }
    return false;
}

bool MeshExchanger::WriteMesh(const std::string& file,
                              const SMDS_Mesh*   mesh,
                              const std::string& groupsFile,
                              const bool         isUseCompression) const
{
    if (IsMedFileName(file)) {
        return WriteMed(file, mesh, groupsFile, isUseCompression);
    } else if (IsMshFileName(file)) {
        return WriteMsh2(file, mesh, groupsFile, isUseCompression);
    } else if (IsMdbFileName(file)) {
        return WriteMdb(file, mesh, groupsFile, isUseCompression);
    } else {
        std::cerr << "File format is not supported \"" << file.c_str() << "\"!" << std::endl;
    }
    return false;
}

bool MeshExchanger::ReadMeshWithGroups(const std::string& file, SMESHDS_Mesh& mesh)
{
    return ReadMesh(file, mesh);
}

bool MeshExchanger::WriteMeshWithGroups(const std::string&  file,
                                        const SMESHDS_Mesh* mesh,
                                        const std::string&  groupsFile,
                                        const bool          isUseCompression) const
{
    return WriteMesh(file, mesh, groupsFile, isUseCompression);
}

bool MeshExchanger::ReadMeshData(const std::string& file, const Handle(RWCae::MeshData)& meshData)
{
    if (IsMshFileName(file)) {
        RWMeshConv::GmshLoader loader(meshData);
        Message_ProgressRange    pr;
        return loader.Perform(file.c_str(), pr);
    } else if (IsMdbFileName(file)) {
        RWCae::MeshBinReader loader(meshData);
        return loader.Perform(file.c_str());
    }

    SMESHDS_Mesh smdsMesh(0, true);
    if (!ReadMesh(file, smdsMesh)) {
        return false;
    }
    Smds2MeshDataConvert::Perform(smdsMesh, meshData);
    return true;
}

bool MeshExchanger::WriteMeshData(const std::string&               file,
                                  const Handle(RWCae::MeshData)& meshData,
                                  int                              options,
                                  const bool                       isUseCompression) const
{
    RWCae::MeshBinWriter writer(meshData);
    writer.SetOptions(options);
    writer.SetToForceSinglePrecision(true);
#ifdef OCCTV_HAVE_DRACO
    if (isUseCompression) {
        RWCae::MeshDracoCompressionInfo compression(20, 20);
        writer.SetUseDracoCompression(compression);
    }
#else
    (void)isUseCompression;
#endif
    bool ok = writer.Perform(file.c_str(), Message_ProgressIndicator::Start(m_progressIndicator));
    return ok;
}

bool MeshExchanger::WriteMeshGroups(const std::string& file, const SMESHDS_Mesh* mesh) const
{
    const std::set<SMESHDS_GroupBase*>& groups = mesh->GetGroups();
    if (groups.empty()) {
        return false;
    }

    std::ofstream stream(file.c_str(), ios::out);
    if (!stream.is_open()) {
        std::cout << "Invalid path to the file: " << file << std::endl;
        return false;
    }

    rapidjson::OStreamWrapper                          streamWrapper(stream);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(streamWrapper);
    writer.SetFormatOptions(rapidjson::PrettyFormatOptions::kFormatSingleLineArray);

    writer.StartObject();
    writer.Key("groups");
    writer.StartArray();
    for (auto& group : groups) {
        WriteGroup(group, writer);
    }
    writer.EndArray();
    writer.EndObject();

    stream.flush();
    stream.close();

    return true;
}

bool MeshExchanger::ReadMed(const std::string& file, SMDS_Mesh& mesh)
{
    SMESHDS_Mesh* smesh = dynamic_cast<SMESHDS_Mesh*>(&mesh);
    if (!smesh) {
        return false;
    }
    smesh->ClearMesh();
    DriverMED_R_SMESHDS_Mesh reader;
    reader.SetFile(file);
    reader.SetMesh(smesh);
    reader.SetMeshId(-1);
    Driver_Mesh::Status    status;
    std::list<std::string> names = reader.GetMeshNames(status);
    if (names.empty()) {
        return false;
    }
    reader.SetMeshName(names.front());

    status = reader.Perform(Message_ProgressIndicator::Start(m_progressIndicator));
    if (status == Driver_Mesh::DRS_ABORTED) {
        smesh->ClearMesh();
        return false;
    }
    // Reading groups
    std::list<TNameAndType>           aGroupNames = reader.GetGroupNamesAndTypes();
    std::list<TNameAndType>::iterator name_type   = aGroupNames.begin();
    for (; name_type != aGroupNames.end(); ++name_type) {
        SMESHDS_Group* aGroup = new SMESHDS_Group(smesh->GetNbGroups() + 1, smesh, name_type->second);
        smesh->AddGroup(aGroup);
        aGroup->SetStoreName(name_type->first.c_str());
        reader.GetGroup(aGroup);
    }
    return true;
}

bool MeshExchanger::WriteMed(const std::string& file,
                             const SMDS_Mesh*   mesh,
                             const std::string& groupsFile,
                             const bool         isUseCompression) const
{
    (void)groupsFile;
    (void)isUseCompression;

    const SMESHDS_Mesh* smesh = dynamic_cast<const SMESHDS_Mesh*>(mesh);
    if (!smesh) {
        return false;
    }
    DriverMED_W_SMESHDS_Mesh writer;
    writer.SetFile(file);
    writer.SetMesh(const_cast<SMESHDS_Mesh*>(smesh));
    writer.SetMeshName("MED mesh");
    writer.SetMeshId(0);
    writer.SetAutoDimension(true);
    writer.SetSaveNumbers(true);

    const std::set<SMESHDS_GroupBase*>&                  groups = smesh->GetGroups();
    std::map<SMDSAbs_ElementType, std::set<std::string>> aGroupNames;
    const int                                            maxNbIter = 10000; // to guarantee cycle finish
    for (auto grp : groups) {
        SMDSAbs_ElementType aType       = grp->GetType();
        std::string         aGroupName0 = grp->GetStoreName();
        std::string         aGroupName  = aGroupName0;
        aGroupName.resize(MAX_MED_GROUP_NAME_LENGTH);
        for (int i = 1; !aGroupNames[aType].insert(aGroupName).second && i < maxNbIter; i++) {
            aGroupName = "GR_" + std::to_string(i) + "_" + aGroupName0;
            aGroupName.resize(MAX_MED_GROUP_NAME_LENGTH);
        }
        grp->SetStoreName(aGroupName.c_str());
        writer.AddGroup(grp);
    }

    // Remove output file before performing, otherwise new data will be appended if file exists
    using std::filesystem::filesystem_error;
    try {
        std::filesystem::remove(file.c_str());
    } catch (const filesystem_error& e) {
        std::cout << "what():  " << e.what() << '\n'
                  << "path1(): " << e.path1() << '\n'
                  << "path2(): " << e.path2() << '\n'
                  << "code().value():    " << e.code().value() << '\n'
                  << "code().message():  " << e.code().message() << '\n'
                  << "code().category(): " << e.code().category().name() << '\n';
    }

    Driver_Mesh::Status st = writer.Perform(Message_ProgressIndicator::Start(m_progressIndicator));
    return st == Driver_Mesh::DRS_OK;
}

bool MeshExchanger::ReadStl(const std::string& file, SMDS_Mesh& mesh)
{
    SMESHDS_Mesh* smesh = dynamic_cast<SMESHDS_Mesh*>(&mesh);
    if (!smesh) {
        return false;
    }
    smesh->ClearMesh();

    DriverSTL_R_SMDS_Mesh readerStl;
    readerStl.SetMesh(smesh);
    readerStl.SetFile(file);
    readerStl.SetMeshId(-1);
    readerStl.Perform(Message_ProgressIndicator::Start(m_progressIndicator));

    return true;
}

bool MeshExchanger::ReadMsh2(const std::string& file, SMDS_Mesh& mesh)
{
    (void)file;

    mesh.Clear();
    return false;
}

bool MeshExchanger::WriteMsh2(const std::string& file,
                              const SMDS_Mesh*   mesh,
                              const std::string& groupsFile,
                              const bool         isUseCompression) const
{
    (void)file;
    (void)mesh;
    (void)groupsFile;
    (void)isUseCompression;
    return false;
}

bool MeshExchanger::WriteMdb(const std::string& file,
                             const SMDS_Mesh*   smdsMesh,
                             const std::string& groupsFile,
                             const bool         isUseCompression) const
{
    Handle(RWCae::MeshData) meshData = new RWCae::MeshData();
    Smds2MeshDataConvert::Perform(*smdsMesh, meshData, groupsFile);
    int options = (int)RWCae::MeshDataImportExportOptions::DEFAULTS;
    return WriteMeshData(file, meshData, options, isUseCompression);
}

bool MeshExchanger::ReadCgns(const std::string& file, SMDS_Mesh& mesh)
{
    SMESHDS_Mesh* smesh = dynamic_cast<SMESHDS_Mesh*>(&mesh);
    if (!smesh) {
        return false;
    }
    smesh->ClearMesh();

    try {
        DriverCGNS_Read readerCgns;
        readerCgns.SetMesh(smesh);
        readerCgns.SetFile(file);
        readerCgns.SetMeshId(0);
        int res = readerCgns.Perform(Message_ProgressIndicator::Start(m_progressIndicator));
        if (res != Driver_Mesh::DRS_OK) {
            std::cerr << "Error: some problems at import from '" << file << "'" << std::endl;
            return false;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: '" << ex.what() << "' at import from '" << file << "'" << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Error: Unknown error at CGNS import from '" << file << "'" << std::endl;
        return false;
    }

    return true;
}

bool MeshExchanger::ReadBdf(const std::string& file, SMDS_Mesh& mesh)
{
    SMESHDS_Mesh* smesh = dynamic_cast<SMESHDS_Mesh*>(&mesh);
    if (!smesh) {
        return false;
    }
    smesh->ClearMesh();

    try {
        DriverBDF_Read readerBdf;
        readerBdf.SetMesh(smesh);
        readerBdf.SetFile(file);
        int res = readerBdf.Perform(Message_ProgressIndicator::Start(m_progressIndicator));
        if (res != Driver_Mesh::DRS_OK) {
            std::cerr << "Error: some problems at import from '" << file << "'" << std::endl;
            return false;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: '" << ex.what() << "' at import from '" << file << "'" << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Error: Unknown error at BDF import from '" << file << "'" << std::endl;
        return false;
    }

    return true;
}

bool MeshExchanger::ReadInp(const std::string& file, SMDS_Mesh& mesh)
{
    SMESHDS_Mesh* smesh = dynamic_cast<SMESHDS_Mesh*>(&mesh);
    if (!smesh) {
        return false;
    }
    smesh->ClearMesh();

    try {
        DriverINP_Read readerInp;
        readerInp.SetMesh(smesh);
        readerInp.SetFile(file);
        int res = readerInp.Perform(Message_ProgressIndicator::Start(m_progressIndicator));
        if (res != Driver_Mesh::DRS_OK) {
            std::cerr << "Error: some problems at import from '" << file << "'" << std::endl;
            return false;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: '" << ex.what() << "' at import from '" << file << "'" << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Error: Unknown error at INP import from '" << file << "'" << std::endl;
        return false;
    }

    return true;
}

