#pragma once
#include "uml/uml-stable.h"
#include "umlAbstractionStoragePolicy.h"

#ifndef MAX_ABSTRACTED_TYPES
#define MAX_ABSTRACTED_TYPES 20
#endif

namespace UML {

    const std::string umlStringID = "string_L&R5eAEq6f3LUNtUmzHzT";
    const std::string umlBoolID = "bool_bzkcabSy3CiFd&HmJOtnVRK";
    const std::string umlIntID = "int_r9nNbBukx47IomXrT2raqtc4";
    const std::string umlRealID = "real_aZG&w6yl61bXVWutgeyScN9";

    template <class, class>
    struct TupleCat;

    template <class ... Left, class ... Right>
    struct TupleCat<std::tuple<Left...>, std::tuple<Right...>> {
        using type = std::tuple<Left..., Right...>;
    };

    template <std::size_t Type, std::size_t TotalTypes>
    struct GeneratedType;

    template <std::size_t I, std::size_t TotalTypes>
    struct GetGeneratedTypes {
        typedef typename TupleCat<std::tuple<GeneratedType<I - 1, TotalTypes>>, typename GetGeneratedTypes<I - 1, TotalTypes>::Types>::type Types;
    };

    template <std::size_t TotalTypes>
    struct GetGeneratedTypes<0, TotalTypes> {
        typedef std::tuple<> Types;
    };

    template <std::size_t TotalTypes>
    struct GetGeneratedTypes<1, TotalTypes> {
        typedef std::tuple<GeneratedType<0, TotalTypes>> Types;
    };

    struct SetInfo {
        std::string name;
        std::unique_ptr<AbstractSet> set;
    };

    struct PrimitiveTypeInfo {
        std::string name;
        std::shared_ptr<AbstractDataPolicy> policy;
    };

    struct AbstractGeneratedType {
        std::unordered_map<ID, SetInfo> m_sets;
        std::unordered_map<ID, PrimitiveTypeInfo> m_primitiveTypeData;
        std::string m_name;
    };

    template <std::size_t Type, std::size_t TotalTypes>
    struct GeneratedType : public BaseElement<typename GetGeneratedTypes<TotalTypes, TotalTypes>::Types>, public AbstractGeneratedType {
        using Info = TypeInfo<std::tuple<>, GeneratedType<Type, TotalTypes>>;
        using ManagerTypes = typename GetGeneratedTypes<TotalTypes, TotalTypes>::Types;
        private:
            template <std::size_t I = 0>
            std::unique_ptr<AbstractSet> makeSet(PropertyPtr property, std::size_t elementType) {
                if constexpr (I < TotalTypes) {
                    if (property->getType() != elementType) {
                        return makeSet<I + 1>(property, elementType);
                    }
                    std::unique_ptr<AbstractSet> ret = 0;
                    if (property->getUpperValue()) {
                        auto upperValue = property->getUpperValue();
                        if (upperValue->is<LiteralInt>()) {
                            if (upperValue->as<LiteralInt>().getValue() == 1) {
                                if (property->isReadOnly()) {
                                    ret = std::make_unique<ReadOnlySingleton<GeneratedType<I, TotalTypes>, GeneratedType<Type, TotalTypes>>>(this);
                                } else {
                                    ret = std::make_unique<Singleton<GeneratedType<I, TotalTypes>, GeneratedType<Type, TotalTypes>>>(this);
                                }
                            } else {
                                // TODO
                                throw ManagerStateException("TODO bounded sets!");
                            }
                        } else if (upperValue->is<LiteralUnlimitedNatural>()) {
                            if (property->isReadOnly()) {
                                if (property->isOrdered()) {
                                    ret = std::make_unique<ReadOnlyOrderedSet<GeneratedType<I, TotalTypes>, GeneratedType<Type, TotalTypes>>>(this);
                                } else {
                                    ret = std::make_unique<ReadOnlySet<GeneratedType<I, TotalTypes>, GeneratedType<Type, TotalTypes>>>(this);
                                }
                            } else {
                                if (property->isOrdered()) {
                                    ret = std::make_unique<OrderedSet<GeneratedType<I, TotalTypes>, GeneratedType<Type, TotalTypes>>>(this);
                                } else {
                                    ret = std::make_unique<Set<GeneratedType<I, TotalTypes>, GeneratedType<Type, TotalTypes>>>(this);
                                }
                            }
                        } else {
                            // TODO
                            throw ManagerStateException("TODO, other type bounds");
                        }
                    }
                    if (property->getAssociation()) {
                        PropertyPtr opposite;
                        for (auto member : property->getAssociation()->getMemberEnds().ptrs()) {
                            if (member.id() != property.id()) {
                                opposite = member;
                            }
                        }

                        if (!opposite) {
                            // TODO throw error
                            throw ManagerStateException("could not find opposite while generating property! bad stat!");
                        }

                        struct GeneratedOppositeInterface : public OppositeInterface<GeneratedType<I, TotalTypes>> {
                            ID oppositeID;
                            GeneratedType<Type, TotalTypes>& me;
                            GeneratedOppositeInterface(ID oppositeID, GeneratedType<Type, TotalTypes>& me) : oppositeID(oppositeID), me(me) {}
                            bool enabled() override {
                                return true;
                            }
                            void addOpposite(GeneratedType<I, TotalTypes>& el) override {
                                this->nonOppositeAddHelper(*el.m_sets.at(oppositeID).set, AbstractElementPtr(&me));
                            }
                            void removeOpposite(GeneratedType<I, TotalTypes>& el) override {
                                this->nonOppositeRemoveHelper(*el.m_sets.at(oppositeID).set, AbstractElementPtr(&me));
                            } 
                        };

                        dynamic_cast<Set<GeneratedType<I, TotalTypes>, GeneratedType<Type, TotalTypes>>*>(ret.get())->opposite(std::make_unique<GeneratedOppositeInterface>(opposite.id(), *this));
                    }
                    return ret;
                }
                throw ManagerStateException("Type not found in manager!");
            }
        public:
            GeneratedType(std::size_t elementType, AbstractManager& manager) : BaseElement<ManagerTypes>(elementType, manager) {
                Manager<ManagerTypes, UmlAbstractionStoragePolicy<ManagerTypes>>& typed_manager = dynamic_cast<Manager<ManagerTypes, UmlAbstractionStoragePolicy<ManagerTypes>>&>(manager);
                // get type from uml
                ClassifierPtr umlType = typed_manager.m_manager->get().abstractGet(typed_manager.m_umlTypeIds.at(Type));

                // set name from uml type
                m_name = umlType->getName();

                // set up sets
                std::list<ClassifierPtr> queue = { umlType };
                std::unordered_set<ClassifierPtr> visited;
                while (!queue.empty()) {
                    ClassifierPtr front = queue.front();
                    queue.pop_front();
                    if (visited.count(front)) {
                        continue;
                    }
                    visited.emplace(front);
                    for (PropertyPtr property : front->getAttributes().ptrs()) {
                        // TODO figure out what type of set from multiplicity
                        TypePtr propertyType = property->getType();
                        if (!propertyType) {
                            // TODO log warning?
                            continue;
                        }

                        auto propertyTypeID = propertyType.id().string();
                        if (propertyTypeID == umlStringID) {
                            struct StringDataPolicy : public AbstractDataPolicy {
                                std::string m_data;
                                std::string getData() override {
                                    return m_data;
                                }
                                void setData(std::string data) override {
                                    m_data = data;
                                }
                            };
                            m_primitiveTypeData.emplace(property.id(), PrimitiveTypeInfo {
                                property->getName(),
                                std::make_shared<StringDataPolicy>()
                            });
                        } else if (propertyTypeID == umlBoolID) {
                            struct BoolDataPolicy : public AbstractDataPolicy {
                                bool m_data = false;
                                std::string getData() override {
                                    return m_data ? "true" : "false";
                                }
                                void setData(std::string data) override {
                                    if (data == "true") {
                                        m_data = true;
                                    } else if (data == "false") {
                                        m_data = false;
                                    } else {
                                        throw ManagerStateException("Bad boolean value given to generated type!");
                                    }
                                }
                            };
                            m_primitiveTypeData.emplace(property.id(), PrimitiveTypeInfo {
                                property->getName(),
                                std::make_shared<BoolDataPolicy>()
                            });
                        } else if (propertyTypeID == umlIntID) {
                            struct IntDataPolicy : public AbstractDataPolicy {
                                std::size_t m_data = 0;
                                std::string getData() override {
                                    return std::to_string(m_data);
                                }
                                void setData(std::string data) override {
                                    m_data = std::stoi(data);
                                }
                            };
                            m_primitiveTypeData.emplace(property.id(), PrimitiveTypeInfo {
                                property->getName(),
                                std::make_shared<IntDataPolicy>()
                            });
                        } else if (propertyTypeID == umlRealID) {
                            struct RealDataPolicy : public AbstractDataPolicy {
                                double m_data = 0;
                                std::string getData() override {
                                    return std::to_string(m_data);
                                }
                                void setData(std::string data) override {
                                    m_data = std::stod(data);
                                }
                            };
                            m_primitiveTypeData.emplace(property.id(), PrimitiveTypeInfo {
                                property->getName(),
                                std::make_shared<RealDataPolicy>()
                            });
                        } else {
                            // it's a set
                            SetInfo& createdSet = m_sets.emplace(
                                property->getID(),
                                SetInfo {
                                    property->getName(), 
                                    this->makeSet(property, typed_manager.m_typesByID.at(property->getType().id()))
                                }
                            ).first->second;

                            for (PropertyPtr subsettedProperty : property->getSubsettedProperties().ptrs()) {
                                createdSet.set->subsets(*m_sets.at(subsettedProperty.id()).set);
                            }

                            for (PropertyPtr redefinedProperty : property->getRedefinedProperties().ptrs()) {
                                createdSet.set->redefines(*m_sets.at(redefinedProperty.id()).set);
                            }
                        }
                    }
                }
            }
    };

    template<std::size_t Type, std::size_t TotalTypes>
    struct ElementInfo<GeneratedType<Type, TotalTypes>> {
        static const bool abstract = false;
        static std::string name(GeneratedType<Type, TotalTypes>& el) {
            return el.m_name;
        }
        static SetList sets(GeneratedType<Type, TotalTypes>& el) {
            SetList ret;
            for (auto& setPair : el.m_sets) {
                ret.push_back(makeSetPair(setPair.second.name.c_str(), *setPair.second.set));
            }
            return ret;
        }
        static const bool extraData = true;
        static DataList data(GeneratedType<Type, TotalTypes>& el) {
            DataList ret;
            struct WrapperDataPolicy : public AbstractDataPolicy {
                std::weak_ptr<AbstractDataPolicy> m_policy;
                WrapperDataPolicy(std::shared_ptr<AbstractDataPolicy> ptr) {
                    m_policy = ptr;
                }
                std::string getData() override {
                    return m_policy.lock()->getData();
                }
                void setData(std::string data) override {
                    m_policy.lock()->setData(data);
                }
            };
            for (auto& dataPair : el.m_primitiveTypeData) {
                ret.push_back(createDataPair(dataPair.second.name.c_str(), new WrapperDataPolicy(dataPair.second.policy)));
            }
            return ret;
        }
    };

    template <std::size_t I = 0>
    std::unique_ptr<AbstractManager> generateHelper(std::unordered_set<ClassifierPtr>& types) {
        if constexpr (I < MAX_ABSTRACTED_TYPES) {
            if (types.size() > I) {
                return generateHelper<I + 1>(types);
            }

            using Types = typename GetGeneratedTypes<I, I>::Types;
            auto ret = std::make_unique<Manager<Types, UmlAbstractionStoragePolicy<Types>>>();
            ret->loadTypes(types);
            return ret;
        }
        throw ManagerStateException("Manager cannot handle this number of abstracted elements in one generated manager! Edit MAX_ABSTRACTED_TYPES with the prepocessor to allow more types to be generated!");
    }

    std::unique_ptr<AbstractManager> generate(PackagePtr package) {
        std::list<PackagePtr> queue = {package};
        std::unordered_set<ClassifierPtr> types;
        while (!queue.empty()) {
            PackagePtr front = queue.front();
            queue.pop_front();
            for (auto packagedEl : front->getPackagedElements().ptrs()) {
                if (packagedEl->is<Package>()) {
                    queue.emplace_back(packagedEl);
                }
                if (packagedEl->is<Classifier>() && !packagedEl->is<Association>()) {
                    types.insert(packagedEl);
                }
            }
        }
        return generateHelper(types);
    }
}