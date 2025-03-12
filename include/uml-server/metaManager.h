#pragma once

#include "uml/uml-stable.h"

#define NUMBER_META_MANAGERS 3
#define ABSTRACTION_SIZE 10

namespace UML {

    struct AbstractMetaElement : virtual public EGM::AbstractElement {
        std::unordered_map<EGM::ID, std::unique_ptr<EGM::AbstractSet>> m_sets;
        std::vector<std::size_t> m_bases;
        std::string m_name = ""; 
    };
    
    template <std::size_t ManagerIndex, std::size_t I, class ManagerPolicy>
    struct MetaElement : public ManagerPolicy , public AbstractMetaElement {
        template <class P>
        using TemplateMetaElement = MetaElement<ManagerIndex, I, P>;
        using Info = EGM::TypeInfo<TemplateMetaElement>;
        MANAGED_ELEMENT_CONSTRUCTOR(MetaElement);
        private:
            void init() {}
    };

    struct AbstractMetaManager {
        virtual std::string name(std::size_t type) const = 0;
        virtual UmlManager& getUmlManager() const = 0;
        virtual std::size_t getIndex() const = 0;
    };
    
    template <std::size_t ManagerIndex, std::size_t I>
    struct CreateList {
        template <class P>
        using TemplateMetaElement = MetaElement<ManagerIndex, I, P>;
        using result = typename EGM::TemplateTypeListCat<EGM::TemplateTypeList<TemplateMetaElement>, typename CreateList<ManagerIndex, I -1>::result>::result;
    };

    template <std::size_t ManagerIndex>
    struct CreateList<ManagerIndex, 0> {
        template <class P>
        using TemplateMetaElement = MetaElement<ManagerIndex, 0, P>;
        using result = EGM::TemplateTypeList<TemplateMetaElement>;
    };

    template <std::size_t ManagerIndex, std::size_t I>
    using CreateList_t = typename CreateList<ManagerIndex, I>::result;

    
    template <std::size_t ManagerIndex, std::size_t Size>
    class MetaManager : public EGM::Manager<CreateList_t<ManagerIndex, Size>>, public AbstractMetaManager {
        friend struct ActiveMetaManagers;
        private:
            using BaseManager = EGM::Manager<CreateList_t<ManagerIndex, Size>>;
            UmlManager& m_uml_manager;
            std::unordered_map<std::size_t, UmlManager::Pointer<Classifier>> m_uml_types;
            std::unordered_map<EGM::ID, std::size_t> m_id_to_type;

            struct AbstractMetaTypePolicy {
                virtual EGM::ManagedPtr<AbstractMetaElement> create() const = 0;
            };

            template <std::size_t I>
            struct MetaTypePolicy : public AbstractMetaTypePolicy {
                template <class Policy>
                using MetaType = MetaElement<ManagerIndex, I, Policy>;
                EGM::ManagedPtr<AbstractMetaElement> create() const override { 
                    auto meta_element = BaseManager::template create<MetaType>();
                    
                    // get representing classifier
                    auto meta_type = m_uml_types.at(I);

                    // set name
                    meta_element->m_name = meta_type->getName();

                    // set bases
                    for (auto base : meta_type->getGenerals().ptrs()) {
                        meta_element->m_bases.push_back(m_id_to_type.at(base.id()));
                    }

                    // set sets
                    std::list<UmlManager::Pointer<Classifier>> queue = { meta_type };
                    while (!queue.empty()) {
                        auto front = queue.front();
                        queue.pop_front();
                        for (auto property : front->getAttributes().ptrs()) {
                            if (property->getType()) {
                                // TODO make proper set type based on multiplicity
                                // auto& property_type_policy = m_meta_types.at(m_id_to_type.at(property->getType().id()));
                                // using TheirType = (typename decltype(property_type_policy))::template TemplateMetaElement;
                                // meta_element->m_sets.emplace(property.id(), std::make_unique<EGM::Set<TheirType, typename BaseManager::Implementation<MetaType>>>(meta_element));
                                create_set<0>::create(meta_element, m_id_to_type.at(property->getType().id()), property.id());
                            }
                        }

                        for (auto base : front->getGenerals().ptrs()) {
                            queue.push_back(base);
                        }
                    }
                    return meta_element;
                }
                private:
                    template <std::size_t J>
                    struct create_set {
                        template <class P>
                        using SetType = MetaElement<ManagerIndex, J, P>;
                        static void create(typename BaseManager::Pointer<MetaType> meta_element, std::size_t type, EGM::ID set_id) {
                            if constexpr (J >= ABSTRACTION_SIZE)
                                throw EGM::ManagerStateException("type too complex! bad state contact dev!");
                            
                            if (type < J)
                                create_set<J + 1>::create(meta_element, type, set_id);
                            else
                                meta_element.m_sets.emplace(set_id, EGM::Set<SetType, typename BaseManager::Implementation<MetaType>>(meta_element));
                        }
                    };
            };

            std::unordered_map<std::size_t, std::unique_ptr<AbstractMetaTypePolicy>> m_meta_types;
        public:
            MetaManager(UmlManager& uml_manager) : m_uml_manager(uml_manager) {}
            EGM::AbstractElementPtr create(std::size_t elementType) override {
                auto& policy = *m_meta_types.at(elementType);
                return policy.create();
            } 

            std::string name(std::size_t type) const override {
                return m_uml_types.at(type)->getName();
            }

            UmlManager& getUmlManager() const override {
                return m_uml_manager;
            }

            std::size_t getIndex() const override {
                return ManagerIndex;
            }
    };

    // singleton
    struct ActiveMetaManagers {
        private:
            static std::vector<std::unique_ptr<AbstractMetaManager>> managers;

            template <std::size_t Index, std::size_t Size = 0>
            static AbstractMetaManager& generate_helper(std::unordered_set<UmlManager::Pointer<Classifier>>& types) {
                if constexpr (Size >= ABSTRACTION_SIZE) 
                    throw EGM::ManagerStateException("manager too abstract!");

                if (types.size() < Size)
                    return generate_helper<Index, Size + 1>(types);

                std::size_t next_type = 0;
                auto meta_manager = std::make_unique<MetaManager<Index, Size>>(types.begin()->getManager());
                for (auto type : types) {
                    meta_manager.m_uml_types.emplace(next_type, type);
                    meta_manager.m_id_to_type.emplace(type.id(), next_type);
                    next_type++;
                }
                managers[Index] = std::move(meta_manager);
                return *managers[Index];
            }
        public:
            template <std::size_t I>
            static AbstractMetaManager& get() {
                return *ActiveMetaManagers::managers.at(I);
            }

            template <std::size_t Index = 0>
            static AbstractMetaManager& generate(UmlManager::Implementation<Package>& package) {
                if constexpr (Index >= NUMBER_META_MANAGERS)
                    throw EGM::ManagerStateException("no more room for managers");
               
                // check if vector has allocated storage to this index and if it is populated  
                if (managers.size() > Index && managers.at(Index)) 
                    return generate<Index + 1>(package);
                
                std::list<UmlManager::Pointer<PackageableElement>> queue = { &package };
                std::unordered_set<UmlManager::Pointer<Classifier>> types_for_manager;
                while (!queue.empty()) {
                    auto front = queue.front();
                    queue.pop_front();
                    if (front->is<Classifier>()) {
                        types_for_manager.emplace(front);
                    }
                    if (front->is<Package>()) {
                        UmlManager::Pointer<Package> package = front;
                        for (auto packagedEl : package->getPackagedElements().ptrs()) {
                            queue.push_back(packagedEl);
                        }
                    }
                }

                return generate_helper<Index>(types_for_manager);
            }
    };

    template <std::size_t ManagerIndex, std::size_t I>
    struct TemplateMetaElement {
        template <class P>
        using result = MetaElement<ManagerIndex, I, P>;
    };
}

namespace EGM {
    template <std::size_t ManagerIndex, std::size_t I>
    struct ElementInfo<UML::TemplateMetaElement<ManagerIndex, I>::template result> {
        static std::string name() {
            return UML::ActiveMetaManagers::get<ManagerIndex>().name(I);
        }
        template <class Policy>
        static SetList sets(UML::MetaElement<ManagerIndex, I, Policy>& el) {
            SetList ret;
            ret.reserve(el.m_sets.size());
            for (auto& pair : el.m_sets) {
                UML::UmlManager::Pointer<UML::Property> prop = dynamic_cast<UML::AbstractMetaManager>(el.getManager()).getUmlManager().get(pair.first);
                ret.push_back(make_set_pair(prop->getName(), pair.second.get()));
            }
            return ret;
        }
    };
}
