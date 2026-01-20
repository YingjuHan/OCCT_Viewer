#ifndef _MESHDATARANGE_HEADERFILE
#define _MESHDATARANGE_HEADERFILE

#include <TCollection_AsciiString.hxx>

namespace RWCae
{
    //! Mesh sub-group definition
    struct MeshDataRange
    {
        Standard_Integer NodeLower; //!< index of lower Vertex
        Standard_Integer NodeUpper; //!< index of upper Vertex
        Standard_Integer ElemLower; //!< index of lower Element
        Standard_Integer ElemUpper; //!< index of upper Element

        //! Constructor.
        explicit MeshDataRange(int theNodeLower = -1, int theNodeUpper = -1, int theElemLower = -1, int theElemUpper = -1)
        {
            SetValues(theNodeLower, theNodeUpper, theElemLower, theElemUpper);
        }

        //! Return true if group is empty.
        bool IsEmpty() const { return NodeLower < 0 || NodeUpper < 0 || ElemLower < 0 || ElemUpper < 0 || NbNodes() < 1; }

        //! Reset range values.
        void SetEmpty() { SetValues(-1, -1, -1, -1); }

        //! Return number of elements in the group.
        int NbElems() const { return ElemUpper - ElemLower + 1; }

        //! Return number of nodes in the group.
        int NbNodes() const { return NodeUpper - NodeLower + 1; }

        //! Set range values.
        void SetValues(int theNodeLower, int theNodeUpper, int theElemLower, int theElemUpper)
        {
            NodeLower = theNodeLower;
            NodeUpper = theNodeUpper;
            ElemLower = theElemLower;
            ElemUpper = theElemUpper;
        }

        //! Append other range.
        void AppendRange(const MeshDataRange &theRange)
        {
            if (theRange.IsEmpty())
            {
                return;
            }

            if (IsEmpty())
            {
                NodeLower = theRange.NodeLower;
                NodeUpper = theRange.NodeUpper;
                ElemLower = theRange.ElemLower;
                ElemUpper = theRange.ElemUpper;
                return;
            }

            NodeLower = Min(NodeLower, theRange.NodeLower);
            NodeUpper = Max(NodeUpper, theRange.NodeUpper);
            ElemLower = Min(ElemLower, theRange.ElemLower);
            ElemUpper = Max(ElemUpper, theRange.ElemUpper);
        }
    };

} // namespace ViewCAE

#endif // _MESHDATARANGE_HEADERFILE