#ifndef _CAEMESHSTREAMLINESBUILDER_HEADERFILE
#define _CAEMESHSTREAMLINESBUILDER_HEADERFILE

#include <RWCae/MeshData.h>

#include <ViewCaeGeom/MeshGeometry.h>

#include <Graphic3d_ArrayOfPolylines.hxx>
#include <Graphic3d_ArrayOfPrimitives.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <NCollection_StdAllocator.hxx>
#include <OpenGl_PrimitiveArray.hxx>
#include <OSD_Timer.hxx>

#include <list>

namespace ViewCae {
//! Struct defines bounding plane that is used for generating of initial streamlines points.
struct MeshStreamlinesPlane {
    gp_Pnt Origin;
    gp_Vec Side;
    gp_Vec Up;

    //! Constructor.
    MeshStreamlinesPlane() : Origin(0.0, 0.0, 0.0), Side(0.0, 1.0, 0.0), Up(0.0, 0.0, 1.0) {}

    //! Constructor.
    MeshStreamlinesPlane(const gp_Pnt& theOrigin, const gp_Vec& theSide, const gp_Vec& theUp)
        : Origin(theOrigin),
          Side(theSide),
          Up(theUp)
    {
    }

    //! Check if planes are equal.
    bool IsEqual(const MeshStreamlinesPlane& theOther) const
    {
        return Origin.IsEqual(theOther.Origin, Precision::Confusion())
            && Side.IsEqual(theOther.Side, Precision::Confusion(), Precision::Angular())
            && Up.IsEqual(theOther.Up, Precision::Confusion(), Precision::Angular());
    }
};

//! Struct for storing streamlines animation parameters.
struct MeshStreamlinesAnimationData {
    Graphic3d_Vec2 TimeRange;
    float          Speed      = 0.25f;
    float          Trail      = 0.5f;
    int            Frequency  = 10;
    bool           IsAnimated = false;
    bool           IsPaused   = false;
};

//! Array of polylines extended with animation data.
class CaeMeshStreamlines_ArrayOfPolylines : public Graphic3d_ArrayOfPolylines, public MeshStreamlinesAnimationData {
public:
    //! Constructor.
    CaeMeshStreamlines_ArrayOfPolylines(Standard_Integer                         theMaxVertexs,
                                        Standard_Integer                         theMaxBounds,
                                        Standard_Integer                         theMaxEdges,
                                        Graphic3d_ArrayFlags                     theArrayFlags,
                                        const std::vector<Graphic3d_TypeOfData>* theCustomAttribs = nullptr)
        : Graphic3d_ArrayOfPolylines(theMaxVertexs, theMaxBounds, theMaxEdges, theArrayFlags, theCustomAttribs)
    {
    }
};

//! Array of triangles extended with animation data.
class CaeMeshStreamlines_ArrayOfTriangles : public Graphic3d_ArrayOfTriangles, public MeshStreamlinesAnimationData {
public:
    //! Constructor.
    CaeMeshStreamlines_ArrayOfTriangles(Standard_Integer                         theMaxVertexs,
                                        Standard_Integer                         theMaxEdges,
                                        Graphic3d_ArrayFlags                     theArrayFlags,
                                        const std::vector<Graphic3d_TypeOfData>* theCustomAttribs = nullptr)
        : Graphic3d_ArrayOfTriangles(theMaxVertexs, theMaxEdges, theArrayFlags, theCustomAttribs)
    {
    }
};

//! Class for calculation of mesh streamlines.
class CaeMeshStreamlinesBuilder {
public:
    //! Constructor.
    Standard_EXPORT CaeMeshStreamlinesBuilder(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
                                              const RWCae::MeshPropertyComponent&     theScalarProp,
                                              const Handle(RWCae::MeshNVecProperty)&  theVecProp,
                                              const MeshStreamlinesPlane&               thePlane);

    //! Switch between regular (true) and random (false) distribution of the initial points in the seed plane.
    void SetIsRegular(bool theIsRegular) { myIsRegular = theIsRegular; }

    //! Set total number of streamlines for random distribution of the initial points in the seed plane.
    void SetNbStreamlines(int theNbStreamlines) { myNbStreamlines = theNbStreamlines; }

    //! Set "U x V" streamlines layout for regular distribution of initial points.
    void SetUVStreamlines(const Graphic3d_Vec2i& theUVStreamlines) { myUVStreamlines = theUVStreamlines; }

    //! Set integration step coefficient, lower value leads to more accurate streamlines.
    void SetStepCoeff(double theStepCoeff) { myStepCoeff = theStepCoeff; }

    //! Set max length coefficient, higher value leads to longer streamlines.
    void SetLengthCoeff(double theLengthCoeff) { myLengthCoeff = theLengthCoeff; }

    //! Set streamlines thickness as a coefficient multiplied by bounding box size.
    void SetThickness(double theThickness) { myThickness = theThickness; }

    //! Set mesh bounding box.
    void SetBox(const Bnd_Box& theBox) { myBox = theBox; }

    //! Build streamlines and fill array of primitives.
    Standard_EXPORT Handle(Graphic3d_ArrayOfPrimitives) Create();

    //! Extract streamlines points as a separate mesh data.
    //! Each streamline stored as array of nodes + 0d element for the last node index.
    //! Points time values are stored as a separate property.
    Standard_EXPORT Handle(RWCae::MeshData) ExtractStreamlines();

    typedef std::vector<float, NCollection_StdAllocator<float>> WeightsVector;

    //! Check if point is inside mesh element.
    Standard_EXPORT static bool CheckPointInElem(const Handle(RWCae::MeshData)& theMeshData,
                                                 int                              theElemIdx,
                                                 const Graphic3d_Vec3&            thePoint,
                                                 WeightsVector&                   theWeights,
                                                 double*                          theElemSize);

protected:
    //! Struct defines streamline point.
    struct MeshStreamlinePoint {
        gp_XYZ Point;
        double Value       = 0.0;
        double Time        = 0.0;
        int    MeshElement = -1;
    };

    typedef std::list<MeshStreamlinePoint, NCollection_StdAllocator<MeshStreamlinePoint>> MeshStreamline;

    //! Calculate streamlines points.
    Standard_EXPORT bool calculateStreamlines();

    //! Compute initial streamlines points.
    Standard_EXPORT void computeInitialPoints();

    //! Compute streamline points.
    Standard_EXPORT void computeStreamline(int theThreadId, int theIndex);

    //! Get mesh element by point and compute properties values in it.
    Standard_EXPORT bool getMeshDataValue(const Handle(NCollection_BaseAllocator)& theAlloc,
                                          const gp_XYZ&                            thePoint,
                                          int&                                     theElemIdx,
                                          gp_XYZ&                                  theVecValue,
                                          double&                                  theMaxMagn,
                                          double&                                  theScalarValue,
                                          double&                                  theElemSize);

    //! Compute the next streamline point.
    Standard_EXPORT bool getNextStreamlinePoint(const Handle(NCollection_BaseAllocator)& theAlloc,
                                                int&                                     theElemIdx,
                                                gp_XYZ&                                  thePoint,
                                                gp_XYZ&                                  theVecValue,
                                                double&                                  theMaxMagn,
                                                double&                                  theScalarValue,
                                                double&                                  theElemSize,
                                                double                                   theStep);

    //! Compute the next streamline point using 2nd order Runde-Kutta method.
    Standard_EXPORT bool getNextPointRungeKutta2(const Handle(NCollection_BaseAllocator)& theAlloc,
                                                 int&                                     theElemIdx,
                                                 gp_XYZ&                                  thePoint,
                                                 gp_XYZ&                                  theVecValue,
                                                 double&                                  theMaxMagn,
                                                 double&                                  theScalarValue,
                                                 double&                                  theElemSize,
                                                 double                                   theStep);

    //! Compute the next streamline point using 4th order Runde-Kutta method.
    Standard_EXPORT bool getNextPointRungeKutta4(const Handle(NCollection_BaseAllocator)& theAlloc,
                                                 int&                                     theElemIdx,
                                                 gp_XYZ&                                  thePoint,
                                                 gp_XYZ&                                  theVecValue,
                                                 double&                                  theMaxMagn,
                                                 double&                                  theScalarValue,
                                                 double&                                  theElemSize,
                                                 double                                   theStep);

    //! Compute properties values in point inside mesh element.
    Standard_EXPORT bool computeValueInPoint(int                  theElemIdx,
                                             const WeightsVector& theWeights,
                                             gp_XYZ&              theVecValue,
                                             double&              theMaxMagn,
                                             double&              theScalarValue);

    //! Compute properties values in point inside mesh element.
    template<class Elem_T>
    Standard_EXPORT bool computeValueInPoint(const Elem_T&        theElem,
                                             const WeightsVector& theWeights,
                                             gp_XYZ&              theVecValue,
                                             double&              theMaxMagn,
                                             double&              theScalarValue);

    //! Add a new point to streamline.
    Standard_EXPORT void addStreamlinePoint(MeshStreamline& theStreamline,
                                            const gp_XYZ&   thePoint,
                                            int             theMeshElem,
                                            double          theValue,
                                            double          theTime,
                                            bool            theToAppend);

    //! Filter streamlines points and simplify it.
    Standard_EXPORT void filterAndSimplifyStreamlines();

    //! Filter streamline points and simplify it.
    Standard_EXPORT void filterAndSimplifyStreamline(MeshStreamline& theStreamline,
                                                     int&            theNbPoints,
                                                     int&            theNbPolylines);

    //! Unite internal streamlines points using direction angle/scalar field value thresholds.
    Standard_EXPORT void simplifyStreamline(MeshStreamline& theStreamline);

    //! Filter streamline points (e.g. by clipping plane, domains).
    //! Calculate total number of points.
    Standard_EXPORT void filterStreamlinePoints(MeshStreamline& theStreamline,
                                                bool            theToFilter,
                                                int&            theNbPoints,
                                                int&            theNbPolylines);

    //! Restore streamlines from mesh data points.
    Standard_EXPORT bool restoreFromMeshData();

    //! Create array of primitives for computed streamlines points.
    Standard_EXPORT Handle(Graphic3d_ArrayOfPrimitives) createPrimitives();

    //! Create array of polylines for computed streamlines points.
    Standard_EXPORT Handle(Graphic3d_ArrayOfPrimitives) createPolylinesPrimitives(Graphic3d_ArrayFlags theFlags);

    //! Create array of triangles for computed streamlines points.
    Standard_EXPORT Handle(Graphic3d_ArrayOfPrimitives) createTrianglesPrimitives(Graphic3d_ArrayFlags theFlags);

    //! Calculate direction in streamline point.
    Standard_EXPORT bool calculatePointDirection(const MeshStreamline&           theStreamline,
                                                 MeshStreamline::const_iterator& theIter,
                                                 gp_XYZ&                         theDir);

    //! Add points and triangles to array of triangles for wide streamline.
    Standard_EXPORT void addStreamlineTubeSection(Handle(Graphic3d_ArrayOfPrimitives)& theTriangles,
                                                  const MeshStreamlinePoint&           thePoint,
                                                  const gp_XYZ&                        theSideVec,
                                                  const gp_XYZ&                        theUpVec,
                                                  double                               theThickness,
                                                  bool                                 theIsFirstPoint);

    //! Compute integration step size.
    Standard_EXPORT bool computeStep(double theElemSize, double theMagn, double& theStep);

    //! Add streamline points to separate mesh data.
    Standard_EXPORT void extractStreamline(const MeshStreamline&                      theStreamline,
                                           const Handle(RWCae::MeshData)&           theMeshData,
                                           const Handle(RWCae::MeshScalarProperty)& theProp,
                                           const Handle(RWCae::MeshScalarProperty)& thePropTime);

    //! Check if point is inside mesh element.
    template<class Elem_T>
    Standard_EXPORT static bool checkPointInElem(const RWCae::MeshAdaptor<Elem_T>& theAdaptor,
                                                 RWCae::MeshEntityId               theElemIdx,
                                                 const Graphic3d_Vec3&               thePoint,
                                                 WeightsVector&                      theWeights,
                                                 double*                             theElemSize);

protected:
    //! Integration method used for streamlines points calculation.
    enum class IntegrationMethod {
        RungeKutta2, //!< 2nd order Runge-Kutta method
        RungeKutta4  //!< 4th order Runge-Kutta method, more accurate than 2nd order but slower
    };

protected:
    Handle(ViewCaeGeom::MeshGeometry) myMeshGeom;
    Handle(RWCae::MeshNVecProperty)  myVecProp;
    RWCae::MeshPropertyComponent     myScalarProp;
    MeshStreamlinesPlane               myPlane;
    double                             myStepCoeff      = 0.2;
    double                             myLengthCoeff    = 1.0;
    double                             myThickness      = 0.0;
    double                             myThicknessCoeff = 0.01;
    Graphic3d_Vec2i                    myUVStreamlines  = Graphic3d_Vec2i(10);
    int                                myNbStreamlines  = 100;
    bool                               myIsRegular      = false;

    Bnd_Box myBox;

    std::vector<gp_XYZ> myInitialPoints;

    std::vector<MeshStreamline> myStreamlines;

    std::vector<Handle(NCollection_BaseAllocator)> myAllocs; //!< per-thread allocators

    int myNbStreamlinesPoints    = 0;
    int myNbStreamlinesPolylines = 0;

    double myValMinMax[2]   = {0.0, 1.0};
    int    myMaxProcElems   = 0;
    int    myMaxNbSameElems = 100;
    int    myNbTubeTrgs     = 5;

    IntegrationMethod myIntegrationMethod = IntegrationMethod::RungeKutta4;
};

//! OpenGl element for mesh streamlines presentation.
class MeshStreamlinesElement : public OpenGl_PrimitiveArray {
public:
    //! Constructor.
    MeshStreamlinesElement(const OpenGl_GraphicDriver*          theDriver,
                           const Graphic3d_TypeOfPrimitiveArray theType,
                           const Handle(Graphic3d_IndexBuffer)& theIndices,
                           const Handle(Graphic3d_Buffer)&      theAttribs,
                           const Handle(Graphic3d_BoundBuffer)& theBounds,
                           const MeshStreamlinesAnimationData*  theAnimData)
        : OpenGl_PrimitiveArray(theDriver, theType, theIndices, theAttribs, theBounds),
          myAnimData(theAnimData)
    {
        myTimer.Start();
    }

    //! Destructor.
    virtual ~MeshStreamlinesElement() {}

    //! Draw the element.
    Standard_EXPORT virtual void Render(const Handle(OpenGl_Workspace)& theWorkspace) const override;

protected:
    const MeshStreamlinesAnimationData* myAnimData;

    OSD_Timer myTimer;
};

//! GLSL shader for mesh streamlines presentation.
class StreamlinesShaderManager {
public:
    //! Return streamlines shader program (with/without lighting).
    static Handle(Graphic3d_ShaderProgram) ShaderProgram(bool theIsLighted);

protected:
    //! Return streamlines vertex shader.
    static TCollection_AsciiString vertexShader(bool theIsLighted);

    //! Return streamlines fragment shader.
    static TCollection_AsciiString fragmentShader(bool theIsLighted);

    //! Return streamlines fragment shader lighting functions.
    static TCollection_AsciiString fragmentShaderLight();
};

} // namespace

#endif // _CAEMESHSTREAMLINESBUILDER_HEADERFILE