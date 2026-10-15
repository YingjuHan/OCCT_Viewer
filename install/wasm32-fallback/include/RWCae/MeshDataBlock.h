
#ifndef RWCBASE_MESHDATABLOCK_HEADERFILE
#define RWCBASE_MESHDATABLOCK_HEADERFILE

#include <RWCae/MeshDataRange.h>

#include <TCollection_AsciiString.hxx>

#include <vector>

namespace RWCae
{
    //! Class that defines a block inside mesh data.
    //! Each block has array of children so they form a tree structure.
    class MeshDataBlock : public Standard_Transient
    {
    public:
        //! Constructor.
        Standard_EXPORT explicit MeshDataBlock(const TCollection_AsciiString &theName);

        //! Destructor.
        Standard_EXPORT virtual ~MeshDataBlock();

        //! Return block's name.
        const TCollection_AsciiString &Name() const { return myName; }

        //! Check if the block has a parent block.
        bool HasParent() const { return myParent != nullptr; }

        //! Return parent block.
        MeshDataBlock *Parent() const { return myParent; }

        //! Check if the block has children blocks.
        bool HasChildren() const { return !myChildren.empty(); }

        //! Return children blocks.
        const std::vector<Handle(MeshDataBlock)> &Children() const { return myChildren; }

        //! Return non-const children blocks.
        std::vector<Handle(MeshDataBlock)> &ChangeChildren() { return myChildren; }

        //! Append child block.
        Standard_EXPORT bool AppendChild(const Handle(MeshDataBlock) & theChild);

        //! Append child block.
        Standard_EXPORT const Handle(MeshDataBlock) & AppendChild(const TCollection_AsciiString &theChildName);

        //! Return child block by name.
        Standard_EXPORT Handle(MeshDataBlock) GetChild(const TCollection_AsciiString &theName);

        //! Return child block by chain of names.
        Standard_EXPORT Handle(MeshDataBlock) GetChild(const std::vector<TCollection_AsciiString> &theNamesChain);

        //! Return nodes/elements range.
        MeshDataRange &Range() { return myRange; }

        //! Fill ranges of inner blocks and sort children in ascending order of ranges.
        Standard_EXPORT bool FillAndSortRanges() const;

        //! Write blocks to JSON string.
        Standard_EXPORT TCollection_AsciiString ToJson() const;

        //! Read blocks from JSON string.
        Standard_EXPORT bool FromJson(const TCollection_AsciiString &theJsonStr);

        //! Return block's path as concatenation of blocks' names from the root to this one separated by '/'.
        Standard_EXPORT TCollection_AsciiString Path() const;

    protected:
        MeshDataBlock *myParent = nullptr;
        std::vector<Handle(MeshDataBlock)> myChildren;
        TCollection_AsciiString myName;
        MeshDataRange myRange;
    };

} // namespace

#endif // _MESHDATABLOCK_HEADERFILE
