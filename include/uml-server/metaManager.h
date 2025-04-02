#pragma once

#include "uml/uml-stable.h"

namespace UML {
    
    template <class ManagerPolicy>
    struct MetaElement : public ManagerPolicy {
        using Info = EGM::TypeInfo<MetaElement>;
        MANAGED_ELEMENT_CONSTRUCTOR(MetaElement);
        std::unordered_map<EGM::ID, std::unique_ptr<EGM::AbstractSet>> sets;
        std::unordered_map<EGM::ID, std::unique_ptr<EGM::AbstractDataPolicy>> data;
        std::vector<std::size_t> m_bases;
        std::string name = "";
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
            UmlManager& m_uml_manager;
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
        public:

            EGM::AbstractElementPtr create(std::size_t elementType) override {
                return create_meta_element(elementType, 0); 
            }

            MetaElementPtr apply(UmlManager::Implementation<Element>& el, std::size_t stereotype) {
                return create_meta_element(stereotype, &el);
            }

            void reindex(EGM::ID oldID, EGM::ID newID) override {
                BaseManager::reindex(oldID, newID);
                m_meta_elements.erase(oldID);
                m_meta_elements.insert(newID);
            }

            MetaElementPtr create(EGM::ID id) {
                return create(m_id_to_type.at(id));
            } 

            UmlManager& getUmlManager() const {
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
