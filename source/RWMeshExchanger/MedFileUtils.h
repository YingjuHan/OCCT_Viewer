#ifndef __MED_FILE_UTILS_H__
#define __MED_FILE_UTILS_H__

#include <string>

/* !
 * \brief Modify contents of MED file directly, without loading mesh in memory
 */
class MedFileUtils {
public:
    static bool Remove3dMesh(const std::string& medFile);

    static bool Move2dMesh(const std::string& fromFile, const std::string& toFile);

    struct MeshInfo {
        long nbNodes;
        long nb0DElements;
        long nbEdges;
        long nbFaces;
        long nbVolumes;
        MeshInfo() : nbNodes(0), nb0DElements(0), nbEdges(0), nbFaces(0), nbVolumes(0) {}
    };
    static MeshInfo GetMeshInfo(const std::string& medFile);
};

#endif