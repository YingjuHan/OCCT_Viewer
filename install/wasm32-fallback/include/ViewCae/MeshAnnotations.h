#ifndef MESHANNOTATIONS_HEADERFILE
#define MESHANNOTATIONS_HEADERFILE

#include <ViewBase/InteractiveOwner.h>

#include <AIS_InteractiveContext.hxx>
#include <Font_FTFont.hxx>
#include <gp_Pnt.hxx>
#include <Graphic3d_AspectMarker3d.hxx>
#include <Prs3d_Presentation.hxx>
#include <Prs3d_TextAspect.hxx>
#include <Select3D_SensitiveSet.hxx>
#include <TCollection_AsciiString.hxx>

namespace ViewCae {
//! Struct for annotation data.
struct MeshAnnotationData {
    TCollection_AsciiString Text;
    TCollection_AsciiString Icon;

    MeshAnnotationData(const TCollection_AsciiString& theText = "", const TCollection_AsciiString& theIcon = "")
        : Text(theText),
          Icon(theIcon)
    {
    }
};

//! Type of visibility of mesh annotation parts.
enum class MeshAnnotationType : int { Text = 0x1, Icon = 0x2, TextIcon = Text | Icon };

//! Owner of annotation line.
class MeshAnnotationLineOwner : public ViewBase::InteractiveOwner {
public:
    //! Main constructor.
    MeshAnnotationLineOwner(const TCollection_AsciiString& theOwnerId,
                            const Font_Rect&               theRect,
                            const std::vector<gp_Pnt>&     thePositions)
        : ViewBase::InteractiveOwner(theOwnerId),
          myRect(theRect),
          myPositions(thePositions)
    {
    }

    //! Return annotation positions.
    const std::vector<gp_Pnt>& Positions() const { return myPositions; }

    //! Handle highlighting.
    Standard_EXPORT virtual void HilightWithColor(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                                  const Handle(Prs3d_Drawer)&               theStyle,
                                                  const Standard_Integer                    theMode) override;

    //! Removes highlighting from the owner.
    Standard_EXPORT virtual void Unhilight(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                           const Standard_Integer                    theMode) override;

protected:
    Font_Rect           myRect;
    std::vector<gp_Pnt> myPositions;
};

//! Class for mesh annotation containing array of 2d text/icons at 3d position.
class MeshAnnotation {
public:
    //! Constructor for empty multi-line annotation.
    MeshAnnotation(const gp_Pnt& thePos, int theType, int theNbLines) : MeshAnnotation(thePos, theType)
    {
        myData.resize(theNbLines);
    }

    //! Constructor for filled multi-line annotation.
    MeshAnnotation(const gp_Pnt& thePos, int theType, const std::vector<MeshAnnotationData>& theData)
        : MeshAnnotation(thePos, theType)
    {
        myData = theData;
    }

    //! Return annotation's index.
    int Index() const { return myIndex; }

    //! Set annotation's index.
    void SetIndex(int theIndex) { myIndex = theIndex; }

    //! Return annotation's position.
    const gp_Pnt& Pos() const { return myPositions[0]; }

    //! Set annotation's position.
    void SetPos(const gp_Pnt& thePos) { myPositions[0] = thePos; }

    //! Return annotation's positions.
    const std::vector<gp_Pnt>& Positions() const { return myPositions; }

    //! Set annotation's positions.
    void SetPositions(const std::vector<gp_Pnt>& thePositions)
    {
        if (!thePositions.empty()) {
            myPositions = thePositions;
        }
    }

    //! Return annotation's per line data.
    const std::vector<MeshAnnotationData>& Data() const { return myData; }

    //! Set annotation's per line data.
    void SetData(const std::vector<MeshAnnotationData>& theData) { myData = theData; }

    //! Return annotation's text at the specified line.
    const TCollection_AsciiString& Text(int theLineIndex = 0) const { return myData[theLineIndex].Text; }

    //! Set annotation's text at the specified line.
    void SetText(const TCollection_AsciiString& theText, int theLineIndex = 0) { myData[theLineIndex].Text = theText; }

    //! Return annotation's icon id at the specified line.
    const TCollection_AsciiString& Icon(int theLineIndex = 0) const { return myData[theLineIndex].Icon; }

    //! Set annotation's icon id at the specified line.
    void SetIcon(const TCollection_AsciiString& theIcon, int theLineIndex = 0) { myData[theLineIndex].Icon = theIcon; }

    //! Return annotation's displaying type.
    unsigned int Type() const { return myType; }

    //! Set annotation's displaying type.
    void SetType(unsigned int theType) { myType = theType; }

    //! Return annotation's visibility.
    bool IsVisible() const { return myIsVisible; }

    //! Set annotation's visibility.
    void SetVisible(bool theIsVisible) { myIsVisible = theIsVisible; }

    //! Check if text should be displayed.
    bool IsText() const { return (myType & (unsigned int)MeshAnnotationType::Text) != 0; }

    //! Check if icon should be displayed.
    bool IsIcon() const { return (myType & (unsigned int)MeshAnnotationType::Icon) != 0; }

    //! Return per line owners.
    NCollection_DataMap<int, Handle(MeshAnnotationLineOwner)>& LineOwners() { return myLineOwners; }

    //! Return per line owners.
    const NCollection_DataMap<int, Handle(MeshAnnotationLineOwner)>& LineOwners() const { return myLineOwners; }

protected:
    //! Constructor.
    MeshAnnotation(const gp_Pnt& thePos, unsigned int theType) : myType(theType) { myPositions.push_back(thePos); }

protected:
    int                                                       myIndex = -1;
    std::vector<gp_Pnt>                                       myPositions;
    std::vector<MeshAnnotationData>                           myData;
    bool                                                      myIsVisible = true;
    unsigned int                                              myType      = 0;
    NCollection_DataMap<int, Handle(MeshAnnotationLineOwner)> myLineOwners;
};

//! Container for mesh annotations.
class MeshAnnotations {
public:
    //! Add annotation.
    void AddAnnotation(const MeshAnnotation& theAnnotation) { myAnnotations.push_back(theAnnotation); }

    //! Return added annotations.
    const std::vector<MeshAnnotation>& Annotations() const { return myAnnotations; }

    //! Return added annotations.
    std::vector<MeshAnnotation>& Annotations() { return myAnnotations; }

    //! Return annotation by index.
    Standard_EXPORT MeshAnnotation* Annotation(int theIndex) const;

    //! Clear all annotations.
    void Clear()
    {
        myAnnotations.clear();
        myTextureAspects.Clear();
    }

    //! Return cached textured aspects.
    const NCollection_DataMap<TCollection_AsciiString, Handle(Graphic3d_AspectFillArea3d)>& Textures() const
    {
        return myTextureAspects;
    }

    //! Set textured aspects.
    void SetTextures(
        const NCollection_DataMap<TCollection_AsciiString, Handle(Graphic3d_AspectFillArea3d)>& theTextures)
    {
        myTextureAspects = theTextures;
    }

    //! Compute presentations using specified aspects.
    Standard_EXPORT void Compute(const Handle(AIS_InteractiveContext)&     theCtx,
                                 const Handle(Prs3d_Presentation)&         thePrs,
                                 const Handle(Prs3d_Presentation)&         theTopPrs,
                                 const Handle(Prs3d_TextAspect)&           theTextAspect,
                                 const Handle(Graphic3d_AspectFillArea3d)& theFillAspect,
                                 const Handle(Graphic3d_AspectMarker3d)&   theTopPointAspect,
                                 const Handle(Graphic3d_AspectMarker3d)&   thePointAspect,
                                 float                                     theResolutionRatio) const;

    //! Compute selection.
    Standard_EXPORT void ComputeSelection(const Handle(ViewBase::InteractiveOwner)&      theOwner,
                                          const Handle(Prs3d_TextAspect)&                 theTextAspect,
                                          NCollection_Map<Handle(Select3D_SensitiveSet)>& theSensitiveSets) const;

    //! Removes highlighting from all annotations.
    Standard_EXPORT void UnhilightAll(const Handle(PrsMgr_PresentationManager)& thePrsMgr);

protected:
    //! Calculate 2d text bounding box.
    Standard_EXPORT Font_Rect calculateTextSize(const TCollection_AsciiString& theText,
                                                const Handle(Font_FTFont)&     theFont) const;

    //! Return font for specified aspect.
    Standard_EXPORT Handle(Font_FTFont) getFont(const Handle(AIS_InteractiveContext)& theCtx,
                                                const Handle(Prs3d_TextAspect)&       theTextAspect) const;

    //! Compute sizes for text/icon for each line.
    Standard_EXPORT void computeAnnotationSizes(const MeshAnnotation&                         theAnnotation,
                                                const Handle(Font_FTFont)&                    theFont,
                                                std::vector<std::pair<Font_Rect, Font_Rect>>& theSizes) const;

    //! Return fill area aspect for icon.
    Standard_EXPORT Handle(Graphic3d_AspectFillArea3d) getTextureAspect(const TCollection_AsciiString& theIcon) const;

    //! Compute icon for the specified line.
    Standard_EXPORT void computeIcon(const MeshAnnotation&             theAnnotation,
                                     const gp_Pnt&                     thePos,
                                     int                               theLine,
                                     const Font_Rect&                  theSize,
                                     const Handle(Prs3d_Presentation)& theTopPrs,
                                     float                             theResolutionRatio) const;

    //! Compute text for the specified line.
    Standard_EXPORT void computeText(const MeshAnnotation&                     theAnnotation,
                                     const gp_Pnt&                             thePos,
                                     int                                       theLine,
                                     const Font_Rect&                          theSize,
                                     const Handle(Prs3d_Presentation)&         theTopPrs,
                                     const Handle(Prs3d_TextAspect)&           theTextAspect,
                                     const Handle(Graphic3d_AspectFillArea3d)& theFillAspect) const;

    //! Compute selection.
    Standard_EXPORT void computeSelection(const Handle(ViewBase::InteractiveOwner)&      theOwner,
                                          MeshAnnotation&                                 theAnnotation,
                                          const Font_Rect&                                theTextSize,
                                          const Font_Rect&                                theIconSize,
                                          int                                             theLineIndex,
                                          NCollection_Map<Handle(Select3D_SensitiveSet)>& theSensitiveSets) const;

protected:
    mutable std::vector<MeshAnnotation> myAnnotations;
    mutable Handle(Font_FTFont)         myFont;
    bool                                myToDrawTextBack = true;

    mutable NCollection_DataMap<TCollection_AsciiString, Handle(Graphic3d_AspectFillArea3d)> myTextureAspects;
};

} // namespace

#endif // MESHANNOTATIONS_HEADERFILE

