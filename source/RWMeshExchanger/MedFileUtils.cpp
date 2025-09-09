#include "MedFileUtils.h"

#include <Standard_WarningsDisable.hxx> // NOLINT
#include <MED_Factory.hxx>
#include <MED_Wrapper.hxx>
#include <Standard_WarningsRestore.hxx> // NOLINT

// ==================================================================================================
/* !
 * \brief Remove data of 3D cells from a given mesh
 */
// ==================================================================================================

bool Erase3DCells(const MED::PWrapper& med, const MED::PMeshInfo& meshInfo)
{
    const int requiredDim = 3;

    bool             done       = false;
    MED::TEntityInfo entityInfo = med->GetEntityInfo(meshInfo, MED::eNOD);
    for (auto& type2size : entityInfo) {
        MED::TGeom2Size& geom2Size = type2size.second;
        for (auto& geom2nb : geom2Size) {
            int dim = MED::GetDimGaussCoord(geom2nb.first);
            if (dim != requiredDim || geom2nb.second == 0) {
                continue;
            }
            MED::PCellInfo cellInfo =
                med->CrCellInfo(meshInfo, type2size.first, geom2nb.first, 0, MED::eNOD, MED::eFAUX, MED::eFAUX);
            med->SetCellInfo(cellInfo);
            done = true;
        }
    }
    return done;
}

// ==================================================================================================
/* !
 * \brief Remove data of 3D cells from a med file
 * \param [in] medFile - the med file
 * \return bool - true if the file has changed
 */
// ==================================================================================================

bool MedFileUtils::Remove3dMesh(const std::string& medFile)
{
    const int requiredDim = 3;
    bool      fileChange  = false;

    MED::PWrapper med      = MED::CrWrapperW(medFile);
    MED::TInt     nbMeshes = med->GetNbMeshes();
    for (MED::TInt iMesh = 0; iMesh < nbMeshes; ++iMesh) {
        MED::PMeshInfo meshInfo = med->GetPMeshInfo(iMesh + 1);
        if (meshInfo->GetDim() == requiredDim) {
            fileChange |= Erase3DCells(med, meshInfo);
        }
    }
    return fileChange;
}

// ==================================================================================================
/* !
 * \brief Move faces (and their groups) from one file to another
 * \param [in] srcMed - source med file
 * \param [in] srcMeshInfo - source mesh
 * \param [out] tgtMed - target med file
 * \return bool - is OK status
 */
// ==================================================================================================

bool Copy2dMesh(const MED::PWrapper& srcMed, const MED::PMeshInfo& srcMeshInfo, const MED::PWrapper& tgtMed)
{
    bool             ok         = false;
    MED::TEntityInfo entityInfo = srcMed->GetEntityInfo(srcMeshInfo, MED::eNOD);
    for (auto& type2size : entityInfo) {
        MED::TGeom2Size& geom2Size = type2size.second;
        for (auto& geom2nb : geom2Size) {
            int       dim         = MED::GetDimGaussCoord(geom2nb.first);
            const int requiredDim = 2;
            if (dim != requiredDim || geom2nb.second == 0) {
                continue;
            }
            MED::PCellInfo faceInfo = srcMed->GetPCellInfo(srcMeshInfo, type2size.first, geom2nb.first);
            tgtMed->SetCellInfo(faceInfo);
            ok = true;
        }
    }
    return ok;
}

// ==================================================================================================
/* !
 * \brief Move faces (and their groups) from one file to another
 * \param [in] fromFile - source file
 * \param [out] toFile - target file
 * \return bool - is OK status
 */
// ==================================================================================================

bool MedFileUtils::Move2dMesh(const std::string& fromFile, const std::string& toFile)
{
    bool          ok       = false;
    MED::PWrapper srcMed   = MED::CrWrapperW(fromFile);
    MED::PWrapper tgtMed   = MED::CrWrapperW(toFile);
    MED::TInt     nbMeshes = srcMed->GetNbMeshes();
    for (MED::TInt iMesh = 0; iMesh < nbMeshes; ++iMesh) {
        MED::PMeshInfo srcMeshInfo = srcMed->GetPMeshInfo(iMesh + 1);
        if (Copy2dMesh(srcMed, srcMeshInfo, tgtMed)) {
            ok = true;
        }
    }
    return ok;
}

// ==================================================================================================
/*!
 * \brief Return mesh size
 *  \param [in] medFile - MED file
 *  \return MeshInfo - info on nb mesh elements
 */
// ==================================================================================================

MedFileUtils::MeshInfo MedFileUtils::GetMeshInfo(const std::string& medFile)
{
    MeshInfo       result;
    MED::PWrapper  med      = MED::CrWrapperW(medFile);
    MED::PMeshInfo meshInfo = med->GetPMeshInfo(1);

    result.nbNodes = med->GetNbNodes(*meshInfo);

    MED::TEntityInfo entityInfo = med->GetEntityInfo(meshInfo, MED::eNOD);
    for (const auto& [entity, geom2size] : entityInfo) {
        if (entity == MED::eNOEUD || entity == MED::eSTRUCT_ELEMENT) {
            continue;
        }
        for (const auto& [geom, size] : geom2size) {
            switch (geom) {
                case MED::eSEG2:
                case MED::eSEG3: result.nbEdges += size; break;
                case MED::eTRIA3:
                case MED::eTRIA6:
                case MED::eTRIA7:
                case MED::eQUAD4:
                case MED::eQUAD8:
                case MED::eQUAD9:
                case MED::ePOLYGONE:
                case MED::ePOLYGON2: result.nbFaces += size; break;
                case MED::eTETRA4:
                case MED::eTETRA10:
                case MED::ePYRA5:
                case MED::ePYRA13:
                case MED::ePENTA6:
                case MED::ePENTA15:
                case MED::ePENTA18:
                case MED::eHEXA8:
                case MED::eHEXA20:
                case MED::eHEXA27:
                case MED::eOCTA12:
                case MED::ePOLYEDRE: result.nbVolumes += size; break;
                case MED::ePOINT1: result.nb0DElements += size; break;
                default:;
            }
        }
    }
    return result;
}

