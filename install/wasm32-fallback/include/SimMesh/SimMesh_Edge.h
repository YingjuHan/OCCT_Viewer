#ifndef SIMMESH_EDGE_HEADERFILE
#define SIMMESH_EDGE_HEADERFILE

#include <SimMesh_Types.h>

#include <Standard_Assert.hxx>

//! Definition of a mesh edge.
class SimMesh_Edge : public SimMesh_Vec2i {
public:
    using SimMesh_Vec2i::NCollection_Vec2;

    //! Return the node opposite to the specified one.
    SimMesh_Id OppositeNode(SimMesh_Id theVert) const
    {
        if (theVert == x()) {
            return y();
        }
        Standard_ASSERT_VOID(theVert == y(), "");
        return x();
    }

    //! Replace the vertex with specified Id (not index!) with the new Id.
    void RemapVertex(SimMesh_Id theFromId, SimMesh_Id theToId)
    {
        for (int aVertIter = 0; aVertIter < 2; ++aVertIter) {
            SimMesh_Id& aVert = ChangeData()[aVertIter];
            if (aVert == theFromId) {
                aVert = theToId;
            }
        }
    }

    //! Returns true if the edge is valid
    bool IsValid() const { return myValid; }

    //! Sets validity
    void SetValid(bool theVal) { myValid = theVal; }

    //! Returns true if it is a beam
    bool IsBeam() const { return myBeam; }

    //! Sets beam flag
    void SetBeam(bool theVal) { myBeam = theVal; }

    //! Returns the mark
    uint8_t GetMark() const { return myMark; }

    //! Changes the mark
    uint8_t& ChangeMark() { return myMark; }

    //! Returns the group ID this edge belongs to
    int GetGroup() const { return myGroup; }

    //! Sets the group ID this edge belongs to
    void SetGroup(const int theGroup) { myGroup = theGroup; }

private:
    int     myGroup = -1;
    bool    myValid = true;  //!< validity flag
    bool    myBeam  = false; //!< if true it is a segment of a topological edge without face support
    uint8_t myMark  = 0;     //!< each edge is tagged with the number of times
                             //!  specific pair of vertices occur in it
};

#endif // SIMMESH_EDGE_HEADERFILE