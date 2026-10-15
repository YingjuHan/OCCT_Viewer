#ifndef _INTERACTIVESHAPEOWNER_HEADERFILE
#define _INTERACTIVESHAPEOWNER_HEADERFILE

#include <ViewShape/InteractiveShapePrsMode.h>
#include <ViewShape/InteractiveShapeSelMode.h>

#include <ViewShape/SensitiveCompound.h>
#include <ViewShape/SensitivePolyEdge.h>
#include <ViewShape/SensitivePolyEdgeOnTri.h>
#include <ViewShape/SensitivePolyFace.h>
#include <ViewShape/SensitiveVertex.h>
#include <ViewShape/ShapeSelElems.h>

#include <ViewBase/InteractiveOwner.h>

#include <RWShape/ShapeData.h>

#include <AIS_InteractiveObject.hxx>
#include <OSD_FileSystem.hxx>
#include <RWMesh_NodeAttributes.hxx>
#include <TColStd_HPackedMapOfInteger.hxx>
#include <TopoDS_Compound.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>

namespace ViewShape
{

    //! Class represents owner for ViewShape::InteractiveShape
    //! and represents link between application, object, and selection.
    class InteractiveShapeOwner : public ViewBase::InteractiveOwner
    {
    public:
        //! Constructor.
        Standard_EXPORT explicit InteractiveShapeOwner(const TCollection_AsciiString &theId);

        //! Constructor.
        Standard_EXPORT explicit InteractiveShapeOwner(const Handle(RWShape::ShapeData) & theShape);

        //! Return parent.
        virtual const InteractiveOwner *Parent() const override { return myParent; }

        //! Set parent.
        void SetParent(InteractiveOwner *theOwner) { myParent = theOwner; }

        //! Return copy of this presentation; default implementation returns NULL.
        Standard_EXPORT virtual Handle(ViewBase::InteractiveOwner) Clone(const TCollection_AsciiString &theNewId) override;

        //! Create interactive object.
        Standard_EXPORT virtual Handle(AIS_InteractiveObject) CreatePresentation() const override;

        //! Highlights selectable object's presentation.
        Standard_EXPORT virtual void HilightWithColor(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                                      const Handle(Prs3d_Drawer) & theStyle,
                                                      const Standard_Integer theMode) override;

        //! Removes highlighting from the owner.
        Standard_EXPORT virtual void Unhilight(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                               const Standard_Integer theMode) override;

        //! Clears the presentation.
        Standard_EXPORT virtual void Clear(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                           const Standard_Integer theMode) override;

        //! Returns true if the presentation is highlighted.
        Standard_EXPORT virtual Standard_Boolean IsHilighted(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                                             const Standard_Integer theMode) const override;

        //! Return true if owner dynamic hilighting presentation should be re-computed on mouse move.
        virtual bool IsForcedHilight() const override { return mySelMode != (int)InteractiveShapeSelMode::Global; }

        //! Fill in local picking info.
        Standard_EXPORT virtual void LocalPickInfo(Handle(SelectMgr_LocalPickInfo) & theInfo,
                                                   const Handle(Select3D_SensitiveEntity) & theEntity) const override;

        //! Return Prs3d_Presentation for selection or highlighting.
        Standard_EXPORT virtual Handle(Prs3d_Presentation) GetHighlightPresentation(
            const Handle(PrsMgr_PresentationManager) & thePrsMgr,
            const Handle(Prs3d_Drawer) & theStyle,
            const bool theIsSelection) override;

    public:
        //! Return shape data.
        const Handle(RWShape::ShapeData) & ShapeData() const { return myShapeData; }

        //! Set shape data.
        void SetShapeData(const Handle(RWShape::ShapeData) & theData) { myShapeData = theData; }

        //! Set a single shape.
        Standard_EXPORT void SetShape(const TopoDS_Shape &theShape);

        //! Return selection shape groups.
        const std::vector<TopoDS_Compound> &ShapeGroups() const { return myShapeGroups; }

        //! Set selection shape groups.
        Standard_EXPORT void SetShapeGroups(const std::vector<TopoDS_Compound> &theGroups);

        //! Add selection shape group.
        Standard_EXPORT void AddShapeGroup(const TopoDS_Compound &theGroup);

        //! Clear caches.
        void ClearSensitivesCache() { mySensCache.clear(); }

        //! Return sensitives.
        Standard_EXPORT const std::vector<Handle(Select3D_SensitiveEntity)> &GetSensitives(
            const Handle(Prs3d_Drawer) & theDrawer);

        //! Return sensitives.
        Standard_EXPORT void GetSensitivesMap(
            NCollection_DataMap<TopoDS_Shape, Handle(Select3D_SensitiveEntity), TopTools_ShapeMapHasher> &theSensMap,
            const Handle(Prs3d_Drawer) & theDrawer);

    public:
        //! Return active presentation mode; MeshDisplayMode_SmoothShading by default.
        InteractiveShapePrsMode PresentationMode() const { return myPrsMode; }

        //! Set new presentation mode.
        void SetPresentationMode(InteractiveShapePrsMode theMode) { myPrsMode = theMode; }

        //! Check if triangulation is loaded.
        virtual bool IsLoaded() const override { return myShapeData->IsLoaded(); }

        //! Return base object attributes.
        const RWMesh_NodeAttributes &BaseAttributes() const { return myBaseAttribs; }

        //! Initialize base style.
        Standard_EXPORT void InitBaseMaterial(const Handle(Prs3d_Drawer) &theBaseDrawer = Handle(Prs3d_Drawer)());

        //! Return base object style.
        const XCAFPrs_Style &BaseStyle() const { return myBaseAttribs.Style; }

        //! Return base object style.
        XCAFPrs_Style &ChangeBaseStyle() { return myBaseAttribs.Style; }

        //! Set base object style.
        void SetBaseStyle(const XCAFPrs_Style &theStyle) { myBaseAttribs.Style = theStyle; }

        //! Return TRUE if overcolor is set; FALSE by default.
        //! Overcolor allows overriding base shape color and subshapes' colors.
        bool HasForcedColor() const { return myForcedColor.r() >= 0.0f; }

        //! Return overcolor.
        Quantity_Color GetForcedColor() const
        {
            Graphic3d_Vec3 aColor = myForcedColor.rgb().cwiseMax(Graphic3d_Vec3(0));
            return Quantity_Color(aColor);
        }

        //! Set overcolor.
        void SetForcedColor(const Quantity_Color &theColor) { myForcedColor.SetValues(theColor, myForcedColor.a()); }

        //! Reset overcolor flag.
        void UnsetForcedColor() { myForcedColor.SetValues(-1.0f, -1.0f, -1.0f, myForcedColor.a()); }

        //! Return TRUE if overtransparency is set; FALSE by default.
        //! Overtransparency allows overriding base shape transparency and subshapes' transparency.
        bool HasForcedTransparency() const { return myForcedColor.a() >= 0.0f; }

        //! Return overtransparency.
        float GetForcedTransparency() const { return HasForcedTransparency() ? myForcedColor.a() : 1.0f; }

        //! Set overtransparency.
        void SetForcedTransparency(float theTransp) { myForcedColor.a() = theTransp; }

        //! Reset overtransparency flag.
        void UnsetForcedTransparency() { myForcedColor.a() = -1.0f; }

        //! Return TRUE if line width is set; FALSE by default.
        bool HasLineWidth() const { return myLineWidth > 1.0f; }

        //! Return line width scale.
        float GetLineWidth() const { return myLineWidth; }

        //! Set line width scale.
        void SetLineWidth(const float theWidth) { myLineWidth = theWidth; }

        //! Reset line width scale.
        void UnsetLineWidth() { myLineWidth = 1.0f; }

    public:
        //! Local picking info holder.
        class ShapeLocalPickInfo;

        //! Structure to describe detected element of shape.
        struct DetectedShapeElem
        {
            int Index;
            ShapeSelElemType Type;

            DetectedShapeElem() : Index(-1), Type(ShapeSelElemType::Face) {}
            DetectedShapeElem(ShapeSelElemType theType, int theIndex) : Index(theIndex), Type(theType) {}
            void Clear() { Index = -1; }
            bool IsValid() const { return Index >= 0; }

            DetectedShapeElem &operator=(const DetectedShapeElem &theOther) = default;

            bool IsEqual(const DetectedShapeElem &theOther) const
            {
                return Index == theOther.Index && Type == theOther.Type;
            }
        };

        //! Check if selection mode is acceptable.
        virtual bool AcceptSelectionMode(const int theMode) const override
        {
            return ((unsigned int)theMode & ~InteractiveShapeSelModeMask) == 0;
        }

        //! Set new selection mode.
        Standard_EXPORT virtual void SetSelectionMode(const int theMode) override;

        //! Return TRUE if local selection mode is enabled.
        bool IsSubshapeSelection(InteractiveShapeSelMode theMode) const
        {
            return ((unsigned int)mySelMode & (unsigned int)theMode) != 0;
        }

        //! Return TRUE if local selection mode is enabled.
        bool IsSubshapeSelection() const
        {
            return IsSubshapeSelection(InteractiveShapeSelMode::Vertex) || IsSubshapeSelection(InteractiveShapeSelMode::Edge) || IsSubshapeSelection(InteractiveShapeSelMode::Wire) || IsSubshapeSelection(InteractiveShapeSelMode::Face) || IsSubshapeSelection(InteractiveShapeSelMode::Shell) || IsSubshapeSelection(InteractiveShapeSelMode::Solid) || IsSubshapeSelection(InteractiveShapeSelMode::CompSolid);
        }

        //! Return TRUE if local selection mode of edges or vertices is enabled.
        bool ToPopoutHighlight(ShapeSelElemType theType) const
        {
            return theType == ShapeSelElemType::Vertex || theType == ShapeSelElemType::Edge || theType == ShapeSelElemType::Wire;
        }

        //! Return index of last detected shape element by its type (-1 if nobody was detected).
        int GetLastDetectedElemByType(const ShapeSelElemType theType) const
        {
            return myDetElem.Type == theType ? myDetElem.Index : -1;
        }

        //! Return last detected shape element (index + type).
        const DetectedShapeElem &GetLastDetectedElem() const { return myDetElem; }

        //! Return indices map of last selected shape elements by their type.
        const Handle(TColStd_HPackedMapOfInteger) & GetSelectedElemsByType(const ShapeSelElemType theType) const
        {
            return mySelElems.Indices(theType);
        }

        //! Return array of last selected shape elements (selection type -> map of indices).
        const ShapeSelElems &GetSelectedElems() const { return mySelElems; }

        //! Set selected shape elements.
        Standard_EXPORT void SetSelectedElems(const ShapeSelElems &theElems);

        //! Clear selected shape elements.
        Standard_EXPORT void ClearSelectedElems();

        //! Return array of disabled (excluded) shape elements (selection type -> map of indices).
        //! Disabled elements couldn't be interactively picked.
        const ShapeSelElems &GetDisabledElems() const { return myDisabledElems; }

        //! Set disabled shape elements.
        Standard_EXPORT void SetDisabledElems(const ShapeSelElems &theElems);

        //! Return indices map of disabled shape elements by their type.
        const Handle(TColStd_HPackedMapOfInteger) & GetDisabledElemsByType(const ShapeSelElemType theType) const
        {
            return myDisabledElems.Indices(theType);
        }

        //! Return indices map of disabled shape elements by their type.
        Handle(TColStd_HPackedMapOfInteger) & ChangeDisabledElemsByType(const ShapeSelElemType theType)
        {
            return myDisabledElems.ChangeIndices(theType);
        }

        //! Return the index map of subshapes of specified type.
        Standard_EXPORT Handle(TColStd_HPackedMapOfInteger) GetSubshapesIndexes(
            const ShapeSelElemType theType,
            const bool theIsFree,
            const TopoDS_Shape &theParent = TopoDS_Shape(),
            const int theVisibility = -1) const;

        //! Find subshapes defined by index map of subshapes of specified type.
        Standard_EXPORT bool GetSubshapes(std::vector<RWShape::EnumeratedShape> &theShapes,
                                          const ShapeSelElemType theType,
                                          const Handle(TColStd_HPackedMapOfInteger) & theIndices) const;

        //! Find subshape of specified type and index.
        TopoDS_Shape GetSubshape(const ShapeSelElemType theType, const int theIndex) const
        {
            if (theType == ShapeSelElemType::Group)
            {
                return myShapeGroups[theIndex];
            }

            const TopAbs_ShapeEnum aShapeType = ShapeSelElemTypeToTopAbs(theType);
            return myShapeData->GetSubshape(aShapeType, theIndex);
        }

        //! Find subshapes of specified type and indices.
        Standard_EXPORT TopoDS_Shape GetSubshapesByIndexMap(const ShapeSelElems &theIndices) const;

        //! Find subshapes of specified type and indices.
        Standard_EXPORT bool GetSubshapesByIndexMap(std::array<TopTools_ListOfShape, ShapeSelElemType_NB> &theShapes,
                                                    const ShapeSelElems &theIndices) const;

        //! Return TRUE if subshape should be hidden.
        bool IsSubshapeHidden(const ShapeSelElemType theType,
                              const int theIndex,
                              const float theMinOpacity = ShortRealSmall()) const
        {
            const TopAbs_ShapeEnum aShapeType = ShapeSelElemTypeToTopAbs(theType);
            return myShapeData->IsSubshapeHidden(aShapeType, theIndex, theMinOpacity);
        }

        //! Return TRUE if subshape should be hidden.
        bool IsSubshapeHidden(const TopoDS_Shape &theShape, const float theMinOpacity = ShortRealSmall()) const
        {
            return myShapeData->IsSubshapeHidden(theShape, theMinOpacity);
        }

        //! Set subshape hidden flag. If map is NULL (not empty!),
        //! then visibility will be applied to all subshapes of specified type.
        bool SetSubshapeHidden(const ShapeSelElemType theType,
                               const Handle(TColStd_HPackedMapOfInteger) & theIndices,
                               const bool theIsHidden,
                               const bool theToInvertOthers)
        {
            const TopAbs_ShapeEnum aShapeType = ShapeSelElemTypeToTopAbs(theType);
            return myShapeData->SetSubshapeHidden(aShapeType, theIndices, theIsHidden, theToInvertOthers);
        }

        //! Return subshape color.
        bool GetSubshapeColor(Quantity_ColorRGBA &theColor, const ShapeSelElemType theType, const int theIndex) const
        {
            const TopAbs_ShapeEnum aShapeType = ShapeSelElemTypeToTopAbs(theType);
            return myShapeData->GetSubshapeColor(theColor, aShapeType, theIndex);
        }

        //! Return subshape color.
        bool GetSubshapeColor(Quantity_ColorRGBA &theColor, const TopoDS_Shape &theSubshape) const
        {
            return myShapeData->GetSubshapeColor(theColor, theSubshape);
        }

        //! Set subshape color. If map is NULL (not empty!), then color will be applied to all subshapes of specified type.
        bool SetSubshapeColor(const Quantity_ColorRGBA *theColor,
                              const ShapeSelElemType theType,
                              const Handle(TColStd_HPackedMapOfInteger) & theIndices)
        {
            const TopAbs_ShapeEnum aShapeType = ShapeSelElemTypeToTopAbs(theType);
            return myShapeData->SetSubshapeColor(theColor, aShapeType, theIndices);
        }

        //! Set subshape color.
        bool SetSubshapeColor(const TopoDS_Shape &theSubshape, const Quantity_ColorRGBA *theColor)
        {
            return myShapeData->SetSubshapeColor(theSubshape, theColor);
        }

        //! Collect subshapes sharing common edges.
        //! This function is for testing purposes only.
        Standard_EXPORT Handle(TColStd_HPackedMapOfInteger) GetConnectedSubshapes(
            const ShapeSelElemType theType,
            const Handle(TColStd_HPackedMapOfInteger) & theIndices) const;

    private:
        //! Fill in the list of sensitive triangulations.
        Standard_EXPORT void fillSensitives(const Handle(Prs3d_Drawer) & theDrawer);

        //! Fill in the list of sensitive faces.
        Standard_EXPORT void fillSensitiveFaces();

        //! Fill in the list of sensitive edges.
        Standard_EXPORT void fillSensitiveEdges(const Handle(Prs3d_Drawer) & theDrawer);

        //! Fill in the list of sensitive vertices.
        Standard_EXPORT void fillSensitiveVertices();

    private:
        //! Highlight part as whole.
        Standard_EXPORT bool hilightGlobal(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                           const Handle(Prs3d_Drawer) & theStyle,
                                           const Standard_Integer theMode,
                                           const bool theIsSelection);

        //! Display picked subshapes.
        Standard_EXPORT void displaySubshapes(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                              const Handle(Prs3d_Drawer) & theStyle,
                                              const AIS_InteractiveObject *theObj,
                                              const bool theIsSelection);

        //! Highlight picked subshapes.
        Standard_EXPORT void highlightSubshapes(const AIS_InteractiveObject *theObj,
                                                const Handle(Prs3d_Presentation) & thePrs,
                                                const Handle(Prs3d_Drawer) & theStyle,
                                                const bool theIsSelection);

        //! Highlight picked subshapes.
        Standard_EXPORT void highlightSubshapes(const Handle(Prs3d_Presentation) & thePrs,
                                                const Handle(Prs3d_Drawer) & theStyle,
                                                const std::array<TopTools_ListOfShape, ShapeSelElemType_NB> &theSelElems);

        //! Add vertex to highlight presentation.
        Standard_EXPORT bool hilightVertices(const TopoDS_Shape &theVertices,
                                             const Handle(Prs3d_Presentation) & thePrs,
                                             const Handle(Prs3d_Drawer) & theStyle);

        //! Add edge to highlight presentation.
        Standard_EXPORT bool hilightEdges(const TopoDS_Shape &theEdges,
                                          const Handle(Prs3d_Presentation) & thePrs,
                                          const Handle(Prs3d_Drawer) & theStyle);

        //! Add face to highlight presentation
        Standard_EXPORT bool hilightFaces(const TopoDS_Shape &theFaces,
                                          const Handle(Prs3d_Presentation) & thePrs,
                                          const Handle(Prs3d_Drawer) & theStyle);

    private:
        //! Compute highlighting presentation.
        Standard_EXPORT void detectSubshapes(const Handle(AIS_InteractiveContext) & theCtx,
                                             DetectedShapeElem *theDetElem,
                                             ShapeSelElems *theSelElems) const;

        //! Compute highlighting presentation.
        Standard_EXPORT void detectEntitySubshapes(const Handle(Select3D_SensitiveEntity) & theEntity,
                                                   DetectedShapeElem *theDetElem,
                                                   ShapeSelElems *theSelElems) const;

        //! Add vertex to highlight presentation.
        Standard_EXPORT bool detectClosestVertex(const TopoDS_Shape &theDetShape,
                                                 const TopAbs_ShapeEnum theAvoidType,
                                                 const gp_Pnt &thePickPoint,
                                                 const double thePickTol,
                                                 TopoDS_Vertex &theResVertex);

        //! Detect highlighting presentation for detected triangulation.
        Standard_EXPORT bool detectFaceSubshape(const Handle(SensitivePolyFace) & theDetTris,
                                                DetectedShapeElem *theDetElem,
                                                ShapeSelElems *theSelElems) const;

        //! Detect highlighting presentation for detected subshape.
        Standard_EXPORT bool detectSomeSubshape(const TopoDS_Shape &theDetShape,
                                                const int theShapeIndex,
                                                DetectedShapeElem *theDetElem,
                                                ShapeSelElems *theSelElems) const;

    private:
        InteractiveOwner *myParent = nullptr;

        Handle(RWShape::ShapeData) myShapeData; //!< main shape data

        std::vector<TopoDS_Compound> myShapeGroups; //!< list of selectable groups of subshapes

        RWMesh_NodeAttributes myBaseAttribs;               //!< base object style
        Graphic3d_Vec4 myForcedColor = Graphic3d_Vec4(-1); //!< temporary color overriding all subcolors
        float myLineWidth = 1.0f;

        InteractiveShapePrsMode myPrsMode = InteractiveShapePrsMode::Shaded; //!< active presentation mode

        mutable std::vector<Handle(Select3D_SensitiveEntity)> mySensCache; //!< cached sensitives for selection

        mutable Handle(Prs3d_Presentation) mySelPrs; //!< optional presentation for highlighting selected owner

        DetectedShapeElem myDetElem;   //!< last detected shape element
        ShapeSelElems mySelElems;      //!< last selected shape elements
        ShapeSelElems myDisabledElems; //!< disabled shape elements
    };

    //! Class represents a list of shape owners.
    class InteractiveShapeListOwner : public ViewBase::InteractiveOwner
    {
    public:
        //! List of owners.
        typedef NCollection_List<Handle(InteractiveShapeOwner)> ShapeOwnerList;

    public:
        //! Constructor.
        Standard_EXPORT InteractiveShapeListOwner();

        //! Return children iterator.
        virtual OwnerList::Iterator ChildIterator() const override { return OwnerList::Iterator(myChildren); }

        //! Return children.
        const ShapeOwnerList &Children() const { return myChildren; }

        //! Add child.
        Standard_EXPORT bool AddChild(const Handle(InteractiveShapeOwner) & theOwner);

        //! Remove child.
        Standard_EXPORT bool RemoveChild(const Handle(InteractiveShapeOwner) & theOwner);

        //! Return active presentation mode.
        InteractiveShapePrsMode PresentationMode() const { return myPrsMode; }

        //! Set new presentation mode.
        void SetPresentationMode(InteractiveShapePrsMode theMode)
        {
            myPrsMode = theMode;
            for (const Handle(ViewShape::InteractiveShapeOwner) &anOwnerIter : myChildren)
            {
                anOwnerIter->SetPresentationMode(theMode);
            }
        }

        //! Return number of triangles.
        int NbTriangles() const
        {
            int aNbTris = 0;
            for (const Handle(ViewShape::InteractiveShapeOwner) &anOwnerIter : myChildren)
            {
                aNbTris += anOwnerIter->ShapeData()->NbTriangles();
            }
            return aNbTris;
        }

        //! Create interactive object.
        Standard_EXPORT virtual Handle(AIS_InteractiveObject) CreatePresentation() const override;

        //! Check if selection mode is acceptable.
        virtual bool AcceptSelectionMode(const int theMode) const override
        {
            return ((unsigned int)theMode & ~InteractiveShapeSelModeMask) == 0;
        }

    private:
        ShapeOwnerList myChildren;
        InteractiveShapePrsMode myPrsMode = InteractiveShapePrsMode::Shaded; //!< active presentation mode
    };

} // namespace

#endif // _INTERACTIVESHAPEOWNER_HEADERFILE
