#include <ViewCaeGeom/MeshGeometryBlock.h>

#include <functional>

namespace {
static constexpr const char* NAME_BLOCKS = "blocks";
}

namespace ViewCaeGeom {

// =======================================================================
// function : MeshDataGeomBlock
// =======================================================================
MeshDataGeomBlock::MeshDataGeomBlock(const TCollection_AsciiString& theName) : RWCae::MeshDataBlock(theName)
{
    //
}

// =======================================================================
// function : MeshDataGeomBlock
// =======================================================================
MeshDataGeomBlock::MeshDataGeomBlock(const Handle(RWCae::MeshDataBlock)& theBlock)
    : RWCae::MeshDataBlock(theBlock->Name())
{
    myRange = theBlock->Range();

    for (const Handle(RWCae::MeshDataBlock)& aChild : theBlock->Children()) {
        AppendChild(new MeshDataGeomBlock(aChild));
    }
}

// =======================================================================
// function : ~MeshDataGeomBlock
// =======================================================================
MeshDataGeomBlock::~MeshDataGeomBlock()
{
    //
}

// =======================================================================
// function : MeshGeometryBlock
// =======================================================================
MeshGeometryBlock::MeshGeometryBlock(const Handle(MeshBvhData)&            theBvhData,
                                     const Handle(RWCae::MeshDataBlock)& theBlocks)
    : MeshGeometry(theBvhData)
{
    myBlocks = new MeshDataGeomBlock(theBlocks);
}

// =======================================================================
// function : MeshGeometryBlock
// =======================================================================
MeshGeometryBlock::~MeshGeometryBlock()
{
}

// =======================================================================
// function : GetEnabled
// =======================================================================
Handle(RWCae::MeshDataBlock) MeshGeometryBlock::GetEnabled() const
{
    Handle(RWCae::MeshDataBlock) aRes;

    std::function<void(const MeshDataGeomBlock*, Handle(RWCae::MeshDataBlock)&)> processNode =
        [&](const MeshDataGeomBlock* theNode, Handle(RWCae::MeshDataBlock)& theResNode) {
            if (theNode->Status() == MeshDataGeomBlockStatus::Disabled) {
                return;
            }

            theResNode = new RWCae::MeshDataBlock(theNode->Name());

            if (theNode->Status() == MeshDataGeomBlockStatus::Enabled) {
                return;
            }

            for (const Handle(RWCae::MeshDataBlock)& aSubNode : theNode->Children()) {
                Handle(RWCae::MeshDataBlock) aChild;
                processNode(dynamic_cast<const MeshDataGeomBlock*>(aSubNode.get()), aChild);
                if (!aChild.IsNull()) {
                    theResNode->AppendChild(aChild);
                }
            }
        };

    processNode(myBlocks.get(), aRes);
    return aRes;
}

// =======================================================================
// function : SetEnabled
// =======================================================================
bool MeshGeometryBlock::SetEnabled(const Handle(RWCae::MeshDataBlock)& theBlocks, bool theEnabled)
{
    std::function<bool(MeshDataGeomBlock*, const Handle(RWCae::MeshDataBlock)&)> processNode =
        [&](MeshDataGeomBlock* theNode, const Handle(RWCae::MeshDataBlock)& theRefNode) -> bool {
        if (theNode == nullptr || theRefNode.IsNull() || theNode->Name() != theRefNode->Name()) {
            return false;
        }

        if (!theRefNode->HasChildren()) {
            if ((theNode->Status() == MeshDataGeomBlockStatus::Enabled && theEnabled)
                || (theNode->Status() == MeshDataGeomBlockStatus::Disabled && !theEnabled)) {
                return false;
            }

            MeshDataGeomBlockStatus aChildStatus = theEnabled ? MeshDataGeomBlockStatus::Enabled
                                                              : MeshDataGeomBlockStatus::Disabled;
            updateChildrenStatus(theNode, aChildStatus);
            updateParentsStatus(theNode);

            return true;
        }

        bool aRes = false;
        for (const Handle(RWCae::MeshDataBlock)& aRefChild : theRefNode->Children()) {
            MeshDataGeomBlock* aChild = dynamic_cast<MeshDataGeomBlock*>(theNode->GetChild(aRefChild->Name()).get());
            if (processNode(aChild, aRefChild)) {
                aRes = true;
            }
        }

        return aRes;
    };

    bool aRes = processNode(myBlocks.get(), theBlocks);
    if (aRes) {
        SetToUpdate();
    }

    return aRes;
}

// =======================================================================
// function : SetOnlyEnabled
// =======================================================================
bool MeshGeometryBlock::SetOnlyEnabled(const Handle(RWCae::MeshDataBlock)& theBlocks)
{
    std::function<bool(MeshDataGeomBlock*, const Handle(RWCae::MeshDataBlock)&)> process =
        [&](MeshDataGeomBlock* theNode, const Handle(RWCae::MeshDataBlock)& theRefNode) -> bool {
        if (theNode == nullptr || theRefNode.IsNull() || theNode->Name() != theRefNode->Name()) {
            return false;
        }

        if (!theRefNode->HasChildren()) {
            if (theNode->Status() == MeshDataGeomBlockStatus::Enabled) {
                return false;
            }

            updateChildrenStatus(theNode, MeshDataGeomBlockStatus::Enabled);
            updateParentsStatus(theNode);

            return true;
        }

        bool aRes = false;
        for (const Handle(RWCae::MeshDataBlock)& aRefChild : theRefNode->Children()) {
            MeshDataGeomBlock* aChild = dynamic_cast<MeshDataGeomBlock*>(theNode->GetChild(aRefChild->Name()).get());
            if (process(aChild, aRefChild)) {
                aRes = true;
            }
        }

        for (const Handle(RWCae::MeshDataBlock)& aChild : theNode->Children()) {
            MeshDataGeomBlock* aGeomChild = dynamic_cast<MeshDataGeomBlock*>(aChild.get());
            if (theRefNode->GetChild(aChild->Name()).IsNull()
                && aGeomChild->Status() != MeshDataGeomBlockStatus::Disabled) {
                updateChildrenStatus(aGeomChild, MeshDataGeomBlockStatus::Disabled);
                updateParentsStatus(aGeomChild);
                aRes = true;
            }
        }

        return aRes;
    };

    bool aRes = false;
    if (!theBlocks.IsNull()) {
        aRes = process(myBlocks.get(), theBlocks);
    } else {
        aRes = myBlocks->Status() != MeshDataGeomBlockStatus::Disabled;
        if (aRes) {
            updateChildrenStatus(myBlocks.get(), MeshDataGeomBlockStatus::Disabled);
        }
    }
    if (aRes) {
        SetToUpdate();
    }

    return aRes;
}

// =======================================================================
// function : updateChildrenStatus
// =======================================================================
void MeshGeometryBlock::updateChildrenStatus(MeshDataGeomBlock* theNode, MeshDataGeomBlockStatus theStatus)
{
    NCollection_List<MeshDataGeomBlock*> aQueue;
    aQueue.Append(theNode);
    while (!aQueue.IsEmpty()) {
        MeshDataGeomBlock* aQueueNode = aQueue.First();
        aQueue.RemoveFirst();

        aQueueNode->SetStatus(theStatus);

        for (const Handle(RWCae::MeshDataBlock)& aChild : aQueueNode->Children()) {
            aQueue.Append(dynamic_cast<MeshDataGeomBlock*>(aChild.get()));
        }
    }
}

// =======================================================================
// function : updateParentsStatus
// =======================================================================
void MeshGeometryBlock::updateParentsStatus(const MeshDataGeomBlock* theNode)
{
    for (MeshDataGeomBlock* aParent = dynamic_cast<MeshDataGeomBlock*>(theNode->Parent()); aParent != nullptr;
         aParent                    = dynamic_cast<MeshDataGeomBlock*>(aParent->Parent())) {
        bool hasEnabled  = false;
        bool hasDisabled = false;
        for (const Handle(RWCae::MeshDataBlock)& aChild : aParent->Children()) {
            MeshDataGeomBlockStatus aStatus = dynamic_cast<MeshDataGeomBlock*>(aChild.get())->Status();
            if (aStatus == MeshDataGeomBlockStatus::Enabled) {
                hasEnabled = true;
            } else if (aStatus == MeshDataGeomBlockStatus::Disabled) {
                hasDisabled = true;
            } else {
                hasEnabled  = true;
                hasDisabled = true;
            }

            if (hasEnabled && hasDisabled) {
                break;
            }
        }

        if (hasEnabled && hasDisabled) {
            aParent->SetStatus(MeshDataGeomBlockStatus::Partial);
        } else if (hasEnabled) {
            aParent->SetStatus(MeshDataGeomBlockStatus::Enabled);
        } else {
            aParent->SetStatus(MeshDataGeomBlockStatus::Disabled);
        }
    }
}

// =======================================================================
// function : acceptElement
// =======================================================================
bool MeshGeometryBlock::acceptElement(const Handle(RWCae::MeshData)&,
                                      const RWCae::MeshEntityId theElemIdx,
                                      const MeshGeometry*) const
{
    for (MeshDataGeomBlock* aNode = myBlocks.get(); aNode != nullptr;) {
        if (aNode->Status() != MeshDataGeomBlockStatus::Partial) {
            return aNode->Status() == MeshDataGeomBlockStatus::Enabled;
        }

        int i;
        for (i = 0; i < (int)aNode->Children().size(); i++) {
            const RWCae::MeshDataRange& aRange = aNode->Children()[i]->Range();
            if ((int)theElemIdx >= aRange.ElemLower && (int)theElemIdx <= aRange.ElemUpper) {
                break;
            }
        }

        if (i == (int)aNode->Children().size())
            return false;

        aNode = dynamic_cast<MeshDataGeomBlock*>(aNode->Children()[i].get());
    }
    return true;
}

// =======================================================================
// function : acceptNode
// =======================================================================
bool MeshGeometryBlock::acceptNode(const RWCae::MeshEntityId theNodeIdx,
                                   const MeshGeometry*,
                                   const Graphic3d_Vec3&) const
{
    for (MeshDataGeomBlock* aNode = myBlocks.get(); aNode != nullptr;) {
        if (aNode->Status() != MeshDataGeomBlockStatus::Partial) {
            return aNode->Status() == MeshDataGeomBlockStatus::Enabled;
        }

        int i;
        for (i = 0; i < (int)aNode->Children().size(); i++) {
            const RWCae::MeshDataRange& aRange = aNode->Children()[i]->Range();
            if ((int)theNodeIdx >= aRange.NodeLower && (int)theNodeIdx <= aRange.NodeUpper) {
                break;
            }
        }

        if (i == (int)aNode->Children().size())
            return false;

        aNode = dynamic_cast<MeshDataGeomBlock*>(aNode->Children()[i].get());
    }
    return true;
}

// =======================================================================
// function : ToJson
// =======================================================================
void MeshGeometryBlock::ToJson(RWBase::JsonHelper::Writer& theJson)
{
    Handle(RWCae::MeshDataBlock) aBlocks = GetEnabled();
    theJson.Key(NAME_BLOCKS);
    theJson.String(!aBlocks.IsNull() ? aBlocks->ToJson().ToCString() : "");
}

// =======================================================================
// function : FromJson
// =======================================================================
bool MeshGeometryBlock::FromJson(const RWBase::JsonHelper::Value& theJson,
                                 const Handle(RWCae::MeshData)&,
                                 int,
                                 bool& theIsChanged)
{
    const RWBase::JsonHelper::Value* aBlocksField = RWBase::JsonHelper::FindStringMember(theJson, NAME_BLOCKS);
    if (aBlocksField == nullptr) {
        return false;
    }

    TCollection_AsciiString aBlocksJson = aBlocksField->GetString();

    Handle(RWCae::MeshDataBlock) aBlocks;
    if (!aBlocksJson.IsEmpty()) {
        aBlocks = new RWCae::MeshDataBlock("");
        if (!aBlocks->FromJson(aBlocksField->GetString())) {
            return false;
        }
    }

    if (SetOnlyEnabled(aBlocks)) {
        theIsChanged = true;
    }

    return true;
}

} // namespace
