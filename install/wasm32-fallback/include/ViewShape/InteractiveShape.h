#ifndef VIEWSHAPE_INTERACTIVESHAPE_HEADERFILE
#define VIEWSHAPE_INTERACTIVESHAPE_HEADERFILE

#include <ViewShape/InteractiveShapeOwner.h>
#include <ViewShape/InteractiveShapePrsMode.h>

#include <ViewBase/IInteractiveObject.h>
#include <ViewBase/InteractiveOwner.h>

#include <AIS_InteractiveObject.hxx>
#include <AIS_ColoredShape.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Graphic3d_AspectLine3d.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_PointAspect.hxx>
#include <TColStd_IndexedDataMapOfStringString.hxx>
#include <TopoDS_Compound.hxx>
#include <XCAFDoc_VisMaterial.hxx>

//! @namespace ViewShape
//! `ViewShape` is a C++ library implementing presentations and tools for B-Rep geometry and assembly structures.
//!
//! The main presentation classes are `ViewShape::InteractiveShape` / `ViewShape::InteractiveShapeOwner`.
//!
//! The data source of B-Rep definition is defined by `TopoDS_Shape` class from OCCT.
//!
//! Native file formats supported for loading data in 3D Viewer:
//! - **glTF**, [open ISO-standardized file format](https://www.khronos.org/gltf/)
//!   supporting triangulated surfaces, assembly structures and PBR materials;
//! - **BREP**, [open file format](https://dev.opencascade.org/doc/overview/html/specification__brep_format.html)
//!   supporting OCCT B-Rep geometry+topology definition.
//!   Binary and text versions of file format are supported for reading.
namespace ViewShape
{

    //! Per-material aspects.
    struct InteractiveShapeMaterial
    {
        Handle(Prs3d_ShadingAspect) FillArea; //!< shaded faces
        Handle(Prs3d_LineAspect) FaceBounds;  //!< face bounds
        Handle(Prs3d_LineAspect) FreeWires;   //!< free wires
        Handle(Prs3d_PointAspect) FreePoints; //!< free vertices

        //! Reset to defaults.
        void Reset(const Handle(Prs3d_Drawer) & theDrawer);

        //! Update shading aspect.
        Standard_EXPORT void UpdateShading(const XCAFPrs_Style &theStyle, const Handle(XCAFDoc_VisMaterial) & theDefMat);

        //! Set PBR metallic property to aspects.
        Standard_EXPORT void SetPbrMetallic(const float theMetallic);

        //! Set PBR roughness property to aspects.
        Standard_EXPORT void SetPbrRoughness(const float theRoughness);

        //! Set PBR transparency property to aspects.
        Standard_EXPORT void SetPbrTransparency(const float theTransp);

        //! Show/hide face bounds.
        void SetFaceBoundsVisible(bool theToShow)
        {
            FaceBounds->Aspect()->SetLineType(theToShow ? Aspect_TOL_SOLID : Aspect_TOL_EMPTY);
        }

        //! Set color to face boundaries.
        void SetFaceBoundsColor(const Quantity_Color &theColor) { FaceBounds->SetColor(theColor); }

        //! Set color to free wires.
        void SetFreeWiresColor(const Quantity_Color &theColor) { FreeWires->SetColor(theColor); }

        //! Set color to free points.
        void SetFreePointsColor(const Quantity_Color &theColor) { FreePoints->SetColor(theColor); }

        //! Set line width scale.
        void SetLineWidth(const float theWidth)
        {
            FaceBounds->SetWidth(theWidth);
            FreeWires->SetWidth(theWidth);
        }
    };

    typedef NCollection_DataMap<XCAFPrs_Style, InteractiveShapeMaterial, XCAFPrs_Style> InteractiveShapeMaterialMap;

    //! Interactive object implementing presentation of pre-tessellated B-Rep shape with sub-shape materials.
    class InteractiveShape : public AIS_ColoredShape, public ViewBase::IInteractiveObject
    {
    public:
        //! Gets triangulation of every face of shape and fills output array of triangles.
        Standard_EXPORT static Handle(Graphic3d_ArrayOfTriangles) FillTriangles(const Handle(RWShape::ShapeData) & theData,
                                                                                const TopoDS_Shape &theShape,
                                                                                const bool theHasTexels);

    public:
        //! Constructor from single owner.
        Standard_EXPORT explicit InteractiveShape(const Handle(ViewShape::InteractiveShapeOwner) & theNewId);

        //! Constructor from owner list.
        Standard_EXPORT explicit InteractiveShape(const Handle(ViewShape::InteractiveShapeListOwner) & theOwnerList);

        //! Destructor.
        Standard_EXPORT virtual ~InteractiveShape();

        //! Return list of owners.
        const InteractiveShapeListOwner::ShapeOwnerList &ShapeOwners() const { return myShapeListOwner->Children(); }

        //! Add shape owner.
        Standard_EXPORT void AddOwner(const Handle(ViewShape::InteractiveShapeOwner) & theOwner);

        //! Remove shape owner.
        Standard_EXPORT void RemoveOwner(const Handle(ViewShape::InteractiveShapeOwner) & theOwner);

        //! Return the mesh owner of current interactive object
        const Handle(ViewShape::InteractiveShapeOwner) & MeshOwner() const { return myShapeListOwner->Children().First(); }

        //! Return global owner.
        Standard_EXPORT virtual Handle(SelectMgr_EntityOwner) GlobalSelOwner() const override;

        //! Return material map
        const InteractiveShapeMaterialMap &Materials() const { return myMaterialMap; }

        //! Return active presentation mode; MeshDisplayMode_SmoothShading by default.
        Standard_EXPORT InteractiveShapePrsMode PresentationMode() const;

        //! Set new presentation mode.
        Standard_EXPORT void SetPresentationMode(InteractiveShapePrsMode theMode);

        //! Update current state.
        Standard_EXPORT virtual void UpdateAspects() override;

        //! Check if it is selectable.
        bool IsSelectable() const { return myIsSelectable; }

        //! Make selectable.
        void SetSelectable(bool theSelectable) { myIsSelectable = theSelectable; }

        //! Set forced closed state.
        void SetForcedClosed(bool theClosed)
        {
            if (myIsClosed == theClosed)
            {
                return;
            }
            myIsClosed = theClosed;
            updateClosed();
        }

        //! Check if object is model or not.
        bool IsModel() const { return dynamic_cast<ViewBase::InteractiveOwner *>(GetOwner().get()) != nullptr; }

        //! Return object id or empty string.
        TCollection_AsciiString ModelId() const
        {
            const ViewBase::InteractiveOwner *anId = dynamic_cast<ViewBase::InteractiveOwner *>(GetOwner().get());
            return anId != nullptr ? anId->Id() : TCollection_AsciiString();
        }

        //! Return name of this type.
        virtual const char *TypeName() const override { return "InteractiveShape"; }

    public:
        //! Sets the material aMat defining this display attribute for the interactive object.
        //! Material aspect determines shading aspect, color and transparency of visible entities.
        Standard_EXPORT virtual void SetMaterial(const Graphic3d_MaterialAspect &theMat) override;

        //! Removes the setting for material.
        Standard_EXPORT virtual void UnsetMaterial() override;

        //! Returns the transparency.
        Standard_EXPORT virtual Standard_Real Transparency() const override;

        //! Sets global transparency overrider, replacing custom materials.
        Standard_EXPORT virtual void SetTransparency(const Standard_Real theValue) override;

        //! Removes global transparency overrider.
        Standard_EXPORT virtual void UnsetTransparency() override;

        //! Returns global color.
        Standard_EXPORT virtual void Color(Quantity_Color &theColor) const override;

        //! Sets global color overrider, replacing custom materials.
        Standard_EXPORT virtual void SetColor(const Quantity_Color &theColor) override;

        //! Removes global color overrider.
        Standard_EXPORT virtual void UnsetColor() override;

        //! Sets global width overrider.
        Standard_EXPORT virtual void SetWidth(const Standard_Real theLineWidth) override;

        //! Sets global width overrider.
        Standard_EXPORT virtual void UnsetWidth() override;

    public:
        //! Return TRUE for supported display mode.
        virtual Standard_Boolean AcceptDisplayMode(const Standard_Integer theMode) const override { return theMode == 0; }

        //! Compute 3D part of View Cube.
        //! @param[in] thePrsMgr presentation manager.
        //! @param[in] thePrs    input presentation that is to be filled with flat presentation primitives.
        //! @param[in] theMode   display mode.
        //! @warning this object accept only 0 display mode.
        Standard_EXPORT virtual void Compute(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                             const Handle(Prs3d_Presentation) & thePrs,
                                             const Standard_Integer theMode) override;

        //! Redefine computing of sensitive entities for View Cube.
        //! @param[in] theSel input selection object that is to be filled with sensitive entities.
        //! @param[in] theMode selection mode.
        //! @warning object accepts only 0 selection mode.
        Standard_EXPORT virtual void ComputeSelection(const Handle(SelectMgr_Selection) & theSel,
                                                      const Standard_Integer theMode) override;

    private:
        using DataMapOfStyleShape = NCollection_DataMap<XCAFPrs_Style, TopoDS_Shape, XCAFPrs_Style>;

        //! Return TRUE if shaded faces should be computed within display mode.
        static bool toComputeShadedFaces(InteractiveShapePrsMode theMode)
        {
            return theMode != InteractiveShapePrsMode::Wireframe;
        }

        //! Return TRUE if face boundaries should be computed within display mode.
        static bool toComputeFaceBoundaries(InteractiveShapePrsMode theMode)
        {
            return theMode == InteractiveShapePrsMode::Wireframe || theMode == InteractiveShapePrsMode::HiddenLine || theMode == InteractiveShapePrsMode::ShadedFaceBoundaries || theMode == InteractiveShapePrsMode::ShadedTessellation;
        }

        //! Update AIS_ColoredShape map of colors.
        Standard_EXPORT void updateShapeColorsMap(const DataMapOfStyleShape &theMatMap);

        //! Define material map.
        Standard_EXPORT void initMaterialMap();

        //! Add Material
        const InteractiveShapeMaterial &addMaterial(const XCAFPrs_Style &theStyle);

        //! Get default PBR material.
        const Handle(XCAFDoc_VisMaterial) & getDefaultMat() const;

        //! Sets Aspect depends on current InteractiveShapePrsMode.
        void updateAspects(const XCAFPrs_Style &theStyle,
                           const Handle(XCAFDoc_VisMaterial) & theDefMat,
                           const Handle(V3d_View) & theView,
                           InteractiveShapeMaterial &theAspect);

        //! update closed
        void updateClosed();

        //! Return type of the most complex free subshape.
        TopAbs_ShapeEnum getTopFreeShapeType() const;

        //! Update display and selection priorities.
        Standard_EXPORT void updatePriorities(const Handle(Prs3d_Presentation) & thePrs);

        //! Compute seam edges.
        Standard_EXPORT static Handle(Graphic3d_ArrayOfSegments) fillSeamEdges(const TopoDS_Shape &theShape,
                                                                               GeomAbs_Shape theUpperContinuity);

        //! Compute tessellation edges.
        Standard_EXPORT static Handle(Graphic3d_ArrayOfSegments) fillTessEdges(const TopoDS_Shape &theShape);

    private:
        InteractiveShape() = delete;
        InteractiveShape(const InteractiveShape &) = delete;
        InteractiveShape &operator=(const InteractiveShape &) = delete;

        class SeamEdgeExplorer;
        class SelectionBuilder;
        struct MaterialMapFiller;

    private:
        Handle(ViewShape::InteractiveShapeListOwner) myShapeListOwner;
        InteractiveShapeMaterialMap myMaterialMap; //!< shapeMaterialMap

        Handle(Graphic3d_AspectLine3d) myTessAsp;

        Standard_Boolean myIsSelectable = true; //!< is selectable or not
        Standard_Boolean myIsClosed = false;    //!< make closed or not.
        Standard_Boolean myToCompShaded = false;
        Standard_Boolean myToCompLines = false;
        Standard_Boolean myToCompTess = false;
    };

} // namespace

#endif // VIEWSHAPE_INTERACTIVESHAPE_HEADERFILE
