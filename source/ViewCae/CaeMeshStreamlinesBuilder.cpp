#include <ViewCae/CaeMeshStreamlinesBuilder.h>

#include <ViewCae/CaeMeshTools.h>

#include <ViewCaeGeom/MeshBvhData.h>
#include <ViewCaeGeom/MeshGeometryGen.h>

#include <RWBase/TaskManager.h>

#include <RWCae/MeshAdaptorsIterator.h>

#include <BVH_Traverse.hxx>
#include <math_BullardGenerator.hxx>
#include <OpenGl_Context.hxx>
#include <OpenGl_ShaderProgram.hxx>
#include <OpenGl_Workspace.hxx>

namespace {
constexpr const char* STREAMLINES_SHADER_ID     = "CaeMeshStreamlinesBuilder_StreamlinesShader";
constexpr int         STREAMLINES_TIME_ATTR_LOC = 4;
constexpr int         STREAMLINES_TIME_ATTR_IDX = 0;

constexpr int STREAMLINES_NB_PASSES = 2;
constexpr int BVH_DIM               = 3;

//! Selector for mesh element by point.
struct MeshElementSelector : public ViewCaeGeom::MeshHBvhBoxSetTraverse<ViewCaeGeom::MeshBvhData::MeshBvhBoxSet> {
public:
    //! Constructor.
    MeshElementSelector(const opencascade::handle<RWCae::MeshData>&       theMeshData,
                        const Graphic3d_Vec3&                               thePoint,
                        ViewCae::CaeMeshStreamlinesBuilder::WeightsVector* theWeights,
                        int                                                 thePrevElem)
        : myMeshData(theMeshData),
          myPoint(thePoint),
          myWeights(theWeights),
          myPrevElem(thePrevElem)
    {
    }

    //! Return index of found mesh element.
    int FoundElem() const { return myFoundElem; }

    //! Return size of found mesh element.
    double FoundElemSize() const { return myFoundElemSize; }

public:
    //! Defines the rules for node rejection by bounding box.
    virtual Standard_Boolean RejectNode(const BVH_Vec3f&  theCornerMin,
                                        const BVH_Vec3f&  theCornerMax,
                                        Standard_Boolean& theIsInside) const override
    {
        theIsInside = false;
        BVH_Box<float, BVH_DIM> aBox(theCornerMin, theCornerMax);
        return aBox.IsOut(myPoint);
    }

    //! Defines the rules for leaf acceptance.
    virtual Standard_Boolean Accept(const ViewCaeGeom::MeshBvhData::MeshBvhBoxSet::SubBoxSet* theBoxSet,
                                    const Standard_Integer                                     theIndex,
                                    const Standard_Boolean                                     theIsInside) override
    {
        (void)theIsInside;

        BVH_Box<float, BVH_DIM> aBox = theBoxSet->Box(theIndex);
        if (aBox.IsOut(myPoint)) {
            return false;
        }

        int anElemIndex = theBoxSet->Element(theIndex);
        if (anElemIndex == myPrevElem) {
            return false;
        }

        bool isIn = ViewCae::CaeMeshStreamlinesBuilder::CheckPointInElem(myMeshData,
                                                                          anElemIndex,
                                                                          myPoint,
                                                                          *myWeights,
                                                                          &myFoundElemSize);
        if (isIn) {
            myFoundElem = anElemIndex;
        }
        return isIn;
    }

    //! Returns the flag controlling the tree descend.
    //! Returns true if the tree descend should be stopped.
    virtual Standard_Boolean Stop() const override { return myFoundElem != -1; }

protected:
    Handle(RWCae::MeshData)                           myMeshData;
    Graphic3d_Vec3                                      myPoint;
    ViewCae::CaeMeshStreamlinesBuilder::WeightsVector* myWeights;
    int                                                 myPrevElem      = -1;
    int                                                 myFoundElem     = -1;
    double                                              myFoundElemSize = -1.0;
};

//! Compute normal and side vectors for streamline tube.
bool computeNormSideVectors(const gp_XYZ& theCurDir, const gp_XYZ& thePrevDir, gp_XYZ& theNorm, gp_XYZ& theSideVec)
{
    theNorm = thePrevDir + theCurDir;

    Standard_Real aD = theNorm.Modulus();
    if (aD <= gp::Resolution()) {
        return false;
    }
    theNorm.Divide(aD);

    theSideVec = theSideVec + thePrevDir * theSideVec.Dot(theNorm) / thePrevDir.Dot(theNorm);
    return true;
}

//! Compute side/up vectors for specified vector.
void computeSideUpVectors(const gp_XYZ& theNorm, gp_XYZ& theSideVec, gp_XYZ& theUpVec)
{
    theUpVec = theSideVec.Crossed(theNorm);
    if (theUpVec.Modulus() < Precision::Confusion()) {
        theUpVec = gp::DX().XYZ().Crossed(theNorm);
        if (theUpVec.Modulus() < Precision::Confusion()) {
            theUpVec = gp::DY().XYZ().Crossed(theNorm);
        }
    }
    theUpVec.Normalize();
    theSideVec = theNorm.Crossed(theUpVec);
}
}

namespace ViewCae {
// =======================================================================
// Function : CaeMeshStreamlinesBuilder
// =======================================================================
CaeMeshStreamlinesBuilder::CaeMeshStreamlinesBuilder(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
                                                     const RWCae::MeshPropertyComponent&     theScalarProp,
                                                     const Handle(RWCae::MeshNVecProperty)&  theVecProp,
                                                     const MeshStreamlinesPlane&               thePlane)
    : myVecProp(theVecProp),
      myScalarProp(theScalarProp),
      myPlane(thePlane)
{
    myMeshGeom = theMeshGeom->GetLastNotModifierChild();

    if (!myVecProp.IsNull()
        && (!myVecProp->IsVector3() || !myVecProp->HasScalarComponents() || !myVecProp->IsNodal())) {
        myVecProp.Nullify();
    }

    if (myScalarProp.IsDefined()
        && (!myScalarProp.Property->IsNodal()
            || myScalarProp.GetScalarComp()->BaseType() != RWCae::MeshScalarPropertyBaseType::Scalar)) {
        myScalarProp.Property.Nullify();
    }

    if (myScalarProp.IsDefined()) {
        myScalarProp.GetScalarComp()->ValueRange().GetBounds(myValMinMax[0], myValMinMax[1]);
    }
}

// =======================================================================
// Function : Create
// =======================================================================
Handle(Graphic3d_ArrayOfPrimitives) CaeMeshStreamlinesBuilder::Create()
{
    if (dynamic_cast<const ViewCaeGeom::MeshGeometryGen*>(myMeshGeom.get()) != nullptr) {
        if (!restoreFromMeshData()) {
            return Handle(Graphic3d_ArrayOfPolylines)();
        }
    } else {
        if (!calculateStreamlines()) {
            return Handle(Graphic3d_ArrayOfPolylines)();
        }
    }

    return createPrimitives();
}

// =======================================================================
// Function : calculateStreamlines
// =======================================================================
bool CaeMeshStreamlinesBuilder::calculateStreamlines()
{
    // Only for initial mesh data for now
    if (!myMeshGeom->GetParent().IsNull()) {
        return false;
    }

    if (myVecProp.IsNull()) {
        return false;
    }

    const Handle(RWCae::MeshData)& aMeshData = myMeshGeom->GetMeshData();
    if (aMeshData.IsNull() || aMeshData->NbElems3d() == 0) {
        return false;
    }

    constexpr double aLengthCoeff = 10.0;
    constexpr double anInv3       = 1.0 / 3.0;
    myMaxProcElems                = (int)(std::pow(aMeshData->NbElems3d(), anInv3) * aLengthCoeff * myLengthCoeff);

    // Update myNbStreamlines for regular seed plane sampling mode
    if (myIsRegular) {
        myNbStreamlines = myUVStreamlines.x() * myUVStreamlines.y();
    }

    computeInitialPoints();

    int aNbThreads = RWBase::TaskManager::Instance()->NbDefaultThreadsToLaunch();
    for (int i = 0; i < aNbThreads; i++) {
        myAllocs.push_back(new NCollection_IncAllocator());
    }

    myStreamlines.resize(myNbStreamlines);

    // Precompute BVH in the main thread
    myMeshGeom->GetBvhData()->BvhBoxSet(myMeshGeom)->BVH();

    auto process = [this](int theThreadId, int theIndex) {
        this->computeStreamline(theThreadId, theIndex);
    };

    RWBase::TaskManager::Instance()->RunInParallel(0, myNbStreamlines, process);

    filterAndSimplifyStreamlines();

    return true;
}

// =======================================================================
// Function : computeInitialPoints
// =======================================================================
void CaeMeshStreamlinesBuilder::computeInitialPoints()
{
    myInitialPoints.resize(myNbStreamlines);

    constexpr double aTwo      = 2.0;
    constexpr double aHalfStep = 0.5;

    math_BullardGenerator aRand;

    for (int i = 0; i < myNbStreamlines; i++) {
        double aU = 0.0;
        double aV = 0.0;
        if (myIsRegular) {
            // Split the seed plane in a regular grid myUVStreamlines.x() x myUVStreamlines.y()
            // and place the initial points in the center of each cell of this grid.
            // This works better than using the grid points because the border streamlines aren't clipped
            // by the presentation's bounding box.
            int aColumn = i % myUVStreamlines.x();
            int aRow    = i / myUVStreamlines.x();
            aU          = (aColumn + aHalfStep) / myUVStreamlines.x();
            aV          = (aRow + aHalfStep) / myUVStreamlines.y();
        } else {
            aU = aRand.NextReal();
            aV = aRand.NextReal();
        }
        myInitialPoints[i] = myPlane.Origin.XYZ();
        myInitialPoints[i] += myPlane.Side.XYZ() * (aU * aTwo - 1.0);
        myInitialPoints[i] += myPlane.Up.XYZ() * (aV * aTwo - 1.0);
    }
}

// =======================================================================
// Function : computeStreamline
// =======================================================================
void CaeMeshStreamlinesBuilder::computeStreamline(int theThreadId, int theIndex)
{
    const gp_XYZ& aFirstPnt = myInitialPoints[theIndex];

    gp_XYZ aFirstVecValue(0.0, 0.0, 0.0);
    int    aFirstElem     = -1;
    double aFirstElemSize = 0.0;
    double aFirstStep     = 0.0;

    double aScalarValue = 0.0;
    double aMaxMagn     = 0.0;

    const Handle(NCollection_BaseAllocator)& anAlloc = myAllocs[theThreadId];

    if (!getMeshDataValue(anAlloc, aFirstPnt, aFirstElem, aFirstVecValue, aMaxMagn, aScalarValue, aFirstElemSize)) {
        return;
    }

    if (!computeStep(aFirstElemSize, aMaxMagn, aFirstStep)) {
        return;
    }

    MeshStreamline& aStreamline = myStreamlines[theIndex];
    aStreamline                 = MeshStreamline(anAlloc);

    addStreamlinePoint(aStreamline, aFirstPnt, aFirstElem, aScalarValue, 0.0, true);

    for (int aDir = 0; aDir < STREAMLINES_NB_PASSES; aDir++) {
        gp_XYZ aCurPnt      = aFirstPnt;
        gp_XYZ aCurVecValue = aFirstVecValue;
        int    aCurElem     = aFirstElem;
        double aCurElemSize = aFirstElemSize;
        double aCurStep     = aDir == 0 ? -aFirstStep : aFirstStep;
        double aTime        = aCurStep;

        for (int aNbPrElems = 0, aNbSameElems = 0; aNbPrElems < myMaxProcElems && aNbSameElems < myMaxNbSameElems;) {
            int aPrevElem = aCurElem;
            if (!getNextStreamlinePoint(anAlloc,
                                        aCurElem,
                                        aCurPnt,
                                        aCurVecValue,
                                        aMaxMagn,
                                        aScalarValue,
                                        aCurElemSize,
                                        aCurStep)) {
                break;
            }

            aNbSameElems = aCurElem == aPrevElem ? aNbSameElems + 1 : 0;
            aNbPrElems += aCurElem != aPrevElem ? 1 : 0;

            if ((aDir == 1 ? aStreamline.back() : aStreamline.front()).Point.IsEqual(aCurPnt, Precision::Confusion())) {
                break;
            }

            addStreamlinePoint(aStreamline, aCurPnt, aCurElem, aScalarValue, aTime, aDir == 1);

            if (!computeStep(aCurElemSize, aMaxMagn, aCurStep)) {
                break;
            }

            aTime += aCurStep;
        }
    }
}

// =======================================================================
// Function : getNextStreamlinePoint
// =======================================================================
bool CaeMeshStreamlinesBuilder::getNextStreamlinePoint(const Handle(NCollection_BaseAllocator)& theAlloc,
                                                       int&                                     theElemIdx,
                                                       gp_XYZ&                                  thePoint,
                                                       gp_XYZ&                                  theVecValue,
                                                       double&                                  theMaxMagn,
                                                       double&                                  theScalarValue,
                                                       double&                                  theElemSize,
                                                       double                                   theStep)
{
    switch (myIntegrationMethod) {
        case IntegrationMethod::RungeKutta2:
            return getNextPointRungeKutta2(theAlloc,
                                           theElemIdx,
                                           thePoint,
                                           theVecValue,
                                           theMaxMagn,
                                           theScalarValue,
                                           theElemSize,
                                           theStep);
        case IntegrationMethod::RungeKutta4:
            return getNextPointRungeKutta4(theAlloc,
                                           theElemIdx,
                                           thePoint,
                                           theVecValue,
                                           theMaxMagn,
                                           theScalarValue,
                                           theElemSize,
                                           theStep);
        default: return false;
    }
}

// =======================================================================
// Function : getNextPointRungeKutta2
// =======================================================================
bool CaeMeshStreamlinesBuilder::getNextPointRungeKutta2(const Handle(NCollection_BaseAllocator)& theAlloc,
                                                        int&                                     theElemIdx,
                                                        gp_XYZ&                                  thePoint,
                                                        gp_XYZ&                                  theVecValue,
                                                        double&                                  theMaxMagn,
                                                        double&                                  theScalarValue,
                                                        double&                                  theElemSize,
                                                        double                                   theStep)
{
    constexpr double aCoeff = 0.5;

    gp_XYZ aHalfPoint = thePoint + theVecValue * theStep * aCoeff;
    gp_XYZ aHalfVecValue;
    double aHalfMaxMagn     = 0.0;
    double aHalfScalarValue = 0.0;

    if (!getMeshDataValue(theAlloc,
                          aHalfPoint,
                          theElemIdx,
                          aHalfVecValue,
                          aHalfMaxMagn,
                          aHalfScalarValue,
                          theElemSize)) {
        return false;
    }

    thePoint += aHalfVecValue * theStep;

    return getMeshDataValue(theAlloc, thePoint, theElemIdx, theVecValue, theMaxMagn, theScalarValue, theElemSize);
}

// =======================================================================
// Function : getNextPointRungeKutta4
// =======================================================================
bool CaeMeshStreamlinesBuilder::getNextPointRungeKutta4(const Handle(NCollection_BaseAllocator)& theAlloc,
                                                        int&                                     theElemIdx,
                                                        gp_XYZ&                                  thePoint,
                                                        gp_XYZ&                                  theVecValue,
                                                        double&                                  theMaxMagn,
                                                        double&                                  theScalarValue,
                                                        double&                                  theElemSize,
                                                        double                                   theStep)
{
    constexpr double aCoeff1 = 2.0;
    constexpr double aCoeff2 = 2.0;
    constexpr double aDiv    = 6.0;

    gp_XYZ aPointToCheck = thePoint + theVecValue * theStep / aCoeff1;

    gp_XYZ aVecValue1;
    double aMaxMagn     = 0.0;
    double aScalarValue = 0.0;

    if (!getMeshDataValue(theAlloc, aPointToCheck, theElemIdx, aVecValue1, aMaxMagn, aScalarValue, theElemSize)) {
        return false;
    }

    aPointToCheck = thePoint + aVecValue1 * theStep / aCoeff2;
    gp_XYZ aVecValue2;

    if (!getMeshDataValue(theAlloc, aPointToCheck, theElemIdx, aVecValue2, aMaxMagn, aScalarValue, theElemSize)) {
        return false;
    }

    aPointToCheck = thePoint + aVecValue2 * theStep;
    gp_XYZ aVecValue3;

    if (!getMeshDataValue(theAlloc, aPointToCheck, theElemIdx, aVecValue3, aMaxMagn, aScalarValue, theElemSize)) {
        return false;
    }

    thePoint += (theVecValue + aVecValue1 * aCoeff1 + aVecValue2 * aCoeff2 + aVecValue3) / aDiv * theStep;

    return getMeshDataValue(theAlloc, thePoint, theElemIdx, theVecValue, theMaxMagn, theScalarValue, theElemSize);
}

// =======================================================================
// Function : getMeshDataValue
// =======================================================================
bool CaeMeshStreamlinesBuilder::getMeshDataValue(const Handle(NCollection_BaseAllocator)& theAlloc,
                                                 const gp_XYZ&                            thePoint,
                                                 int&                                     theElemIdx,
                                                 gp_XYZ&                                  theVecValue,
                                                 double&                                  theMaxMagn,
                                                 double&                                  theScalarValue,
                                                 double&                                  theElemSize)
{
    NCollection_StdAllocator<float> anAlloc(theAlloc);

    WeightsVector  aWeights(anAlloc);
    Graphic3d_Vec3 aPoint((float)thePoint.X(), (float)thePoint.Y(), (float)thePoint.Z());
    bool           hasFound = false;

    if (theElemIdx != -1) {
        hasFound = CheckPointInElem(myMeshGeom->GetMeshData(), theElemIdx, aPoint, aWeights, nullptr);
    }

    if (!hasFound) {
        const Handle(ViewCaeGeom::MeshBvhData::MeshBvhBoxSet)& aBvhBoxSet = myMeshGeom->GetBvhData()->BvhBoxSet(
            myMeshGeom);

        MeshElementSelector aSelector(myMeshGeom->GetMeshData(), aPoint, &aWeights, theElemIdx);
        aSelector.SetBVHSet(aBvhBoxSet.get());
        aSelector.Select();

        theElemIdx  = aSelector.FoundElem();
        theElemSize = aSelector.FoundElemSize();
        hasFound    = theElemIdx != -1;
    }

    if (!hasFound) {
        return false;
    }

    return computeValueInPoint(theElemIdx, aWeights, theVecValue, theMaxMagn, theScalarValue);
}

// =======================================================================
// Function : CheckPointInElem
// =======================================================================
bool CaeMeshStreamlinesBuilder::CheckPointInElem(const Handle(RWCae::MeshData)& theMeshData,
                                                 int                              theElemIdx,
                                                 const Graphic3d_Vec3&            thePoint,
                                                 WeightsVector&                   theWeights,
                                                 double*                          theElemSize)
{
    const RWCae::MeshData::IndexedElement& anElem = theMeshData->IndexedElements()[theElemIdx];

    auto perform = [&anElem, &thePoint, &theWeights, &theElemSize](const auto& theAdaptor) {
        return checkPointInElem(theAdaptor, anElem.Index, thePoint, theWeights, theElemSize);
    };

    return RWCae::MeshAdaptorsIterator3dElems::Switch(anElem.Type, perform, theMeshData);
}

// =======================================================================
// Function : checkPointInElem
// =======================================================================
template<class Elem_T>
bool CaeMeshStreamlinesBuilder::checkPointInElem(const RWCae::MeshAdaptor<Elem_T>& theAdaptor,
                                                 RWCae::MeshEntityId               theElemIdx,
                                                 const Graphic3d_Vec3&               thePoint,
                                                 WeightsVector&                      theWeights,
                                                 double*                             theElemSize)
{
    const Elem_T& anElem = theAdaptor.GetElement(theElemIdx);

    if (theElemSize != nullptr) {
        BVH_Box<float, BVH_DIM> aBox;
        Graphic3d_Vec3          aNode;
        for (RWCae::MeshElemEntityId i = 0; i < anElem.NbNodes(); i++) {
            theAdaptor.NodePosition(aNode, anElem[i]);
            aBox.Add(aNode);
        }
        if (aBox.IsValid()) {
            *theElemSize = (double)aBox.Size().Modulus();
        }
    }

    theWeights.resize(anElem.NbNodes());
    return theAdaptor.ProbePoint(anElem, thePoint, theWeights.data(), theWeights.get_allocator().Allocator());
}

// =======================================================================
// Function : computeValueInPoint
// =======================================================================
bool CaeMeshStreamlinesBuilder::computeValueInPoint(int                  theElemIdx,
                                                    const WeightsVector& theWeights,
                                                    gp_XYZ&              theVecValue,
                                                    double&              theMaxMagn,
                                                    double&              theScalarValue)
{
    const Handle(RWCae::MeshData)& aMeshData = myMeshGeom->GetMeshData();

    const RWCae::MeshData::IndexedElement& anElem = aMeshData->IndexedElements()[theElemIdx];

    auto perform = [this, &theWeights, &theVecValue, &theMaxMagn, &theScalarValue, &anElem](const auto& theAdaptor) {
        return computeValueInPoint(theAdaptor.GetElement(anElem.Index),
                                   theWeights,
                                   theVecValue,
                                   theMaxMagn,
                                   theScalarValue);
    };

    return RWCae::MeshAdaptorsIterator3dElems::Switch(anElem.Type, perform, aMeshData);
}

// =======================================================================
// Function : computeValueInPoint
// =======================================================================
template<class Elem_T>
bool CaeMeshStreamlinesBuilder::computeValueInPoint(const Elem_T&        theElem,
                                                    const WeightsVector& theWeights,
                                                    gp_XYZ&              theVecValue,
                                                    double&              theMaxMagn,
                                                    double&              theScalarValue)
{
    theVecValue.SetCoord(0.0, 0.0, 0.0);
    theMaxMagn     = 0.0;
    theScalarValue = myScalarProp.IsDefined() ? 0.0 : std::numeric_limits<double>::quiet_NaN();
    Graphic3d_Vec4d aVecValue(0.0);
    bool            isNaN = false;
    for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < theElem.NbNodes(); aNodeIter++) {
        myVecProp->GetRawVector<Graphic3d_Vec4d>(RWCae::MeshPropertyEntityInfo(theElem[aNodeIter]), aVecValue);
        if (RWCae::IsNaN(aVecValue.x())) {
            theVecValue.SetCoord(std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::quiet_NaN());
            isNaN = true;
        } else if (!isNaN) {
            theVecValue.SetCoord(theVecValue.X() + aVecValue.y() * theWeights[aNodeIter],
                                 theVecValue.Y() + aVecValue.z() * theWeights[aNodeIter],
                                 theVecValue.Z() + aVecValue.w() * theWeights[aNodeIter]);
            theMaxMagn = Max(theMaxMagn, aVecValue.x());
        }

        if (!myScalarProp.IsDefined()) {
            continue;
        }

        double aValue = myScalarProp.GetScalarComp()->Value(RWCae::MeshPropertyEntityInfo(theElem[aNodeIter]));
        if (RWCae::IsNaN(aValue)) {
            theScalarValue = aValue;
        } else if (!RWCae::IsNaN(theScalarValue)) {
            theScalarValue += aValue * theWeights[aNodeIter];
        }
    }

    return !isNaN;
}

// =======================================================================
// Function : computeStep
// =======================================================================
bool CaeMeshStreamlinesBuilder::computeStep(double theElemSize, double theMagn, double& theStep)
{
    if (theMagn < Precision::Confusion()) {
        return false;
    }

    theStep = theElemSize / theMagn * myStepCoeff * Sign(1.0, theStep);
    return true;
}

// =======================================================================
// Function : addStreamlinePoint
// =======================================================================
void CaeMeshStreamlinesBuilder::addStreamlinePoint(MeshStreamline& theStreamline,
                                                   const gp_XYZ&   thePoint,
                                                   int             theMeshElem,
                                                   double          theValue,
                                                   double          theTime,
                                                   bool            theToAppend)
{
    if (theToAppend) {
        theStreamline.push_back(MeshStreamlinePoint());
    } else {
        theStreamline.push_front(MeshStreamlinePoint());
    }
    MeshStreamlinePoint& aStreamlinePoint = theToAppend ? theStreamline.back() : theStreamline.front();

    aStreamlinePoint.Point       = thePoint;
    aStreamlinePoint.MeshElement = theMeshElem;
    aStreamlinePoint.Value       = theValue;
    aStreamlinePoint.Time        = theTime;
}

// =======================================================================
// Function : ExtractStreamlines
// =======================================================================
Handle(RWCae::MeshData) CaeMeshStreamlinesBuilder::ExtractStreamlines()
{
    Handle(RWCae::MeshData) aRes = new RWCae::MeshData();

    if (!calculateStreamlines()) {
        return aRes;
    }

    Handle(RWCae::MeshNVecProperty)   aProp;
    Handle(RWCae::MeshScalarProperty) aComp;
    if (myScalarProp.IsDefined()) {
        aProp = new RWCae::MeshNVecProperty(myScalarProp.Property->Name(),
                                              1,
                                              RWCae::MeshPropertyBase::MappingType::Nodal);
        aProp->Init<RWCae::MeshScalarProperty>();
        aProp->SetId(myScalarProp.Property->Id());
        aRes->ChangeProperties().Append(aProp);

        aComp = Handle(RWCae::MeshScalarProperty)::DownCast(aProp->GetRawComponent(0));
        aComp->Init(myNbStreamlinesPoints);
        aComp->SetLoaded(true);
        aComp->SetId(myScalarProp.GetScalarComp()->Id());
        aComp->SetValueRange(myScalarProp.GetScalarComp()->ValueRange());
    }

    Handle(RWCae::MeshNVecProperty) aPropTime =
        new RWCae::MeshNVecProperty("nodes_time", 1, RWCae::MeshPropertyBase::MappingType::Nodal);
    aPropTime->Init<RWCae::MeshScalarProperty>();
    aPropTime->SetId("nodes_time");
    Handle(RWCae::MeshScalarProperty) aCompTime = Handle(RWCae::MeshScalarProperty)::DownCast(
        aPropTime->GetRawComponent(0));
    aCompTime->Init(myNbStreamlinesPoints);
    aCompTime->SetLoaded(true);
    aRes->ChangeProperties().Append(aPropTime);

    RWCae::MeshElement0d anElem;

    for (const MeshStreamline& aStreamline : myStreamlines) {
        extractStreamline(aStreamline, aRes, aComp, aCompTime);
    }

    if (!aProp.IsNull()) {
        aProp->UpdateComponents(false);
    }

    return aRes;
}

// =======================================================================
// Function : extractStreamline
// =======================================================================
void CaeMeshStreamlinesBuilder::extractStreamline(const MeshStreamline&                      theStreamline,
                                                  const Handle(RWCae::MeshData)&           theMeshData,
                                                  const Handle(RWCae::MeshScalarProperty)& theProp,
                                                  const Handle(RWCae::MeshScalarProperty)& thePropTime)
{
    if (theStreamline.size() <= 1) {
        return;
    }

    RWCae::MeshElement0d anElem;

    int aNbPointsInPolyline = 0;
    for (const MeshStreamlinePoint& aPoint : theStreamline) {
        if (aPoint.MeshElement != -1) {
            RWCae::MeshEntityId aNodeIdx = theMeshData->AddNodePosition(
                Graphic3d_Vec3((float)aPoint.Point.X(), (float)aPoint.Point.Y(), (float)aPoint.Point.Z()));
            if (!theProp.IsNull()) {
                theProp->SetValue(RWCae::MeshPropertyEntityInfo(aNodeIdx), aPoint.Value, false);
            }
            thePropTime->SetValue(RWCae::MeshPropertyEntityInfo(aNodeIdx), aPoint.Time);
            aNbPointsInPolyline++;
            continue;
        }

        if (aNbPointsInPolyline > 1) {
            anElem[0] = theMeshData->NbNodes();
            theMeshData->AddElement(anElem);
        }
        aNbPointsInPolyline = 0;
    }
    if (aNbPointsInPolyline > 1) {
        anElem[0] = theMeshData->NbNodes();
        theMeshData->AddElement(anElem);
    }
}

// =======================================================================
// Function : restoreFromMeshData
// =======================================================================
bool CaeMeshStreamlinesBuilder::restoreFromMeshData()
{
    const Handle(RWCae::MeshData)& aMeshData = myMeshGeom->GetMeshData();
    if (aMeshData.IsNull()) {
        return false;
    }

    myNbStreamlinesPoints    = aMeshData->NbNodes();
    myNbStreamlinesPolylines = aMeshData->NbElems0d();
    if (myNbStreamlinesPoints == 0 || myNbStreamlinesPolylines == 0) {
        return false;
    }

    myStreamlines.resize(myNbStreamlinesPolylines);

    Handle(RWCae::MeshScalarPropertyBase) aScalarProp = myScalarProp.GetScalarComp();

    const Handle(RWCae::MeshNVecProperty)& aTimeProp = aMeshData->GetPropertyById("nodes_time");
    Handle(RWCae::MeshScalarPropertyBase)  aTimeComp = !aTimeProp.IsNull() ? aTimeProp->GetRawComponent(0) : nullptr;

    RWCae::MeshAdaptor0d anAdaptor(*aMeshData);

    int aStartNodeIdx = 0;
    for (int aPolylineIter = 0; aPolylineIter < myNbStreamlinesPolylines; aPolylineIter++) {
        int anEndNodeIdx = anAdaptor.GetElement(aPolylineIter)[0];

        MeshStreamline& aStreamline = myStreamlines[aPolylineIter];
        for (int aNodeIter = aStartNodeIdx; aNodeIter < anEndNodeIdx; aNodeIter++) {
            Graphic3d_Vec3 aPoint = aMeshData->Nodes()[aNodeIter];

            double aValue = !aScalarProp.IsNull() ? aScalarProp->Value(RWCae::MeshPropertyEntityInfo(aNodeIter))
                                                  : std::numeric_limits<double>::quiet_NaN();

            double aTimeValue = !aTimeComp.IsNull() ? aTimeComp->Value(RWCae::MeshPropertyEntityInfo(aNodeIter))
                                                    : 0.0;
            addStreamlinePoint(aStreamline, gp_XYZ(aPoint.x(), aPoint.y(), aPoint.z()), 0, aValue, aTimeValue, true);
        }

        aStartNodeIdx = anEndNodeIdx;
    }

    return true;
}

// =======================================================================
// Function : filterAndSimplifyStreamline
// =======================================================================
void CaeMeshStreamlinesBuilder::filterAndSimplifyStreamline(MeshStreamline& theStreamline,
                                                            int&            theNbPoints,
                                                            int&            theNbPolylines)
{
    if (theStreamline.size() <= 1) {
        return;
    }

    theNbPoints    = 0;
    theNbPolylines = 0;
    filterStreamlinePoints(theStreamline, true, theNbPoints, theNbPolylines);
    simplifyStreamline(theStreamline);
    theNbPoints    = 0;
    theNbPolylines = 0;
    filterStreamlinePoints(theStreamline, false, theNbPoints, theNbPolylines);
}

// =======================================================================
// Function : simplifyStreamline
// =======================================================================
void CaeMeshStreamlinesBuilder::simplifyStreamline(MeshStreamline& theStreamline)
{
    // Max deflection - 1 degree
    const double anAngleThreshold   = 1.0;
    const double aCosAngleThreshold = Cos(M_PI / 180.0 * anAngleThreshold);
    const double aValueThreshold    = myValMinMax[1] - myValMinMax[0] * 0.01;

    gp_XYZ aPrevPoint;
    gp_XYZ aFirstDir;
    double aFirstValue   = 0.0;
    bool   isFirstPoint  = true;
    bool   isSecondPoint = false;
    for (auto anIter = theStreamline.begin(); anIter != theStreamline.end(); ++anIter) {
        if (anIter->MeshElement == -1) {
            isFirstPoint = true;
            continue;
        } else if (isFirstPoint) {
            isFirstPoint  = false;
            isSecondPoint = true;
            aPrevPoint    = anIter->Point;
            aFirstValue   = anIter->Value;
            continue;
        }

        // Restart the chain when difference between the values exceeds the threshold or in case of NaN<->!NaN jumps
        if ((!RWCae::IsNaN(anIter->Value) && !RWCae::IsNaN(aFirstValue)
             && Abs(anIter->Value - aFirstValue) > aValueThreshold)
            || (RWCae::IsNaN(anIter->Value) != RWCae::IsNaN(aFirstValue))) {
            --anIter;
            isFirstPoint = true;
            continue;
        }

        gp_XYZ        aCurDir = anIter->Point - aPrevPoint;
        Standard_Real aD      = aCurDir.Modulus();
        if (aD <= gp::Resolution()) {
            anIter = theStreamline.erase(anIter);
            --anIter;
            continue;
        }
        aCurDir /= aD;

        aPrevPoint = anIter->Point;

        if (isSecondPoint) {
            isSecondPoint = false;
            aFirstDir     = aCurDir;
            continue;
        }

        --anIter;
        if (aCurDir.Dot(aFirstDir) > aCosAngleThreshold) {
            anIter = theStreamline.erase(anIter);
            continue;
        }

        --anIter;
        isFirstPoint = true;
    }
}

// =======================================================================
// Function : filterStreamlinePoints
// =======================================================================
void CaeMeshStreamlinesBuilder::filterStreamlinePoints(MeshStreamline& theStreamline,
                                                       bool            theToFilter,
                                                       int&            theNbPoints,
                                                       int&            theNbPolylines)
{
    std::pair<int, bool> aPrevElem = std::make_pair(-1, true);

    int aNbPointsInSubLine = 0;
    for (auto anIter = theStreamline.begin(); anIter != theStreamline.end(); ++anIter) {
        bool isAccepted = true;
        if (!theToFilter) {
            isAccepted = anIter->MeshElement != -1;
        } else if (anIter->MeshElement == aPrevElem.first) {
            isAccepted = aPrevElem.second;
        } else {
            isAccepted       = myMeshGeom->AcceptElement(anIter->MeshElement);
            aPrevElem.first  = anIter->MeshElement;
            aPrevElem.second = isAccepted;
        }

        if (isAccepted) {
            aNbPointsInSubLine++;
            continue;
        }

        anIter->MeshElement = -1;
        theNbPoints += aNbPointsInSubLine > 1 ? aNbPointsInSubLine : 0;
        theNbPolylines += aNbPointsInSubLine > 1 ? 1 : 0;
        if (aNbPointsInSubLine == 1 && theToFilter) {
            --anIter;
            anIter->MeshElement = -1;
            ++anIter;
        }
        aNbPointsInSubLine = 0;
    }
    if (aNbPointsInSubLine > 1) {
        theNbPoints += aNbPointsInSubLine;
        theNbPolylines += 1;
    } else if (aNbPointsInSubLine == 1 && theToFilter) {
        theStreamline.back().MeshElement = -1;
    }
}

// =======================================================================
// Function : filterAndSimplifyStreamlines
// =======================================================================
void CaeMeshStreamlinesBuilder::filterAndSimplifyStreamlines()
{
    const int aNbStreamlines = (int)myStreamlines.size();

    std::vector<std::pair<int, int>> aNbPoints(aNbStreamlines);

    auto process = [this, &aNbPoints](int, int theIndex) {
        filterAndSimplifyStreamline(myStreamlines[theIndex], aNbPoints[theIndex].first, aNbPoints[theIndex].second);
    };

    RWBase::TaskManager::Instance()->RunInParallel(0, aNbStreamlines, process);

    myNbStreamlinesPoints    = 0;
    myNbStreamlinesPolylines = 0;

    for (int i = 0; i < aNbStreamlines; i++) {
        myNbStreamlinesPoints += aNbPoints[i].first;
        myNbStreamlinesPolylines += aNbPoints[i].second;
    }
}

// =======================================================================
// Function : createPrimitives
// =======================================================================
Handle(Graphic3d_ArrayOfPrimitives) CaeMeshStreamlinesBuilder::createPrimitives()
{
    if (myNbStreamlinesPoints == 0) {
        return Handle(Graphic3d_ArrayOfPolylines)();
    }

    Graphic3d_ArrayFlags aFlags = myScalarProp.IsDefined() ? Graphic3d_ArrayFlags_VertexTexel
                                                           : Graphic3d_ArrayFlags_None;

    if (myThickness > 0.0) {
        aFlags |= Graphic3d_ArrayFlags_VertexNormal;
        return createTrianglesPrimitives(aFlags);
    } else {
        return createPolylinesPrimitives(aFlags);
    }
}

// =======================================================================
// Function : createPolylinesPrimitives
// =======================================================================
Handle(Graphic3d_ArrayOfPrimitives) CaeMeshStreamlinesBuilder::createPolylinesPrimitives(Graphic3d_ArrayFlags theFlags)
{
    std::vector<Graphic3d_TypeOfData>           aTimeAttr = {Graphic3d_TOD_FLOAT};
    Handle(CaeMeshStreamlines_ArrayOfPolylines) aPolylines =
        new CaeMeshStreamlines_ArrayOfPolylines(myNbStreamlinesPoints,
                                                myNbStreamlinesPolylines,
                                                0,
                                                theFlags,
                                                &aTimeAttr);

    Graphic3d_Vec2 aTimeRange(ShortRealLast(), -ShortRealLast());
    for (const MeshStreamline& aStreamline : myStreamlines) {
        if (aStreamline.size() <= 1) {
            continue;
        }
        int aNbPointsInPolyline = 0;
        for (const MeshStreamlinePoint& aStreamlinePoint : aStreamline) {
            if (aStreamlinePoint.MeshElement != -1) {
                const int      aVertIdx   = aPolylines->AddVertex(aStreamlinePoint.Point);
                Graphic3d_Vec2 aTexCoords = CaeMeshTools::GetUVCoords(aStreamlinePoint.Value,
                                                                      myValMinMax[0],
                                                                      myValMinMax[1],
                                                                      CaeMeshTools::NbColorscaleColors);
                aPolylines->SetVertexTexel(aVertIdx, aTexCoords.x(), aTexCoords.y());
                aPolylines->SetCustomAttrValue(STREAMLINES_TIME_ATTR_IDX, aVertIdx, (float)aStreamlinePoint.Time);
                aTimeRange.x() = Min(aTimeRange.x(), (float)aStreamlinePoint.Time);
                aTimeRange.y() = Max(aTimeRange.y(), (float)aStreamlinePoint.Time);
                aNbPointsInPolyline++;
                continue;
            }

            if (aNbPointsInPolyline > 1) {
                aPolylines->AddBound(aNbPointsInPolyline);
            }
            aNbPointsInPolyline = 0;
        }
        if (aNbPointsInPolyline > 1) {
            aPolylines->AddBound(aNbPointsInPolyline);
        }
    }

    if (aTimeRange.y() - aTimeRange.x() > ShortRealEpsilon()) {
        aPolylines->TimeRange = aTimeRange;
    }

    return aPolylines;
}

// =======================================================================
// Function : createPolylinesPrimitives
// =======================================================================
Handle(Graphic3d_ArrayOfPrimitives) CaeMeshStreamlinesBuilder::createTrianglesPrimitives(Graphic3d_ArrayFlags theFlags)
{
    constexpr int aNbQuadTrgs = 2;
    constexpr int aNbTrgIdx   = 3;

    const Bnd_Box& aBox = myBox.IsVoid() ? myMeshGeom->GetMeshData()->BoundingBox() : myBox;

    double aThickness = (aBox.CornerMax().XYZ() - aBox.CornerMin().XYZ()).Modulus() * myThicknessCoeff * myThickness;

    int aNbTriangles = myNbTubeTrgs * aNbQuadTrgs * (myNbStreamlinesPoints - myNbStreamlinesPolylines);

    std::vector<Graphic3d_TypeOfData>           aTimeAttr = {Graphic3d_TOD_FLOAT};
    Handle(CaeMeshStreamlines_ArrayOfTriangles) aTriangles =
        new CaeMeshStreamlines_ArrayOfTriangles(myNbTubeTrgs * myNbStreamlinesPoints,
                                                aNbTriangles * aNbTrgIdx,
                                                theFlags,
                                                &aTimeAttr);

    gp_XYZ aSideVec;
    gp_XYZ anUpVec;
    gp_XYZ aPrevDir;

    Graphic3d_Vec2 aTimeRange(ShortRealLast(), -ShortRealLast());
    for (const MeshStreamline& aStreamline : myStreamlines) {
        if (aStreamline.size() <= 1) {
            continue;
        }
        bool isFirstPoint = true;
        for (auto anIter = aStreamline.begin(); anIter != aStreamline.end(); ++anIter) {
            if (anIter->MeshElement == -1) {
                isFirstPoint = true;
                continue;
            }

            gp_XYZ aCurDir = aPrevDir;
            if (!calculatePointDirection(aStreamline, anIter, aCurDir)) {
                continue;
            }

            gp_XYZ aNorm = aCurDir;
            if (!isFirstPoint && !computeNormSideVectors(aCurDir, aPrevDir, aNorm, aSideVec)) {
                continue;
            }

            computeSideUpVectors(aNorm, aSideVec, anUpVec);

            addStreamlineTubeSection(aTriangles, *anIter, aSideVec, anUpVec, aThickness, isFirstPoint);

            aTimeRange.x() = Min(aTimeRange.x(), (float)(*anIter).Time);
            aTimeRange.y() = Max(aTimeRange.y(), (float)(*anIter).Time);

            isFirstPoint = false;
            aPrevDir     = aCurDir;
        }
    }

    if (aTimeRange.y() - aTimeRange.x() > ShortRealEpsilon()) {
        aTriangles->TimeRange = aTimeRange;
    }

    return aTriangles;
}

// =======================================================================
// Function : calculatePointDirection
// =======================================================================
bool CaeMeshStreamlinesBuilder::calculatePointDirection(const MeshStreamline&           theStreamline,
                                                        MeshStreamline::const_iterator& theIter,
                                                        gp_XYZ&                         theDir)
{
    ++theIter;
    if (theIter == theStreamline.end() || theIter->MeshElement == -1) {
        --theIter;
        return true;
    }

    theDir = theIter->Point;
    --theIter;
    theDir -= theIter->Point;
    Standard_Real aD = theDir.Modulus();
    if (aD <= gp::Resolution()) {
        return false;
    }
    theDir.Divide(aD);

    return true;
}

// =======================================================================
// Function : addStreamlineTubeSection
// =======================================================================
void CaeMeshStreamlinesBuilder::addStreamlineTubeSection(Handle(Graphic3d_ArrayOfPrimitives)& theTriangles,
                                                         const MeshStreamlinePoint&           thePoint,
                                                         const gp_XYZ&                        theSideVec,
                                                         const gp_XYZ&                        theUpVec,
                                                         double                               theThickness,
                                                         bool                                 theIsFirstPoint)
{
    constexpr double anAngle360 = M_PI * 2.0;
    for (int i = 0; i < myNbTubeTrgs; i++) {
        double         anAngle   = (double)i / myNbTubeTrgs * anAngle360;
        gp_XYZ         aNormal   = theSideVec * Cos(anAngle) + theUpVec * Sin(anAngle);
        gp_XYZ         aPntToAdd = thePoint.Point + aNormal * theThickness;
        const int      aVertIdx  = theTriangles->AddVertex(aPntToAdd);
        Graphic3d_Vec2 aTexCoords =
            CaeMeshTools::GetUVCoords(thePoint.Value, myValMinMax[0], myValMinMax[1], CaeMeshTools::NbColorscaleColors);
        theTriangles->SetVertexTexel(aVertIdx, aTexCoords.x(), aTexCoords.y());
        theTriangles->SetVertexNormal(aVertIdx, aNormal.X(), aNormal.Y(), aNormal.Z());
        theTriangles->SetCustomAttrValue(STREAMLINES_TIME_ATTR_IDX, aVertIdx, (float)thePoint.Time);
    }

    if (theIsFirstPoint) {
        return;
    }

    const int aCurFirstVert  = theTriangles->VertexNumber() - myNbTubeTrgs + 1;
    const int aPrevFirstVert = aCurFirstVert - myNbTubeTrgs;
    for (int i = 0; i < myNbTubeTrgs; i++) {
        int aVert1 = aPrevFirstVert + i;
        int aVert2 = aPrevFirstVert + (i + 1) % myNbTubeTrgs;
        int aVert3 = aCurFirstVert + i;
        int aVert4 = aCurFirstVert + (i + 1) % myNbTubeTrgs;
        theTriangles->AddQuadTriangleEdges(aVert1, aVert3, aVert4, aVert2);
    }
}

// =======================================================================
// Function : Render
// =======================================================================
void MeshStreamlinesElement::Render(const Handle(OpenGl_Workspace)& theWorkspace) const
{
    const Handle(OpenGl_Context)& aCtx = theWorkspace->GetGlContext();

    const OpenGl_Aspects* anAspectFace = theWorkspace->ApplyAspects(false);

    const Handle(Graphic3d_ShaderProgram)& aShaderProgram = anAspectFace->Aspect()->ShaderProgram();
    if (!aShaderProgram.IsNull() && aShaderProgram->GetId().StartsWith(STREAMLINES_SHADER_ID)) {
        if (aShaderProgram->Header().IsEmpty()) {
            aShaderProgram->SetHeader(aCtx->GraphicsLibrary() == Aspect_GraphicsLibrary_OpenGLES ? "#version 300 es"
                                                                                                 : "#version 130");
        }

        constexpr float aSpeedCoeff = 0.1f;

        float aTime = (float)myTimer.ElapsedTime() * myAnimData->Speed * aSpeedCoeff;
        aTime       = aTime - std::floor(aTime);

        aShaderProgram->PushVariable("uIsAnimated", myAnimData->IsAnimated && !myAnimData->IsPaused ? 1 : 0);
        aShaderProgram->PushVariable("uTime", aTime);
        aShaderProgram->PushVariable("uInterval", 1.0f / (float)Max(myAnimData->Frequency, 1));
        aShaderProgram->PushVariable("uTrail", myAnimData->Trail);
        aShaderProgram->PushVariable("uTimeRange", myAnimData->TimeRange);
    }

    OpenGl_PrimitiveArray::Render(theWorkspace);
}

// =======================================================================
// Function : ShaderProgram
// =======================================================================
Handle(Graphic3d_ShaderProgram) StreamlinesShaderManager::ShaderProgram(bool theIsLighted)
{
    Handle(Graphic3d_ShaderProgram) aProgSrc = new Graphic3d_ShaderProgram();
    aProgSrc->SetId(TCollection_AsciiString() + STREAMLINES_SHADER_ID + (theIsLighted ? "_light" : "_unlit"));

    Graphic3d_ShaderObject::ShaderVariableList aUniforms, aStageInOuts;
    aUniforms.Append(Graphic3d_ShaderObject::ShaderVariable("int uIsAnimated", Graphic3d_TOS_FRAGMENT));
    aUniforms.Append(Graphic3d_ShaderObject::ShaderVariable("float uTime", Graphic3d_TOS_FRAGMENT));
    aUniforms.Append(Graphic3d_ShaderObject::ShaderVariable("float uInterval", Graphic3d_TOS_FRAGMENT));
    aUniforms.Append(Graphic3d_ShaderObject::ShaderVariable("float uTrail", Graphic3d_TOS_FRAGMENT));
    aUniforms.Append(Graphic3d_ShaderObject::ShaderVariable("vec2 uTimeRange", Graphic3d_TOS_FRAGMENT));

    aStageInOuts.Append(
        Graphic3d_ShaderObject::ShaderVariable("float Time", Graphic3d_TOS_VERTEX | Graphic3d_TOS_FRAGMENT));
    aStageInOuts.Append(
        Graphic3d_ShaderObject::ShaderVariable("vec2 TexCoord", Graphic3d_TOS_VERTEX | Graphic3d_TOS_FRAGMENT));
    aStageInOuts.Append(
        Graphic3d_ShaderObject::ShaderVariable("vec3 Coord", Graphic3d_TOS_VERTEX | Graphic3d_TOS_FRAGMENT));
    if (theIsLighted) {
        aStageInOuts.Append(
            Graphic3d_ShaderObject::ShaderVariable("vec3 Normal", Graphic3d_TOS_VERTEX | Graphic3d_TOS_FRAGMENT));
        aStageInOuts.Append(
            Graphic3d_ShaderObject::ShaderVariable("vec3 View", Graphic3d_TOS_VERTEX | Graphic3d_TOS_FRAGMENT));
    }

    TCollection_AsciiString aSrcVert = vertexShader(theIsLighted);
    TCollection_AsciiString aSrcFrag = fragmentShader(theIsLighted);

    Graphic3d_ShaderAttributeList anAttributes;
    anAttributes.Append(new Graphic3d_ShaderAttribute("scTime", STREAMLINES_TIME_ATTR_LOC));
    aProgSrc->SetVertexAttributes(anAttributes);

    aProgSrc->AttachShader(
        Graphic3d_ShaderObject::CreateFromSource(aSrcVert, Graphic3d_TOS_VERTEX, aUniforms, aStageInOuts));
    aProgSrc->AttachShader(
        Graphic3d_ShaderObject::CreateFromSource(aSrcFrag, Graphic3d_TOS_FRAGMENT, aUniforms, aStageInOuts));

    return aProgSrc;
}

// =======================================================================
// Function : vertexShader
// =======================================================================
TCollection_AsciiString StreamlinesShaderManager::vertexShader(bool theIsLighted)
{
    TCollection_AsciiString aVert;
    aVert += "in float scTime;\n";
    aVert += "void main()\n";
    aVert += "{\n";
    if (theIsLighted) {
        aVert += "    if (occProjectionMatrix[3][3] == 1.0)\n";
        aVert += "    {\n";
        aVert += "        View = (occWorldViewMatrixInverse * vec4(0.0, 0.0, 1.0, 0.0)).xyz;\n";
        aVert += "    }\n";
        aVert += "    else\n";
        aVert += "    {\n";
        aVert += "        vec3 anEye = (occWorldViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0)).xyz;\n";
        aVert += "        View = normalize (anEye - (occModelWorldMatrix * occVertex).xyz);\n";
        aVert += "    }\n";
        aVert += "    Normal = normalize((occModelWorldMatrixInverseTranspose * vec4(occNormal, 0.0)).xyz);\n";
    }
    aVert += "    gl_Position = occProjectionMatrix * occWorldViewMatrix * occModelWorldMatrix * occVertex;\n";
    aVert += "    Time = scTime;\n";
    aVert += "    TexCoord = occTexCoord.xy;\n";
    aVert += "    Coord = occVertex.xyz;\n";
    aVert += "}";

    return aVert;
}

// =======================================================================
// Function : fragmentShader
// =======================================================================
TCollection_AsciiString StreamlinesShaderManager::fragmentShader(bool theIsLighted)
{
    TCollection_AsciiString aFrag;
    aFrag += "vec4 getColor()\n";
    aFrag += "{\n";
    aFrag += "    vec4 aColor = occColor;\n";
    aFrag += "    if (occTextureEnable == 1) {\n";
    aFrag += "        aColor = occTexture2D(occSamplerBaseColor, TexCoord.xy);\n";
    aFrag += "    }\n";
    aFrag += "   return aColor;\n";
    aFrag += "}\n";
    if (theIsLighted) {
        aFrag += fragmentShaderLight();
    }
    aFrag += "float randFloat(vec3 thePos)\n";
    aFrag += "{\n";
    aFrag += "   return fract(sin(dot(thePos, vec3(12.9898, 78.233, 45.337))) * 43758.5453123);\n";
    aFrag += "}\n";
    aFrag += "void main()\n";
    aFrag += "{\n";
    if (theIsLighted) {
        aFrag += "    vec4 aColor = computeLighting(normalize(Normal), normalize(View), gl_FrontFacing);\n";
    } else {
        aFrag += "    vec4 aColor = getColor();\n";
    }
    aFrag += "    if (uIsAnimated == 1) {\n";
    aFrag += "        float aCoeff = (Time - uTimeRange.x) / (uTimeRange.y - uTimeRange.x);\n";
    aFrag += "        aCoeff = (aCoeff - uTime) / uInterval;\n";
    aFrag += "        aCoeff = aCoeff - floor(aCoeff);\n";
    aFrag += "        aCoeff = max(aCoeff - 1.0 + uTrail, 0.0) / uTrail;\n";
    aFrag += "        if (step(1.0 - smoothstep(0.0, 0.3, aCoeff), randFloat(Coord)) < 0.5) {\n";
    aFrag += "            discard;\n";
    aFrag += "        }\n";
    aFrag += "    }\n";
    aFrag += "    occSetFragColor(aColor);\n";
    aFrag += "}";

    return aFrag;
}

// =======================================================================
// Function : fragmentShaderLight
// =======================================================================
TCollection_AsciiString StreamlinesShaderManager::fragmentShaderLight()
{
    TCollection_AsciiString aFrag;
    aFrag += "vec3 Ambient;\n";
    aFrag += "vec3 Diffuse;\n";
    aFrag += "vec3 Specular;\n";
    aFrag += "void directionalLightFirst(in vec3 theNormal,\n";
    aFrag += "                           in vec3 theView,\n";
    aFrag += "                           in bool theIsFront,\n";
    aFrag += "                           in float theShadow)\n";
    aFrag += "{\n";
    aFrag += "    vec3 aLight = occLight_Position (0);\n";
    aFrag += "    vec3 aHalf = normalize (aLight + theView);\n";
    aFrag += "    vec3  aFaceSideNormal = theIsFront ? theNormal : -theNormal;\n";
    aFrag += "    float aNdotL = max (0.0, dot (aFaceSideNormal, aLight));\n";
    aFrag += "    float aNdotH = max (0.0, dot (aFaceSideNormal, aHalf ));\n";
    aFrag += "    float aSpecl = 0.0;\n";
    aFrag += "    if (aNdotL > 0.0) {\n";
    aFrag += "        aSpecl = pow (aNdotH, occMaterial_Shininess(theIsFront));\n";
    aFrag += "    }\n";
    aFrag += "    Diffuse  += occLight_Diffuse(0)  * aNdotL * theShadow;\n";
    aFrag += "    Specular += occLight_Specular(0) * aSpecl * theShadow;\n";
    aFrag += "}\n";
    aFrag += "vec4 computeLighting(in vec3 theNormal,\n";
    aFrag += "                     in vec3 theView,\n";
    aFrag += "                     in bool theIsFront)\n";
    aFrag += "{\n";
    aFrag += "    Ambient  = occLightAmbient.rgb;\n";
    aFrag += "    Diffuse  = vec3 (0.0);\n";
    aFrag += "    Specular = vec3 (0.0);\n";
    aFrag += "    directionalLightFirst(theNormal, theView, theIsFront, 1.0);\n";
    aFrag += "    vec3 aMatAmbient  = occMaterial_Ambient(theIsFront);\n";
    aFrag += "    vec4 aMatDiffuse  = occMaterial_Diffuse(theIsFront);\n";
    aFrag += "    vec3 aMatSpecular = occMaterial_Specular(theIsFront);\n";
    aFrag += "    vec4 aColor = vec4(Ambient * aMatAmbient +\n";
    aFrag += "                       Diffuse * aMatDiffuse.rgb +\n";
    aFrag += "                       Specular * aMatSpecular, aMatDiffuse.a);\n";
    aFrag += "    aColor *= getColor();\n";
    aFrag += "    return aColor;\n";
    aFrag += "}\n";

    return aFrag;
}

} // namespace
