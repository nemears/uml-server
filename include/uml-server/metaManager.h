#pragma once

#include "uml/uml-stable.h"

namespace UML {

    template <class>
    struct MetaElementSetPolicy;

    
    template <class, template <template<class> class, class, class> class>
    class MetaElementSet;

    template <class ManagerPolicy>
    struct MetaElement : public ManagerPolicy {
        using Info = EGM::TypeInfo<MetaElement>;
        MANAGED_ELEMENT_CONSTRUCTOR(MetaElement);
        std::unordered_map<EGM::ID, std::unique_ptr<EGM::AbstractSet>> sets;
        std::unordered_map<EGM::ID, std::unique_ptr<EGM::AbstractDataPolicy>> data;
        std::vector<std::size_t> m_bases;
        std::string name = "";
        UmlManager::Pointer<InstanceSpecification> uml_representation;
        UmlManager::Pointer<Classifier> meta_type;
        UmlManager::Pointer<Element> applying_element;
        using Set = MetaElementSet<ManagerPolicy, EGM::Set>;
        using OrderedSet = MetaElementSet<ManagerPolicy, EGM::OrderedSet>;
        using Singleton = MetaElementSet<ManagerPolicy, EGM::Singleton>;
        Set& getSet(EGM::ID id) const {
            return dynamic_cast<Set&>(*sets.at(id));
        }
        OrderedSet& getOrderedSet(EGM::ID id) const {
            return dynamic_cast<OrderedSet&>(*sets.at(id));
        }
        Singleton& getSingleton(EGM::ID id) const {
            return dynamic_cast<Singleton&>(*sets.at(id));
        }
        private:
            void init() {}
    };

    class MetaManager;

    template <class Policy, template<template<class> class, class, class> class SetImpl>
    class MetaElementSet : public SetImpl<MetaElement, MetaElement<Policy>, MetaElementSetPolicy<Policy>> {
        protected:
            void addToOpposite(EGM::AbstractElementPtr ptr) override {
                EGM::ManagedPtr<MetaElement<Policy>> meta_ptr = ptr;
                std::list<std::shared_ptr<EGM::SetStructure>> queue;
                std::unordered_set<std::shared_ptr<EGM::SetStructure>> visited;
                queue.push_back(this->m_structure->m_rootRedefinedSet);

                auto add_to_opposite = [this, meta_ptr](EGM::AbstractSet& set) {
                    if (dynamic_cast<MetaElementSetPolicy<Policy>*>(&set)) {
                        this->run_add_opposite_for_set(set, *meta_ptr);
                    } else {
                        // it is a uml set
                        this->run_add_opposite_for_set(set, *meta_ptr->applying_element);
                    } 
                };

                while (!queue.empty()) {
                    auto front = queue.front();
                    queue.pop_front();
                    if (visited.count(front)) {
                        continue;
                    }
                    visited.insert(front);
                    bool oppositeRan = false;
                    if (ptr.loaded()) {
                        if (!oppositeRan && this->check_opposite_enabled_for_set(front->m_set)) {
                            add_to_opposite(front->m_set);
                            oppositeRan = true;
                        }
                    }
                    for (auto redefinedSet : front->m_redefinedSets) {
                        if (ptr.loaded()) {
                            if (!oppositeRan && this->check_opposite_enabled_for_set(redefinedSet->m_set)) {
                                add_to_opposite(redefinedSet->m_set);
                                oppositeRan = true;
                            }
                        }
                    }
                    if (!oppositeRan) {
                        for (auto superSet : front->m_superSets) {
                           queue.push_back(superSet);
                        }
                    }
                }
            }
            void nonOppositeAdd(EGM::AbstractElementPtr ptr) override {
                this->nonPolicyAdd(ptr);

                EGM::ManagedPtr<MetaElement<Policy>> meta_ptr = ptr;
                std::list<std::shared_ptr<EGM::SetStructure>> queue;
                std::unordered_set<std::shared_ptr<EGM::SetStructure>> visited;
                queue.push_back(this->m_structure->m_rootRedefinedSet);
                
                auto run_set_policies = [this, meta_ptr] (EGM::AbstractSet& set) {
                    if (dynamic_cast<MetaElementSetPolicy<Policy>*>(&set)) {
                        this->run_add_policy_for_set(set, *meta_ptr);
                    } else {
                        // it is a uml set
                        this->run_add_policy_for_set(set, *meta_ptr->applying_element);
                    }
                };
 
                while (!queue.empty()) {
                    auto front = queue.front();
                    queue.pop_front();
                    if (visited.count(front)) {
                        continue;
                    }
                    visited.insert(front);
                    run_set_policies(front->m_set);
                    for (auto redefinedSet : front->m_redefinedSets) {
                        run_set_policies(redefinedSet->m_set);
                    }
                    for (auto superSet : front->m_superSets) {
                        queue.push_back(superSet);
                    }
                } 
            }
            using BaseSet = SetImpl<MetaElement, MetaElement<Policy>, MetaElementSetPolicy<Policy>>;
        public:
            using BaseSet::BaseSet;
    };

    UmlManager::Pointer<UML::Element> get_element_from_uml_manager(EGM::AbstractElementPtr ptr, EGM::ID id);

    // policy to put in all of the meta element sets, keeps track of
    // the uml implementation as the set is added and removed from
    template <class Policy>
    struct MetaElementSetPolicy {
        EGM::ManagerTypes<UmlTypes>* uml_manager;
        UmlManager::Pointer<Slot> uml_slot;
        using MetaElementImpl = typename Policy::manager::Implementation<MetaElement>;
        void elementAdded(MetaElementImpl& el, MetaElementImpl& me) {
            // update slot
            auto inst_val = uml_manager->create<InstanceValue>();
            inst_val->setInstance(uml_manager->abstractGet(el.getID()));
            uml_slot->getValues().add(inst_val);
        }
        void elementRemoved(MetaElementImpl& el, MetaElementImpl& me) {
            // update slot
            UmlManager::Pointer<InstanceValue> val_match;
            for (auto& val : uml_slot->getValues()) {
                if (!val.is<InstanceValue>()) {
                    continue;
                }
    
                auto& inst_val = val.as<InstanceValue>();
                auto inst = inst_val.getInstance();
                if (inst.id() == el.getID()) {
                    val_match = &inst_val;
                    break;
                }
            }
    
            if (!val_match) {
                throw EGM::ManagerStateException("could not find value that matches meta element set value that is being removed! contact dev!");
            }
    
            // get rid of the value
            uml_manager->erase(*val_match);
        }
    };
}

namespace EGM {
    template <>
    struct ElementInfo<UML::MetaElement> {
        static std::string name() {
            return "MetaElement";
        }
        template <class Policy>
        static SetList sets(UML::MetaElement<Policy>& el) {
            SetList ret;
            ret.reserve(el.sets.size());
            for (auto& pair : el.sets) {
                UML::UmlManager::Pointer<UML::Property> prop = get_element_from_uml_manager(&el, pair.first);
                ret.push_back(make_set_pair(prop->getName().c_str(), *pair.second));
            }
            return ret;
        }
    };
}

namespace UML {

    struct MetaElementSerializationPolicy : public EGM::YamlSerializationPolicy<EGM::TemplateTypeList<MetaElement>> {
        protected:
            MetaManager* m_meta_manager;
            EGM::AbstractElementPtr parseNode(YAML::Node node) override;
            EGM::AbstractElementPtr parse_meta_element_node(YAML::Node node, std::function<EGM::AbstractElementPtr(std::size_t)> f);
            void emitIndividual(YAML::Emitter& emitter, EGM::AbstractElement& el) override;
            std::string emitIndividual(EGM::AbstractElement& el) override { 
                return EGM::YamlSerializationPolicy<EGM::TemplateTypeList<MetaElement>>::emitIndividual(el);
            }
    };

    class MetaManager : public EGM::Manager<EGM::TemplateTypeList<MetaElement>, EGM::SerializedStoragePolicy<MetaElementSerializationPolicy, EGM::FilePersistencePolicy>> {
        friend struct MetaElementSerializationPolicy;
        template <class>
        friend class GenerativeManager;
        private:
            using BaseManager = EGM::Manager<EGM::TemplateTypeList<MetaElement>, EGM::SerializedStoragePolicy<MetaElementSerializationPolicy, EGM::FilePersistencePolicy>>;
            using MetaElementPtr = BaseManager::Pointer<MetaElement>;
            using MetaElementImpl = BaseManager::Implementation<MetaElement>;
            EGM::ManagerTypes<UmlTypes>& m_uml_manager;
            UmlManager::Pointer<Package> m_storage_root;
            UmlManager::Pointer<Package> m_generation_root;
            std::unordered_map<std::size_t, UmlManager::Pointer<Classifier>> m_uml_types;
            std::unordered_map<EGM::ID, std::size_t> m_id_to_type;
            std::unordered_map<std::string, std::size_t> m_name_to_type;
            std::unordered_set<EGM::ID> m_meta_elements;

        public:
            MetaManager(UmlManager::Implementation<Package>& abstraction_root);
            std::size_t get_type_by_name(std::string name) { return m_name_to_type.at(name); }
        private:
            // create_meta_element
            // element_type - element_type of meta_element
            // applying_element - pointer (can be null) to element applying meta_element to as stereotype
            // return - the meta_element created as a ptr
            EGM::AbstractElementPtr create_meta_element(std::size_t element_type, UmlManager::Pointer<Element> applying_element);
            void create_uml_representation(MetaManager::Pointer<MetaElement> meta_element);
        public:

            EGM::AbstractElementPtr create(std::size_t elementType) override {
                return create_meta_element(elementType, 0); 
            }

            MetaElementPtr apply(UmlManager::Implementation<Element>& el, std::size_t stereotype) {
                return create_meta_element(stereotype, &el);
            }

            EGM::AbstractElementPtr reindex(EGM::ID oldID, EGM::ID newID) override {
                MetaElementPtr meta_element = BaseManager::reindex(oldID, newID);
                m_meta_elements.erase(oldID);
                m_meta_elements.insert(newID);

                // delete old instance, and set it up again with new id
                m_uml_manager.erase(*meta_element->uml_representation);
                create_uml_representation(meta_element);
                
                return meta_element;
            }

            MetaElementPtr create(EGM::ID id) {
                return create(m_id_to_type.at(id));
            }

            MetaElementPtr apply(UmlManager::Implementation<Element>& el, EGM::ID stereotype_id) {
                return apply(el, m_id_to_type.at(stereotype_id));
            }

            EGM::ManagerTypes<UmlTypes>& getUmlManager() const {
                return m_uml_manager;
            }

            UmlManager::Pointer<Package> get_generation_root() const {
                return m_generation_root;
            }

            std::string emit_meta_element(MetaManager::Implementation<MetaElement>& el) {
                return emitIndividual(el);
            }

            void emit_meta_element(YAML::Emitter& emitter, MetaManager::Implementation<MetaElement>& el) {
                emitIndividual(emitter, el);
            }

            MetaElementPtr parse_node(YAML::Node node) {
                return parseNode(node);
            }

            MetaElementPtr parse_stereotype_node(UmlManager::Implementation<Element>& el, YAML::Node node);

            void dump_all_data(YAML::Emitter& emitter) {
                emitter << YAML::BeginSeq;
                for (auto id : m_meta_elements) {
                    MetaElementPtr meta_element = get(id);
                    emitter << YAML::BeginMap;
                    FindValidScopeVisitor scope_visitor { meta_element };
                    scope_visitor.visit<MetaElement>();
                    if (scope_visitor.validMatch) {
                        emitter << YAML::Key << scope_visitor.validMatch->first;
                        emitter << YAML::Value << (*scope_visitor.validMatch->second->beginPtr()).getCurr().id().string();
                    }
                    emitter << YAML::Key << meta_element->name;
                    emitter << YAML::Value << YAML::BeginMap;
                    emitter << YAML::Key << "id" << YAML::Value << id.string();
                    EmitVisitor visitor { meta_element, emitter };
                    visitor.visit<MetaElement>();
                    emitter << YAML::EndMap;
                }
                emitter << YAML::EndSeq;
            }

    };
}
