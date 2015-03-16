/*
 * Copyright (C) 2014 Pavel Kirienko <pavel.kirienko@gmail.com>
 */

#ifndef UAVCAN_NODE_GLOBAL_DATA_TYPE_REGISTRY_HPP_INCLUDED
#define UAVCAN_NODE_GLOBAL_DATA_TYPE_REGISTRY_HPP_INCLUDED

#include <cassert>
#include <uavcan/error.hpp>
#include <uavcan/stdint.hpp>
#include <uavcan/data_type.hpp>
#include <uavcan/util/bitset.hpp>
#include <uavcan/util/templates.hpp>
#include <uavcan/util/linked_list.hpp>
#if UAVCAN_DEBUG
# include <uavcan/debug.hpp>
#endif

namespace uavcan
{
/**
 * Bit mask where bit at index X is set if there's a Data Type with ID X.
 */
typedef BitSet<DataTypeID::Max + 1> DataTypeIDMask;

/**
 * This singleton is shared among all existing node instances. It is instantiated automatically
 * when the C++ runtime executes contstructors before main().
 *
 * Its purpose is to keep the list of all UAVCAN data types known and used by this application.
 *
 * Also, the mapping between Data Type name and its Data Type ID is also stored in this singleton.
 * UAVCAN data types with default Data Type ID that are autogenerated by the libuavcan DSDL compiler
 * are registered automatically before main() (refer to the generated headers to see how exactly).
 * Data types that don't have a default Data Type ID must be registered manually using the methods
 * of this class (read the method documentation).
 *
 * Attempt to use a data type that was not registered with this singleton (e.g. publish, subscribe,
 * perform a service call etc.) will fail with an error code @ref ErrUnknownDataType.
 */
class UAVCAN_EXPORT GlobalDataTypeRegistry : Noncopyable
{
    struct Entry : public LinkedListNode<Entry>
    {
        DataTypeDescriptor descriptor;

        Entry() { }

        Entry(DataTypeKind kind, DataTypeID id, const DataTypeSignature& signature, const char* name)
            : descriptor(kind, id, signature, name)
        { }
    };

    struct EntryInsertionComparator
    {
        const DataTypeID id;
        explicit EntryInsertionComparator(const Entry* dtd)
            : id((dtd == NULL) ? DataTypeID() : dtd->descriptor.getID())
        {
            UAVCAN_ASSERT(dtd != NULL);
        }
        bool operator()(const Entry* entry) const
        {
            UAVCAN_ASSERT(entry != NULL);
            return entry->descriptor.getID() > id;
        }
    };

public:
    /**
     * Result of data type registration
     */
    enum RegistrationResult
    {
        RegistrationResultOk,            ///< Success, data type is now registered and can be used.
        RegistrationResultCollision,     ///< Data type name or ID is not unique.
        RegistrationResultInvalidParams, ///< Invalid input parameters.
        RegistrationResultFrozen         ///< Data Type Registery has been frozen and can't be modified anymore.
    };

private:
    typedef LinkedListRoot<Entry> List;
    mutable List msgs_;
    mutable List srvs_;
    bool frozen_;

    GlobalDataTypeRegistry() : frozen_(false) { }

    List* selectList(DataTypeKind kind) const;

    RegistrationResult remove(Entry* dtd);
    RegistrationResult registImpl(Entry* dtd);

public:
    /**
     * Returns the reference to the singleton.
     */
    static GlobalDataTypeRegistry& instance();

    /**
     * Register a data type 'Type' with ID 'id'.
     * If this data type was registered earlier, its old registration will be overridden.
     * This method will fail if the data type registry is frozen.
     *
     * @tparam Type     Autogenerated UAVCAN data type to register. Data types are generated by the
     *                  libuavcan DSDL compiler from DSDL definitions.
     *
     * @param id        Data Type ID for this data type.
     */
    template <typename Type>
    RegistrationResult registerDataType(DataTypeID id);

    /**
     * Data Type registry needs to be frozen before a node instance can use it in
     * order to prevent accidental change in data type configuration on a running
     * node.
     *
     * This method will be called automatically by the node during start up, so
     * the user does not need to call it from the application manually. Subsequent
     * calls will not have any effect.
     *
     * Once frozen, data type registry can't be unfrozen.
     */
    void freeze();
    bool isFrozen() const { return frozen_; }

    /**
     * Finds data type descriptor by full data type name, e.g. "uavcan.protocol.NodeStatus".
     * Returns null pointer if the data type with this name is not registered.
     * @param kind  Data Type Kind - message or service
     * @param name  Full data type name
     * @return      Descriptor for this data type or null pointer if not found
     */
    const DataTypeDescriptor* find(DataTypeKind kind, const char* name) const;

    /**
     * Finds data type descriptor by data type ID.
     * Returns null pointer if the data type with this ID is not registered.
     * @param kind  Data Type Kind - message or service
     * @param dtid  Data Type ID
     * @return      Descriptor for this data type or null pointer if not found
     */
    const DataTypeDescriptor* find(DataTypeKind kind, DataTypeID dtid) const;

    /**
     * Computes Aggregate Signature for all known data types selected by the mask.
     * Please read the DSDL specification.
     * @param[in]       kind            Data Type Kind - messages or services.
     * @param[inout]    inout_id_mask   Data types to compute aggregate signature for; bits at
     *                                  the positions of unknown data types will be cleared.
     * @return          Computed data type signature.
     */
    DataTypeSignature computeAggregateSignature(DataTypeKind kind, DataTypeIDMask& inout_id_mask) const;

    /**
     * Sets the mask so that only bits corresponding to known data types will be set.
     * In other words:
     *  for data_type_id := [0, 1023]
     *      mask[data_type_id] := data_type_exists(data_type_id)
     *
     * @param[in]   kind    Data Type Kind - messages or services.
     * @param[out]  mask    Output mask of known data types.
     */
    void getDataTypeIDMask(DataTypeKind kind, DataTypeIDMask& mask) const;

    /**
     * Returns the number of registered message types.
     */
    unsigned getNumMessageTypes() const { return msgs_.getLength(); }

    /**
     * Returns the number of registered service types.
     */
    unsigned getNumServiceTypes() const { return srvs_.getLength(); }

#if UAVCAN_DEBUG
    /// Required for unit testing
    void reset()
    {
        UAVCAN_TRACE("GlobalDataTypeRegistry", "Reset; was frozen: %i, num msgs: %u, num srvs: %u",
                     int(frozen_), getNumMessageTypes(), getNumServiceTypes());
        frozen_ = false;
        while (msgs_.get())
        {
            msgs_.remove(msgs_.get());
        }
        while (srvs_.get())
        {
            srvs_.remove(srvs_.get());
        }
    }
#endif
};

/**
 * This class is used by autogenerated data types to register with the
 * data type registry automatically before main() is called. Note that
 * if a generated data type header file is not included by any translation
 * unit of the application, the data type will not be registered.
 *
 * Data type needs to have a default ID to be registrable by this class.
 */
template <typename Type>
struct UAVCAN_EXPORT DefaultDataTypeRegistrator
{
    DefaultDataTypeRegistrator()
    {
        const GlobalDataTypeRegistry::RegistrationResult res =
            GlobalDataTypeRegistry::instance().registerDataType<Type>(Type::DefaultDataTypeID);

        if (res != GlobalDataTypeRegistry::RegistrationResultOk)
        {
            handleFatalError("Type reg failed");
        }
    }
};

// ----------------------------------------------------------------------------

/*
 * GlobalDataTypeRegistry
 */
template <typename Type>
GlobalDataTypeRegistry::RegistrationResult GlobalDataTypeRegistry::registerDataType(DataTypeID id)
{
    if (isFrozen())
    {
        return RegistrationResultFrozen;
    }
    static Entry entry;
    {
        const RegistrationResult remove_res = remove(&entry);
        if (remove_res != RegistrationResultOk)
        {
            return remove_res;
        }
    }
    entry = Entry(DataTypeKind(Type::DataTypeKind), id, Type::getDataTypeSignature(), Type::getDataTypeFullName());
    {
        const RegistrationResult remove_res = remove(&entry);
        if (remove_res != RegistrationResultOk)
        {
            return remove_res;
        }
    }
    return registImpl(&entry);
}

}

#endif // UAVCAN_NODE_GLOBAL_DATA_TYPE_REGISTRY_HPP_INCLUDED
