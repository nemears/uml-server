#pragma once

#include "uml/uml-stable.h"

namespace UML {
    
    template <class ManagerPolicy>
    struct MetaElement : public ManagerPolicy {
        using Info = EGM::TypeInfo<MetaElement>;
        MANAGED_ELEMENT_CONSTRUCTOR(MetaElement);
        std::unordered_map<EGM::ID, std::unique_ptr<EGM::AbstractSet>> m_sets;
        std::vector<std::size_t> m_bases;
        std::string m_name = "";
        private:
            void init() {}
    };


    class MetaManager;

    UmlManager::Pointer<UML::Element> get_element_from_uml_manager(EGM::AbstractElementPtr ptr, EGM::ID id);
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
            ret.reserve(el.m_sets.size());
            for (auto& pair : el.m_sets) {
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
            MetaManager* m_meta_manager = 0;
            EGM::AbstractElementPtr parseNode(YAML::Node node) override;  
            std::string emitIndividual(EGM::AbstractElement& el) override;
    };

    class MetaManager : public EGM::Manager<EGM::TemplateTypeList<MetaElement>, EGM::SerializedStoragePolicy<MetaElementSerializationPolicy, EGM::FilePersistencePolicy>> {
        friend struct MetaElementSerializationPolicy;
        private:
            using BaseManager = EGM::Manager<EGM::TemplateTypeList<MetaElement>, EGM::SerializedStoragePolicy<MetaElementSerializationPolicy, EGM::FilePersistencePolicy>>;
            using MetaElementPtr = BaseManager::Pointer<MetaElement>;
            using MetaElementImpl = BaseManager::Implementation<MetaElement>;
            UmlManager& m_uml_manager;
            std::unordered_map<std::size_t, UmlManager::Pointer<Classifier>> m_uml_types;
            std::unordered_map<EGM::ID, std::size_t> m_id_to_type;
            std::unordered_map<std::string, std::size_t> m_name_to_type;

        public:
            MetaManager(UmlManager::Implementation<Package>& pckg) : m_uml_manager(dynamic_cast<UmlManager&>(pckg.getManager())) {
                
                // link to serialization policy
                this->m_meta_manager = this;
               
                // set up types 
                std::list<UmlManager::Pointer<PackageableElement>> queue = { &pckg };
                std::size_t next_type = 0;
                while (!queue.empty()) {
                    auto front = queue.front();
                    queue.pop_front();
                    if (front->is<Classifier>()) {
                        if (m_uml_types.count(front)) {
                            continue;
                        }
                        m_uml_types.emplace(next_type, front);
                        m_id_to_type.emplace(front.id(), next_type);
                        m_name_to_type.emplace(front->as<NamedElement>().getName(), next_type);
                        next_type++;
                        for (auto prop : front->as<Classifier>().getAttributes().ptrs()) {
                            if (prop->getType()) {
                                queue.push_back(prop->getType());
                            }
                        }
                        for (auto base : front->as<Classifier>().getGenerals().ptrs()) {
                            queue.push_back(base);
                        }
                    }
                    if (front->is<Package>()) {
                        UmlManager::Pointer<Package> package = front;
                        for (auto packagedEl : package->getPackagedElements().ptrs()) {
                            queue.push_back(packagedEl);
                        }
                    }
                }
            }
            EGM::AbstractElementPtr create(std::size_t elementType) override {
                
                auto meta_element = BaseManager::template create<MetaElement>();
                
                // get representing classifier
                auto meta_type = m_uml_types.at(elementType);

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
                            meta_element->m_sets.emplace(property.id(), std::make_unique<EGM::Set<MetaElement, MetaElementImpl>>(&*meta_element));
                        }
                    }

                    for (auto base : front->getGenerals().ptrs()) {
                        queue.push_back(base);
                    }
                }
                return meta_element; 

            } 

            UmlManager& getUmlManager() const {
                return m_uml_manager;
            }
    };
}
