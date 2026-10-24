#ifndef SIMMESH_VECTOROFID_HEADERFILE
#define SIMMESH_VECTOROFID_HEADERFILE

#include <SimMesh_Types.h>

#include <algorithm>
#include <vector>

//! This class serves to store a list of Ids.
//! Usually an exemlar of it stores a direct neighbors of a mesh vertex.
class SimMesh_IdList : public std::vector<SimMesh_Id> {
public:
    explicit SimMesh_IdList(size_t theNbReserved = 0) { reserve(theNbReserved); }

    //! Remove specific item by swapping and popping out back.
    bool RemoveItem(SimMesh_Id theItem)
    {
        auto aFound = std::remove(begin(), end(), theItem);
        if (aFound != end()) {
            erase(aFound, end());
            return true;
        }
        return false;
    }

    //! Return TRUE if the list contains specific item.
    bool Contains(SimMesh_Id theItem) const { return std::find(cbegin(), cend(), theItem) != cend(); }
};

#endif

