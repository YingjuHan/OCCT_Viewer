#ifndef __MESHEXCHANGER_H__
#define __MESHEXCHANGER_H__

#include <RWCae/MeshData.h>

#include <Message_ProgressIndicator.hxx>

#include <string>

#include <Standard_WarningsDisable.hxx> // NOLINT
#include <SMESHDS_Mesh.hxx>
#include <Standard_WarningsRestore.hxx> // NOLINT

class MeshExchanger {
public:
    MeshExchanger(const Handle(Message_ProgressIndicator)& pi = nullptr) : m_progressIndicator(pi) {}

    bool ReadMesh(const std::string& file, SMDS_Mesh& mesh);
    bool WriteMesh(const std::string& file,
                   const SMDS_Mesh*   mesh,
                   const std::string& groupsFile       = "",
                   const bool         isUseCompression = false) const;

    bool ReadMeshWithGroups(const std::string& file, SMESHDS_Mesh& mesh);
    bool WriteMeshWithGroups(const std::string&  file,
                             const SMESHDS_Mesh* mesh,
                             const std::string&  groupsFile       = "",
                             const bool          isUseCompression = false) const;

    bool ReadMeshData(const std::string& file, const Handle(RWCae::MeshData)& meshData);
    bool WriteMeshData(const std::string&               file,
                       const Handle(RWCae::MeshData)& meshData,
                       int                              options,
                       const bool                       isUseCompression = false) const;

    bool WriteMeshGroups(const std::string& file, const SMESHDS_Mesh* mesh) const;

private:
    bool ReadMed(const std::string& file, SMDS_Mesh& mesh);
    bool WriteMed(const std::string& file,
                  const SMDS_Mesh*   mesh,
                  const std::string& groupsFile       = "",
                  const bool         isUseCompression = false) const;

    bool ReadMsh2(const std::string& file, SMDS_Mesh& mesh);
    bool WriteMsh2(const std::string& file,
                   const SMDS_Mesh*   mesh,
                   const std::string& groupsFile       = "",
                   const bool         isUseCompression = false) const;

    bool WriteMdb(const std::string& file,
                  const SMDS_Mesh*   mesh,
                  const std::string& groupsFile       = "",
                  const bool         isUseCompression = false) const;

    bool ReadStl(const std::string& file, SMDS_Mesh& mesh);
    bool ReadCgns(const std::string& file, SMDS_Mesh& mesh);
    bool ReadBdf(const std::string& file, SMDS_Mesh& mesh);
    bool ReadInp(const std::string& file, SMDS_Mesh& mesh);

    Handle(Message_ProgressIndicator) m_progressIndicator;
};

#endif