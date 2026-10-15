
#ifndef SIMMESH_VERTEXPAIRHEAP_HEADERFILE
#define SIMMESH_VERTEXPAIRHEAP_HEADERFILE

#include <SimMesh_Vertex.h>
#include <SimMesh_VertexPair.h>

#include <Standard_Assert.hxx>

#include <vector>

//! Heap structure sorting items on insertion
//! and supporting removal of arbitrary item and re-ordering item after changing it's weight.
//! It's internal organization is a binary tree.
class SimMesh_VertexPairHeap : private std::vector<SimMesh_VertexPair*> {
public:
    //! Empty constructor.
    SimMesh_VertexPairHeap() { reserve(64); }

    using std::vector<SimMesh_VertexPair*>::reserve;
    using std::vector<SimMesh_VertexPair*>::clear;

    //! Add new item into the heap.
    void Insert(SimMesh_VertexPair* theItem)
    {
        const size_t anIndex = size();
        push_back(theItem);
        theItem->HeapPos = anIndex;
        upHeap(anIndex);
    }

    //! Update item in the heap or insert new one.
    void Update(SimMesh_VertexPair* theItem, float theVal)
    {
        theItem->HeapKey = theVal;
        if (!theItem->IsInHeap()) {
            Insert(theItem);
            return;
        }

        size_t anIndex = theItem->HeapPos;
        if (anIndex > 0 && theVal > at(getParent(anIndex))->HeapKey) {
            upHeap(anIndex);
        } else {
            downHeap(anIndex);
        }
    }

    //! Pop out a front item.
    SimMesh_VertexPair* Extract()
    {
        if (empty()) {
            return nullptr;
        }

        swapItems(0, size() - 1);
        SimMesh_VertexPair* aDead = back();
        pop_back();

        if (!empty()) {
            downHeap(0);
        }
        aDead->SetNotInHeap();
        return aDead;
    }

    //! Return heap front.
    SimMesh_VertexPair* Top() const { return !empty() ? front() : nullptr; }

    //! Remove an item from the heap.
    bool Remove(SimMesh_VertexPair* theItem)
    {
        if (!theItem->IsInHeap()) {
            return false;
        }

        size_t anIndex = theItem->HeapPos;
        swapItems(anIndex, size() - 1);
        pop_back();
        theItem->SetNotInHeap();
        if (anIndex >= size()) {
            return true;
        }

        if (at(anIndex)->HeapKey < theItem->HeapKey) {
            downHeap(anIndex);
        } else {
            upHeap(anIndex);
        }

        return true;
    }

private: // internal functions for manipulating the heap
    //! Access item at specific position.
    SimMesh_VertexPair* at(size_t theIndex) const
    {
        Standard_ASSERT_VOID(theIndex < size(), "");
        return data()[theIndex];
    }

    //! Access item at specific position.
    SimMesh_VertexPair*& at(size_t theIndex)
    {
        Standard_ASSERT_VOID(theIndex < size(), "");
        return data()[theIndex];
    }

    //! Put item at specified position and updates it's internal index.
    void placeItemTo(SimMesh_VertexPair* theItem, size_t theIndex)
    {
        at(theIndex)     = theItem;
        theItem->HeapPos = theIndex;
    }

    //! Swap items at the given indices.
    void swapItems(size_t theA, size_t theB)
    {
        SimMesh_VertexPair* aTmp = at(theA);
        placeItemTo(at(theB), theA);
        placeItemTo(aTmp, theB);
    }

    //! Get index of the parent item for the item with the given index.
    size_t getParent(size_t theIndex) { return (theIndex - 1) / 2; }

    //! Get index of the left child item for the item with the given index.
    size_t getLeft(size_t theIndex) { return 2 * theIndex + 1; }

    //! Get index of the right child item for the item with the given index.
    size_t getRight(size_t theIndex) { return 2 * theIndex + 2; }

    //! Advance the item with the given index to the correct place up the tree.
    void upHeap(size_t theIndex)
    {
        SimMesh_VertexPair* aMoving = at(theIndex);
        size_t              anIndex = theIndex;
        size_t              aParent = getParent(theIndex);

        while (anIndex > 0 && aMoving->HeapKey > at(aParent)->HeapKey) {
            placeItemTo(at(aParent), anIndex);
            anIndex = aParent;
            aParent = getParent(aParent);
        }

        if (anIndex != theIndex) {
            placeItemTo(aMoving, anIndex);
        }
    }

    //! Advance the item with the given index to the correct place down the tree.
    void downHeap(size_t theIndex)
    {
        SimMesh_VertexPair* aMoving  = at(theIndex);
        size_t              anIndex  = theIndex;
        size_t              aLeft    = getLeft(theIndex);
        size_t              aRight   = getRight(theIndex);
        size_t              aLargest = 0;
        while (aLeft < size()) {
            if (aRight < size() && at(aLeft)->HeapKey < at(aRight)->HeapKey) {
                aLargest = aRight;
            } else {
                aLargest = aLeft;
            }

            if (aMoving->HeapKey < at(aLargest)->HeapKey) {
                placeItemTo(at(aLargest), anIndex);
                anIndex = aLargest;
                aLeft   = getLeft(anIndex);
                aRight  = getRight(anIndex);
            } else {
                break;
            }
        }

        if (anIndex != theIndex) {
            placeItemTo(aMoving, anIndex);
        }
    }
};

#endif // SIMMESH_VERTEXPAIRHEAP_HEADERFILE

