#include <ViewCae/CaeMeshArrowsTools.h>

#include <ViewCae/CaeMeshTools.h>

#include <ViewCaeGeom/MeshGeometryGen.h>

#include <RWCae/MeshAdaptorsIterator.h>

#include <Graphic3d_FrameStatsData.hxx>
#include <Graphic3d_TransformUtils.hxx>
#include <math_BullardGenerator.hxx>
#include <Message.hxx>
#include <OpenGl_ArbIns.hxx>
#include <OpenGl_ShaderManager.hxx>
#include <OpenGl_Workspace.hxx>

#include <random>

namespace {
constexpr const char* INSTANCING_SHADER_ID  = "CaeMeshArrowTools_InstancingShader";
constexpr const char* INSTANCE_ATTR_NAME[4] = {"scInstMatRow1", "scInstMatRow2", "scInstMatRow3", "scInstTex"};

constexpr int INSTANCE_ATTR_LOC[4] = {Graphic3d_TOA_CUSTOM,
                                      Graphic3d_TOA_CUSTOM + 1,
                                      Graphic3d_TOA_CUSTOM + 2,
                                      Graphic3d_TOA_CUSTOM + 3};

constexpr int BASE_GEOMETRY_NB_ATTRIBUTES = 1;
constexpr int NB_INSTANCING_ATTRIBUTES    = 4;

//! Convert data type to GL info
static GLenum toGlDataType(const Graphic3d_TypeOfData theType, GLint& theNbComp)
{
    constexpr int aNb1 = 1;
    constexpr int aNb2 = 2;
    constexpr int aNb3 = 3;
    constexpr int aNb4 = 4;
    switch (theType) {
        case Graphic3d_TOD_USHORT: theNbComp = aNb1; return GL_UNSIGNED_SHORT;
        case Graphic3d_TOD_UINT: theNbComp = aNb1; return GL_UNSIGNED_INT;
        case Graphic3d_TOD_VEC2: theNbComp = aNb2; return GL_FLOAT;
        case Graphic3d_TOD_VEC3: theNbComp = aNb3; return GL_FLOAT;
        case Graphic3d_TOD_VEC4: theNbComp = aNb4; return GL_FLOAT;
        case Graphic3d_TOD_VEC4UB: theNbComp = aNb4; return GL_UNSIGNED_BYTE;
        case Graphic3d_TOD_FLOAT: theNbComp = aNb1; return GL_FLOAT;
        default: break;
    }
    theNbComp = 0;
    return GL_NONE;
}

//! UniformRandomBitGenerator wrapper for math_BullardGenerator.
struct RandInt {
    using result_type = unsigned int;

    Standard_NODISCARD static constexpr result_type min() { return 0; }

    Standard_NODISCARD static constexpr result_type max() { return std::numeric_limits<unsigned int>::max(); }

    result_type operator()() { return myRand.NextInt(); }

protected:
    math_BullardGenerator myRand;
};

}

namespace ViewCae {

//! Auxiliary template for VBO with interleaved attributes.
template<int NbAttributes>
class MeshVectorsElement::ArrowVertexBuffer : public OpenGl_VertexBuffer {
public:
    //! Create uninitialized VBO.
    explicit ArrowVertexBuffer(const Graphic3d_Buffer& theAttribs, const int* theAttribIds = nullptr)
        : myStride(theAttribs.Stride)
    {
        for (int i = 0; i < NbAttributes; i++) {
            myAttribs[i]   = theAttribs.AttributesArray()[i];
            myAttribIds[i] = theAttribIds != nullptr ? theAttribIds[i] : myAttribs[i].Id;
        }
    }

    //! Bind all vertex attributes.
    virtual void BindAllAttributes(const Handle(OpenGl_Context)& theGlCtx) const Standard_OVERRIDE
    {
        if (!IsValid()) {
            return;
        }

        Bind(theGlCtx);
        GLint          aNbComp;
        const GLubyte* anOffset = myOffset;
        for (Standard_Integer anAttribIter = 0; anAttribIter < NbAttributes; ++anAttribIter) {
            const Graphic3d_Attribute& anAttrib  = myAttribs[anAttribIter];
            const GLenum               aDataType = toGlDataType(anAttrib.DataType, aNbComp);
            if (aDataType != GL_NONE) {
                theGlCtx->core20fwd->glEnableVertexAttribArray(myAttribIds[anAttribIter]);
                theGlCtx->core20fwd->glVertexAttribPointer(myAttribIds[anAttribIter],
                                                           aNbComp,
                                                           aDataType,
                                                           aDataType != GL_FLOAT,
                                                           myStride,
                                                           anOffset);
            }
            anOffset += Graphic3d_Attribute::Stride(anAttrib.DataType);
        }
    }

    //! Unbind all vertex attributes.
    virtual void UnbindAllAttributes(const Handle(OpenGl_Context)& theGlCtx) const Standard_OVERRIDE
    {
        if (!IsValid()) {
            return;
        }

        Unbind(theGlCtx);

        for (Standard_Integer anAttribIter = 0; anAttribIter < NbAttributes; ++anAttribIter) {
            theGlCtx->core20fwd->glDisableVertexAttribArray(myAttribIds[anAttribIter]);
        }
    }

protected:
    Graphic3d_Attribute myAttribs[NbAttributes];
    int                 myAttribIds[NbAttributes];
    int                 myStride;
};

// ================================================================
// Function : MeshVectorsIterator
// ================================================================
MeshVectorsIterator::MeshVectorsIterator(const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
                                         const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
                                         const ViewCaeGeom::MapOfMeshGeomNVecProps&   theVecProps,
                                         MeshVectorDistrType                           theDistrType,
                                         double                                        theSamplingRatio)
    : myGeom(theMeshGeom),
      myScalarProps(theGeomProps),
      myVectorProps(theVecProps),
      myDistrType(theDistrType),
      myRatio(theSamplingRatio)
{
    myGeom = theMeshGeom->GetLastNotModifierChild();

    init();
}

// ================================================================
// Function : init
// ================================================================
void MeshVectorsIterator::init()
{
if (myGeom.IsNull()) {
        return;
    }

    if (!myVectorProps.Find(myGeom->Id(), myCurVectorProp) || myCurVectorProp.IsNull()
        || !myCurVectorProp->HasScalarComponents() || !myCurVectorProp->IsVector3()) {
        myCurVectorProp.Nullify();
        return;
    }

    myIsNodal = myCurVectorProp->IsNodal();

    RWCae::MeshPropertyComponent aPropComponent;
    myScalarProps.Find(myGeom->Id(), aPropComponent);
    myCurScalarProp = aPropComponent.GetScalarComp();

    myCurMinMax.SetValues(0.0, 1.0);
    if (!myCurScalarProp.IsNull()) {
        myCurScalarProp->ValueRange().GetBounds(myCurMinMax.x(), myCurMinMax.y());
        if (myCurScalarProp->IsNodal() != myIsNodal) {
            myCurScalarProp.Nullify();
        }
    }

    const Handle(RWCae::MeshData)& aMeshData = myGeom->GetMeshData();

    myCurEntities.clear();
    if (!aMeshData.IsNull()) {
        if (myDistrType == MeshVectorDistrType::Volume || myCurVectorProp->IsElemental()) {
            myCurEntities.resize(myIsNodal ? aMeshData->NbNodes() : aMeshData->NbIndexedElems());
            for (int i = 0; i < (int)myCurEntities.size(); i++) {
                myCurEntities[i] = i;
            }
        } else if (myDistrType == MeshVectorDistrType::Surface) {
            NCollection_Map<int> anElemsNodes;

            auto compute = [this, &anElemsNodes](const auto& theAdaptor) {
                getElementsNodes(theAdaptor, &anElemsNodes);
            };

            RWCae::MeshAdaptorsIterator2dElems::For(compute, aMeshData);
            RWCae::MeshAdaptorsIterator2dElems::For(compute, aMeshData, true);

            myCurEntities.resize(anElemsNodes.Extent());
            std::copy(anElemsNodes.cbegin(), anElemsNodes.cend(), myCurEntities.begin());
        }

        // sort nodes in random order and process first 'size * theSamplingRatio' nodes
        if (myRatio > RealSmall() && myRatio < 1.0 - RealSmall()) {
            std::shuffle(myCurEntities.begin(), myCurEntities.end(), RandInt());
        }
    }

    myCurNbGeomEntities = int(myCurEntities.size() * myRatio);
    myCurNbProcessed    = -1;

    if (More()) {
        Next();
    }
}

// ================================================================
// Function : getElementsNodes
// ================================================================
template<class Elem_T>
void MeshVectorsIterator::getElementsNodes(const RWCae::MeshAdaptor<Elem_T>& theAdaptor,
                                           NCollection_Map<int>*               theNodes) const
{
    Graphic3d_Vec3 aNode;

    RWCae::MeshEntityId aNbElems = theAdaptor.NbElements();
    for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; ++anElemIter) {
        const Elem_T& anElem = theAdaptor.GetElement(anElemIter);
        for (RWCae::MeshElemEntityId aNodeIndex = 0; aNodeIndex < anElem.NbNodes(); aNodeIndex++) {
            theNodes->Add(anElem[aNodeIndex]);
        }
    }
}

// ================================================================
// Function : More
// ================================================================
bool MeshVectorsIterator::More() const
{
    return !myGeom.IsNull() && !myCurVectorProp.IsNull();
}

// ================================================================
// Function : Next
// ================================================================
void MeshVectorsIterator::Next()
{
    if (++myCurNbProcessed >= myCurNbGeomEntities) {
        myGeom = myGeom->GetChild();
        init();
        return;
    }

    if (myIsNodal) {
        for (bool isAccepted = false; !isAccepted && myCurNbProcessed < myCurNbGeomEntities;) {
            myCurNode = myGeom->GetMeshData()->Nodes().Value(myCurEntities[myCurNbProcessed]);
            myGeom->ModifyNode(myCurEntities[myCurNbProcessed], myCurNode);
            isAccepted = myGeom->AcceptNode(myCurEntities[myCurNbProcessed], myCurNode);
            if (!isAccepted) {
                myCurNbProcessed++;
            }
        }
    } else {
        for (bool isAccepted = false; !isAccepted && myCurNbProcessed < myCurNbGeomEntities;) {
            isAccepted = myGeom->AcceptElement(myCurEntities[myCurNbProcessed]);
            if (!isAccepted) {
                myCurNbProcessed++;
            }
        }
    }

    if (myCurNbProcessed >= myCurNbGeomEntities) {
        myGeom = myGeom->GetChild();
        init();
        return;
    }

    if (!myIsNodal) {
        getElementCenter(myCurEntities[myCurNbProcessed], myCurNode);
    }

    myCurVectorProp->GetRawVector(RWCae::MeshPropertyEntityInfo(myCurEntities[myCurNbProcessed]), myCurVectorValue);

    myCurScalarValue = !myCurScalarProp.IsNull()
                         ? myCurScalarProp->Value(RWCae::MeshPropertyEntityInfo(myCurEntities[myCurNbProcessed]))
                         : std::numeric_limits<double>::quiet_NaN();
}

// ================================================================
// Function : getElementCenter
// ================================================================
void MeshVectorsIterator::getElementCenter(int theElemIdx, Graphic3d_Vec3& theCenter) const
{
    const Handle(RWCae::MeshData)& aMeshData = myGeom->GetMeshData();

    const RWCae::MeshData::IndexedElement& anElem = aMeshData->IndexedElements()[theElemIdx];

    auto compute = [&anElem, &theCenter](const auto& theAdaptor) {
        theCenter = theAdaptor.ElementCenter(anElem.Index);
        return true;
    };

    RWCae::MeshAdaptorsIteratorAllElems::Switch(anElem.Type, compute, aMeshData);
}

// ================================================================
// Function : IsSameProps
// ================================================================
bool MeshVectorsIterator::IsSameProps() const
{
    if (myCurScalarProp.IsNull()) {
        return true;
    }

    for (int i = 0; i < myCurVectorProp->NbRawComponents(); i++) {
        if (myCurVectorProp->GetRawComponent(i)->Id() == myCurScalarProp->Id()) {
            return true;
        }
    }

    return false;
}

// ================================================================
// Function : ExtractVectorNodes
// ================================================================
Handle(RWCae::MeshData) MeshVectorsPrimitiveArray::ExtractVectorNodes(
    const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
    const RWCae::MeshPropertyComponent&     theScalarProp,
    const Handle(RWCae::MeshNVecProperty)&  theVecProp,
    MeshVectorDistrType                       theDistrType,
    double                                    theSamplingRatio)
{
    Handle(RWCae::MeshData) aRes = new RWCae::MeshData();

    bool isSameProps = !theVecProp.IsNull() && !theScalarProp.Property.IsNull()
                         ? theVecProp->Id() == theScalarProp.Property->Id()
                         : false;

    Handle(ViewCaeGeom::MeshGeometry) aMeshGeom = theMeshGeom->GetLastNotModifierChild();

    ViewCaeGeom::MapOfMeshGeomScalarProps aScalarProps;
    ViewCaeGeom::MapOfMeshGeomNVecProps   aVectorProps;
    for (Handle(ViewCaeGeom::MeshGeometry) aGeom = aMeshGeom; !aGeom.IsNull(); aGeom = aGeom->GetChild()) {
        Handle(RWCae::MeshNVecProperty) aGeomProp = aGeom->CalculateProperty(theVecProp);
        aVectorProps.Bind(aGeom->Id(), aGeomProp);
        if (!isSameProps) {
            aGeomProp = aGeom->CalculateProperty(theScalarProp.Property);
        }
        aScalarProps.Bind(aGeom->Id(),
                          !aGeomProp.IsNull() ? RWCae::MeshPropertyComponent(aGeomProp, theScalarProp.ComponentIndex) : RWCae::MeshPropertyComponent());
    }

    MeshVectorsIterator aVectorIter(aMeshGeom, aScalarProps, aVectorProps, theDistrType, theSamplingRatio);
    if (!aVectorIter.More()) {
        return aRes;
    }

    isSameProps = aVectorIter.IsSameProps();

    Handle(RWCae::MeshNVecProperty) aVecProp = initVectorProperty(theVecProp);
    aRes->ChangeProperties().Append(aVecProp);

    Handle(RWCae::MeshNVecProperty) aScalarProp;
    if (!isSameProps) {
        aScalarProp = initScalarProperty(theScalarProp);
        aRes->ChangeProperties().Append(aScalarProp);
    }

    for (; aVectorIter.More(); aVectorIter.Next()) {
        RWCae::MeshEntityId aNodeIdx = aRes->AddNodePosition(aVectorIter.Node());

        RWCae::MeshPropertyEntityInfo anInfo(aNodeIdx);
        aVecProp->SetRawVector<Graphic3d_Vec4d>(anInfo, aVectorIter.VectorValue());
        if (!aScalarProp.IsNull()) {
            aScalarProp->GetRawComponent(0)->SetValue(anInfo, aVectorIter.ScalarValue(), false);
        }
    }

    aVecProp->UpdateComponents(false);
    if (!aScalarProp.IsNull()) {
        aScalarProp->UpdateComponents(false);
    }

    return aRes;
}

// ================================================================
// Function : initVectorProperty
// ================================================================
Handle(RWCae::MeshNVecProperty) MeshVectorsPrimitiveArray::initVectorProperty(
    const Handle(RWCae::MeshNVecProperty)& theProp)
{
    Handle(RWCae::MeshNVecProperty) aVecProp =
        new RWCae::MeshNVecProperty(theProp->Name(),
                                      theProp->NbComponents(),
                                      RWCae::MeshPropertyBase::MappingType::Nodal,
                                      RWCae::MeshNVecPropertyType::Vector);
    aVecProp->SetId(theProp->Id());
    aVecProp->Init<RWCae::MeshScalarProperty>();
    for (int aCompIter = 0; aCompIter < aVecProp->NbRawComponents(); aCompIter++) {
        Handle(RWCae::MeshScalarProperty) aComp = Handle(RWCae::MeshScalarProperty)::DownCast(
            aVecProp->GetRawComponent(aCompIter));
        aComp->Init(0);
        aComp->SetLoaded(true);
        aComp->SetId(theProp->GetRawComponent(aCompIter)->Id());
        aComp->SetValueRange(theProp->GetRawComponent(aCompIter)->ValueRange());
    }

    return aVecProp;
}

// ================================================================
// Function : initScalarProperty
// ================================================================
Handle(RWCae::MeshNVecProperty) MeshVectorsPrimitiveArray::initScalarProperty(
    const RWCae::MeshPropertyComponent& theProp)
{
    Handle(RWCae::MeshNVecProperty) aScalarProp =
        new RWCae::MeshNVecProperty(theProp.Property->Name(), 1, RWCae::MeshPropertyBase::MappingType::Nodal);
    aScalarProp->SetId(theProp.Property->Id());
    aScalarProp->Init<RWCae::MeshScalarProperty>();
    Handle(RWCae::MeshScalarProperty) aComp = Handle(RWCae::MeshScalarProperty)::DownCast(
        aScalarProp->GetRawComponent(0));
    aComp->Init(0);
    aComp->SetLoaded(true);
    aComp->SetId(theProp.GetScalarComp()->Id());
    aComp->SetValueRange(theProp.GetScalarComp()->ValueRange());

    return aScalarProp;
}

// ================================================================
// Function : MeshVectorsPrimitiveArray
// ================================================================
MeshVectorsPrimitiveArray::MeshVectorsPrimitiveArray(const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
                                                     const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
                                                     const ViewCaeGeom::MapOfMeshGeomNVecProps&   theVecProps,
                                                     MeshVectorDistrType                           theDistrType,
                                                     double                                        theScaleCoeff,
                                                     double                                        theSamplingRatio)
{
    createBaseGeometry();
    createInstancingArray(theMeshGeom, theGeomProps, theVecProps, theDistrType, theScaleCoeff, theSamplingRatio);
}

// ================================================================
// Function : createInstancingArray
// ================================================================
void MeshVectorsPrimitiveArray::createInstancingArray(const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
                                                      const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
                                                      const ViewCaeGeom::MapOfMeshGeomNVecProps&   theVecProps,
                                                      MeshVectorDistrType                           theDistrType,
                                                      double                                        theScaleCoeff,
                                                      double                                        theSamplingRatio)
{
    const ViewCaeGeom::MeshGeometryGen* aGenGeom = dynamic_cast<const ViewCaeGeom::MeshGeometryGen*>(
        theMeshGeom.get());

    MeshVectorDistrType aDistrType     = aGenGeom == nullptr ? theDistrType : MeshVectorDistrType::Volume;
    double              aSamplingRatio = aGenGeom == nullptr ? theSamplingRatio : 1.0;

    MeshVectorsIterator aVectorIter(theMeshGeom, theGeomProps, theVecProps, aDistrType, aSamplingRatio);
    if (!aVectorIter.More()) {
        return;
    }

    int aNbEntities = 0;

    for (Handle(ViewCaeGeom::MeshGeometry) aGeom = theMeshGeom->GetLastNotModifierChild(); !aGeom.IsNull();
         aGeom                                    = aGeom->GetChild()) {
        if (Handle(RWCae::MeshData) aMeshData = aGeom->GetMeshData()) {
            aNbEntities += aVectorIter.IsNodal() ? aMeshData->NbNodes() : aMeshData->NbIndexedElems();
        }
    }

    const Handle(NCollection_BaseAllocator)& anAlloc = Graphic3d_Buffer::DefaultAllocator();

    myInstancingAttribs = new Graphic3d_Buffer(anAlloc);

    // 12 values for transformation matrix, 2 values for texture coordinates
    Graphic3d_Attribute anAttribInfo[NB_INSTANCING_ATTRIBUTES] = {{Graphic3d_TOA_CUSTOM, Graphic3d_TOD_VEC4},
                                                                  {Graphic3d_TOA_CUSTOM, Graphic3d_TOD_VEC4},
                                                                  {Graphic3d_TOA_CUSTOM, Graphic3d_TOD_VEC4},
                                                                  {Graphic3d_TOA_CUSTOM, Graphic3d_TOD_VEC2}};

    aNbEntities = (int)(aNbEntities * aSamplingRatio);

    if (aNbEntities <= 0) {
        return;
    }

    if (!myInstancingAttribs->Init(aNbEntities, anAttribInfo, NB_INSTANCING_ATTRIBUTES)) {
        return;
    }

    myInstancingAttribs->NbElements = 0;

    for (; aVectorIter.More(); aVectorIter.Next()) {
        calculateNodeInstancingData(aVectorIter.Node(),
                                    aVectorIter.VectorValue(),
                                    aVectorIter.ScalarValue(),
                                    aVectorIter.MinMax(),
                                    theScaleCoeff);
    }
}

// ================================================================
// Function : calculateNodeInstancingData
// ================================================================
void MeshVectorsPrimitiveArray::calculateNodeInstancingData(const Graphic3d_Vec3&  theNode,
                                                            const Graphic3d_Vec4d& theVectorValue,
                                                            double                 theScalarValue,
                                                            const Graphic3d_Vec2d& theMinMaxValues,
                                                            double                 theScaleCoeff)
{
    constexpr int aR0 = 0;
    constexpr int aR1 = 1;
    constexpr int aR2 = 2;

    Graphic3d_Vec3 aVector((float)theVectorValue.y(), (float)theVectorValue.z(), (float)theVectorValue.w());
    double         aMagnitude = theVectorValue.x();
    if (RWCae::IsNaN(aMagnitude) || aMagnitude <= gp::Resolution()) {
        return;
    }

    OpenGl_Mat4 aMat;
    calculateTransformation(aMat, theNode, aVector, float(aMagnitude * theScaleCoeff));

    VectorAttribs& anAttribs = myInstancingAttribs->ChangeValue<VectorAttribs>(myInstancingAttribs->NbElements);

    anAttribs.Row1 = aMat.GetRow(aR0);
    anAttribs.Row2 = aMat.GetRow(aR1);
    anAttribs.Row3 = aMat.GetRow(aR2);

    Graphic3d_Vec2 aTexUV = CaeMeshTools::GetUVCoords(theScalarValue,
                                                      theMinMaxValues[0],
                                                      theMinMaxValues[1],
                                                      CaeMeshTools::NbColorscaleColors);
    anAttribs.Tex         = OpenGl_Vec2(aTexUV.x(), aTexUV.y());

    ++myInstancingAttribs->NbElements;

    myBndBox.Update(theNode.x(), theNode.y(), theNode.z());
    myBndBox.Update(theNode.x() + aVector.x() * theScaleCoeff,
                    theNode.y() + aVector.y() * theScaleCoeff,
                    theNode.z() + aVector.z() * theScaleCoeff);
}

// ================================================================
// Function : calculateTransformation
// ================================================================
void MeshVectorsPrimitiveArray::calculateTransformation(OpenGl_Mat4&          theResMat,
                                                        const Graphic3d_Vec3& thePos,
                                                        const Graphic3d_Vec3& theDir,
                                                        const float           theScale)
{
    constexpr double aHalfAngle = 180.0;

    Graphic3d_TransformUtils::Translate(theResMat, thePos.x(), thePos.y(), thePos.z());

    gp_Dir aDir(theDir.x(), theDir.y(), theDir.z());
    gp_Dir aRefDir = gp::DZ();
    double anAngle = aDir.Angle(aRefDir);
    if (anAngle > gp::Resolution()) {
        gp_Dir anAxis = (anAngle < (M_PI - gp::Resolution())) ? aRefDir.Crossed(aDir) : gp::DX();
        Graphic3d_TransformUtils::Rotate(theResMat,
                                         (float)(anAngle * aHalfAngle / M_PI),
                                         (float)anAxis.X(),
                                         (float)anAxis.Y(),
                                         (float)anAxis.Z());
    }

    Graphic3d_TransformUtils::Scale(theResMat, theScale, theScale, theScale);
}

// =======================================================================
// Function : createBaseGeometry
// =======================================================================
void MeshVectorsPrimitiveArray::createBaseGeometry()
{
    constexpr int    aNbSegments = 5;
    constexpr double aCylLength  = 0.7;
    constexpr double aCylRad     = 0.03;
    constexpr double aConeLength = 0.3;
    constexpr double aConeRad    = 0.1;

    constexpr int n2 = 2;
    constexpr int n3 = 3;

    constexpr int aNbPoints  = aNbSegments * n3 + 1;
    constexpr int aNbIndices = (aNbSegments - n2 + aNbSegments * n2 + aNbSegments + aNbSegments) * n3;

    myBaseGeometry = new Graphic3d_ArrayOfTriangles(aNbPoints, aNbIndices);

    for (int i = 0; i < aNbSegments; i++) {
        double anAngle = i * M_PI * n2 / aNbSegments;
        myBaseGeometry->AddVertex(gp_Pnt(Cos(anAngle) * aCylRad, Sin(anAngle) * aCylRad, 0.0));
    }
    for (int i = 0; i < aNbSegments; i++) {
        double anAngle = i * M_PI * n2 / aNbSegments;
        myBaseGeometry->AddVertex(gp_Pnt(Cos(anAngle) * aCylRad, Sin(anAngle) * aCylRad, aCylLength));
    }
    for (int i = 0; i < aNbSegments; i++) {
        double anAngle = i * M_PI * n2 / aNbSegments;
        myBaseGeometry->AddVertex(gp_Pnt(Cos(anAngle) * aConeRad, Sin(anAngle) * aConeRad, aCylLength));
    }
    myBaseGeometry->AddVertex(gp_Pnt(0.0, 0.0, aCylLength + aConeLength));

    // arrow cylinder bottom
    for (int i = 2; i < aNbSegments; i++) {
        myBaseGeometry->AddEdges(1, i, i + 1);
    }
    // arrow cylinder side
    for (int i = 0; i < aNbSegments; i++) {
        myBaseGeometry->AddEdges(i + 1, 1 + (i + 1) % aNbSegments, i + 1 + aNbSegments);
        myBaseGeometry->AddEdges(1 + (i + 1) % aNbSegments,
                                 1 + (i + 1) % aNbSegments + aNbSegments,
                                 i + 1 + aNbSegments);
    }
    // arrow cone bottom
    for (int i = 2; i < aNbSegments; i++) {
        myBaseGeometry->AddEdges(aNbSegments * n2 + 1, aNbSegments * n2 + i, aNbSegments * n2 + i + 1);
    }
    // arrow cone side
    for (int i = 0; i < aNbSegments; i++) {
        myBaseGeometry->AddEdges(i + 1 + aNbSegments * n2,
                                 1 + (i + 1) % aNbSegments + aNbSegments * n2,
                                 aNbSegments * n3 + 1);
    }
}

// ================================================================
// Function : MeshVectorsElement
// ================================================================
MeshVectorsElement::MeshVectorsElement(const Handle(MeshVectorsPrimitiveArray)& thePrimArray)
    : myPrimArray(thePrimArray)
{
    //
}

// =======================================================================
// Function : ~MeshVectorsElement
// =======================================================================
MeshVectorsElement::~MeshVectorsElement()
{
    //
}

// =======================================================================
// Function : Release
// =======================================================================
void MeshVectorsElement::Release(OpenGl_Context* theContext)
{
    myIsVboInit = Standard_False;
    if (!myVboIndices.IsNull()) {
        if (theContext != nullptr) {
            theContext->DelayedRelease(myVboIndices);
        }
        myVboIndices.Nullify();
    }
    if (!myVboAttribs.IsNull()) {
        if (theContext != nullptr) {
            theContext->DelayedRelease(myVboAttribs);
        }
        myVboAttribs.Nullify();
    }
    if (!myInstancingVboAttribs.IsNull()) {
        if (theContext != nullptr) {
            theContext->DelayedRelease(myInstancingVboAttribs);
        }
        myInstancingVboAttribs.Nullify();
    }
}

// =======================================================================
// Function : EstimatedDataSize
// =======================================================================
Standard_Size MeshVectorsElement::EstimatedDataSize() const
{
    Standard_Size aSize = 0;
    if (!myVboAttribs.IsNull()) {
        aSize += myVboAttribs->EstimatedDataSize();
    }
    if (!myVboIndices.IsNull()) {
        aSize += myVboIndices->EstimatedDataSize();
    }
    if (!myInstancingVboAttribs.IsNull()) {
        aSize += myInstancingVboAttribs->EstimatedDataSize();
    }
    return aSize;
}

// =======================================================================
// Function : UpdateDrawStats
// =======================================================================
void MeshVectorsElement::UpdateDrawStats(Graphic3d_FrameStatsDataTmp& theStats, bool theIsDetailed) const
{
    ++theStats[Graphic3d_FrameStatsCounter_NbElemsNotCulled];
    ++theStats[Graphic3d_FrameStatsCounter_NbElemsFillNotCulled];

    if (!theIsDetailed || myVboAttribs.IsNull() || !myVboAttribs->IsValid() || myVboIndices.IsNull()
        || !myVboIndices->IsValid() || myInstancingVboAttribs.IsNull() || !myInstancingVboAttribs->IsValid()) {
        return;
    }

    const Standard_Integer aNbIndices    = myVboIndices->GetElemsNb() * myInstancingVboAttribs->GetElemsNb();
    constexpr int          aNbTrgIndices = 3;
    theStats[Graphic3d_FrameStatsCounter_NbTrianglesNotCulled] += aNbIndices / aNbTrgIndices;
}

// =======================================================================
// Function : Render
// =======================================================================
void MeshVectorsElement::Render(const Handle(OpenGl_Workspace)& theWorkspace) const
{
    const Handle(OpenGl_Context)& aCtx = theWorkspace->GetGlContext();

    if (!checkOpenGl(aCtx)) {
        return;
    }

    if (!myIsVboInit) {
        if (!buildVBO(aCtx)) {
            return;
        }
        myIsVboInit = Standard_True;
    }

    const OpenGl_Aspects* anAspectFace = theWorkspace->ApplyAspects(false);

    const Handle(Graphic3d_ShaderProgram)& aShaderProgram = anAspectFace->Aspect()->ShaderProgram();
    if (!aShaderProgram.IsNull() && aShaderProgram->GetId() == INSTANCING_SHADER_ID
        && aShaderProgram->Header().IsEmpty()) {
        aShaderProgram->SetHeader(aCtx->GraphicsLibrary() == Aspect_GraphicsLibrary_OpenGLES ? "#version 300 es"
                                                                                             : "#version 130");
    }

    const Handle(OpenGl_TextureSet)& aTextureSet = theWorkspace->TextureSet();
    aCtx->ShaderManager()->BindFaceProgram(aTextureSet,
                                           Graphic3d_TypeOfShadingModel_Unlit,
                                           Graphic3d_AlphaMode_Opaque,
                                           Aspect_IS_SOLID,
                                           false,
                                           false,
                                           false,
                                           anAspectFace->ShaderProgramRes(aCtx));
    aCtx->BindTextures(aTextureSet, aCtx->ActiveProgram());
    aCtx->SetSampleAlphaToCoverage(aCtx->ShaderManager()->MaterialState().HasAlphaCutoff());
    const OpenGl_Vec4& anInteriorColor = theWorkspace->InteriorColor();
    aCtx->SetColor4fv(anInteriorColor);

    myVboAttribs->BindAllAttributes(aCtx);
    myVboIndices->Bind(aCtx);
    myInstancingVboAttribs->BindAllAttributes(aCtx);
    const OpenGl_GlFunctions* aFuncs = aCtx->Functions();
    for (int i = 0; i < NB_INSTANCING_ATTRIBUTES; i++) {
        aFuncs->glVertexAttribDivisor(INSTANCE_ATTR_LOC[i], 1);
    }
    const Handle(Graphic3d_Buffer)& anInstancingAttribs = myPrimArray->InstancingAttribs();
    aCtx->arbIns->glDrawElementsInstanced(GL_TRIANGLES,
                                          myVboIndices->GetElemsNb(),
                                          myVboIndices->GetDataType(),
                                          myVboIndices->GetDataOffset(),
                                          anInstancingAttribs->NbElements);
    myInstancingVboAttribs->UnbindAllAttributes(aCtx);
    myVboIndices->Unbind(aCtx);
    myVboAttribs->UnbindAllAttributes(aCtx);

    aCtx->BindProgram(NULL);
}

// =======================================================================
// Function : buildVBO
// =======================================================================
bool MeshVectorsElement::buildVBO(const Handle(OpenGl_Context)& theCtx) const
{
    clearMemoryGL(theCtx);

    const Handle(Graphic3d_ArrayOfTriangles)& aBaseGeometry       = myPrimArray->BaseGeometry();
    const Handle(Graphic3d_Buffer)&           anInstancingAttribs = myPrimArray->InstancingAttribs();

    if (aBaseGeometry.IsNull() || aBaseGeometry->Attributes()->IsEmpty() || aBaseGeometry->Attributes()->NbElements < 1
        || anInstancingAttribs.IsNull() || anInstancingAttribs->IsEmpty() || anInstancingAttribs->NbElements < 1) {
        return false;
    }

    myVboAttribs = new ArrowVertexBuffer<BASE_GEOMETRY_NB_ATTRIBUTES>(*aBaseGeometry->Attributes());

    if (!myVboAttribs->init(theCtx,
                            aBaseGeometry->Attributes()->Stride,
                            aBaseGeometry->Attributes()->NbElements,
                            aBaseGeometry->Attributes()->Data(),
                            GL_UNSIGNED_BYTE,
                            aBaseGeometry->Attributes()->Stride)) {
        clearMemoryGL(theCtx);
        return false;
    }

    myVboIndices = new OpenGl_IndexBuffer();

    if (!myVboIndices->Init(theCtx,
                            1,
                            aBaseGeometry->Indices()->NbElements,
                            reinterpret_cast<const GLushort*>(aBaseGeometry->Indices()->Data()))) {
        clearMemoryGL(theCtx);
        return false;
    }

    myInstancingVboAttribs = new ArrowVertexBuffer<NB_INSTANCING_ATTRIBUTES>(*anInstancingAttribs, INSTANCE_ATTR_LOC);
    if (!myInstancingVboAttribs->init(theCtx,
                                      anInstancingAttribs->Stride,
                                      anInstancingAttribs->NbElements,
                                      anInstancingAttribs->Data(),
                                      GL_UNSIGNED_BYTE,
                                      anInstancingAttribs->Stride)) {
        clearMemoryGL(theCtx);
        return false;
    }

    return true;
}

// =======================================================================
// Function : clearMemoryGL
// =======================================================================
void MeshVectorsElement::clearMemoryGL(const Handle(OpenGl_Context)& theGlCtx) const
{
    if (!myVboIndices.IsNull()) {
        myVboIndices->Release(theGlCtx.operator->());
        myVboIndices.Nullify();
    }
    if (!myVboAttribs.IsNull()) {
        myVboAttribs->Release(theGlCtx.operator->());
        myVboAttribs.Nullify();
    }
    if (!myInstancingVboAttribs.IsNull()) {
        myInstancingVboAttribs->Release(theGlCtx.operator->());
        myInstancingVboAttribs.Nullify();
    }
}

// =======================================================================
// Function : checkOpenGl
// =======================================================================
bool MeshVectorsElement::checkOpenGl(const Handle(OpenGl_Context)& theCtx) const
{
    if (!theCtx->IsGlGreaterEqual(3, 0)) {
        Message::SendFail() << "Error: unsupported OpenGl version '" << theCtx->VersionMajor() << "."
                            << theCtx->VersionMinor() << "', should be >= '3.0'";
        return false;
    }

    if (!theCtx->ToUseVbo() || theCtx->arbIns == nullptr) {
        Message::SendFail() << "Error: OpenGl instancing is not supported";
        return false;
    }

    return true;
}

// =======================================================================
// Function : ShaderProgram
// =======================================================================
Handle(Graphic3d_ShaderProgram) ArrowShaderManager::ShaderProgram()
{
    Handle(Graphic3d_ShaderProgram) aProgSrc = new Graphic3d_ShaderProgram();
    aProgSrc->SetId(INSTANCING_SHADER_ID);

    Graphic3d_ShaderObject::ShaderVariableList aUniforms, aStageInOuts;
    aStageInOuts.Append(
        Graphic3d_ShaderObject::ShaderVariable("vec2 TexCoord", Graphic3d_TOS_VERTEX | Graphic3d_TOS_FRAGMENT));

    TCollection_AsciiString aSrcVert;
    aSrcVert += "in vec4 scInstMatRow1;\n";
    aSrcVert += "in vec4 scInstMatRow2;\n";
    aSrcVert += "in vec4 scInstMatRow3;\n";
    aSrcVert += "in vec2 scInstTex;\n";
    aSrcVert += "void main()\n";
    aSrcVert += "{\n";
    aSrcVert += "    mat4 aMat;\n";
    aSrcVert += "    aMat[0] = vec4(scInstMatRow1.x, scInstMatRow2.x, scInstMatRow3.x, 0.0);\n";
    aSrcVert += "    aMat[1] = vec4(scInstMatRow1.y, scInstMatRow2.y, scInstMatRow3.y, 0.0);\n";
    aSrcVert += "    aMat[2] = vec4(scInstMatRow1.z, scInstMatRow2.z, scInstMatRow3.z, 0.0);\n";
    aSrcVert += "    aMat[3] = vec4(scInstMatRow1.w, scInstMatRow2.w, scInstMatRow3.w, 1.0);\n";
    aSrcVert +=
        "    gl_Position = occProjectionMatrix * occWorldViewMatrix * occModelWorldMatrix * aMat * occVertex;\n";
    aSrcVert += "    TexCoord = scInstTex;\n";
    aSrcVert += "}";

    TCollection_AsciiString aSrcFrag;
    aSrcFrag += "void main()\n";
    aSrcFrag += "{\n";
    aSrcFrag += "    vec4 aColor = occColor;\n";
    aSrcFrag += "    if (occTextureEnable == 1) {\n";
    aSrcFrag += "        aColor = occTexture2D(occSamplerBaseColor, TexCoord.xy);\n";
    aSrcFrag += "    }\n";
    aSrcFrag += "    occSetFragColor(aColor);\n";
    aSrcFrag += "}";

    Graphic3d_ShaderAttributeList anAttributes;
    for (int i = 0; i < NB_INSTANCING_ATTRIBUTES; i++) {
        anAttributes.Append(new Graphic3d_ShaderAttribute(INSTANCE_ATTR_NAME[i], INSTANCE_ATTR_LOC[i]));
    }
    aProgSrc->SetVertexAttributes(anAttributes);

    aProgSrc->AttachShader(
        Graphic3d_ShaderObject::CreateFromSource(aSrcVert, Graphic3d_TOS_VERTEX, aUniforms, aStageInOuts));
    aProgSrc->AttachShader(
        Graphic3d_ShaderObject::CreateFromSource(aSrcFrag, Graphic3d_TOS_FRAGMENT, aUniforms, aStageInOuts));

    return aProgSrc;
}

} // namespace
