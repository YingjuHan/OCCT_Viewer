#ifndef SIMMESH_VERTEXPAIR_HEADERFILE
#define SIMMESH_VERTEXPAIR_HEADERFILE

#include <SimMesh_TargetType.h>
#include <SimMesh_Types.h>

//! Vertex pair definition with auxiliary data
//! and compare operation for storing in the heap.
struct SimMesh_VertexPair {
    SimMesh_Vec3d      TargetVec; //!< vertex position after contraction
    SimMesh_Id         Vert1;     //!< index of the first  vertex in the pair
    SimMesh_Id         Vert2;     //!< index of the second vertex in the pair
    size_t             HeapPos;   //!< current position in a heap; InvalidPos by default (not in a heap)
    float              HeapKey;   //!< a value for sorting in a heap; 0 by default
    float              BackupKey; //!< backup value of the heap key
    float              Distance;  //!< distance of the pair
    SimMesh_TargetType TargetType;

    static constexpr size_t InvalidPos = std::numeric_limits<size_t>::max();

    //! Empty constructor.
    SimMesh_VertexPair()
        : TargetVec(),
          Vert1(0),
          Vert2(0),
          HeapPos(InvalidPos),
          HeapKey(0.f),
          BackupKey(0.f),
          Distance(0.f),
          TargetType(SimMesh_TargetType::Undefined)
    {
    }

    //! Return TRUE if item has been put into a heap,
    bool IsInHeap() const { return HeapPos != InvalidPos; }

    //! Unset flag to unregister item from a heap,
    void SetNotInHeap() { HeapPos = InvalidPos; }

    //! Tells if the originally computed value has been backed up
    bool HasBackup() const { return BackupKey != 0.f; }

    //! Backs up the original heap key
    void Backup()
    {
        if (!HasBackup()) {
            BackupKey = HeapKey;
        }
    }

    void ClearBackup() { BackupKey = 0.f; }
};

#endif // SIMMESH_VERTEXPAIR_HEADERFILE