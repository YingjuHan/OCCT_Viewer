#ifndef SIMMESH_TRIANGLE_HEADERFILE
#define SIMMESH_TRIANGLE_HEADERFILE

#include <SimMesh_Types.h>

#include <Standard_Assert.hxx>

//! Definition of a mesh triangle.
class SimMesh_Triangle : public SimMesh_Vec3i {
public:
    using SimMesh_Vec3i::NCollection_Vec3;

    //! Replace the vertex with specified Id (not index!) with the new Id.
    void RemapVertex(SimMesh_Id theFromId, SimMesh_Id theToId)
    {
        for (int aVertIter = 0; aVertIter < 3; ++aVertIter) {
            SimMesh_Id& aVert = ChangeData()[aVertIter];
            if (aVert == theFromId) {
                aVert   = theToId;
                myDirty = true;
            }
        }
    }

    //! Return the node opposite to specified edge.
    SimMesh_Id OppositeNode(SimMesh_Id theVert0, SimMesh_Id theVert1) const
    {
        const SimMesh_Id* aData = GetData();
        if (aData[0] != theVert0 && aData[0] != theVert1) {
            return aData[0];
        } else if (aData[1] != theVert0 && aData[1] != theVert1) {
            return aData[1];
        } else {
            Standard_ASSERT_VOID(aData[2] != theVert0 && aData[2] != theVert1, "");
            return aData[2];
        }
    }

    //! Returns triangle normal
    const SimMesh_Vec3d& GetNormal() const { return myNormal; }

    //! Returns non-const triangle normal
    SimMesh_Vec3d& ChangeNormal() const { return myNormal; }

    //! Returns triangle area
    double GetArea() const { return myArea; }

    //! Returns non-const triangle area
    double& ChangeArea() const { return myArea; }

    //! Returns true if the triangle is valid
    bool IsValid() const { return myValid; }

    //! Sets validity
    void SetValid(bool theVal) { myValid = theVal; }

    //! Returns true if the triangle is dirty
    bool IsDirty() const { return myDirty; }

    //! Sets dirty flag
    void SetDirty(bool theVal) const { myDirty = theVal; }

    //! Returns the mark
    uint8_t GetMark() const { return myMark; }

    //! Changes the mark
    uint8_t& ChangeMark() { return myMark; }

    //! Returns the group ID this triangle belongs to
    int GetGroup() const { return myGroup; }

    //! Sets the group ID this triangle belongs to
    void SetGroup(const int theGroup) { myGroup = theGroup; }

private:
    mutable SimMesh_Vec3d myNormal;
    mutable double        myArea  = 0.;
    int                   myGroup = -1;
    mutable bool          myDirty = true; //!< flag of actuality of derived data
    bool                  myValid = true; //!< validity flag
    uint8_t               myMark  = 0;    //!< each triangle is tagged with the number of times
                                          //!  specific pair of vertices occur in it
};

#endif // SIMMESH_TRIANGLE_HEADERFILE