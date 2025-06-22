#pragma once

#include "uml/uml-stable.h"
#include "metaElement.h"

namespace UML {
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
                        // if it's not a meta_ptr don't run the policy we can keep track of it
                        if (meta_ptr) {
                            this->run_add_policy_for_set(set, *meta_ptr);
                        }
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
        EGM::ID default_value;
        using MetaElementImpl = typename Policy::manager::Implementation<MetaElement>;
        void elementAdded(MetaElementImpl& el, MetaElementImpl& me) {
            // update slot
            if (uml_slot) {
                auto inst_val = uml_manager->create<InstanceValue>();
                inst_val->setInstance(uml_manager->abstractGet(el.getID()));
                uml_slot->getValues().add(inst_val);
            }
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
