#include <LodCae/Simplifier.h>

#include <RWCae/MeshAdaptor.h>

#include <SimMesh_Model.h>
#include <SimMesh_Simplifier.h>

#include <Message.hxx>
#include <OSD_Parallel.hxx>

#include <deque>
#include <functional>
#include <map>

namespace LodCae {

// ================================================================
// Function : AddMesh
// ================================================================
void Simplifier::AddMesh(const Handle(RWCae::MeshData)& theMesh)
{
    m_srcMesh = theMesh;
}

// ================================================================
// Function : Perform
// ================================================================
bool Simplifier::Perform(const SimplParameters& theParam)
{
    m_dstMesh.Nullify();

    bool isSurface = isToUseSurface();
    if (!prepareModels(isSurface)) {
        return false;
    }

    simplifyModels(theParam);

    return generateMeshData(isSurface);
}

//! Counts elements of type \p MeshElement in the mesh having global index in the given range.
//! \param[in] theMesh The input mesh.
//! \param[in] theRange The range of global indices to consider.
//! \param[in] theIsSurface If true consider surface elements of 3D elements, otherwise 2d elements.
template<typename MeshElement>
int countElemsInRange(const Handle(RWCae::MeshData)& theMesh,
                      const RWCae::MeshDataRange&    theRange,
                      const bool                       theIsSurface)
{
    int aCount = 0;
    if (theIsSurface) {
        RWCae::MeshAdaptor<MeshElement> aMeshAdap(*theMesh, theIsSurface);
        RWCae::MeshEntityId             aNbElems = aMeshAdap.NbElements();
        for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; anElemIter++) {
            const MeshElement& anElem = aMeshAdap.GetElement(anElemIter);
            if (anElem.GlobalIndex() >= theRange.ElemLower && anElem.GlobalIndex() <= theRange.ElemUpper) {
                ++aCount;
            }
        }
    } else {
        for (int anIndex = theRange.ElemLower; anIndex <= theRange.ElemUpper; ++anIndex) {
            if (theMesh->IndexedElements()[anIndex].Type == MeshElement::Type) {
                ++aCount;
            }
        }
    }
    return aCount;
}

//! Count 2D elements (separately triangles and other elements) of the mesh belonging to the given domain.
//! \param[in] theMesh The input mesh.
//! \param[in] theDomain The domain providing range of global indices to consider.
//! \param[in] theIsSurface If true consider surface elements of 3D elements, otherwise 2d elements.
//! \param[out] theNbTris The output number of triangles.
//! \param[out] theNbOther The output number of quads and polygons.
inline void countElems(const Handle(RWCae::MeshData)& theMesh,
                       RWCae::MeshDataBlock*          theDomain,
                       const bool                       theIsSurface,
                       int&                             theNbTris,
                       int&                             theNbOther)
{
    theNbTris  = countElemsInRange<RWCae::MeshElementTriangle>(theMesh, theDomain->Range(), theIsSurface);
    theNbOther = countElemsInRange<RWCae::MeshElementQuad>(theMesh, theDomain->Range(), theIsSurface)
               + countElemsInRange<RWCae::MeshElementPolygon>(theMesh, theDomain->Range(), theIsSurface);
}

// ================================================================
// Function : isToUseSurface
// ================================================================
bool Simplifier::isToUseSurface() const
{
    if (m_elemType != SourceElemType::PreferSurface) {
        return m_elemType == SourceElemType::ElemSurface;
    }
    if (m_srcMesh->NbSurfElems() > 0) {
        return true;
    }
    if (m_srcMesh->NbElems2d() > 0) {
        return false;
    }
    return true;
}

//! Passes down the tree of domains and gather the leaf domains. The output container
//! is ordered according to the lower global element index.
std::vector<Simplifier::DomainData> getLeafDomains(const Handle(RWCae::MeshDataBlock)& theRoot)
{
    std::vector<Simplifier::DomainData> aResult;
    for (std::deque<RWCae::MeshDataBlock*> aQueue{theRoot.get()}; !aQueue.empty(); aQueue.pop_front()) {
        RWCae::MeshDataBlock* aBlock = aQueue.front();
        if (aBlock->HasChildren()) {
            for (const Handle(RWCae::MeshDataBlock)& aChild : aBlock->Children()) {
                aQueue.push_back(aChild.get());
            }
        } else {
            aResult.push_back(Simplifier::DomainData{aBlock, aBlock->Path()});
        }
    }

    std::sort(aResult.begin(),
              aResult.end(),
              [](const Simplifier::DomainData& theDom1, const Simplifier::DomainData& theDom2) -> bool {
                  return theDom1.Block->Range().ElemLower < theDom2.Block->Range().ElemLower;
              });
    return aResult;
}

//! Removes empty leaf domains from the given tree of domains.
void removeEmptyLeafDomains(const Handle(RWCae::MeshDataBlock)& theRoot)
{
    for (std::deque<RWCae::MeshDataBlock*> aQueue{theRoot.get()}; !aQueue.empty(); aQueue.pop_front()) {
        RWCae::MeshDataBlock* aBlock = aQueue.front();
        if (aBlock->HasChildren()) {
            for (const Handle(RWCae::MeshDataBlock)& aChild : aBlock->Children()) {
                aQueue.push_back(aChild.get());
            }
        } else {
            if (aBlock->Range().IsEmpty() && aBlock->HasParent()) {
                aQueue.push_back(aBlock->Parent()); // Parent may become a leaf.
                std::vector<Handle(RWCae::MeshDataBlock)>& aBlocks = aBlock->Parent()->ChangeChildren();
                aBlocks.erase(std::remove(aBlocks.begin(), aBlocks.end(), aBlock));
            }
        }
    }
}

// ================================================================
// Function : prepareModels
// ================================================================
bool Simplifier::prepareModels(const bool theIsSurface)
{
    if (m_srcMesh.IsNull() || !m_srcMesh->HasGeometry()) {
        Message::SendFail() << "Error: no geometry in mesh data";
        return false;
    }

    m_leafDomains = getLeafDomains(m_srcMesh->Blocks());
    m_models.resize(m_leafDomains.size());

    for (size_t aDomIter = 0; aDomIter < m_leafDomains.size(); ++aDomIter) {
        RWCae::MeshDataBlock* aDomain = m_leafDomains[aDomIter].Block;
        m_models[aDomIter].Model        = createModel(aDomain, theIsSurface);
        if (m_models[aDomIter].Model.IsNull()) {
            continue;
        }
    }
    return true;
}

//! Adds mesh vertices belonging to the given domain into the given simplification model.
static void transferNodes(const Handle(RWCae::MeshData)& theMesh,
                          RWCae::MeshDataBlock*          theDomain,
                          const Handle(SimMesh_Model)&     theModel)
{
    RWCae::MeshDataRange& aRange = theDomain->Range();
    for (int aNodeIter = aRange.NodeLower; aNodeIter <= aRange.NodeUpper; aNodeIter++) {
        const Graphic3d_Vec3& aNode   = theMesh->Nodes()[aNodeIter];
        SimMesh_Id            aVertId = theModel->AddVertex(SimMesh_Vertex(aNode));
        if (theModel->IsVertexNormals()) {
            theModel->ChangeVertexAttrib(aVertId).ChangeNormal() = theMesh->Normals()[aNodeIter];
        }
        if (theModel->IsVertexTextureCoords()) {
            theModel->ChangeVertexAttrib(aVertId).ChangeTexCoord() = theMesh->NodesUV()[aNodeIter];
        }
        if (theModel->IsVertexColors()) {
            theModel->ChangeVertexAttrib(aVertId).ChangeColor() = theMesh->NodalColors()[aNodeIter];
        }
    }
}

//! Adds mesh triangles belonging to the given domain into the given simplification model.
//! \param[in] theIsSurface If true consider surface elements of 3D elements, otherwise 2d elements.
inline void transferElems(const Handle(RWCae::MeshData)& theMesh,
                          RWCae::MeshDataBlock*          theDomain,
                          const bool                       theIsSurface,
                          const Handle(SimMesh_Model)&     theModel)
{
    RWCae::MeshDataRange&                                  aRange = theDomain->Range();
    RWCae::MeshAdaptorTriangle                             aMeshAdap(*theMesh, theIsSurface);
    std::function<void(const RWCae::MeshElementTriangle&)> addTriangle =
        [&theModel, &aRange](const RWCae::MeshElementTriangle& theElem) {
            theModel->AddTriangle(SimMesh_Triangle(theElem[0] - aRange.NodeLower,
                                                   theElem[1] - aRange.NodeLower,
                                                   theElem[2] - aRange.NodeLower));
        };
    if (theIsSurface) {
        RWCae::MeshEntityId aNbElems = aMeshAdap.NbElements();
        for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; ++anElemIter) {
            const RWCae::MeshElementTriangle& anElem = aMeshAdap.GetElement(anElemIter);
            if (anElem.GlobalIndex() >= aRange.ElemLower && anElem.GlobalIndex() <= aRange.ElemUpper) {
                addTriangle(anElem);
            }
        }
    } else {
        for (int anIndex = aRange.ElemLower; anIndex <= aRange.ElemUpper; ++anIndex) {
            if (theMesh->IndexedElements()[anIndex].Type == RWCae::MeshElemType::Triangle) {
                const RWCae::MeshElementTriangle& anElem = aMeshAdap.GetElement(
                    theMesh->IndexedElements()[anIndex].Index);
                addTriangle(anElem);
            }
        }
    }
}

// ================================================================
// Function : createModel
// ================================================================
Handle(SimMesh_Model) Simplifier::createModel(RWCae::MeshDataBlock* theDomain, const bool theIsSurface)
{
    // Count nodes and elements.
    const int aNbNodes = theDomain->Range().NbNodes();
    int       aNbTris  = 0;
    int       aNbOther = 0;
    countElems(m_srcMesh, theDomain, theIsSurface, aNbTris, aNbOther);
    if (aNbNodes < 3 || aNbTris < 1) {
        Message::SendWarning() << "Warning: no triangles to simplify in domain '" << theDomain->Path() << "'";
        return nullptr;
    } else if (aNbOther > 0) {
        Message::SendWarning() << "Warning: simplifier ignores non-triangle 2D elements in domain '"
                               << theDomain->Path() << "'";
    }

    // Create new simplification model.
    Handle(SimMesh_Model) aModel = new SimMesh_Model(aNbNodes, aNbTris, 0);
    aModel->SetVertexNormals(m_srcMesh->HasNormals());
    aModel->SetVertexTextureCoords(m_srcMesh->HasUV());
    aModel->SetVertexColors(m_srcMesh->HasNodalColors());

    // Add vertices
    transferNodes(m_srcMesh, theDomain, aModel);

    // Add triangles
    transferElems(m_srcMesh, theDomain, theIsSurface, aModel);
    return aModel;
}

//! The functor for parallelization of simplification operations.
//! Each simplification model (corresponding to some domain) is treated as a separate independent task.
struct SimplificationFunctor {
public:
    SimplificationFunctor(const SimplParameters& theParam, std::vector<Simplifier::ModelData>& theModels)
        : m_param(theParam),
          m_models(theModels)
    {
    }

    void operator()(int theIndex) const
    {
        const Handle(SimMesh_Model)& aModel = m_models[theIndex].Model;
        if (aModel.IsNull()) {
            return;
        }
        // Perform simplification of the model at the given index.
        SimMesh_Simplifier aTool;
        aTool.Initialize(aModel);
        aTool.SetAggregationMode(m_param.Aggregation);
        aTool.SetAllowInversion(m_param.AllowInversion);
        aTool.SetTargetRatio(m_param.TargetRatio);
        aTool.SetAllowedError(m_param.AllowedError);

        if (aTool.Perform()) {
            m_models[theIndex].MaxError = aTool.GetMaxError();
            m_models[theIndex].NbInversions += aTool.NbInversions();
        }

        // Clean up the simplified model.
        aModel->CompactVertices();
        aModel->SplitVertices();
    }

    SimplificationFunctor(const SimplificationFunctor&) = delete;
    SimplificationFunctor& operator=(const SimplificationFunctor&) = delete;

private:
    const SimplParameters&              m_param;
    std::vector<Simplifier::ModelData>& m_models;
};

// ================================================================
// Function : simplifyModels
// ================================================================
void Simplifier::simplifyModels(const SimplParameters& theParam)
{
    SimplificationFunctor aFunctor(theParam, m_models);
    OSD_Parallel::For(0, static_cast<int>(m_models.size()), aFunctor);
}

//! Adds nodes from the simplified model to the destination mesh data.
//! \param[in,out] theMesh The destination mesh data.
//! \param[in] theModel The simplified model.
//! \param[out] theRange A mesh data range to store the range of nodes' indices.
void addDestinationNodes(const Handle(RWCae::MeshData)& theMesh,
                         const Handle(SimMesh_Model)&     theModel,
                         RWCae::MeshDataRange&          theRange)
{
    theRange.NodeLower = theMesh->NbNodes();
    for (SimMesh_Id aVertIter = 0; aVertIter < theModel->GetNbVertices(); ++aVertIter) {
        const SimMesh_Vertex& aVert = theModel->GetVertex(aVertIter);
        theMesh->AddNodePosition(aVert);
        if (theModel->IsVertexNormals()) {
            const SimMesh_Vec3f& aVertNorm = theModel->GetVertexAttrib(aVertIter).GetNormal();
            theMesh->AddNodeNormal(aVertNorm);
        }
        if (theModel->IsVertexTextureCoords()) {
            const SimMesh_Vec2f& aTexCoord = theModel->GetVertexAttrib(aVertIter).GetTexCoord();
            theMesh->AddNodeUV(aTexCoord);
        }
        if (theModel->IsVertexColors()) {
            const SimMesh_Color& aColor = theModel->GetVertexAttrib(aVertIter).GetColor();
            theMesh->AddNodeColor(aColor);
        }
    }
    theRange.NodeUpper = theMesh->NbNodes() - 1;
}

//! Adds triangles from the simplified model to the destination mesh data.
//! \param[in,out] theMesh The destination mesh data.
//! \param[in] theModel The simplified model.
//! \param[in] theIsSurface Defines whether to add surface elements or 2d elements.
//! \param[in,out] theRange A mesh data range to store the range of elements' indices.
void addDestinationTriangles(const Handle(RWCae::MeshData)& theMesh,
                             const Handle(SimMesh_Model)&     theModel,
                             bool                             theIsSurface,
                             RWCae::MeshDataRange&          theRange)
{
    theRange.ElemLower = theIsSurface ? theMesh->NbSurfElems() : theMesh->NbIndexedElems();
    // Add triangles from the simplified model to the destination mesh data.
    for (SimMesh_Id aTriIter = 0; aTriIter < theModel->GetNbTriangles(); ++aTriIter) {
        const SimMesh_Triangle& aTri = theModel->GetTriangle(aTriIter);
        if (theModel->IsTriangleValid(aTriIter)) {
            Standard_ASSERT_VOID(aTri[0] < theModel->GetNbVertices() && aTri[1] < theModel->GetNbVertices()
                                     && aTri[2] < theModel->GetNbVertices(),
                                 "Invalid triangle node index");

            NCollection_Vec3<RWCae::MeshEntityId> anElemNodes(RWCae::MeshEntityId(aTri[0] + theRange.NodeLower),
                                                                RWCae::MeshEntityId(aTri[1] + theRange.NodeLower),
                                                                RWCae::MeshEntityId(aTri[2] + theRange.NodeLower));

            RWCae::MeshElementTriangle anElem(anElemNodes);
            if (theIsSurface) {
                anElem.SetGlobalIndex(theRange.ElemLower + (int)aTriIter);
                theMesh->AddSurfaceElement(anElem);
            } else {
                theMesh->AddElement(anElem);
            }
        }
    }
    theRange.ElemUpper = (theIsSurface ? theMesh->NbSurfElems() : theMesh->NbIndexedElems()) - 1;
}

//! Restores ordering of domains in \p theNewList according to \p theOrigList
//! matching domains using their path name.
void restoreOriginalOrder(const std::vector<Simplifier::DomainData>& theOrigList,
                          std::vector<Simplifier::DomainData>&       theNewList)
{
    Standard_ASSERT_VOID(theOrigList.size() == theNewList.size(), "");
    std::map<TCollection_AsciiString, size_t> aMapNameIndex;
    for (size_t i = 0; i < theNewList.size(); ++i) {
        aMapNameIndex[theNewList[i].Path] = i;
    }
    std::vector<Simplifier::DomainData> aSortedList(theNewList.size());
    for (size_t i = 0; i < theOrigList.size(); ++i) {
        aSortedList[i] = theNewList[aMapNameIndex[theOrigList[i].Path]];
    }
    theNewList = std::move(aSortedList);
}

// ================================================================
// Function : generateMeshData
// ================================================================
bool Simplifier::generateMeshData(const bool theIsSurface)
{
    m_dstMesh = new RWCae::MeshData(m_id);
    m_dstMesh->SetName(m_srcMesh->Name());

    Handle(RWCae::MeshDataBlock) aNewRoot = new RWCae::MeshDataBlock("");
    aNewRoot->FromJson(m_srcMesh->Blocks()->ToJson());
    std::vector<DomainData> aNewLeafDomains = getLeafDomains(aNewRoot);
    restoreOriginalOrder(m_leafDomains, aNewLeafDomains);

    m_maxError     = 0.;
    m_nbInversions = 0;
    for (size_t aModelIter = 0; aModelIter < m_models.size(); ++aModelIter) {
        const Handle(SimMesh_Model)& aModel = m_models[aModelIter].Model;
        if (aModel.IsNull()) {
            continue;
        }
        RWCae::MeshDataRange& aRange = aNewLeafDomains[aModelIter].Block->Range();
        addDestinationNodes(m_dstMesh, aModel, aRange);
        addDestinationTriangles(m_dstMesh, aModel, theIsSurface, aRange);
        m_maxError = std::max(m_maxError, m_models[aModelIter].MaxError);
        m_nbInversions += m_models[aModelIter].NbInversions;
    }

    removeEmptyLeafDomains(aNewRoot);
    m_dstMesh->SetBlocks(aNewRoot);
    return true;
}

} // namespace