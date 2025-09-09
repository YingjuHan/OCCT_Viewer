#ifndef SIMMESH_VERTEXPAIRLIST_HEADERFILE
#define SIMMESH_VERTEXPAIRLIST_HEADERFILE

#include <SimMesh_VertexPair.h>

#include <vector>

//! Vector of pairs with remove item operation.
class SimMesh_VertexPairList : public std::vector<SimMesh_VertexPair*> {
public:
    //! Empty constructor.
    SimMesh_VertexPairList() { reserve(6); }

    //! Remove specific item by swapping and popping out back.
    bool RemoveItem(SimMesh_VertexPair* theValue)
    {
        auto aFound = std::remove(begin(), end(), theValue);
        if (aFound != end()) {
            erase(aFound, end());
            return true;
        }
        return false;
    }
};

#endif // SIMMESH_VERTEXPAIRLIST_HEADERFILE