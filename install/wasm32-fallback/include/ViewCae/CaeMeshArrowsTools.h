#ifndef CAEMESHARROWSTOOLS_HEADERFILE
#define CAEMESHARROWSTOOLS_HEADERFILE

#include <ViewCaeGeom/MeshGeometry.h>

#include <RWCae/MeshData.h>
#include <RWCae/MeshAdaptor.h>

#include <Graphic3d_Buffer.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <OpenGl_Context.hxx>
#include <OpenGl_Element.hxx>
#include <OpenGl_IndexBuffer.hxx>
#include <OpenGl_VertexBuffer.hxx>

namespace ViewCae {
//! Enum for defining vectors distribution type.
enum class MeshVectorDistrType {
    Volume, //!< vectors are calculated for whole mesh volume
    Surface //!< vectors are calculated for mesh surface
};

//! Iterator for vectors points.
class MeshVectorsIterator {
public:
    //! Constructor.
    Standard_EXPORT MeshVectorsIterator(const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
                                        const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
                                        const ViewCaeGeom::MapOfMeshGeomNVecProps&   theVecProps,
                                        MeshVectorDistrType                           theDistrType,
                                        double                                        theSamplingRatio);

    //! Check if there are more points.
    Standard_EXPORT bool More() const;

    //! Move to the next point.
    Standard_EXPORT void Next();

    //! Return current node.
    const Graphic3d_Vec3& Node() const { return myCurNode; }

    //! Return vector value in the current node.
    const Graphic3d_Vec4d& VectorValue() const { return myCurVectorValue; }

    //! Return scalar value in the current node.
    double ScalarValue() const { return myCurScalarValue; }

    //! Check if property is nodal.
    bool IsNodal() const { return myIsNodal; }

    //! Return min/max property values.
    const Graphic3d_Vec2d& MinMax() const { return myCurMinMax; }

    //! Check if scalar and vector properties are the same.
    Standard_EXPORT bool IsSameProps() const;

    //! Return vector property.
    const Handle(RWCae::MeshNVecProperty)& VectorProp() const { return myCurVectorProp; }

    //! Return scalar property.
    const Handle(RWCae::MeshScalarPropertyBase)& ScalarProp() const { return myCurScalarProp; }

protected:
    //! Init the iterator.
    Standard_EXPORT void init();

    //! Fill map of elements nodes.
    template<class Elem_T>
    Standard_EXPORT void getElementsNodes(const RWCae::MeshAdaptor<Elem_T>& theAdaptor,
                                          NCollection_Map<int>*               theNodes) const;

    //! Calculate element's center.
    Standard_EXPORT void getElementCenter(int theElemIdx, Graphic3d_Vec3& theCenter) const;

protected:
    Handle(ViewCaeGeom::MeshGeometry)            myGeom;
    const ViewCaeGeom::MapOfMeshGeomScalarProps& myScalarProps;
    const ViewCaeGeom::MapOfMeshGeomNVecProps&   myVectorProps;
    MeshVectorDistrType                           myDistrType;
    double                                        myRatio;

    Handle(RWCae::MeshScalarPropertyBase) myCurScalarProp;
    Handle(RWCae::MeshNVecProperty)       myCurVectorProp;
    std::vector<int>                        myCurEntities;
    Graphic3d_Vec2d                         myCurMinMax;
    int                                     myCurNbGeomEntities = 0;
    int                                     myCurNbProcessed    = 0;
    bool                                    myIsNodal           = false;

    Graphic3d_Vec3  myCurNode;
    Graphic3d_Vec4d myCurVectorValue;
    double          myCurScalarValue = 0.0;
};

//! Class that stores array of instancing primitives for mesh vectors presentation.
class MeshVectorsPrimitiveArray : public Standard_Transient {
public:
    //! Constructor.
    Standard_EXPORT MeshVectorsPrimitiveArray(const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
                                              const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
                                              const ViewCaeGeom::MapOfMeshGeomNVecProps&   theVecProps,
                                              MeshVectorDistrType                           theDistrType,
                                              double                                        theScaleCoeff,
                                              double                                        theSamplingRatio);

public:
    //! Return computed primitive array for instancing drawing.
    const Handle(Graphic3d_ArrayOfTriangles)& BaseGeometry() const { return myBaseGeometry; }

    //! Return instancing data (transformation + texture coordinates).
    const Handle(Graphic3d_Buffer)& InstancingAttribs() const { return myInstancingAttribs; }

    //! Return bounding box of instancing geometry.
    const Bnd_Box& BndBox() const { return myBndBox; }

    //! Extract vectors nodes as a separate mesh data.
    Standard_EXPORT static Handle(RWCae::MeshData) ExtractVectorNodes(
        const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
        const RWCae::MeshPropertyComponent&     theScalarProp,
        const Handle(RWCae::MeshNVecProperty)&  theVecProp,
        MeshVectorDistrType                       theDistrType,
        double                                    theSamplingRatio);

protected:
    //! Compute base geometry for instancing drawing.
    Standard_EXPORT void createBaseGeometry();

    //! Compute instancing data.
    Standard_EXPORT void createInstancingArray(const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
                                               const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
                                               const ViewCaeGeom::MapOfMeshGeomNVecProps&   theVecProps,
                                               MeshVectorDistrType                           theDistrType,
                                               double                                        theScaleCoeff,
                                               double                                        theSamplingRatio);

    //! Compute instancing data for node.
    Standard_EXPORT void calculateNodeInstancingData(const Graphic3d_Vec3&  theNode,
                                                     const Graphic3d_Vec4d& theVectorValue,
                                                     double                 theScalarValue,
                                                     const Graphic3d_Vec2d& theMinMaxValues,
                                                     double                 theScaleCoeff);

    //! Fill transformation matrix.
    Standard_EXPORT void calculateTransformation(OpenGl_Mat4&          theResMat,
                                                 const Graphic3d_Vec3& thePos,
                                                 const Graphic3d_Vec3& theDir,
                                                 const float           theScale);

    //! Return a copy of vector property.
    Standard_EXPORT static Handle(RWCae::MeshNVecProperty) initVectorProperty(
        const Handle(RWCae::MeshNVecProperty)& theProp);

    //! Return a copy of scalar property.
    Standard_EXPORT static Handle(RWCae::MeshNVecProperty) initScalarProperty(
        const RWCae::MeshPropertyComponent& theProp);

protected:
    //! Struct for instancing element (rows of 4x3 matrix + texture coordinates).
    struct VectorAttribs {
        OpenGl_Vec4 Row1;
        OpenGl_Vec4 Row2;
        OpenGl_Vec4 Row3;
        OpenGl_Vec2 Tex;
    };

protected:
    Handle(Graphic3d_ArrayOfTriangles) myBaseGeometry;
    Handle(Graphic3d_Buffer)           myInstancingAttribs;
    Bnd_Box                            myBndBox;
};

//! OpenGl element for mesh vectors presentation.
class MeshVectorsElement : public OpenGl_Element {
public:
    //! Constructor.
    Standard_EXPORT MeshVectorsElement(const Handle(MeshVectorsPrimitiveArray)& thePrimArray);

    //! Destructor.
    Standard_EXPORT virtual ~MeshVectorsElement();

    //! Draw the element.
    Standard_EXPORT virtual void Render(const Handle(OpenGl_Workspace)& theWorkspace) const override;

    //! Release OpenGL resources.
    Standard_EXPORT virtual void Release(OpenGl_Context* theCtx) override;

    //! Returns estimated GPU memory usage for holding data without considering overheads
    //! and allocation alignment rules.
    Standard_EXPORT virtual Standard_Size EstimatedDataSize() const override;

    //! Increment draw calls statistics.
    Standard_EXPORT virtual void UpdateDrawStats(Graphic3d_FrameStatsDataTmp& theStats,
                                                 bool                         theIsDetailed) const override;

protected:
    //! Check OpenGl version and instancing availability.
    Standard_EXPORT bool checkOpenGl(const Handle(OpenGl_Context)& theCtx) const;

    //! VBO initialization procedures
    Standard_EXPORT bool buildVBO(const Handle(OpenGl_Context)& theCtx) const;

    //! Release GL memory.
    Standard_EXPORT void clearMemoryGL(const Handle(OpenGl_Context)& theGlCtx) const;

protected:
    template<int NbAttributes>
    class ArrowVertexBuffer;

protected:
    Handle(MeshVectorsPrimitiveArray)   myPrimArray;
    mutable Handle(OpenGl_IndexBuffer)  myVboIndices;
    mutable Handle(OpenGl_VertexBuffer) myVboAttribs;
    mutable Handle(OpenGl_VertexBuffer) myInstancingVboAttribs;
    mutable bool                        myIsVboInit = false;
};

//! GLSL shader for mesh vectors presentation.
class ArrowShaderManager {
public:
    static Handle(Graphic3d_ShaderProgram) ShaderProgram();
};

} // namespace

#endif // CAEMESHARROWSTOOLS_HEADERFILE