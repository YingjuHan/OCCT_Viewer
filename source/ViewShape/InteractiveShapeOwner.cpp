#include <ViewShape/InteractiveShapeOwner.h>

#include <ViewShape/InteractiveShape.h>

#include <ViewBase/BaseDefaultDrawer.h>

#include <AIS_InteractiveContext.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Graphic3d_ArrayOfPoints.hxx>
#include <Graphic3d_ArrayOfPolylines.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Graphic3d_Group.hxx>
#include <Message.hxx>
#include <Prs3d_BndBox.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <Select3D_SensitiveCurve.hxx>
#include <Select3D_SensitiveSegment.hxx>
#include <SelectMgr_ViewerSelector.hxx>
#include <StdPrs_Curve.hxx>
#include <StdPrs_ShadedShape.hxx>
#include <StdPrs_ToolTriangulatedShape.hxx>
#include <StdPrs_WFShape.hxx>
#include <StdSelect_BRepSelectionTool.hxx>
#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>

namespace ViewShape
{

    // ================================================================
    // Function : InteractiveShapeListOwner
    // ================================================================
    InteractiveShapeListOwner::InteractiveShapeListOwner() : ViewBase::InteractiveOwner(TCollection_AsciiString())
    {
        //
    }

    // ================================================================
    // Function : AddChild
    // ================================================================
    bool InteractiveShapeListOwner::AddChild(const Handle(InteractiveShapeOwner) & theOwner)
    {
        if (Parent() != nullptr)
        {
            throw Standard_ProgramError("Internal error: nested AddChild()");
        }

        if (myChildren.Contains(theOwner))
        {
            return false;
        }

        theOwner->SetParent(this);
        myChildren.Append(theOwner);
        return true;
    }

    // ================================================================
    // Function : RemoveChild
    // ================================================================
    bool InteractiveShapeListOwner::RemoveChild(const Handle(InteractiveShapeOwner) & theOwner)
    {
        Handle(InteractiveShapeOwner) anOwner = theOwner;
        if (!myChildren.Remove(theOwner))
        {
            return false;
        }
        theOwner->SetParent(nullptr);
        return true;
    }

    // =======================================================================
    // function : CreatePresentation
    // =======================================================================
    Handle(AIS_InteractiveObject) InteractiveShapeListOwner::CreatePresentation() const
    {
        return new ViewShape::InteractiveShape(this);
    }

    // ================================================================
    // Function : hilightVertex
    // ================================================================
    bool InteractiveShapeOwner::hilightVertices(const TopoDS_Shape &theVertices,
                                                const Handle(Prs3d_Presentation) & thePrs,
                                                const Handle(Prs3d_Drawer) & theStyle)
    {
        if (theVertices.IsNull() || theVertices.NbChildren() == 0 || thePrs.IsNull() || theStyle.IsNull())
        {
            return false;
        }

        Handle(Graphic3d_ArrayOfPoints) aPoints = new Graphic3d_ArrayOfPoints(theVertices.NbChildren(),
                                                                              Graphic3d_ArrayFlags_None);
        for (TopoDS_Iterator aVertIter(theVertices); aVertIter.More(); aVertIter.Next())
        {
            const TopoDS_Vertex &aVert = TopoDS::Vertex(aVertIter.Value());
            const gp_Pnt aPnt = BRep_Tool::Pnt(aVert);
            aPoints->AddVertex(aPnt);
        }

        Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
        aGroup->SetGroupPrimitivesAspect(theStyle->PointAspect()->Aspect());
        aGroup->AddPrimitiveArray(aPoints);
        return true;
    }

    // ================================================================
    // Function : hilightEdges
    // ================================================================
    bool InteractiveShapeOwner::hilightEdges(const TopoDS_Shape &theEdges,
                                             const Handle(Prs3d_Presentation) & thePrs,
                                             const Handle(Prs3d_Drawer) & theStyle)
    {
        if (theEdges.IsNull() || thePrs.IsNull() || theStyle.IsNull())
        {
            return false;
        }

        StdPrs_WFShape::Add(thePrs, theEdges, theStyle);
        return true;
    }

    // ================================================================
    // Function : hilightFaces
    // ================================================================
    bool InteractiveShapeOwner::hilightFaces(const TopoDS_Shape &theFaces,
                                             const Handle(Prs3d_Presentation) & thePrs,
                                             const Handle(Prs3d_Drawer) & theStyle)
    {
        if (theFaces.IsNull() || thePrs.IsNull() || theStyle.IsNull())
        {
            return false;
        }

        if (Handle(Graphic3d_ArrayOfTriangles) aTriangles = InteractiveShape::FillTriangles(myShapeData, theFaces, false))
        {
            Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
            aGroup->SetGroupPrimitivesAspect(theStyle->ShadingAspect()->Aspect());
            aGroup->AddPrimitiveArray(aTriangles);
            return true;
        }
        return false;
    }

    // ================================================================
    // Function : detectClosestVertex
    // ================================================================
    bool InteractiveShapeOwner::detectClosestVertex(const TopoDS_Shape &theDetShape,
                                                    const TopAbs_ShapeEnum theAvoidType,
                                                    const gp_Pnt &thePickPoint,
                                                    const double thePickTol,
                                                    TopoDS_Vertex &theResVertex)
    {
        if (theDetShape.IsNull())
        {
            return false;
        }

        for (TopExp_Explorer aVertIter(theDetShape, TopAbs_VERTEX, theAvoidType); aVertIter.More(); aVertIter.Next())
        {
            const TopoDS_Vertex &aVertex = TopoDS::Vertex(aVertIter.Value());
            const gp_Pnt aPnt = BRep_Tool::Pnt(aVertex);
            if (thePickPoint.IsEqual(aPnt, thePickTol))
            {
                theResVertex = aVertex;
                return true;
            }
        }
        return false;
    }

    //! Return index of sub-shape in flat list of elements with this type.
    static int getElemIndex(const TopTools_SequenceOfShape &theShapes,
                            const std::vector<TopoDS_Compound> &theGroups,
                            const TopoDS_Shape &theElemToFind)
    {
        if (theElemToFind.ShapeType() == TopAbs_COMPOUND)
        {
            for (int aGroupIter = 0; aGroupIter < (int)theGroups.size(); ++aGroupIter)
            {
                if (theElemToFind.IsSame(theGroups[aGroupIter]))
                { // ignore orientation
                    return aGroupIter;
                }
            }
            return -1;
        }

        int anElemCounter = 0;
        for (const TopoDS_Shape &aRootIter : theShapes)
        {
            for (TopExp_Explorer anElemIter(aRootIter, theElemToFind.ShapeType()); anElemIter.More();
                 anElemIter.Next(), ++anElemCounter)
            {
                // @c if (theElemToFind == anElemIter.Value()) {
                if (theElemToFind.IsSame(anElemIter.Value()))
                { // ignore orientation
                    return anElemCounter;
                }
            }
        }
        return -1;
    }

    //! Return list of sub-shapes for each selection type.
    static void getElemsByIds(TopTools_ListOfShape &theResShapes,
                              ViewShape::ShapeSelElems *theSelElems,
                              const TopTools_SequenceOfShape &theShapes,
                              const TColStd_PackedMapOfInteger &theElemsToSelect,
                              const ViewShape::ShapeSelElemType theType,
                              const RWShape::SubShapesBoundsMap &theShapesBounds)
    {
        const TopAbs_ShapeEnum aShapeType = ViewShape::ShapeSelElemTypeToTopAbs(theType);
        if (aShapeType == TopAbs_SHAPE || theType == ViewShape::ShapeSelElemType::Group)
        {
            return;
        }

        Graphic3d_Vec2i aRange(IntegerLast(), IntegerFirst());
        for (int anIndex : theElemsToSelect)
        {
            aRange.x() = Min(aRange.x(), anIndex);
            aRange.y() = Max(aRange.y(), anIndex);
        }

        for (RWShape::ShapeDataExplorer anIter(theShapes, theShapesBounds, aRange, aShapeType); anIter.More();
             anIter.Next())
        {
            if (theElemsToSelect.Contains(anIter.Index()))
            {
                theResShapes.Append(anIter.Value());
                if (theSelElems != nullptr)
                {
                    theSelElems->Add(theType, anIter.Index());
                }
            }
        }
    }

    //! Return list of selected groups.
    static void getGroupsByIds(std::array<TopTools_ListOfShape, ShapeSelElemType_NB> &theResShapes,
                               ViewShape::ShapeSelElems *theSelElems,
                               const std::vector<TopoDS_Compound> &theGroups,
                               const ViewShape::ShapeSelElems &theElemsToSelect)
    {
        const Handle(TColStd_HPackedMapOfInteger) &anIndices = theElemsToSelect.Indices(
            ViewShape::ShapeSelElemType::Group);
        if (anIndices.IsNull() || anIndices->Map().IsEmpty())
        {
            return;
        }

        for (int aGroupIndex = 0; aGroupIndex < (int)theGroups.size(); ++aGroupIndex)
        {
            if (anIndices->Map().Contains(aGroupIndex))
            {
                theResShapes[(int)ViewShape::ShapeSelElemType::Group].Append(theGroups[aGroupIndex]);
                if (theSelElems != nullptr)
                {
                    theSelElems->Add(ViewShape::ShapeSelElemType::Group, aGroupIndex);
                }
            }
        }
    }

    //! Return list of sub-shapes for each selection type.
    static bool getElemsByIds(std::array<TopTools_ListOfShape, ShapeSelElemType_NB> &theResShapes,
                              ViewShape::ShapeSelElems *theSelElems,
                              const TopTools_SequenceOfShape &theShapes,
                              const std::vector<TopoDS_Compound> &theGroups,
                              const ViewShape::ShapeSelElems &theElemsToSelect,
                              const RWShape::SubShapesBoundsMap &theShapesBounds)
    {
        if (theSelElems != nullptr)
        {
            theSelElems->Clear();
        }

        if (theElemsToSelect.Extent() == 0)
        {
            return true;
        }

        getGroupsByIds(theResShapes, theSelElems, theGroups, theElemsToSelect);

        for (int aTypeIter = 0; aTypeIter < ViewShape::ShapeSelElemType_NB; ++aTypeIter)
        {
            const ViewShape::ShapeSelElemType aType = (ViewShape::ShapeSelElemType)aTypeIter;
            const Handle(TColStd_HPackedMapOfInteger) &anIndices = theElemsToSelect.Indices(aType);
            if (anIndices.IsNull() || anIndices->Map().IsEmpty())
            {
                continue;
            }

            getElemsByIds(theResShapes[(int)aType], theSelElems, theShapes, anIndices->Map(), aType, theShapesBounds);
        }

        if (theSelElems == nullptr)
        {
            return true;
        }

        for (int aTypeIter = 0; aTypeIter < ViewShape::ShapeSelElemType_NB; ++aTypeIter)
        {
            const ViewShape::ShapeSelElemType aType = (ViewShape::ShapeSelElemType)aTypeIter;
            if (theSelElems->Extent(aType) == theElemsToSelect.Extent(aType))
            {
                continue;
            }

            TColStd_PackedMapOfInteger aMap;
            aMap.Subtraction(!theElemsToSelect.Indices(aType).IsNull() ? theElemsToSelect.Indices(aType)->Map()
                                                                       : TColStd_PackedMapOfInteger(),
                             !theSelElems->Indices(aType).IsNull() ? theSelElems->Indices(aType)->Map()
                                                                   : TColStd_PackedMapOfInteger());
            std::string aStr;
            for (TColStd_PackedMapOfInteger::Iterator anIndexIter(aMap); anIndexIter.More(); anIndexIter.Next())
            {
                if (!aStr.empty())
                {
                    aStr += ",";
                }
                aStr += std::to_string(anIndexIter.Key());
            }
            Message::SendFail() << "Error: InteractiveShapeOwner, invalid " << ShapeSelElemTypeToString(aType)
                                << " indexes [" << aStr << "] have been specified";
            return false;
        }
        return true;
    }

    // =======================================================================
    // function : InteractiveShapeOwner
    // =======================================================================
    InteractiveShapeOwner::InteractiveShapeOwner(const TCollection_AsciiString &theId) : InteractiveOwner(theId)
    {
        SetPriority((int)SensitiveShape::SensPriority::Global);
    }

    // =======================================================================
    // function : InteractiveShapeOwner
    // =======================================================================
    InteractiveShapeOwner::InteractiveShapeOwner(const Handle(RWShape::ShapeData) & theShape)
        : InteractiveShapeOwner(theShape->Id())
    {
        myShapeData = theShape;
    }

    //! Deep copy materials.
    static void CloneMaterials(NCollection_DataMap<Handle(XCAFDoc_VisMaterial), Handle(XCAFDoc_VisMaterial)> &theMap,
                               XCAFPrs_Style &theStyle)
    {
        if (theStyle.Material().IsNull())
        {
            return;
        }

        Handle(XCAFDoc_VisMaterial) aCopy;
        if (!theMap.Find(theStyle.Material(), aCopy))
        {
            aCopy = new XCAFDoc_VisMaterial();
            aCopy->SetPbrMaterial(theStyle.Material()->PbrMaterial());
            theMap.Bind(theStyle.Material(), aCopy);
        }
        theStyle.SetMaterial(aCopy);
    }

    // =======================================================================
    // function : Clone
    // =======================================================================
    Handle(ViewBase::InteractiveOwner) InteractiveShapeOwner::Clone(const TCollection_AsciiString &theNewId)
    {
        Handle(RWShape::ShapeData) aDataCopy = new RWShape::ShapeData(theNewId, myShapeData);

        Handle(InteractiveShapeOwner) aCopy = new InteractiveShapeOwner(theNewId);
        aCopy->SetShapeData(aDataCopy);

        aCopy->myPrsMode = this->myPrsMode;
        aCopy->myBaseAttribs = this->myBaseAttribs;
        aCopy->myForcedColor = this->myForcedColor;

        // deep copy of materials
        NCollection_DataMap<Handle(XCAFDoc_VisMaterial), Handle(XCAFDoc_VisMaterial)> aMatMap;
        CloneMaterials(aMatMap, aCopy->myBaseAttribs.Style);
        for (RWMesh_NodeAttributeMap::Iterator anAttribIter(aDataCopy->AttributeMap()); anAttribIter.More();
             anAttribIter.Next())
        {
            CloneMaterials(aMatMap, anAttribIter.ChangeValue().Style);
        }

        return aCopy;
    }

    // =======================================================================
    // function : CreatePresentation
    // =======================================================================
    Handle(AIS_InteractiveObject) InteractiveShapeOwner::CreatePresentation() const
    {
        return new ViewShape::InteractiveShape(this);
    }

    // =======================================================================
    // function : InitBaseMaterial
    // =======================================================================
    void InteractiveShapeOwner::InitBaseMaterial(const Handle(Prs3d_Drawer) & theBaseDrawer)
    {
        if (!myBaseAttribs.Style.Material().IsNull())
        {
            return;
        }

        myBaseAttribs.Style.SetMaterial(new XCAFDoc_VisMaterial());

        ViewShape::InteractiveShape *aPrs = dynamic_cast<ViewShape::InteractiveShape *>(mySelectable);

        ViewBase::BaseDefaultDrawer *aDefDrawer = !theBaseDrawer.IsNull() ? dynamic_cast<ViewBase::BaseDefaultDrawer *>(
                                                                                !theBaseDrawer->Link().IsNull() ? theBaseDrawer->Link().get()
                                                                                                                : theBaseDrawer.get())
                                                                          : nullptr;
        if (aPrs != nullptr && aPrs->HasInteractiveContext())
        {
            aPrs->SetToUpdate();
            if (aDefDrawer == nullptr)
            {
                aDefDrawer = dynamic_cast<ViewBase::BaseDefaultDrawer *>(aPrs->InteractiveContext()->DefaultDrawer().get());
            }
        }

        if (aDefDrawer != nullptr)
        {
            myBaseAttribs.Style.Material()->SetPbrMaterial(aDefDrawer->VisMaterial()->PbrMaterial());
        }
    }

    // =======================================================================
    // function : detectFaceSubshape
    // =======================================================================
    bool InteractiveShapeOwner::detectFaceSubshape(const Handle(SensitivePolyFace) & theDetTris,
                                                   DetectedShapeElem *theDetElem,
                                                   ShapeSelElems *theSelElems) const
    {
        bool isHilighted = false;
        DetectedShapeElem aDetElem;
        if (IsSubshapeSelection(InteractiveShapeSelMode::Face))
        {
            if (!isHilighted && IsSubshapeSelection(InteractiveShapeSelMode::Face))
            {
                isHilighted = true;
                aDetElem.Type = ShapeSelElemType::Face;
                aDetElem.Index = theDetTris->GetShapeIndex();
            }
        }

        if (aDetElem.IsValid())
        {
            if (theSelElems != nullptr)
            {
                theSelElems->Add(aDetElem.Type, aDetElem.Index);
            }
            else if (theDetElem != nullptr)
            {
                *theDetElem = aDetElem;
            }
        }
        return isHilighted;
    }

    // =======================================================================
    // function : detectSomeSubshape
    // =======================================================================
    bool InteractiveShapeOwner::detectSomeSubshape(const TopoDS_Shape &theDetShape,
                                                   const int theShapeIndex,
                                                   DetectedShapeElem *theDetElem,
                                                   ShapeSelElems *theSelElems) const
    {
        if (theDetShape.IsNull())
        {
            return false;
        }

        const ShapeSelElemType aSelType = ShapeSelElemTypeFromTopAbs(theDetShape.ShapeType());
        const InteractiveShapeSelMode aSelMode = ShapeElemTypeToSelMode(aSelType);
        if (((unsigned int)mySelMode & (unsigned int)aSelMode) != 0)
        {
            DetectedShapeElem aDetElem(aSelType,
                                       theShapeIndex != -1
                                           ? theShapeIndex
                                           : getElemIndex(myShapeData->Shapes(), myShapeGroups, theDetShape));
            if (theSelElems != nullptr)
            {
                theSelElems->Add(aDetElem.Type, aDetElem.Index);
            }
            else
            {
                *theDetElem = aDetElem;
            }
            return true;
        }
        return false;
    }

    // =======================================================================
    // function : detectSubshapes
    // =======================================================================
    void InteractiveShapeOwner::detectSubshapes(const Handle(AIS_InteractiveContext) & theCtx,
                                                DetectedShapeElem *theDetElem,
                                                ShapeSelElems *theSelElems) const
    {
        const Handle(SelectMgr_ViewerSelector) &aSelector = theCtx->MainSelector();

        bool isPointSel = aSelector->GetManager().GetActiveSelectionType() == SelectMgr_SelectionType_Point;
        if (isPointSel && theCtx->DetectedOwner() != this)
        {
            return;
        }

        int aPickIter = 1;
        for (; aPickIter <= aSelector->NbPicked(); ++aPickIter)
        {
            if (aSelector->Picked(aPickIter) == this)
            {
                break;
            }
        }

        if (aPickIter > aSelector->NbPicked())
        {
            return;
        }

        Handle(Select3D_SensitiveEntity) anEntity = aSelector->PickedEntity(aPickIter);
        detectEntitySubshapes(anEntity, theDetElem, theSelElems);
    }

    // =======================================================================
    // function : detectEntitySubshapes
    // =======================================================================
    void InteractiveShapeOwner::detectEntitySubshapes(const Handle(Select3D_SensitiveEntity) & theEntity,
                                                      DetectedShapeElem *theDetElem,
                                                      ShapeSelElems *theSelElems) const
    {
        const SensitiveShape *aSensShape = dynamic_cast<SensitiveShape *>(theEntity.get());
        if (aSensShape == nullptr)
        {
            return;
        }

        TopoDS_Shape aShape = aSensShape->GetShape();
        if (!aShape.IsNull())
        {
            if (Handle(SensitivePolyFace) aSensTris = Handle(SensitivePolyFace)::DownCast(theEntity))
            {
                detectFaceSubshape(aSensTris, theDetElem, theSelElems);
                return;
            }

            detectSomeSubshape(aShape, aSensShape->GetShapeIndex(), theDetElem, theSelElems);
            return;
        }

        if (Handle(SensitiveCompound) aSensComp = Handle(SensitiveCompound)::DownCast(theEntity))
        {
            if (theSelElems != nullptr && !aSensComp->GetLastDetectedMap().IsEmpty())
            {
                for (TColStd_PackedMapOfInteger::Iterator aMultiIter(aSensComp->GetLastDetectedMap()); aMultiIter.More();
                     aMultiIter.Next())
                {
                    const Handle(Select3D_SensitiveEntity) &aSubEntity = aSensComp->GetEntities()[aMultiIter.Key()];
                    detectEntitySubshapes(aSubEntity, theDetElem, theSelElems);
                }
                return;
            }

            detectEntitySubshapes(aSensComp->GetLastDetected(), theDetElem, theSelElems);
        }
    }

    // =======================================================================
    // function : highlightSubshapes
    // =======================================================================
    void InteractiveShapeOwner::highlightSubshapes(const AIS_InteractiveObject *theObj,
                                                   const Handle(Prs3d_Presentation) & thePrs,
                                                   const Handle(Prs3d_Drawer) & theStyle,
                                                   const bool theIsSelection)
    {
        (void)theObj;

        std::array<TopTools_ListOfShape, ShapeSelElemType_NB> aSelElems;
        if (theIsSelection)
        {
            getElemsByIds(aSelElems,
                          nullptr,
                          myShapeData->Shapes(),
                          myShapeGroups,
                          mySelElems,
                          myShapeData->GetSubShapesBoundsMap());
        }
        else
        {
            if (myDetElem.IsValid())
            {
                ShapeSelElems aTmpSel;
                aTmpSel.Add(myDetElem.Type, myDetElem.Index);
                getElemsByIds(aSelElems,
                              nullptr,
                              myShapeData->Shapes(),
                              myShapeGroups,
                              aTmpSel,
                              myShapeData->GetSubShapesBoundsMap());
            }
        }
        highlightSubshapes(thePrs, theStyle, aSelElems);
    }

    //! Return subshape primitive type to select.
    static TopAbs_ShapeEnum getSubshapePrimitive(const TopoDS_Shape &theShape)
    {
        if (theShape.ShapeType() == TopAbs_VERTEX)
        {
            return TopAbs_VERTEX;
        }
        else if (theShape.ShapeType() == TopAbs_EDGE || theShape.ShapeType() == TopAbs_WIRE)
        {
            return TopAbs_EDGE;
        }
        else if (theShape.ShapeType() == TopAbs_FACE || theShape.ShapeType() == TopAbs_SHELL || theShape.ShapeType() == TopAbs_SOLID || theShape.ShapeType() == TopAbs_COMPSOLID)
        {
            return TopAbs_FACE;
        }
        else if (theShape.ShapeType() == TopAbs_COMPOUND)
        {
            TopoDS_Iterator aSubIter(theShape);
            if (aSubIter.More())
            {
                return getSubshapePrimitive(aSubIter.Value());
            }
        }
        return TopAbs_SHAPE;
    }

    // =======================================================================
    // function : highlightSubshapes
    // =======================================================================
    void InteractiveShapeOwner::highlightSubshapes(const Handle(Prs3d_Presentation) & thePrs,
                                                   const Handle(Prs3d_Drawer) & theStyle,
                                                   const std::array<TopTools_ListOfShape, ShapeSelElemType_NB> &theSelElems)
    {
        TopoDS_Compound aFaces;
        TopoDS_Compound anEdges;
        TopoDS_Compound aVerts;
        BRep_Builder aBuilder;
        for (int aTypeIter = 0; aTypeIter < ShapeSelElemType_NB; ++aTypeIter)
        {
            for (const TopoDS_Shape &aShapeIter : theSelElems[aTypeIter])
            {
                if (IsSubshapeHidden(aShapeIter))
                {
                    continue;
                }

                TopoDS_Compound *aComp = nullptr;
                const TopAbs_ShapeEnum aSubType = getSubshapePrimitive(aShapeIter);
                if (aSubType == TopAbs_VERTEX)
                {
                    aComp = &aVerts;
                }
                else if (aSubType == TopAbs_EDGE)
                {
                    aComp = &anEdges;
                }
                else if (aSubType == TopAbs_FACE)
                {
                    aComp = &aFaces;
                }
                else
                {
                    continue;
                }

                if (aComp == nullptr)
                {
                    continue;
                }

                if (aComp->IsNull())
                {
                    aBuilder.MakeCompound(*aComp);
                }
                aBuilder.Add(*aComp, aShapeIter);
            }
        }
        hilightFaces(aFaces, thePrs, theStyle);
        hilightEdges(anEdges, thePrs, theStyle);
        hilightVertices(aVerts, thePrs, theStyle);
    }

    // =======================================================================
    // function : displaySubshapes
    // =======================================================================
    void InteractiveShapeOwner::displaySubshapes(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                                 const Handle(Prs3d_Drawer) & theStyle,
                                                 const AIS_InteractiveObject *theObj,
                                                 const bool theIsSelection)
    {
        const bool isLocal = !theIsSelection && myDetElem.IsValid() && ToPopoutHighlight(myDetElem.Type);

        const Handle(Prs3d_Drawer) &aStyle = isLocal ? theObj->InteractiveContext()->HighlightStyle(
                                                           Prs3d_TypeOfHighlight_LocalDynamic)
                                                     : theStyle;

        Handle(Prs3d_Presentation) aPrs = GetHighlightPresentation(thePrsMgr, aStyle, theIsSelection);
        highlightSubshapes(theObj, aPrs, aStyle, theIsSelection);

        if (!theIsSelection)
        {
            if (myDetElem.IsValid())
            {
                SetSelectionPart(ViewBase::InteractiveOwner::SelectionPart((int)ShapeElemTypeToSelMode(myDetElem.Type),
                                                                           myDetElem.Index));
            }
            thePrsMgr->AddToImmediateList(aPrs);
        }
        else
        {
            aPrs->Display();
        }
    }

    //! Local picking info holder.
    class InteractiveShapeOwner::ShapeLocalPickInfo : public SelectMgr_LocalPickInfo
    {
    public:
        //! Empty constructor.
        ShapeLocalPickInfo() {}

        //! Compare two info.
        virtual bool IsEqual(const Handle(SelectMgr_LocalPickInfo) & theOther) const Standard_OVERRIDE
        {
            if (theOther == this)
            {
                return true;
            }

            ShapeLocalPickInfo *anOther = dynamic_cast<ShapeLocalPickInfo *>(theOther.get());
            return anOther != nullptr && anOther->Element.IsEqual(Element);
        }

    public:
        InteractiveShapeOwner::DetectedShapeElem Element;
    };

    // =======================================================================
    // function : LocalPickInfo
    // =======================================================================
    void InteractiveShapeOwner::LocalPickInfo(Handle(SelectMgr_LocalPickInfo) & theInfo,
                                              const Handle(Select3D_SensitiveEntity) & theEntity) const
    {
        Handle(ShapeLocalPickInfo) anInfo = Handle(ShapeLocalPickInfo)::DownCast(theInfo);
        if (anInfo.IsNull())
        {
            anInfo = new ShapeLocalPickInfo();
            theInfo = anInfo;
        }

        anInfo->Element.Clear();
        detectEntitySubshapes(theEntity, &anInfo->Element, nullptr);
    }

    // =======================================================================
    // function : GetHighlightPresentation
    // =======================================================================
    Handle(Prs3d_Presentation) InteractiveShapeOwner::GetHighlightPresentation(
        const Handle(PrsMgr_PresentationManager) & thePrsMgr,
        const Handle(Prs3d_Drawer) & theStyle,
        const bool theIsSelection)
    {
        InteractiveShape *anObj = dynamic_cast<InteractiveShape *>(mySelectable);

        Handle(Prs3d_Presentation) aPrs;
        if (anObj->ShapeOwners().Size() <= 1 || !theIsSelection)
        {
            aPrs = !theIsSelection ? mySelectable->GetHilightPresentation(thePrsMgr)
                                   : mySelectable->GetSelectPresentation(thePrsMgr);
        }
        else
        {
            if (mySelPrs.IsNull() && !thePrsMgr.IsNull())
            {
                mySelPrs = new Prs3d_Presentation(thePrsMgr->StructureManager());
                mySelPrs->SetTransformPersistence(anObj->TransformPersistence());
                mySelPrs->SetClipPlanes(anObj->ClipPlanes());
                mySelPrs->SetTransformation(anObj->TransformationGeom());
            }
            aPrs = mySelPrs;
        }

        if (!thePrsMgr.IsNull())
        {
            PrepareHighlightPresentation(aPrs, thePrsMgr, theStyle, theIsSelection);
        }
        return aPrs;
    }

    // =======================================================================
    // function : hilightGlobal
    // =======================================================================
    bool InteractiveShapeOwner::hilightGlobal(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                              const Handle(Prs3d_Drawer) & theStyle,
                                              const Standard_Integer theMode,
                                              const bool theIsSelection)
    {
        if (mySelMode > (int)InteractiveShapeSelMode::Global)
        {
            return false;
        }

        const InteractiveShape *anObj = dynamic_cast<InteractiveShape *>(mySelectable);

        const Graphic3d_ZLayerId aZLayer = GetHighlightZLayer(theStyle, theIsSelection);
        if (anObj->ShapeOwners().Size() <= 1)
        {
            thePrsMgr->Color(mySelectable, theStyle, theMode, nullptr, aZLayer);
            return true;
        }

        Handle(Prs3d_Presentation) aPrs = GetHighlightPresentation(thePrsMgr, theStyle, theIsSelection);
        if (PresentationMode() == InteractiveShapePrsMode::Wireframe)
        {
            StdPrs_WFShape::Add(aPrs, myShapeData->GetSingleShape(), theStyle);
        }
        else
        {
            StdPrs_ShadedShape::Add(aPrs, myShapeData->GetSingleShape(), theStyle);
        }
        if (!theIsSelection)
        {
            thePrsMgr->AddToImmediateList(aPrs);
        }
        else
        {
            aPrs->Display();
        }
        return true;
    }

    // =======================================================================
    // function : HilightWithColor
    // =======================================================================
    void InteractiveShapeOwner::HilightWithColor(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                                 const Handle(Prs3d_Drawer) & theStyle,
                                                 const Standard_Integer theMode)
    {
        if (thePrsMgr->IsImmediateModeOn())
        {
            SetSelectionPart(ViewBase::InteractiveOwner::SelectionPart());
        }

        InteractiveShape *anObj = dynamic_cast<InteractiveShape *>(mySelectable);
        if (anObj == nullptr || !anObj->IsEnabled())
        {
            return;
        }
        else if (!anObj->HasInteractiveContext() || anObj->InteractiveContext()->SelectionManager().IsNull() || anObj->InteractiveContext()->SelectionManager()->Selector().IsNull())
        {
            mySelElems.Clear();
            myDetElem.Clear();
            return;
        }

        const bool isSelection = !thePrsMgr->IsImmediateModeOn();
        AIS_SelectionScheme aSelScheme = GetCurrentSelectionScheme();
        ShapeSelElems anOldSel;
        if (isSelection)
        {
            MarkSelected(true);
            if (aSelScheme == AIS_SelectionScheme_XOR)
            {
                std::swap(anOldSel, mySelElems);
            }
            else if (aSelScheme != AIS_SelectionScheme_Add)
            {
                mySelElems.Clear();
            }
        }
        else
        {
            myDetElem.Clear();
        }

        if (hilightGlobal(thePrsMgr, theStyle, theMode, isSelection))
        {
            return;
        }

        detectSubshapes(anObj->InteractiveContext(),
                        !isSelection ? &myDetElem : nullptr,
                        isSelection ? &mySelElems : nullptr);
        if (aSelScheme == AIS_SelectionScheme_XOR && anOldSel.IsValid())
        {
            mySelElems.XOR(anOldSel);
        }

        displaySubshapes(thePrsMgr, theStyle, anObj, isSelection);
    }

    // =======================================================================
    // function : Unhilight
    // =======================================================================
    void InteractiveShapeOwner::Unhilight(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                          const Standard_Integer theMode)
    {
        mySelElems.Clear();

        InteractiveShape *anObj = dynamic_cast<InteractiveShape *>(mySelectable);
        if (anObj != nullptr)
        {
            MarkSelected(false);

            Handle(Prs3d_Presentation) aPrs = GetHighlightPresentation(Handle(PrsMgr_PresentationManager)(),
                                                                       Handle(Prs3d_Drawer)(),
                                                                       true);
            if (!aPrs.IsNull())
            {
                aPrs->Erase();
            }
        }
        InteractiveOwner::Unhilight(thePrsMgr, theMode);
    }

    // =======================================================================
    // Function : Clear
    // =======================================================================
    void InteractiveShapeOwner::Clear(const Handle(PrsMgr_PresentationManager) & thePrsMgr, const Standard_Integer theMode)
    {
        if (!mySelPrs.IsNull())
        {
            mySelPrs->Clear();
            mySelPrs.Nullify();
        }
        InteractiveOwner::Clear(thePrsMgr, theMode);
    }

    // =======================================================================
    // Function : IsHilighted
    // =======================================================================
    Standard_Boolean InteractiveShapeOwner::IsHilighted(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                                        const Standard_Integer theMode) const
    {
        return !mySelPrs.IsNull() ? mySelPrs->IsHighlighted() : InteractiveOwner::IsHilighted(thePrsMgr, theMode);
    }

    // ================================================================
    // Function : SetShapeGroups
    // ================================================================
    void InteractiveShapeOwner::SetShapeGroups(const std::vector<TopoDS_Compound> &theGroups)
    {
        myShapeGroups = theGroups;
        if (((unsigned int)mySelMode & (unsigned int)InteractiveShapeSelMode::Group) == 0)
        {
            return;
        }

        Handle(AIS_InteractiveObject) anObj = Handle(AIS_InteractiveObject)::DownCast(mySelectable);
        if (!anObj.IsNull() && anObj->HasInteractiveContext())
        {
            anObj->InteractiveContext()->RecomputeSelectionOnly(anObj);
        }
    }

    // ================================================================
    // Function : AddShapeGroup
    // ================================================================
    void InteractiveShapeOwner::AddShapeGroup(const TopoDS_Compound &theGroup)
    {
        myShapeGroups.push_back(theGroup);
        if (((unsigned int)mySelMode & (unsigned int)InteractiveShapeSelMode::Group) == 0)
        {
            return;
        }

        Handle(AIS_InteractiveObject) anObj = Handle(AIS_InteractiveObject)::DownCast(mySelectable);
        if (!anObj.IsNull() && anObj->HasInteractiveContext())
        {
            anObj->InteractiveContext()->RecomputeSelectionOnly(anObj);
        }
    }

    // ================================================================
    // Function : SetSelectionMode
    // ================================================================
    void InteractiveShapeOwner::SetSelectionMode(const int theMode)
    {
        if (mySelMode == theMode)
        {
            return;
        }

        InteractiveOwner::SetSelectionMode(theMode);

        Handle(AIS_InteractiveObject) anObj = Handle(AIS_InteractiveObject)::DownCast(mySelectable);
        if (!anObj.IsNull() && anObj->HasInteractiveContext())
        {
            // it would be better to postpone re-computation in case of batch changes
            anObj->InteractiveContext()->RecomputeSelectionOnly(anObj);
        }
    }

    // ================================================================
    // Function : SetShape
    // ================================================================
    void InteractiveShapeOwner::SetShape(const TopoDS_Shape &theShape)
    {
        myShapeData->SetShape(theShape);
        mySensCache.clear();
    }

    // ================================================================
    // Function : fillSensitives
    // ================================================================
    void InteractiveShapeOwner::fillSensitives(const Handle(Prs3d_Drawer) & theDrawer)
    {
        mySensCache.clear();
        fillSensitiveFaces();
        fillSensitiveEdges(theDrawer);
        fillSensitiveVertices();

        myShapeData->GetSubShapesBoundsMap(true);
    }

    // ================================================================
    // Function : fillSensitiveFaces
    // ================================================================
    void InteractiveShapeOwner::fillSensitiveFaces()
    {
        Handle(SelectMgr_EntityOwner) aGlobOwner = this;
        TopTools_MapOfShape aFilter;
        int aSubshapeIndex = 0;

        for (const TopoDS_Shape &aRootIter : myShapeData->Shapes())
        {
            for (TopExp_Explorer aFaceIter(aRootIter, TopAbs_FACE); aFaceIter.More(); aFaceIter.Next(), ++aSubshapeIndex)
            {
                const TopoDS_Face &aFace = TopoDS::Face(aFaceIter.Value());
                if (!aFilter.Add(aFace))
                {
                    continue;
                }

                TopLoc_Location aLoc;
                const Handle(Poly_Triangulation) &aPolyTri = BRep_Tool::Triangulation(aFace, aLoc);
                if (!aPolyTri.IsNull())
                {
                    Handle(SensitivePolyFace) aSensTris = new SensitivePolyFace(aGlobOwner, aFace, aPolyTri, aLoc);
                    aSensTris->SetShapeIndex(aSubshapeIndex);
                    mySensCache.push_back(aSensTris);
                }
            }
        }
    }

    // ================================================================
    // Function : fillSensitiveEdges
    // ================================================================
    void InteractiveShapeOwner::fillSensitiveEdges(const Handle(Prs3d_Drawer) & theDrawer)
    {
        Handle(SelectMgr_EntityOwner) aGlobOwner = this;
        TopTools_MapOfShape aFilter;
        int aSubshapeIndex = 0;

        Handle(SelectMgr_Selection) aTmpSel = new SelectMgr_Selection(); // for exceptional case
        for (const TopoDS_Shape &aRootIter : myShapeData->Shapes())
        {
            const double aDefl = StdPrs_ToolTriangulatedShape::GetDeflection(aRootIter, theDrawer);

            for (TopExp_Explorer aShapeIter(aRootIter, TopAbs_EDGE); aShapeIter.More();
                 aShapeIter.Next(), ++aSubshapeIndex)
            {
                const TopoDS_Edge &anEdge = TopoDS::Edge(aShapeIter.Current());
                if (!aFilter.Add(anEdge))
                {
                    continue;
                }

                if (Handle(SensitivePolyEdgeOnTri) aTriSens = SensitivePolyEdgeOnTri::GetEdgeSensitive(anEdge,
                                                                                                       aGlobOwner))
                {
                    // face boundaries
                    aTriSens->SetShapeIndex(aSubshapeIndex);
                    mySensCache.push_back(aTriSens);
                }
                else if (Handle(SensitivePolyEdge) aSens =
                             SensitivePolyEdge::GetEdgeSensitive(anEdge, aGlobOwner, theDrawer, aDefl, aTmpSel))
                {
                    // isolated edges
                    aSens->SetShapeIndex(aSubshapeIndex);
                    mySensCache.push_back(aSens);
                }
            }
        }
        for (const Handle(SelectMgr_SensitiveEntity) &aSensIter : aTmpSel->Entities())
        {
            mySensCache.push_back(aSensIter->BaseSensitive()); // exceptional case
        }
    }

    // ================================================================
    // Function : fillSensitiveVertices
    // ================================================================
    void InteractiveShapeOwner::fillSensitiveVertices()
    {
        Handle(SelectMgr_EntityOwner) aGlobOwner = this;
        TopTools_MapOfShape aFilter;
        int aSubshapeIndex = 0;

        for (const TopoDS_Shape &aRootIter : myShapeData->Shapes())
        {
            for (TopExp_Explorer aShapeIter(aRootIter, TopAbs_VERTEX); aShapeIter.More();
                 aShapeIter.Next(), ++aSubshapeIndex)
            {
                const TopoDS_Vertex &aVert = TopoDS::Vertex(aShapeIter.Current());
                if (!aFilter.Add(aVert))
                {
                    continue;
                }

                Handle(SensitiveVertex) aSens = new SensitiveVertex(aGlobOwner, aVert);
                aSens->SetShapeIndex(aSubshapeIndex);
                mySensCache.push_back(aSens);
            }
        }
    }

    // ================================================================
    // Function : GetSensitives
    // ================================================================
    const std::vector<Handle(Select3D_SensitiveEntity)> &InteractiveShapeOwner::GetSensitives(
        const Handle(Prs3d_Drawer) & theDrawer)
    {
        if (mySensCache.empty())
        {
            fillSensitives(theDrawer);
        }

        Handle(SelectMgr_EntityOwner) anOwner = this;
        for (const Handle(Select3D_SensitiveEntity) &anIter : mySensCache)
        {
            anIter->Set(anOwner);
        }
        return mySensCache;
    }

    // ================================================================
    // Function : GetSensitivesMap
    // ================================================================
    void InteractiveShapeOwner::GetSensitivesMap(
        NCollection_DataMap<TopoDS_Shape, Handle(Select3D_SensitiveEntity), TopTools_ShapeMapHasher> &theSensMap,
        const Handle(Prs3d_Drawer) & theDrawer)
    {
        if (mySensCache.empty())
        {
            fillSensitives(theDrawer);
        }

        for (const Handle(Select3D_SensitiveEntity) &aSensIter : mySensCache)
        {
            const SensitiveShape *aSensShape = dynamic_cast<SensitiveShape *>(aSensIter.get());
            TopoDS_Shape aShape = aSensShape != nullptr ? aSensShape->GetShape() : TopoDS_Shape();
            if (!aShape.IsNull())
            {
                theSensMap.Bind(aShape, aSensIter);
            }
        }
    }

    // ================================================================
    // Function : ClearSelectedElems
    // ================================================================
    void InteractiveShapeOwner::ClearSelectedElems()
    {
        InteractiveShape *anObj = dynamic_cast<InteractiveShape *>(mySelectable);
        if (anObj == nullptr || !anObj->HasInteractiveContext())
        {
            return;
        }

        Handle(Prs3d_Presentation) aPrs = anObj->GetSelectPresentation(Handle(PrsMgr_PresentationManager)());
        if (!aPrs.IsNull())
        {
            aPrs->Clear();
        }
        mySelElems.Clear();
    }

    // ================================================================
    // Function : SetSelectedElems
    // ================================================================
    void InteractiveShapeOwner::SetSelectedElems(const ShapeSelElems &theElems)
    {
        InteractiveShape *anObj = dynamic_cast<InteractiveShape *>(mySelectable);
        if (anObj == nullptr || !anObj->HasInteractiveContext())
        {
            return;
        }

        std::array<TopTools_ListOfShape, ShapeSelElemType_NB> aSelElems;
        getElemsByIds(aSelElems,
                      &mySelElems,
                      myShapeData->Shapes(),
                      myShapeGroups,
                      theElems,
                      myShapeData->GetSubShapesBoundsMap());

        const Handle(PrsMgr_PresentationManager) &aPrsMgr = anObj->InteractiveContext()->MainPrsMgr();

        Handle(Prs3d_Presentation) aPrs = anObj->GetSelectPresentation(aPrsMgr);
        aPrs->Clear();

        const Handle(Prs3d_Drawer) &aStyle = anObj->HilightAttributes().IsNull()
                                                 ? anObj->InteractiveContext()->SelectionStyle()
                                                 : anObj->HilightAttributes();
        const Graphic3d_ZLayerId aZLayer = GetHighlightZLayer(aStyle, true);
        if (aPrs->GetZLayer() != aZLayer)
        {
            aPrs->SetZLayer(aZLayer);
        }

        highlightSubshapes(aPrs, aStyle, aSelElems);
        if (mySelElems.IsValid())
        {
            aPrs->CalculateBoundBox();
            aPrs->Display();
            aPrsMgr->StructureManager()->Update(aPrs->GetZLayer());
        }
        else
        {
            aPrs->Erase();
        }
    }

    // ================================================================
    // Function : GetSubshapesIndexes
    // ================================================================
    Handle(TColStd_HPackedMapOfInteger) InteractiveShapeOwner::GetSubshapesIndexes(const ShapeSelElemType theType,
                                                                                   const bool theIsFree,
                                                                                   const TopoDS_Shape &theParent,
                                                                                   const int theVisibility) const
    {
        if (theType == ShapeSelElemType::Group)
        {
            Handle(TColStd_HPackedMapOfInteger) aMap = new TColStd_HPackedMapOfInteger((int)myShapeGroups.size());
            for (int aGroupIter = 0; aGroupIter < (int)myShapeGroups.size(); ++aGroupIter)
            {
                aMap->ChangeMap().Add(aGroupIter);
            }
            return aMap;
        }

        const TopAbs_ShapeEnum aShapeType = ShapeSelElemTypeToTopAbs(theType);
        return myShapeData->GetSubshapesIndexes(aShapeType, theIsFree, theParent, theVisibility);
    }

    // =======================================================================
    // function : GetSubshapes
    // =======================================================================
    bool InteractiveShapeOwner::GetSubshapes(std::vector<RWShape::EnumeratedShape> &theShapes,
                                             const ShapeSelElemType theType,
                                             const Handle(TColStd_HPackedMapOfInteger) & theIndices) const
    {
        theShapes.clear();
        if (theType == ShapeSelElemType::Group)
        {
            theShapes.reserve(theIndices->Map().Extent());
            for (int aGroupIter = 0; aGroupIter < (int)myShapeGroups.size(); ++aGroupIter)
            {
                if (theIndices->Map().Contains(aGroupIter))
                {
                    RWShape::EnumeratedShape aShape;
                    aShape.Shape = myShapeGroups[aGroupIter];
                    aShape.Index = aGroupIter;
                    theShapes.push_back(aShape);
                }
            }
            return (int)theShapes.size() == theIndices->Map().Extent();
        }

        const TopAbs_ShapeEnum aShapeType = ShapeSelElemTypeToTopAbs(theType);
        return myShapeData->GetSubshapes(theShapes, aShapeType, theIndices);
    }

    // ================================================================
    // Function : GetSubshapesByIndexMap
    // ================================================================
    bool InteractiveShapeOwner::GetSubshapesByIndexMap(std::array<TopTools_ListOfShape, ShapeSelElemType_NB> &theShapes,
                                                       const ShapeSelElems &theIndices) const
    {
        return getElemsByIds(theShapes,
                             nullptr,
                             myShapeData->Shapes(),
                             myShapeGroups,
                             theIndices,
                             myShapeData->GetSubShapesBoundsMap());
    }

    // ================================================================
    // Function : GetSubshapesByIndexMap
    // ================================================================
    TopoDS_Shape InteractiveShapeOwner::GetSubshapesByIndexMap(const ShapeSelElems &theIndices) const
    {
        if (!theIndices.IsValid())
        {
            return TopoDS_Shape();
        }

        std::array<TopTools_ListOfShape, ShapeSelElemType_NB> aSubshapes;
        if (!getElemsByIds(aSubshapes,
                           nullptr,
                           myShapeData->Shapes(),
                           myShapeGroups,
                           theIndices,
                           myShapeData->GetSubShapesBoundsMap()))
        {
            return TopoDS_Shape();
        }

        TopoDS_Compound aComp;
        BRep_Builder().MakeCompound(aComp);
        for (int aTypeIter = (int)aSubshapes.size() - 1; aTypeIter >= 0; --aTypeIter)
        {
            const TopTools_ListOfShape &aShapeList = aSubshapes[aTypeIter];
            if (aShapeList.IsEmpty())
            {
                continue;
            }

            TopoDS_Compound aSubComp;
            BRep_Builder().MakeCompound(aSubComp);
            for (const TopoDS_Shape &aShapeIter : aShapeList)
            {
                BRep_Builder().Add(aSubComp, aShapeIter);
            }
            if (aSubComp.NbChildren() == 1)
            {
                BRep_Builder().Add(aComp, TopoDS_Iterator(aSubComp).Value());
            }
            else
            {
                BRep_Builder().Add(aComp, aSubComp);
            }
        }

        if (aComp.NbChildren() == 1)
        {
            return TopoDS_Iterator(aComp).Value();
        }
        else
        {
            return aComp;
        }
    }

    // ================================================================
    // Function : SetDisabledElems
    // ================================================================
    void InteractiveShapeOwner::SetDisabledElems(const ShapeSelElems &theElems)
    {
        myDisabledElems = theElems;
    }

    // =======================================================================
    // function : GetConnectedSubshapes
    // =======================================================================
    Handle(TColStd_HPackedMapOfInteger) InteractiveShapeOwner::GetConnectedSubshapes(
        const ShapeSelElemType theType,
        const Handle(TColStd_HPackedMapOfInteger) & theIndices) const
    {
        const TopAbs_ShapeEnum aShapeType = ViewShape::ShapeSelElemTypeToTopAbs(theType);
        TopAbs_ShapeEnum aSubType = TopAbs_SHAPE;
        switch (aShapeType)
        {
        case TopAbs_SHELL:
            aSubType = TopAbs_EDGE;
            break;
        case TopAbs_FACE:
            aSubType = TopAbs_EDGE;
            break;
        case TopAbs_EDGE:
            aSubType = TopAbs_VERTEX;
            break;
        case TopAbs_WIRE:
            aSubType = TopAbs_VERTEX;
            break;
        default:
            break;
        }

        if (aSubType == TopAbs_SHAPE || theIndices.IsNull() || theIndices->Map().IsEmpty())
        {
            return Handle(TColStd_HPackedMapOfInteger)();
        }

        int anElemCounter = 0;

        TopTools_IndexedDataMapOfShapeListOfShape aSubshapesToParent;
        TopTools_IndexedMapOfShape anEdgesOfSelected;
        for (const TopoDS_Shape &aRootIter : myShapeData->Shapes())
        {
            // fill in global subshape -> shape map
            TopExp::MapShapesAndAncestors(aRootIter, aSubType, aShapeType, aSubshapesToParent);

            // get all subshapes of selected shapes
            for (TopExp_Explorer anElemIter(aRootIter, aShapeType); anElemIter.More(); anElemIter.Next(), ++anElemCounter)
            {
                if (theIndices->Map().Contains(anElemCounter))
                {
                    TopExp::MapShapes(anElemIter.Current(), aSubType, anEdgesOfSelected);
                }
            }
        }

        // add all shapes from geometry sharing common subshapes
        Handle(TColStd_HPackedMapOfInteger) aRes = new TColStd_HPackedMapOfInteger();
        TopTools_MapOfShape aFilter;
        for (TopTools_IndexedDataMapOfShapeListOfShape::Iterator aSubshapeIter(aSubshapesToParent); aSubshapeIter.More();
             aSubshapeIter.Next())
        {
            if (!anEdgesOfSelected.Contains(aSubshapeIter.Key()))
            {
                continue;
            }

            for (const TopoDS_Shape &aParentIter : aSubshapeIter.Value())
            {
                if (aFilter.Add(aParentIter))
                {
                    // counter-efficient lookup
                    aRes->ChangeMap().Add(getElemIndex(myShapeData->Shapes(), myShapeGroups, aParentIter));
                }
            }
        }
        return aRes;
    }

} // namespace