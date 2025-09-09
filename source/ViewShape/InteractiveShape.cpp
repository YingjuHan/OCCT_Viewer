#include <ViewShape/InteractiveShape.h>

#include <ViewShape/InteractiveShapeOwner.h>
#include <ViewShape/InteractiveShapeDrawer.h>

#include <ViewBase/BaseDefaultDrawer.h>
#include <ViewBase/PrsTools.h>

#include <AIS_InteractiveContext.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_TypeOfShadingModel.hxx>
#include <Message.hxx>
#include <NCollection_IncAllocator.hxx>
#include <Poly.hxx>
#include <Prs3d.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <StdPrs_ShadedShape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp_DfsExplorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

namespace ViewShape
{

    //! Set display priority to presentation structure.
    static void setPrsDisplayPriority(const Handle(Prs3d_Presentation) & thePrs,
                                      Graphic3d_DisplayPriority thePriority,
                                      bool theIsHighlighted)
    {
        if (!theIsHighlighted)
        {
            thePrs->SetDisplayPriority(thePriority);
        }
        else
        {
            thePrs->CStructure()->SetPreviousPriority(thePriority);
        }
    }

    //! Return default color.
    static const Quantity_ColorRGBA &defaultShapeColor()
    {
        static const Quantity_ColorRGBA aColor(0.243137f, 0.243137f, 0.243137f, 1.0f);
        return aColor;
    }

    //! Return TRUE if shape requires a complex wireframe presentation.
    static bool needsComplexWireframe(const TopoDS_Shape &theShape)
    {
        if (theShape.IsNull())
        {
            return false;
        }

        for (TopExp_Explorer aFaceExp(theShape, TopAbs_FACE); aFaceExp.More(); aFaceExp.Next())
        {
            if (BRep_Tool::IsGeometric(TopoDS::Face(aFaceExp.Current())))
            {
                return true;
            }
        }
        return false;
    }

    //! Iterator over seam edges in the map of edges.
    class InteractiveShape::SeamEdgeExplorer
    {
    public:
        //! Main constructor.
        SeamEdgeExplorer(const TopTools_IndexedDataMapOfShapeListOfShape &theEdgesMap,
                         const GeomAbs_Shape theUpperContinuity)
            : m_iterator(theEdgesMap),
              m_upperContinuity(theUpperContinuity)
        {
            next();
        }

        //! Return TRUE if iterator points to a valid item.
        bool More() const { return m_edgePoly != nullptr && !m_edgePoly->IsNull(); }

        //! Go to the next seam edge.
        void Next()
        {
            if (m_iterator.More())
            {
                m_iterator.Next();
            }
            next();
        }

        //! Return current location.
        const TopLoc_Location &Location() const { return m_location; }

        //! Return current triangulation.
        const Handle(Poly_Triangulation) & Triangulation() const { return *m_tris; }

        //! Return current polygon.
        const Handle(Poly_PolygonOnTriangulation) & Polygon() const { return *m_edgePoly; }

    private:
        //! Go to the next seam edge.
        void next()
        {
            static constexpr int nbFacePair2 = 2;

            m_tris = nullptr;
            m_edgePoly = nullptr;
            for (; m_iterator.More(); m_iterator.Next())
            {
                if (m_iterator.Value().Extent() != nbFacePair2)
                {
                    continue;
                }

                const TopoDS_Face &aFace1 = TopoDS::Face(m_iterator.Value().First());
                m_tris = &BRep_Tool::Triangulation(aFace1, m_location);
                if (m_tris->IsNull())
                {
                    continue;
                }

                const TopoDS_Face &aFace2 = TopoDS::Face(m_iterator.Value().Last());
                const TopoDS_Edge &anEdge = TopoDS::Edge(m_iterator.Key());
                if (BRep_Tool::Continuity(anEdge, aFace1, aFace2) <= m_upperContinuity)
                {
                    continue;
                }

                m_edgePoly = &BRep_Tool::PolygonOnTriangulation(anEdge, *m_tris, m_location);
                if (m_edgePoly->IsNull() || (*m_edgePoly)->Nodes().Length() <= 1)
                {
                    continue;
                }

                break;
            }
        }

    private:
        TopTools_IndexedDataMapOfShapeListOfShape::Iterator m_iterator;
        TopLoc_Location m_location;
        const Handle(Poly_Triangulation) *m_tris = nullptr;
        const Handle(Poly_PolygonOnTriangulation) *m_edgePoly = nullptr;

        GeomAbs_Shape m_upperContinuity;
    };

    // ================================================================
    // Function : fillSeamEdges
    // ================================================================
    Handle(Graphic3d_ArrayOfSegments) InteractiveShape::fillSeamEdges(const TopoDS_Shape &theShape,
                                                                      GeomAbs_Shape theUpperContinuity)
    {
        if (theUpperContinuity >= GeomAbs_CN)
        {
            return Handle(Graphic3d_ArrayOfSegments)();
        }

        // collection of all triangulation nodes on edges
        // for computing boundaries presentation
        int aNodeNumber = 0;
        int aNbPolylines = 0;

        // explore all boundary edges
        TopTools_IndexedDataMapOfShapeListOfShape anEdgesMap;
        TopExp::MapShapesAndAncestors(theShape, TopAbs_EDGE, TopAbs_FACE, anEdgesMap);
        for (SeamEdgeExplorer anEdgeIter(anEdgesMap, theUpperContinuity); anEdgeIter.More(); anEdgeIter.Next())
        {
            const Handle(Poly_PolygonOnTriangulation) &anEdgePoly = anEdgeIter.Polygon();
            aNodeNumber += anEdgePoly->Nodes().Length();
            ++aNbPolylines;
        }
        if (aNodeNumber == 0)
        {
            return Handle(Graphic3d_ArrayOfSegments)();
        }

        // create indexed segments array to pack polylines from different edges into single array
        const int aSegmentEdgeNb = (aNodeNumber - aNbPolylines) * 2;
        Handle(Graphic3d_ArrayOfSegments) aSegments = new Graphic3d_ArrayOfSegments(aNodeNumber, aSegmentEdgeNb);
        for (SeamEdgeExplorer anEdgeIter(anEdgesMap, theUpperContinuity); anEdgeIter.More(); anEdgeIter.Next())
        {
            const Handle(Poly_Triangulation) &aTriangulation = anEdgeIter.Triangulation();
            const Handle(Poly_PolygonOnTriangulation) &anEdgePoly = anEdgeIter.Polygon();
            const TColStd_Array1OfInteger &anEdgeNodes = anEdgePoly->Nodes();

            // collect the edge nodes
            int aSegmentEdge = aSegments->VertexNumber() + 1;
            for (int aNodeIdx = anEdgeNodes.Lower(); aNodeIdx <= anEdgeNodes.Upper(); ++aNodeIdx)
            {
                // node index in face triangulation
                // get node and apply location transformation to the node
                const int aTriIndex = anEdgeNodes.Value(aNodeIdx);
                Standard_ASSERT(aTriIndex >= 1 && aTriIndex <= aTriangulation->NbNodes(),
                                "Invalid PolygonOnTriangulation",
                                break);

                gp_Pnt aTriNode = aTriangulation->Node(aTriIndex);
                if (!anEdgeIter.Location().IsIdentity())
                {
                    aTriNode.Transform(anEdgeIter.Location());
                }

                aSegments->AddVertex(aTriNode);
                if (aNodeIdx != anEdgeNodes.Lower())
                {
                    aSegments->AddEdge(aSegmentEdge);
                    aSegments->AddEdge(++aSegmentEdge);
                }
            }
        }

        return aSegments;
    }

    // ================================================================
    // Function : fillTessEdges
    // ================================================================
    Handle(Graphic3d_ArrayOfSegments) InteractiveShape::fillTessEdges(const TopoDS_Shape &theShape)
    {
        static constexpr int nbSNodes2 = 2;
        static constexpr int nbTNodes3 = 3;

        int aNbTris = 0;
        int aNbNodes = 0;
        TopLoc_Location aLoc;
        for (TopExp_Explorer aFaceIter(theShape, TopAbs_FACE); aFaceIter.More(); aFaceIter.Next())
        {
            const TopoDS_Face &aFace = TopoDS::Face(aFaceIter.Value());
            if (const Handle(Poly_Triangulation) &aTris = BRep_Tool::Triangulation(aFace, aLoc))
            {
                aNbTris += aTris->NbTriangles();
                aNbNodes += aTris->NbNodes();
            }
        }

        const int aNbIndices = (nbTNodes3 * aNbTris) * nbSNodes2;
        if (aNbIndices == 0)
        {
            return Handle(Graphic3d_ArrayOfSegments)();
        }

        Handle(Graphic3d_ArrayOfSegments) aSegs = new Graphic3d_ArrayOfSegments(aNbNodes,
                                                                                aNbIndices,
                                                                                Graphic3d_ArrayFlags_None);

        for (TopExp_Explorer aFaceIter(theShape, TopAbs_FACE); aFaceIter.More(); aFaceIter.Next())
        {
            const TopoDS_Face &aFace = TopoDS::Face(aFaceIter.Value());
            const Handle(Poly_Triangulation) &aTris = BRep_Tool::Triangulation(aFace, aLoc);
            if (aTris.IsNull())
            {
                continue;
            }

            const int aFirstNode = aSegs->VertexNumber() + 1;
            for (int aNodeIter = 1; aNodeIter <= aTris->NbNodes(); ++aNodeIter)
            {
                gp_Pnt aNode = aTris->Node(aNodeIter);
                aNode.Transform(aLoc.Transformation());
                aSegs->AddVertex(aNode);
            }

            Handle(NCollection_IncAllocator) anAllocator = new NCollection_IncAllocator();

            NCollection_Map<Graphic3d_Vec2i, ViewBase::PrsTools::SegmentsHasher> aPairsMap(aTris->NbTriangles(),
                                                                                           anAllocator);
            for (int aTriIter = 1; aTriIter <= aTris->NbTriangles(); ++aTriIter)
            {
                const Poly_Triangle &aTri = aTris->Triangle(aTriIter);

                Graphic3d_Vec2i aSeg1(aTri.Value(1), aTri.Value(2));
                if (aPairsMap.Add(aSeg1))
                    aSegs->AddEdges(aFirstNode + aSeg1[0] - 1, aFirstNode + aSeg1[1] - 1);

                Graphic3d_Vec2i aSeg2(aTri.Value(2), aTri.Value(3));
                if (aPairsMap.Add(aSeg2))
                    aSegs->AddEdges(aFirstNode + aSeg2[0] - 1, aFirstNode + aSeg2[1] - 1);

                Graphic3d_Vec2i aSeg3(aTri.Value(3), aTri.Value(1));
                if (aPairsMap.Add(aSeg3))
                    aSegs->AddEdges(aFirstNode + aSeg3[0] - 1, aFirstNode + aSeg3[1] - 1);
            }
        }
        return aSegs;
    }

    // ================================================================
    // Function : InteractiveShape
    // ================================================================
    InteractiveShape::InteractiveShape(const Handle(ViewShape::InteractiveShapeOwner) & theNewId)
        : AIS_ColoredShape(TopoDS_Shape())
    {
        myDrawer->SetAutoTriangulation(false);
        myDrawer->SetHiddenLineAspect(new Prs3d_LineAspect(Quantity_NOC_GRAY90, Aspect_TOL_DASH, 1.0f));
        myDrawer->HiddenLineAspect()->SetColor(defaultShapeColor().GetRGB());

        myTessAsp = new Graphic3d_AspectLine3d(Quantity_NOC_GRAY60, Aspect_TOL_SOLID, 1.0f);

        myShapeListOwner = new ViewShape::InteractiveShapeListOwner();
        myOwner = myShapeListOwner;
        AddOwner(theNewId);

        myOwnWidth = theNewId->GetLineWidth();
    }

    // ================================================================
    // Function : InteractiveShape
    // ================================================================
    InteractiveShape::InteractiveShape(const Handle(ViewShape::InteractiveShapeListOwner) & theOwnerList)
        : AIS_ColoredShape(TopoDS_Shape())
    {
        myDrawer->SetAutoTriangulation(false);
        myDrawer->SetHiddenLineAspect(new Prs3d_LineAspect(Quantity_NOC_GRAY90, Aspect_TOL_DASH, 1.0f));
        myDrawer->HiddenLineAspect()->SetColor(defaultShapeColor().GetRGB());

        myTessAsp = new Graphic3d_AspectLine3d(Quantity_NOC_GRAY60, Aspect_TOL_SOLID, 1.0f);

        myShapeListOwner = theOwnerList;
        myOwner = theOwnerList;
        theOwnerList->SetSelectablePointer(this);
        for (const Handle(ViewShape::InteractiveShapeOwner) &anOwnerIter : ShapeOwners())
        {
            anOwnerIter->SetSelectablePointer(this);
        }

        myOwnWidth = !theOwnerList->Children().IsEmpty() ? theOwnerList->Children().First()->GetLineWidth() : 1.0f;
    }

    // ================================================================
    // Function : ~InteractiveShape
    // ================================================================
    InteractiveShape::~InteractiveShape()
    {
        //
    }

    // ================================================================
    // Function : GlobalSelOwner
    // ================================================================
    Handle(SelectMgr_EntityOwner) InteractiveShape::GlobalSelOwner() const
    {
        if (myShapeListOwner->Children().Size() == 1)
        {
            Handle(ViewShape::InteractiveShapeOwner) anOwner = myShapeListOwner->Children().First();
            return (Handle(SelectMgr_EntityOwner))anOwner;
        }
        return Handle(SelectMgr_EntityOwner)();
    }

    // ================================================================
    // Function : AddOwner
    // ================================================================
    void InteractiveShape::AddOwner(const Handle(ViewShape::InteractiveShapeOwner) & theOwner)
    {
        if (!myShapeListOwner->AddChild(theOwner))
        {
            return;
        }
        theOwner->SetSelectablePointer(this);
    }

    // ================================================================
    // Function : RemoveOwner
    // ================================================================
    void InteractiveShape::RemoveOwner(const Handle(ViewShape::InteractiveShapeOwner) & theOwner)
    {
        Handle(ViewShape::InteractiveShapeOwner) anOwner = theOwner;
        if (!myShapeListOwner->RemoveChild(theOwner))
        {
            return;
        }
        theOwner->SetSelectablePointer(nullptr);
    }

    // ================================================================
    // Function : updateShapeColorsMap
    // ================================================================
    void InteractiveShape::updateShapeColorsMap(const DataMapOfStyleShape &theMatMap)
    {
        BRep_Builder aBuilder;
        TopoDS_Compound aComp;
        aBuilder.MakeCompound(aComp);
        for (const Handle(ViewShape::InteractiveShapeOwner) &anOwnerIter : ShapeOwners())
        {
            for (const TopoDS_Shape &aRootIter : anOwnerIter->ShapeData()->Shapes())
            {
                aBuilder.Add(aComp, aRootIter);
            }
        }
        myshape = aComp;
        myShapeColors.Clear();

        for (DataMapOfStyleShape::Iterator aMatIter(theMatMap); aMatIter.More(); aMatIter.Next())
        {
            const XCAFPrs_Style &aStyle = aMatIter.Key();
            const TopoDS_Shape &aShape = aMatIter.Value();

            Handle(AIS_ColoredDrawer) aDrawer = new AIS_ColoredDrawer(myDrawer);
            myShapeColors.Bind(aShape, aDrawer);
            aDrawer->SetHidden(!aStyle.IsVisible());
            aDrawer->SetOwnMaterial();
            aDrawer->SetOwnColor(Quantity_Color());

            const InteractiveShapeMaterial &anAspects = addMaterial(aStyle);
            aDrawer->SetShadingAspect(anAspects.FillArea);
            aDrawer->SetFaceBoundaryAspect(anAspects.FaceBounds);
            aDrawer->SetWireAspect(anAspects.FreeWires);
            aDrawer->SetPointAspect(anAspects.FreePoints);

            aDrawer->SetLineAspect(anAspects.FaceBounds);
            aDrawer->SetFreeBoundaryAspect(anAspects.FaceBounds);
            aDrawer->SetUnFreeBoundaryAspect(anAspects.FaceBounds);
        }
    }

    struct InteractiveShape::MaterialMapFiller
    {
        RWMesh_NodeAttributes DefAttribs;
        DataMapOfStyleShape MatMap;

        void Init(const Handle(ViewShape::InteractiveShapeListOwner) & theList)
        {
            for (const Handle(ViewShape::InteractiveShapeOwner) &anOwnerIter : theList->Children())
            {
                for (const TopoDS_Shape &aRootIter : anOwnerIter->ShapeData()->Shapes())
                {
                    AddFiltered(anOwnerIter, aRootIter);
                }
            }
        }

    private:
        //! Merge two styles in assembly structure.
        static void MergeStyles(XCAFPrs_Style &theStyle, const XCAFPrs_Style &theOther)
        {
            if (!theOther.IsVisible())
            {
                theStyle.SetVisibility(false);
            }
            if (!theOther.Material().IsNull())
            {
                theStyle.SetMaterial(theOther.Material());
            }
            if (theOther.IsSetColorSurf())
            {
                theStyle.SetColorSurf(theOther.GetColorSurfRGBA());
            }
            if (theOther.IsSetColorCurv())
            {
                theStyle.SetColorCurv(theOther.GetColorCurv());
            }
        }

        //! Add shape.
        void Add(const Handle(ViewShape::InteractiveShapeOwner) & theOwner,
                 const TopoDS_Shape &theShape,
                 const XCAFPrs_Style &theStyle,
                 const TopAbs_ShapeEnum theType,
                 const TopAbs_ShapeEnum theAvoidType)
        {
            if ((int)theShape.ShapeType() > (int)theType || theShape.ShapeType() == theAvoidType)
            {
                return;
            }

            const RWMesh_NodeAttributes *anAttribs = theOwner->ShapeData()->AttributeMap().Seek(theShape);
            if (anAttribs == nullptr || anAttribs->Style.IsEmpty())
            {
                anAttribs = theOwner->ShapeData()->AttributeMap().Seek(theShape.Located(TopLoc_Location()));
            }
            if (anAttribs == nullptr || anAttribs->Style.IsEmpty())
            {
                anAttribs = !theOwner->BaseAttributes().Style.IsEmpty() ? &theOwner->BaseAttributes() : nullptr;
            }

            XCAFPrs_Style aStyle = theStyle;
            if (anAttribs != nullptr)
            {
                MergeStyles(aStyle, anAttribs->Style);
            }

            if ((int)theShape.ShapeType() < (int)theType)
            {
                for (const TopoDS_Shape &aSubIter : theShape)
                {
                    Add(theOwner, aSubIter, aStyle, theType, theAvoidType);
                }
                return;
            }

            TopoDS_Shape *aVal = this->MatMap.ChangeSeek(aStyle);
            if (aVal == nullptr)
            {
                TopoDS_Compound aComp;
                BRep_Builder().MakeCompound(aComp);
                aVal = this->MatMap.Bound(aStyle, aComp);
            }
            BRep_Builder().Add(*aVal, theShape);
        }

        //! Explore shape into faces and add to filter.
        void AddFiltered(const Handle(ViewShape::InteractiveShapeOwner) & theOwner, const TopoDS_Shape &theRoot)
        {
            constexpr TopAbs_ShapeEnum aShapeTypes[3] = {TopAbs_FACE, TopAbs_EDGE, TopAbs_VERTEX};
            constexpr TopAbs_ShapeEnum anIgnoreTypes[3] = {TopAbs_SHAPE, TopAbs_FACE, TopAbs_EDGE};

            XCAFPrs_Style aStyle = this->DefAttribs.Style;
            for (size_t aTypeIter = 0; aTypeIter < sizeof(aShapeTypes) / sizeof(*aShapeTypes); ++aTypeIter)
            {
                const TopAbs_ShapeEnum aShapeType = aShapeTypes[aTypeIter];
                const TopAbs_ShapeEnum aTypeIgnore = anIgnoreTypes[aTypeIter];
                Add(theOwner, theRoot, aStyle, aShapeType, aTypeIgnore);
            }
        }
    };

    // ================================================================
    // Function : initMaterialMap
    // ================================================================
    void InteractiveShape::initMaterialMap()
    {
        MaterialMapFiller aMatFiller;
        aMatFiller.Init(myShapeListOwner);
        updateShapeColorsMap(aMatFiller.MatMap);
        UpdateAspects();
    }

    // ================================================================
    // Function : Compute
    // ================================================================
    void InteractiveShape::Compute(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                   const Handle(Prs3d_Presentation) & thePrs,
                                   const Standard_Integer theMode)
    {
        thePrs->SetInfiniteState(MeshOwner()->IsInfiniteState());
        if (!AcceptDisplayMode(theMode))
        {
            return;
        }
        else if (!myShapeListOwner->IsLoaded())
        {
            Message::SendFail() << "Object '" << ModelId() << "' wasn't loaded";
            return;
        }

        myMaterialMap.Clear();

        // compute minimal presentation first time
        myToCompShaded = myToCompShaded || toComputeShadedFaces(PresentationMode());
        myToCompLines = myToCompLines || toComputeFaceBoundaries(PresentationMode());
        myToCompTess = myToCompTess || PresentationMode() == InteractiveShapePrsMode::ShadedTessellation;

        initMaterialMap();

        myTessAsp->SetColor(myDrawer->ShadingAspect()->Aspect()->EdgeColor());
        myTessAsp->SetLineType(PresentationMode() == InteractiveShapePrsMode::ShadedTessellation ? Aspect_TOL_SOLID
                                                                                                 : Aspect_TOL_EMPTY);
        if (myToCompTess)
        {
            Handle(Graphic3d_ArrayOfSegments) aMeshEdges = fillTessEdges(myshape);
            if (!aMeshEdges.IsNull())
            {
                Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
                aGroup->SetGroupPrimitivesAspect(myTessAsp);
                aGroup->AddPrimitiveArray(aMeshEdges);
            }
        }

        myDrawer->SetFaceBoundaryDraw(myToCompLines);
        AIS_ColoredShape::Compute(thePrsMgr, thePrs, myToCompShaded ? AIS_Shaded : AIS_WireFrame);

        // compute seam edges presentation independently to show it in Wireframe mode only
        myDrawer->HiddenLineAspect()->SetColor(hasOwnColor ? myDrawer->Color() : defaultShapeColor().GetRGB());
        myDrawer->HiddenLineAspect()->SetTypeOfLine(
            PresentationMode() == InteractiveShapePrsMode::Wireframe ? Aspect_TOL_DASH : Aspect_TOL_EMPTY);
        if (myToCompLines)
        {
            Handle(Graphic3d_ArrayOfSegments) aSeams = fillSeamEdges(myshape, myDrawer->FaceBoundaryUpperContinuity());
            if (!aSeams.IsNull())
            {
                Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
                aGroup->SetGroupPrimitivesAspect(myDrawer->HiddenLineAspect()->Aspect());
                aGroup->AddPrimitiveArray(aSeams);
            }
        }

        updatePriorities(thePrs);

        // flush computed primitive arrays to VBOs to free cpu memory
        for (const auto &aGroup : thePrs->Groups())
        {
            aGroup->FlushBuffers();
        }
    }

    // ================================================================
    // Function : updatePriorities
    // ================================================================
    void InteractiveShape::updatePriorities(const Handle(Prs3d_Presentation) & thePrs)
    {
        const bool isHighlighted = thePrs->IsHighlighted();
        const TopAbs_ShapeEnum aTopFreeShapeType = getTopFreeShapeType();
        const int aPriorityModif = MeshOwner()->PriorityModifier(); // should be the same for all owners
        for (const Handle(ViewShape::InteractiveShapeOwner) &anOwnerIter : ShapeOwners())
        {
            SensitiveShape::SensPriority anOwnerPriority = SensitiveShape::SensPriority::Global;
            switch (aTopFreeShapeType)
            {
            case TopAbs_FACE:
                anOwnerPriority = SensitiveShape::SensPriority::Face;
                break;
            case TopAbs_EDGE:
                anOwnerPriority = SensitiveShape::SensPriority::Edge;
                break;
            case TopAbs_VERTEX:
                anOwnerPriority = SensitiveShape::SensPriority::Vertex;
                break;
            default:
                break;
            }
            int aSelPriority = Max(ViewBase::MinOwnerSelectionPriority,
                                   Min(ViewBase::MaxOwnerSelectionPriority, (int)anOwnerPriority + aPriorityModif));
            anOwnerIter->SetPriority(aSelPriority);
        }

        Graphic3d_DisplayPriority aPrsPriority = Graphic3d_DisplayPriority_Normal;
        switch (aTopFreeShapeType)
        {
        case TopAbs_FACE:
            aPrsPriority = Graphic3d_DisplayPriority_Above1;
            break;
        case TopAbs_EDGE:
            aPrsPriority = Graphic3d_DisplayPriority_Above2;
            break;
        case TopAbs_VERTEX:
            aPrsPriority = Graphic3d_DisplayPriority_Highlight;
            break;
        default:
            break;
        }
        aPrsPriority = (Graphic3d_DisplayPriority)Max(Graphic3d_DisplayPriority_Bottom,
                                                      Min(Graphic3d_DisplayPriority_Topmost,
                                                          (int)aPrsPriority + aPriorityModif));
        setPrsDisplayPriority(thePrs, aPrsPriority, isHighlighted);
    }

    //! Tool for filling in selection entities.
    class InteractiveShape::SelectionBuilder
    {
    public:
        //! Empty constructor.
        SelectionBuilder() {}

        //! Fill in selection.
        void ComputeSelection(const Handle(SelectMgr_Selection) & theSel,
                              const Handle(ViewShape::InteractiveShapeOwner) & theOwner,
                              const Handle(Prs3d_Drawer) & theDrawer)
        {
            const std::vector<Handle(Select3D_SensitiveEntity)> &aSensList = theOwner->GetSensitives(theDrawer);
            if (aSensList.empty())
            {
                return;
            }

            m_compShapeCounter.fill(0);

            // implicitly disable incompatible selection modes
            m_selMask = (unsigned int)theOwner->GetSelectionMode();
            if ((m_selMask & (unsigned int)InteractiveShapeSelMode::Solid) != 0)
            {
                m_selMask = m_selMask & ~(unsigned int)InteractiveShapeSelMode::CompSolid;
            }
            if ((m_selMask & (unsigned int)InteractiveShapeSelMode::Face) != 0)
            {
                m_selMask = m_selMask & ~(unsigned int)InteractiveShapeSelMode::Shell & ~(unsigned int)InteractiveShapeSelMode::Solid & ~(unsigned int)InteractiveShapeSelMode::CompSolid;
            }
            if ((m_selMask & (unsigned int)InteractiveShapeSelMode::Edge) != 0)
            {
                m_selMask = m_selMask & ~(unsigned int)InteractiveShapeSelMode::Wire;
            }

            if (theOwner->ShapeData()->AttributeMap().IsEmpty() && m_selMask == (unsigned int)InteractiveShapeSelMode::Global)
            {
                // skip extra filtering for global object picking,
                // if there are no extra subshape attributes (as some subshape might be hidden)
                Handle(SensitiveCompound) aSensShape = new SensitiveCompound(TopoDS_Shape(), theOwner, aSensList);
                theSel->Add(aSensShape);
                return;
            }

            theOwner->GetSensitivesMap(m_sensMap, theDrawer);

            // add subshapes of groups
            AddGroupsSubshapes(theOwner);

            // add other subshapes
            AddSubshapes(theOwner);

            Handle(SensitiveCompound) aSensShape = new SensitiveCompound(TopoDS_Shape(), theOwner, m_sensListComp);
            theSel->Add(aSensShape);
        }

    private:
        //! Collect sensitives for sub shapes.
        void CollectSubshapesSens(const Handle(ViewShape::InteractiveShapeOwner) & theOwner,
                                  const TopoDS_Shape &theShape,
                                  const TopAbs_ShapeEnum theSubShapeType,
                                  std::vector<Handle(Select3D_SensitiveEntity)> &theSubSens)
        {
            for (m_subShapeExp.Init(theShape); m_subShapeExp.More(); m_subShapeExp.Next())
            {
                const TopoDS_Shape &aSubShape = m_subShapeExp.Current();
                if (theOwner->IsSubshapeHidden(aSubShape))
                {
                    m_subShapeExp.SetExploreCurrent(false);
                    continue;
                }

                m_subShapeExp.SetExploreCurrent(aSubShape.ShapeType() < theSubShapeType);
                if (aSubShape.ShapeType() != theSubShapeType)
                {
                    continue;
                }

                Handle(Select3D_SensitiveEntity) aSens;
                if (m_sensMap.Find(aSubShape, aSens))
                {
                    theSubSens.push_back(aSens);
                    m_filter.Add(aSens);
                }
            }
        }

        //! Add subshapes of groups.
        void AddGroupsSubshapes(const Handle(ViewShape::InteractiveShapeOwner) & theOwner)
        {
            if ((m_selMask & (unsigned int)InteractiveShapeSelMode::Group) == 0)
            {
                return;
            }

            for (int aGroupIter = 0; aGroupIter < (int)theOwner->ShapeGroups().size(); ++aGroupIter)
            {
                const TopoDS_Compound &aGroup = theOwner->ShapeGroups()[aGroupIter];

                std::vector<Handle(Select3D_SensitiveEntity)> aSubSubSens;

                TopTools_IndexedMapOfShape aSubShapeMap;
                for (TopoDS_Iterator aCompIter(aGroup); aCompIter.More(); aCompIter.Next()) {
                    const TopoDS_Shape& aSubShape = aCompIter.Value();
                    aSubShapeMap.Add(aSubShape);
                }

                for (TopTools_IndexedMapOfShape::Iterator aSubShapeIter(aSubShapeMap); aSubShapeIter.More(); aSubShapeIter.Next())
                {
                    const TopoDS_Shape &aSubShape = aSubShapeIter.Value();
                    TopAbs_ShapeEnum aSubType = aSubShape.ShapeType() == TopAbs_VERTEX
                                                    ? TopAbs_VERTEX
                                                    : (aSubShape.ShapeType() <= TopAbs_FACE ? TopAbs_FACE : TopAbs_EDGE);
                    CollectSubshapesSens(theOwner, aSubShape, aSubType, aSubSubSens);
                }

                Handle(SensitiveCompound) aSensGroup = new SensitiveCompound(aGroup, theOwner, aSubSubSens);
                aSensGroup->SetShapeIndex(aGroupIter);
                m_sensListComp.push_back(aSensGroup);
            }
        }

        //! Add subshapes of complex subshapes
        void AddComplexSubshapesT(const Handle(ViewShape::InteractiveShapeOwner) & theOwner, const TopoDS_Shape &theShape)
        {
            const int anIndex = m_compShapeCounter[theShape.ShapeType()]++;

            const TopAbs_ShapeEnum aSubSubShapeType = theShape.ShapeType() == TopAbs_WIRE ? TopAbs_EDGE : TopAbs_FACE;

            std::vector<Handle(Select3D_SensitiveEntity)> aSubSubSens;
            CollectSubshapesSens(theOwner, theShape, aSubSubShapeType, aSubSubSens);
            if (aSubSubSens.empty())
            {
                return;
            }

            Handle(SensitiveCompound) aSensWire = new SensitiveCompound(theShape, theOwner, aSubSubSens);
            aSensWire->SetShapeIndex(anIndex);
            m_sensListComp.push_back(aSensWire);
        }

        //! Return TRUE if currently iterate sub-shape is a free VERTEX, free EDGE, or any FACE.
        bool IsCurrentFreeSubshape(const TopExp_DfsExplorer &theExp, const TopAbs_ShapeEnum theType) const
        {
            if (theType == TopAbs_VERTEX)
            {
                return theExp.Depth() < 1 || theExp.CurrentType(theExp.Depth() - 1) != TopAbs_EDGE;
            }
            else if (theType == TopAbs_EDGE)
            {
                return theExp.Depth() < 2 || theExp.CurrentType(theExp.Depth() - 2) != TopAbs_FACE;
            }
            else if (theType == TopAbs_FACE)
            {
                return true;
            }
            return false;
        }

        //! Add simple subshapes for all roots.
        void AddSubshapes(const Handle(ViewShape::InteractiveShapeOwner) & theOwner)
        {
            for (const TopoDS_Shape &aRootIter : theOwner->ShapeData()->Shapes())
            {
                AddSubshapes(theOwner, aRootIter);
            }
        }

        //! Add simple subshapes of specified root.
        void AddSubshapes(const Handle(ViewShape::InteractiveShapeOwner) & theOwner, const TopoDS_Shape &theRoot)
        {
            int aMaxExpandType = TopAbs_FACE;
            int aMaxExpandCompType = TopAbs_COMPOUND;
            if ((m_selMask & (unsigned int)InteractiveShapeSelMode::Vertex) != 0)
            {
                aMaxExpandType = aMaxExpandCompType = TopAbs_VERTEX;
            }
            else if ((m_selMask & (unsigned int)InteractiveShapeSelMode::Edge) != 0)
            {
                aMaxExpandType = aMaxExpandCompType = TopAbs_EDGE;
            }
            else if ((m_selMask & (unsigned int)InteractiveShapeSelMode::Wire) != 0)
            {
                aMaxExpandType = TopAbs_WIRE;
            }

            for (m_rootExp.Init(theRoot); m_rootExp.More(); m_rootExp.Next())
            {
                const TopoDS_Shape &aShape = m_rootExp.Current();
                if (theOwner->IsSubshapeHidden(aShape))
                {
                    m_rootExp.SetExploreCurrent(false);
                    continue;
                }

                const TopAbs_ShapeEnum aType = aShape.ShapeType();
                m_rootExp.SetExploreCurrent(aType < aMaxExpandType);

                const bool isSel = IsSubshapeTypeSel(aType) || (IsGlobalSel() && IsCurrentFreeSubshape(m_rootExp, aType));
                if (!isSel || aType == TopAbs_COMPOUND)
                {
                    continue;
                }

                if (aType != TopAbs_FACE && aType != TopAbs_EDGE && aType != TopAbs_VERTEX)
                {
                    m_rootExp.SetExploreCurrent(aType < aMaxExpandCompType);
                    AddComplexSubshapesT(theOwner, aShape);
                    continue;
                }

                const Handle(Select3D_SensitiveEntity) *aSens = m_sensMap.Seek(aShape);
                if (aSens != nullptr && !m_filter.Contains(*aSens))
                {
                    m_sensListComp.push_back(*aSens);
                }
            }
        }

        //! Return TRUE if global selection mode is active.
        bool IsGlobalSel() const { return m_selMask == (unsigned int)InteractiveShapeSelMode::Global; }

        //! Return TRUE if selection mask includes specified shape type.
        bool IsSubshapeTypeSel(TopAbs_ShapeEnum theType) const
        {
            const InteractiveShapeSelMode aSelMode = ShapeSelModeFromTopAbs(theType);
            return (m_selMask & (unsigned int)aSelMode) != 0;
        }

        //! Return selection mode from TopAbs shape type.
        static InteractiveShapeSelMode ShapeSelModeFromTopAbs(const TopAbs_ShapeEnum theType)
        {
            // @c return ShapeElemTypeToSelMode(ShapeSelElemTypeFromTopAbs(theType));
            switch (theType)
            {
            case TopAbs_VERTEX:
                return InteractiveShapeSelMode::Vertex;
            case TopAbs_EDGE:
                return InteractiveShapeSelMode::Edge;
            case TopAbs_WIRE:
                return InteractiveShapeSelMode::Wire;
            case TopAbs_FACE:
                return InteractiveShapeSelMode::Face;
            case TopAbs_SHELL:
                return InteractiveShapeSelMode::Shell;
            case TopAbs_SOLID:
                return InteractiveShapeSelMode::Solid;
            case TopAbs_COMPSOLID:
                return InteractiveShapeSelMode::CompSolid;
            case TopAbs_COMPOUND:
                return InteractiveShapeSelMode::Group;
            default:
                return InteractiveShapeSelMode::Global;
            }
        }

    private:
        NCollection_DataMap<TopoDS_Shape, Handle(Select3D_SensitiveEntity), TopTools_ShapeMapHasher> m_sensMap;

        std::vector<Handle(Select3D_SensitiveEntity)> m_sensListComp;
        NCollection_Map<Handle(Select3D_SensitiveEntity)> m_filter;

        std::array<int, TopAbs_SHAPE> m_compShapeCounter;
        TopExp_DfsExplorer m_rootExp;
        TopExp_DfsExplorer m_subShapeExp;

        unsigned int m_selMask = 0;
    };

    // ================================================================
    // Function : ComputeSelection
    // ================================================================
    void InteractiveShape::ComputeSelection(const Handle(SelectMgr_Selection) & theSel, const Standard_Integer theMode)
    {
        if (theMode != 0 || !myShapeListOwner->IsLoaded())
        {
            return;
        }

        for (const Handle(ViewShape::InteractiveShapeOwner) &anOwnerIter : ShapeOwners())
        {
            SelectionBuilder aBuilder;
            aBuilder.ComputeSelection(theSel, anOwnerIter, myDrawer);
        }
    }

    // =======================================================================
    // function : getDefaultMat
    // =======================================================================
    const Handle(XCAFDoc_VisMaterial) & InteractiveShape::getDefaultMat() const
    {
        ViewBase::BaseDefaultDrawer *aDefDrawer = HasInteractiveContext() ? dynamic_cast<ViewBase::BaseDefaultDrawer *>(
                                                                                InteractiveContext()->DefaultDrawer().get())
                                                                          : nullptr;
        if (aDefDrawer == nullptr)
        {
            static const Handle(XCAFDoc_VisMaterial) aDefMat;

            return aDefMat;
        }
        return aDefDrawer->VisMaterial();
    }

    // =======================================================================
    // function : addMaterial
    // =======================================================================
    const InteractiveShapeMaterial &InteractiveShape::addMaterial(const XCAFPrs_Style &theStyle)
    {
        InteractiveShapeMaterial *aVal = myMaterialMap.ChangeSeek(theStyle);
        if (aVal != nullptr)
        {
            return *aVal;
        }

        aVal = myMaterialMap.Bound(theStyle, InteractiveShapeMaterial());
        aVal->Reset(myDrawer);
        return *aVal;
    }

    // ================================================================
    // Function : SetMaterial
    // ================================================================
    void InteractiveShape::SetMaterial(const Graphic3d_MaterialAspect &theMat)
    {
        const Graphic3d_PBRMaterial &aMatFrom = theMat.PBRMaterial();
        XCAFDoc_VisMaterialPBR aPbrMat;
        aPbrMat.IsDefined = true;
        aPbrMat.BaseColor = aMatFrom.Color();
        aPbrMat.Metallic = aMatFrom.Metallic();
        aPbrMat.EmissiveFactor = aMatFrom.Emission();
        aPbrMat.RefractionIndex = aMatFrom.IOR();
        aPbrMat.Roughness = aMatFrom.Roughness();

        bool toUpdate = false;
        for (const Handle(ViewShape::InteractiveShapeOwner) &anOwnerIter : ShapeOwners())
        {
            if (anOwnerIter->BaseStyle().Material().IsNull())
            {
                toUpdate = true;
                anOwnerIter->ChangeBaseStyle().SetMaterial(new XCAFDoc_VisMaterial());
            }
            anOwnerIter->BaseStyle().Material()->SetPbrMaterial(aPbrMat);
        }
        if (toUpdate)
        {
            SetToUpdate();
        }
        UpdateAspects();
    }

    // ================================================================
    // Function : UnsetMaterial
    // ================================================================
    void InteractiveShape::UnsetMaterial()
    {
        bool toUpdate = false;
        for (const Handle(ViewShape::InteractiveShapeOwner) &anOwnerIter : ShapeOwners())
        {
            if (!anOwnerIter->BaseStyle().Material().IsNull())
            {
                anOwnerIter->ChangeBaseStyle().SetMaterial(Handle(XCAFDoc_VisMaterial)());
                toUpdate = true;
            }
        }
        if (toUpdate)
        {
            SetToUpdate();
            UpdateAspects();
        }
    }

    // ================================================================
    // Function : Transparency
    // ================================================================
    Standard_Real InteractiveShape::Transparency() const
    {
        return !MeshOwner()->BaseStyle().Material().IsNull()
                   ? 1.0f - MeshOwner()->BaseStyle().Material()->PbrMaterial().BaseColor.Alpha()
                   : 0.0f;
    }

    // ================================================================
    // Function : SetTransparency
    // ================================================================
    void InteractiveShape::SetTransparency(const Standard_Real theValue)
    {
        const float aNewTransp = float(theValue);
        for (const Handle(ViewShape::InteractiveShapeOwner) &anOwnerIter : ShapeOwners())
        {
            anOwnerIter->InitBaseMaterial();

            XCAFDoc_VisMaterialPBR aPbrMat = anOwnerIter->BaseStyle().Material()->PbrMaterial();
            aPbrMat.BaseColor.SetAlpha(1.0f - aNewTransp);
            anOwnerIter->ChangeBaseStyle().Material()->SetPbrMaterial(aPbrMat);
        }
        UpdateAspects();
    }

    // ================================================================
    // Function : UnsetTransparency
    // ================================================================
    void InteractiveShape::UnsetTransparency()
    {
        bool toUpdate = false;
        for (const Handle(ViewShape::InteractiveShapeOwner) &anOwnerIter : ShapeOwners())
        {
            if (anOwnerIter->BaseStyle().Material().IsNull())
            {
                continue;
            }

            toUpdate = true;
            XCAFDoc_VisMaterialPBR aPbrMat = anOwnerIter->BaseStyle().Material()->PbrMaterial();
            if (const Handle(XCAFDoc_VisMaterial) &aDefMat = getDefaultMat())
            {
                aPbrMat.BaseColor.SetAlpha(aDefMat->PbrMaterial().BaseColor.Alpha());
            }
            anOwnerIter->ChangeBaseStyle().Material()->SetPbrMaterial(aPbrMat);
        }
        if (toUpdate)
        {
            UpdateAspects();
        }
    }

    // ================================================================
    // Function : Color
    // ================================================================
    void InteractiveShape::Color(Quantity_Color &theColor) const
    {
        if (!MeshOwner()->BaseStyle().Material().IsNull())
        {
            theColor = MeshOwner()->BaseStyle().Material()->PbrMaterial().BaseColor.GetRGB();
        }
        else if (const Handle(XCAFDoc_VisMaterial) &aDefMat = getDefaultMat())
        {
            theColor = aDefMat->PbrMaterial().BaseColor.GetRGB();
        }
    }

    // ================================================================
    // Function : SetColor
    // ================================================================
    void InteractiveShape::SetColor(const Quantity_Color &theColor)
    {
        hasOwnColor = true;
        myDrawer->SetColor(theColor);
        for (const Handle(ViewShape::InteractiveShapeOwner) &anOwnerIter : ShapeOwners())
        {
            anOwnerIter->InitBaseMaterial();

            XCAFDoc_VisMaterialPBR aPbrMat = anOwnerIter->BaseStyle().Material()->PbrMaterial();
            aPbrMat.BaseColor.SetRGB(theColor);
            anOwnerIter->ChangeBaseStyle().Material()->SetPbrMaterial(aPbrMat);
        }
        UpdateAspects();
    }

    // ================================================================
    // Function : UnsetColor
    // ================================================================
    void InteractiveShape::UnsetColor()
    {
        hasOwnColor = false;

        bool toUpdate = false;
        for (const Handle(ViewShape::InteractiveShapeOwner) &anOwnerIter : ShapeOwners())
        {
            if (anOwnerIter->BaseStyle().Material().IsNull())
            {
                continue;
            }

            toUpdate = true;

            XCAFDoc_VisMaterialPBR aPbrMat = anOwnerIter->BaseStyle().Material()->PbrMaterial();
            if (const Handle(XCAFDoc_VisMaterial) &aDefMat = getDefaultMat())
            {
                aPbrMat.BaseColor.SetRGB(aDefMat->PbrMaterial().BaseColor.GetRGB());
            }
            anOwnerIter->ChangeBaseStyle().Material()->SetPbrMaterial(aPbrMat);
        }
        if (toUpdate)
        {
            UpdateAspects();
        }
    }

    // ================================================================
    // Function : SetWidth
    // ================================================================
    void InteractiveShape::SetWidth(const Standard_Real theLineWidth)
    {
        hasOwnColor = true;
        myOwnWidth = float(theLineWidth);
        for (const Handle(ViewShape::InteractiveShapeOwner) &anOwnerIter : ShapeOwners())
        {
            anOwnerIter->SetLineWidth(myOwnWidth);
        }
        UpdateAspects();
    }

    // ================================================================
    // Function : UnsetWidth
    // ================================================================
    void InteractiveShape::UnsetWidth()
    {
        SetWidth(1.0);
    }

    // ================================================================
    // Function : Reset
    // ================================================================
    void InteractiveShapeMaterial::Reset(const Handle(Prs3d_Drawer) & theDrawer)
    {
        if (FillArea.IsNull())
        {
            FillArea = new Prs3d_ShadingAspect();
            FaceBounds = new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 1.0f);
            FreeWires = new Prs3d_LineAspect(Quantity_NOC_RED, Aspect_TOL_SOLID, 1.0f);
            FreePoints = new Prs3d_PointAspect(Aspect_TOM_PLUS, Quantity_NOC_YELLOW, 1.0f);
        }

        *FillArea->Aspect() = *theDrawer->ShadingAspect()->Aspect();
        *FaceBounds->Aspect() = *theDrawer->FaceBoundaryAspect()->Aspect();
        *FreeWires->Aspect() = *theDrawer->WireAspect()->Aspect();
        *FreePoints->Aspect() = *theDrawer->PointAspect()->Aspect();
    }

    // ================================================================
    // Function : UpdateShading
    // ================================================================
    void InteractiveShapeMaterial::UpdateShading(const XCAFPrs_Style &theStyle,
                                                 const Handle(XCAFDoc_VisMaterial) & theDefMat)
    {
        const Handle(Graphic3d_Aspects) &aFillAsp = FillArea->Aspect();
        if (!theStyle.Material().IsNull())
        {
            theStyle.Material()->FillAspect(aFillAsp);
            if (aFillAsp->FrontMaterial().Alpha() <= 0.96f && (aFillAsp->FrontMaterial().PBRMaterial().Metallic() - 1.0f) < Precision::Confusion())
            {
                // workaround bad transparency
                SetPbrTransparency(1.0f - aFillAsp->FrontMaterial().Alpha());
            }
        }
        else if (!theDefMat.IsNull())
        {
            theDefMat->FillAspect(aFillAsp);
        }
        if (theStyle.IsSetColorSurf())
        {
            aFillAsp->ChangeFrontMaterial().SetColor(theStyle.GetColorSurf());
            aFillAsp->ChangeBackMaterial().SetColor(theStyle.GetColorSurf());
            if (theStyle.GetColorSurfRGBA().Alpha() <= 0.96f)
            {
                SetPbrTransparency(1.0f - theStyle.GetColorSurfRGBA().Alpha());
            }
        }
    }

    // ================================================================
    // Function : SetPbrMetallic
    // ================================================================
    void InteractiveShapeMaterial::SetPbrMetallic(const float theMetallic)
    {
        const Handle(Graphic3d_Aspects) &aFillAsp = FillArea->Aspect();
        {
            Graphic3d_PBRMaterial aPbrFront = aFillAsp->FrontMaterial().PBRMaterial();
            aPbrFront.SetMetallic(theMetallic);
            aFillAsp->ChangeFrontMaterial().SetPBRMaterial(aPbrFront);
        }
        {
            Graphic3d_PBRMaterial aPbrBack = aFillAsp->BackMaterial().PBRMaterial();
            aPbrBack.SetMetallic(theMetallic);
            aFillAsp->ChangeBackMaterial().SetPBRMaterial(aPbrBack);
        }
    }

    // ================================================================
    // Function : SetPbrRoughness
    // ================================================================
    void InteractiveShapeMaterial::SetPbrRoughness(const float theRoughness)
    {
        const Handle(Graphic3d_Aspects) &aFillAsp = FillArea->Aspect();
        {
            Graphic3d_PBRMaterial aPbrFront = aFillAsp->FrontMaterial().PBRMaterial();
            aPbrFront.SetRoughness(theRoughness);
            aFillAsp->ChangeFrontMaterial().SetPBRMaterial(aPbrFront);
        }
        {
            Graphic3d_PBRMaterial aPbrBack = aFillAsp->BackMaterial().PBRMaterial();
            aPbrBack.SetRoughness(theRoughness);
            aFillAsp->ChangeBackMaterial().SetPBRMaterial(aPbrBack);
        }
    }

    // ================================================================
    // Function : SetPbrTransparency
    // ================================================================
    void InteractiveShapeMaterial::SetPbrTransparency(const float theTransp)
    {
        const Handle(Graphic3d_Aspects) &aFillAsp = FillArea->Aspect();
        aFillAsp->SetAlphaMode(Graphic3d_AlphaMode_Blend);
        {
            aFillAsp->ChangeFrontMaterial().SetTransparency(theTransp);
            Graphic3d_PBRMaterial aPbrFront = aFillAsp->FrontMaterial().PBRMaterial();
            aPbrFront.SetMetallic(0.0f);
            aFillAsp->SetInteriorColor(Quantity_ColorRGBA(aPbrFront.Color().GetRGB(), aPbrFront.Color().Alpha()));
            aFillAsp->ChangeFrontMaterial().SetPBRMaterial(aPbrFront);
        }
        {
            aFillAsp->ChangeBackMaterial().SetTransparency(theTransp);
            Graphic3d_PBRMaterial aPbrBack = aFillAsp->BackMaterial().PBRMaterial();
            aPbrBack.SetMetallic(0.0f);
            aFillAsp->SetBackInteriorColor(Quantity_ColorRGBA(aPbrBack.Color().GetRGB(), aPbrBack.Color().Alpha()));
            aFillAsp->ChangeBackMaterial().SetPBRMaterial(aPbrBack);
        }
    }

    // =======================================================================
    // function : updateAspects
    // =======================================================================
    void InteractiveShape::updateAspects(const XCAFPrs_Style &theStyle,
                                         const Handle(XCAFDoc_VisMaterial) & theDefMat,
                                         const Handle(V3d_View) & theView,
                                         InteractiveShapeMaterial &theAspect)
    {
        theAspect.Reset(myDrawer);
        theAspect.UpdateShading(theStyle, theDefMat);

        const Handle(ViewShape::InteractiveShapeOwner) &anOwner = MeshOwner(); // to be mapped into Style
        const Handle(Graphic3d_Aspects) &aFillAsp = theAspect.FillArea->Aspect();
        // global overriders
        if (anOwner->HasForcedColor())
        {
            aFillAsp->ChangeFrontMaterial().SetColor(anOwner->GetForcedColor());
            aFillAsp->ChangeBackMaterial().SetColor(anOwner->GetForcedColor());
        }
        if (anOwner->HasForcedTransparency())
        {
            theAspect.SetPbrTransparency(anOwner->GetForcedTransparency());
        }
        theAspect.SetLineWidth(anOwner->GetLineWidth());

        theAspect.SetFreePointsColor(aFillAsp->FrontMaterial().DiffuseColor());
        theAspect.SetFreeWiresColor(aFillAsp->FrontMaterial().DiffuseColor());
        if (PresentationMode() == InteractiveShapePrsMode::Wireframe || myShapeListOwner->NbTriangles() == 0)
        {
            theAspect.SetFaceBoundsColor(aFillAsp->FrontMaterial().DiffuseColor());
        }

        if (PresentationMode() == InteractiveShapePrsMode::ShadedFlat)
        {
            aFillAsp->SetShadingModel(Graphic3d_TOSM_PBR_FACET);
        }

        const bool isInvisible = anOwner->HasForcedTransparency() && anOwner->GetForcedTransparency() >= 0.99f;
        if (isInvisible)
        {
            aFillAsp->SetInteriorStyle(Aspect_IS_EMPTY);
        }
        else if (PresentationMode() == InteractiveShapePrsMode::HiddenLine)
        {
            Quantity_Color aHlrColor = (theView.IsNull() || theView->BackgroundColor().Light() < 0.5) ? Quantity_NOC_WHITE
                                                                                                      : Quantity_NOC_BLACK;
            aFillAsp->SetInteriorStyle(Aspect_IS_HIDDENLINE);
            aFillAsp->SetDrawSilhouette(true);
            aFillAsp->SetEdgeColor(aHlrColor);
            theAspect.SetFaceBoundsColor(aHlrColor);
        }
        else
        {
            aFillAsp->SetInteriorStyle(PresentationMode() != InteractiveShapePrsMode::Wireframe ? Aspect_IS_SOLID
                                                                                                : Aspect_IS_EMPTY);
        }

        const bool toShowLines = toComputeFaceBoundaries(PresentationMode()) || myShapeListOwner->NbTriangles() == 0;
        theAspect.SetFaceBoundsVisible(toShowLines && !isInvisible);
    }

    // ================================================================
    // Function : PresentationMode
    // ================================================================
    InteractiveShapePrsMode InteractiveShape::PresentationMode() const
    {
        return myShapeListOwner->PresentationMode();
    }

    // ================================================================
    // Function : SetPresentationMode
    // ================================================================
    void InteractiveShape::SetPresentationMode(InteractiveShapePrsMode theMode)
    {
        const InteractiveShapePrsMode anOldMode = PresentationMode();
        if (anOldMode == theMode)
        {
            return;
        }

        // compute minimal presentation first time, but cache both sub-presentations next time for faster switching;
        // set InteractiveShapePrsMode_EdgeShading for first display to ensure full presentation being cached
        bool toShowShaded = myToCompShaded || toComputeShadedFaces(theMode);
        bool toShowLines = myToCompLines || toComputeFaceBoundaries(theMode);
        bool toCompTess = myToCompTess || theMode == InteractiveShapePrsMode::ShadedTessellation;

        if ((theMode == InteractiveShapePrsMode::Wireframe || anOldMode == InteractiveShapePrsMode::Wireframe) && needsComplexWireframe(myshape))
        {
            SetToUpdate(); // disable optimization - isolines should be handled  specifically
            toShowShaded = toComputeShadedFaces(theMode);
            toShowLines = toComputeFaceBoundaries(theMode);
        }

        myShapeListOwner->SetPresentationMode(theMode);
        if (toShowShaded != myToCompShaded || toShowLines != myToCompLines || toCompTess != myToCompTess)
        {
            myToCompShaded = toShowShaded;
            myToCompLines = toShowLines;
            myToCompTess = toCompTess;
            SetToUpdate();
        }

        UpdateAspects();
    }

    // =======================================================================
    // function : UpdateAspects
    // =======================================================================
    void InteractiveShape::UpdateAspects()
    {
        myDrawer->HiddenLineAspect()->SetColor(hasOwnColor ? myDrawer->Color() : defaultShapeColor().GetRGB());
        myDrawer->HiddenLineAspect()->SetTypeOfLine(
            PresentationMode() == InteractiveShapePrsMode::Wireframe ? Aspect_TOL_DASH : Aspect_TOL_EMPTY);

        myTessAsp->SetColor(myDrawer->ShadingAspect()->Aspect()->EdgeColor());
        myTessAsp->SetLineType(PresentationMode() == InteractiveShapePrsMode::ShadedTessellation ? Aspect_TOL_SOLID
                                                                                                 : Aspect_TOL_EMPTY);

        const Handle(V3d_View) &aView = GetVisibleInAnyView(this);

        const Handle(XCAFDoc_VisMaterial) &aDefMat = getDefaultMat();
        for (InteractiveShapeMaterialMap::Iterator aMatIter(myMaterialMap); aMatIter.More(); aMatIter.Next())
        {
            updateAspects(aMatIter.Key(), aDefMat, aView, aMatIter.ChangeValue());
        }
        SynchronizeAspects();
    }

    // =======================================================================
    // function : updateClosed
    // =======================================================================
    void InteractiveShape::updateClosed()
    {
        const bool isClosed = myIsClosed || PresentationMode() == InteractiveShapePrsMode::HiddenLine;
        for (const Handle(PrsMgr_Presentation) &aPrsIter : Presentations())
        {
            for (const Handle(Graphic3d_Group) &aGroupIt : aPrsIter->Groups())
            {
                if (const Graphic3d_AspectFillArea3d *aFillArea = dynamic_cast<const Graphic3d_AspectFillArea3d *>(
                        aGroupIt->Aspects().get()))
                {
                    (void)aFillArea;
                    aGroupIt->SetClosed(isClosed);
                }
            }
        }
    }

    // =======================================================================
    // function : getTopFreeShapeType
    // =======================================================================
    TopAbs_ShapeEnum InteractiveShape::getTopFreeShapeType() const
    {
        constexpr TopAbs_ShapeEnum aShapeTypes[4] = {TopAbs_SOLID, TopAbs_FACE, TopAbs_EDGE, TopAbs_VERTEX};
        constexpr TopAbs_ShapeEnum anIgnoreTypes[4] = {TopAbs_SHAPE, TopAbs_SOLID, TopAbs_FACE, TopAbs_EDGE};

        TopExp_Explorer anExp;

        auto findTopType = [&anExp](const Handle(InteractiveShapeOwner) & theOwner,
                                    const TopAbs_ShapeEnum theShapeType,
                                    const TopAbs_ShapeEnum theIgnoreType)
        {
            for (const TopoDS_Shape &aRootIter : theOwner->ShapeData()->Shapes())
            {
                if (anExp.Init(aRootIter, theShapeType, theIgnoreType).More())
                {
                    return theShapeType;
                }
            }
            return TopAbs_SHAPE;
        };

        for (size_t aTypeIter = 0; aTypeIter < sizeof(aShapeTypes) / sizeof(*aShapeTypes); ++aTypeIter)
        {
            for (const Handle(InteractiveShapeOwner) &anOwnerIter : ShapeOwners())
            {
                TopAbs_ShapeEnum aRes = findTopType(anOwnerIter, aShapeTypes[aTypeIter], anIgnoreTypes[aTypeIter]);
                if (aRes != TopAbs_SHAPE)
                {
                    return aRes;
                }
            }
        }

        return TopAbs_COMPOUND;
    }

    //! Create array for triangles.
    static Handle(Graphic3d_ArrayOfTriangles) createTrianglesArray(const TopoDS_Shape &theShape, const bool theHasTexels)
    {
        TopLoc_Location aLoc;

        int aNbTriangles = 0, aNbVertices = 0;
        bool hasNormals = true, hasTexels = theHasTexels;
        for (const TopoDS_Face &aFace : TopExp_ExplorerT<TopoDS_Face>(theShape))
        {
            if (const Handle(Poly_Triangulation) &aT = BRep_Tool::Triangulation(aFace, aLoc))
            {
                if (!aT->HasNormals())
                {
                    Poly::ComputeNormals(aT);
                }

                aNbTriangles += aT->NbTriangles();
                aNbVertices += aT->NbNodes();
                hasNormals = hasNormals && aT->HasNormals();
                hasTexels = hasTexels && aT->HasUVNodes();
            }
        }
        if (aNbVertices < 3 || aNbTriangles < 1)
        {
            return Handle(Graphic3d_ArrayOfTriangles)();
        }

        Graphic3d_ArrayFlags aFlags = (hasNormals ? Graphic3d_ArrayFlags_VertexNormal : Graphic3d_ArrayFlags_None) | (hasTexels ? Graphic3d_ArrayFlags_VertexTexel : Graphic3d_ArrayFlags_None);

        Handle(Graphic3d_ArrayOfTriangles) anArray = new Graphic3d_ArrayOfTriangles(aNbVertices, 3 * aNbTriangles, aFlags);
        return anArray;
    }

    // =======================================================================
    // function : FillTriangles
    // =======================================================================
    Handle(Graphic3d_ArrayOfTriangles) InteractiveShape::FillTriangles(const Handle(RWShape::ShapeData) & theData,
                                                                       const TopoDS_Shape &theShape,
                                                                       const bool theHasTexels)
    {
        Handle(Graphic3d_ArrayOfTriangles) anArray = createTrianglesArray(theShape, theHasTexels);
        if (anArray.IsNull())
        {
            return anArray;
        }

        TopLoc_Location aLoc;
        for (const TopoDS_Face &aFace : TopExp_ExplorerT<TopoDS_Face>(theShape))
        {
            const Handle(Poly_Triangulation) &aT = BRep_Tool::Triangulation(aFace, aLoc);
            if (aT.IsNull() || theData->IsSubshapeHidden(aFace))
            {
                continue;
            }

            // Determinant of transform matrix less then 0 means that mirror transform applied.
            const gp_Trsf &aTrsf = aLoc.Transformation();
            const bool isMirrored = aTrsf.VectorialPart().Determinant() < 0;

            // Extracts vertices & normals from nodes
            const int aVertFrom = anArray->VertexNumber();
            for (int aNodeIter = 1; aNodeIter <= aT->NbNodes(); ++aNodeIter)
            {
                gp_Pnt aPoint = aT->Node(aNodeIter);
                gp_Dir aNorm = aT->HasNormals() ? aT->Normal(aNodeIter) : gp::DZ();
                if ((aFace.Orientation() == TopAbs_REVERSED) != isMirrored)
                {
                    aNorm.Reverse();
                }
                if (!aLoc.IsIdentity())
                {
                    aPoint.Transform(aTrsf);
                    aNorm.Transform(aTrsf);
                }

                const int aVertId = anArray->AddVertex(aPoint, aNorm);
                if (anArray->HasVertexTexels())
                {
                    const gp_Pnt2d aTexel = aT->UVNode(aNodeIter);
                    anArray->SetVertexTexel(aVertId, aTexel.X(), aTexel.Y());
                }
            }

            // Fill array with vertex and edge visibility info
            int anIndex[3] = {};
            for (int aTriIter = 1; aTriIter <= aT->NbTriangles(); ++aTriIter)
            {
                aT->Triangle(aTriIter).Get(anIndex[0], anIndex[1], anIndex[2]);
                if (aFace.Orientation() == TopAbs_REVERSED)
                {
                    std::swap(anIndex[1], anIndex[2]);
                }
                anArray->AddEdges(anIndex[0] + aVertFrom, anIndex[1] + aVertFrom, anIndex[2] + aVertFrom);
            }
        }
        return anArray;
    }

} // namespace