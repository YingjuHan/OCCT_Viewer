#include <RWCae/MeshDataBlock.h>

#include <NCollection_List.hxx>
#include <Standard_ArrayStreamBuffer.hxx>

#include <algorithm>
#include <functional>

#include <Standard_WarningsDisable.hxx> // NOLINT
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>
#include <Standard_WarningsRestore.hxx> // NOLINT

namespace RWCae
{

    // ================================================================
    // Function : MeshDataBlock
    // ================================================================
    MeshDataBlock::MeshDataBlock(const TCollection_AsciiString &theName) : myName(theName)
    {
        //
    }

    // ================================================================
    // Function : ~MeshDataBlock
    // ================================================================
    MeshDataBlock::~MeshDataBlock()
    {
        myParent = nullptr;
    }

    // ================================================================
    // Function : AppendChild
    // ================================================================
    bool MeshDataBlock::AppendChild(const Handle(MeshDataBlock) & theChild)
    {
        for (const Handle(MeshDataBlock) &aChildToCheck : myChildren)
        {
            if (aChildToCheck->myName == theChild->myName)
            {
                return false;
            }
        }

        myChildren.push_back(theChild);
        theChild->myParent = this;

        return true;
    }

    // ================================================================
    // Function : AppendChild
    // ================================================================
    const Handle(MeshDataBlock) & MeshDataBlock::AppendChild(const TCollection_AsciiString &theChildName)
    {
        for (const Handle(MeshDataBlock) &aChildToCheck : myChildren)
        {
            if (aChildToCheck->myName == theChildName)
            {
                return aChildToCheck;
            }
        }

        myChildren.push_back(new MeshDataBlock(theChildName));
        const Handle(MeshDataBlock) &aChild = myChildren.back();
        aChild->myParent = this;
        return aChild;
    }

    // ================================================================
    // Function : GetChild
    // ================================================================
    Handle(MeshDataBlock) MeshDataBlock::GetChild(const TCollection_AsciiString &theName)
    {
        for (const Handle(MeshDataBlock) &aChild : myChildren)
        {
            if (aChild->myName == theName)
            {
                return aChild;
            }
        }

        return nullptr;
    }

    // ================================================================
    // Function : GetChild
    // ================================================================
    Handle(MeshDataBlock) MeshDataBlock::GetChild(const std::vector<TCollection_AsciiString> &theNamesChain)
    {
        if (theNamesChain.empty() || myName != theNamesChain[0])
        {
            return nullptr;
        }

        Handle(MeshDataBlock) aNode = this;

        for (int i = 1; i < (int)theNamesChain.size() && !aNode.IsNull(); i++)
        {
            aNode = aNode->GetChild(theNamesChain[i]);
        }

        return aNode;
    }

    // ================================================================
    // Function : FillAndSortRanges
    // ================================================================
    bool MeshDataBlock::FillAndSortRanges() const
    {
        auto sortByRange = [](const Handle(MeshDataBlock) & theNode1, const Handle(MeshDataBlock) & theNode2) -> bool
        {
            return theNode1->Range().ElemLower < theNode2->Range().ElemLower;
        };

        std::function<bool(const Handle(MeshDataBlock) &)> fillAndSortRanges = [&](const Handle(MeshDataBlock) & theNode)
        {
            if (!theNode->HasChildren())
            {
                return !theNode->Range().IsEmpty();
            }

            theNode->myRange.SetEmpty();

            for (Handle(MeshDataBlock) &aChild : theNode->myChildren)
            {
                if (!fillAndSortRanges(aChild))
                {
                    return false;
                }
                theNode->myRange.AppendRange(aChild->Range());
            }

            std::sort(theNode->myChildren.begin(), theNode->myChildren.end(), sortByRange);

            int aPrevNodeUpper = theNode->myChildren[0]->Range().NodeLower - 1;
            int aPrevElemUpper = theNode->myChildren[0]->Range().ElemLower - 1;

            for (Handle(MeshDataBlock) &aChild : theNode->myChildren)
            {
                if (aChild->Range().NodeLower != aPrevNodeUpper + 1 || aChild->Range().ElemLower != aPrevElemUpper + 1)
                {
                    return false;
                }

                aPrevNodeUpper = aChild->Range().NodeUpper;
                aPrevElemUpper = aChild->Range().ElemUpper;
            }

            return true;
        };

        return fillAndSortRanges(this);
    }

    // ================================================================
    // Function : ToJson
    // ================================================================
    TCollection_AsciiString MeshDataBlock::ToJson() const
    {
        std::ostringstream aStream;
        rapidjson::OStreamWrapper aStringStream(aStream);
        rapidjson::Writer<rapidjson::OStreamWrapper> aWriter(aStringStream);

        std::function<void(const Handle(MeshDataBlock) &)> writeNode = [&](const Handle(MeshDataBlock) & theNode)
        {
            aWriter.StartObject();

            aWriter.Key("name");
            aWriter.String(theNode->Name().ToCString());

            if (theNode->HasChildren())
            {
                aWriter.Key("children");
                aWriter.StartArray();
                for (Handle(MeshDataBlock) &aChild : theNode->myChildren)
                {
                    writeNode(aChild);
                }
                aWriter.EndArray();
            }

            aWriter.EndObject();
        };

        writeNode(this);

        return TCollection_AsciiString(aStream.str().c_str());
    }

    // ================================================================
    // Function : FromJson
    // ================================================================
    bool MeshDataBlock::FromJson(const TCollection_AsciiString &theJsonStr)
    {
        Standard_ArrayStreamBuffer aStreamBuffer(theJsonStr.ToCString(), theJsonStr.Length());
        std::istream aStream(&aStreamBuffer);

        rapidjson::Document aReader;

        rapidjson::IStreamWrapper aFileStream(aStream);
        rapidjson::ParseResult aRes =
            aReader.ParseStream<rapidjson::kParseStopWhenDoneFlag | rapidjson::kParseFullPrecisionFlag,
                                rapidjson::UTF8<>,
                                rapidjson::IStreamWrapper>(aFileStream);
        if (aRes.IsError())
        {
            return false;
        }

        auto findObjectMember = [](const rapidjson::Document::ValueType *theObject,
                                   const char *theName) -> const rapidjson::Document::ValueType *
        {
            if (!theObject->IsObject())
            {
                return nullptr;
            }

            rapidjson::Document::ConstMemberIterator anIter = theObject->FindMember(theName);
            return anIter != theObject->MemberEnd() ? &anIter->value : nullptr;
        };

        std::function<bool(const Handle(MeshDataBlock) &, const rapidjson::Document::ValueType *)> readNode =
            [&](const Handle(MeshDataBlock) & theNode, const rapidjson::Document::ValueType *theValue) -> bool
        {
            const rapidjson::Document::ValueType *aNameField = findObjectMember(theValue, "name");

            if (aNameField == nullptr)
            {
                return false;
            }

            theNode->myName = aNameField->GetString();

            const rapidjson::Document::ValueType *aChildrenField = findObjectMember(theValue, "children");
            if (aChildrenField == nullptr)
            {
                return true;
            }

            for (rapidjson::Value::ConstValueIterator anIter = aChildrenField->Begin(); anIter != aChildrenField->End();
                 ++anIter)
            {
                Handle(MeshDataBlock) aChild = new MeshDataBlock("");
                if (!readNode(aChild, anIter))
                {
                    return false;
                }
                if (!aChild.IsNull())
                {
                    theNode->AppendChild(aChild);
                }
            }

            return true;
        };

        Handle(MeshDataBlock) aNode = this;
        return readNode(aNode, &aReader);
    }

    // ================================================================
    // Function : Path
    // ================================================================
    TCollection_AsciiString MeshDataBlock::Path() const
    {
        TCollection_AsciiString aPath(Name());
        for (MeshDataBlock *aParent = Parent(); aParent != nullptr; aParent = aParent->Parent())
        {
            aPath = aParent->Name() + '/' + aPath;
        }
        return aPath;
    }
}