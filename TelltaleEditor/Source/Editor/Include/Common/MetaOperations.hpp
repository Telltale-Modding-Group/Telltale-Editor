#pragma once

// META OPERATIONS. These are not part of the meta system but provide interfaces to be overriden by common classes

#include <Meta/Meta.hpp>
#include <Resource/ResourceRegistry.hpp>

#include <type_traits>

template<typename MyCommonT>
class AbstractMetaOperationsBucketDerived;

// A collection of operations in relation to each other providing meta classes with virtual function implementations for the common type counterparts
class AbstractMetaOperationsBucket
{
public:

    template<typename OBucket>
    static inline WeakPtr<OBucket> CreateBucketReference(Ptr<Handleable> pObj)
    {
        static_assert(std::is_base_of<AbstractMetaOperationsBucketDerived<typename OBucket::ReferenceCommonType>, OBucket>::value, "This type is not a operation bucket!");
        if(pObj)
        {
            return std::dynamic_pointer_cast<OBucket>(pObj);
        }
        else
        {
            return {};
        }
    }

    template<typename OBucket>
    static inline WeakPtr<OBucket> CreateBucketReference(Ptr<ResourceRegistry> pRegistry, Symbol resourceName, Bool bEnsureLoaded)
    {
        HandleBase hType{};
        hType.SetObject(resourceName);
        return CreateBucketReference<OBucket>(hType.GetBlindObject(pRegistry, bEnsureLoaded));
    }

};

template<typename ReferencedCommonT>
class AbstractMetaOperationsBucketDerived : public AbstractMetaOperationsBucket
{
public:

    using ReferenceCommonType = ReferencedCommonT;

    virtual ~AbstractMetaOperationsBucketDerived() = default;

};

// CHORE RELATED OPERATION BUCKETS

class Chore;

/**
 * Meta operations required for a common type to be a chore resource
 */
class MetaOperationsBucket_ChoreResource : public AbstractMetaOperationsBucketDerived<Chore>
{
public:

    /**
     * Gets the rendering information for rendering this chore resource in the Chore editor.
     */
    virtual void GetRenderParameters(Vector3& bgColourOut, CString& iconName) const = 0;

    /**
     * Gets the length of this chore resource, MetaOperation_GetLength
     */
    virtual Float GetLength() const = 0;

};