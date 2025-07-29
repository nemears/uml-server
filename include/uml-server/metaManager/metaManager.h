#pragma once

#include "metaElementSet.h"
#include "proxyElement.h"

namespace UML {

    struct MetaElementSerializationPolicy : public EGM::JsonSerializationPolicy<EGM::TemplateTypeList<MetaElement, ProxyElement>> {
        protected:
            MetaManager* m_meta_manager;
            EGM::AbstractElementPtr parseNode(YAML::Node node) override;
            EGM::AbstractElementPtr parse_meta_element_node(YAML::Node node, std::function<EGM::AbstractElementPtr(std::size_t, EGM::ID)> f);
            void emitIndividual(YAML::Emitter& emitter, EGM::AbstractElement& el) override;
            std::string emitIndividual(EGM::AbstractElement& el) override { 
                return EGM::JsonSerializationPolicy<EGM::TemplateTypeList<MetaElement, ProxyElement>>::emitIndividual(el);
            }
            void emit_set(YAML::Emitter& emitter, std::string set_name, EGM::AbstractSet& set) override;
            void parse_set(YAML::Node node, std::string set_name, EGM::AbstractSet& set) override;
    };

    struct MetaElementStoragePolicy {
        EGM::AbstractElementPtr loadElement(EGM::ID id);
        void saveElement(EGM::AbstractElement& el);
        void eraseEl(EGM::ID id);
        protected:
            MetaManager* m_meta_manager;
    };

    class MetaManager : public EGM::Manager<EGM::TemplateTypeList<MetaElement, ProxyElement>, MetaElementStoragePolicy>, public MetaElementSerializationPolicy {
        friend struct MetaElementSerializationPolicy;
        template <class>
        friend class GenerativeManager; // TODO maybe move all of this inside generative manager as an inner class
        friend class UmlServer;
        friend class MetaElementStoragePolicy;
        friend class GenerativeSerializationPolicy;
        template <class, template <template<class> class, class, class> class>
        friend class ProxyElementSet;
        private:
            using BaseManager = EGM::Manager<EGM::TemplateTypeList<MetaElement, ProxyElement>, MetaElementStoragePolicy>;
        public:
            using MetaElementPtr = BaseManager::Pointer<MetaElement>;
            using MetaElementImpl = BaseManager::Implementation<MetaElement>;
            using ProxyElementPtr = BaseManager::Pointer<ProxyElement>;
        protected:
            EGM::ManagerTypes<UmlTypes>& m_uml_manager;
            UmlManager::Pointer<Package> m_storage_root;
            UmlManager::Pointer<Element> m_generation_root;
            std::unordered_map<std::size_t, UmlManager::Pointer<Classifier>> m_uml_types;
            std::unordered_map<EGM::ID, std::size_t> m_id_to_type;
            std::unordered_map<std::string, std::size_t> m_name_to_type;
            std::unordered_set<EGM::ID> m_meta_elements;
            std::unordered_map<EGM::ID, UmlManager::Pointer<Element>> m_stereotyped_elements;
            std::unordered_map<EGM::ID, ProxyElementPtr> m_proxy_elements;

        public:
            MetaManager(UmlManager::Implementation<Element>& abstraction_root);
            std::optional<std::size_t> get_type_by_name(std::string name) { 
                auto match = m_name_to_type.find(name);
                if (match != m_name_to_type.end()) {
                    return match->second;
                }
                return std::nullopt;
            }
            std::optional<std::size_t> get_type_with_id(EGM::ID id) const { 
                auto match = m_id_to_type.find(id); 
                if (match != m_id_to_type.end()) {
                    return match->second;
                }
                return std::nullopt;
            }
            UmlManager::Pointer<Element> get_stereotyped_element(EGM::ID id) const {
                auto match = m_stereotyped_elements.find(id);
                if (match != m_stereotyped_elements.end()) {
                    return match->second;
                }
                return UmlManager::Pointer<Element>();
            }
            UmlManager::Pointer<Element> get_generation_root() const { return m_generation_root; }
        private:
            // create_meta_element
            // element_type - element_type of meta_element
            // applying_element - pointer (can be null) to element applying meta_element to as stereotype
            // return - the meta_element created as a ptr
            BaseManager::Pointer<MetaElement> create_meta_element_object(std::size_t element_type, UmlManager::Pointer<Element> applying_element);
            EGM::AbstractElementPtr create_meta_element(std::size_t element_type, UmlManager::Pointer<Element> applying_element);
            void create_uml_representation(BaseManager::Pointer<MetaElement> meta_element);
            EGM::ID next_id; // id used for create
        public:

            EGM::AbstractElementPtr create(std::size_t elementType) override {
                return create_meta_element(elementType, 0); 
            }

            MetaElementPtr apply(UmlManager::Implementation<Element>& el, std::size_t stereotype) {
                return create_meta_element(stereotype, &el);
            }

            ProxyElementPtr create_proxy_element(UmlManager::Pointer<Element> el) {
                ProxyElementPtr proxy_element = BaseManager::create<ProxyElement>();
                proxy_element->m_uml_element = el;
                m_proxy_elements.emplace(el.id(), proxy_element);
                return proxy_element;
            }

            EGM::AbstractElementPtr reindex(EGM::ID oldID, EGM::ID newID) override {
                MetaElementPtr meta_element = BaseManager::reindex(oldID, newID);
                m_meta_elements.erase(oldID);
                m_meta_elements.insert(newID);

                // delete old instance, and set it up again with new id
                UmlManager::Pointer<InstanceSpecification> overwritten_element = m_storage_root->getPackagedElements().get(newID);
                if (overwritten_element){
                    m_uml_manager.erase(*overwritten_element);
                }
                if (meta_element->uml_representation) {
                    m_uml_manager.erase(*meta_element->uml_representation);
                    
                    create_uml_representation(meta_element);
                }
                return meta_element;
            }

            MetaElementPtr create(EGM::ID id) {
                return create(m_id_to_type.at(id));
            }

            MetaElementPtr create(EGM::ID type_id, EGM::ID element_id) {
                next_id = element_id;
                return create(type_id);
            }

            MetaElementPtr create(std::size_t type, EGM::ID element_id) {
                next_id = element_id;
                return create(type);
            }

            MetaElementPtr apply(UmlManager::Implementation<Element>& el, EGM::ID stereotype_id) {
                return apply(el, m_id_to_type.at(stereotype_id));
            }

            MetaElementPtr apply(UmlManager::Implementation<Element>& el, EGM::ID stereotype_id, EGM::ID element_id) {
                next_id = element_id;
                return apply(el, stereotype_id);
            }

            MetaElementPtr apply(UmlManager::Implementation<Element>& el, std::size_t stereotype_type, EGM::ID element_id) {
                next_id = element_id;
                return apply(el, stereotype_type);
            }

            EGM::ManagerTypes<UmlTypes>& getUmlManager() const {
                return m_uml_manager;
            }

            std::string emit_meta_element(BaseManager::Implementation<MetaElement>& el) {
                return emitIndividual(el);
            }

            void emit_meta_element(YAML::Emitter& emitter, BaseManager::Implementation<MetaElement>& el) {
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
