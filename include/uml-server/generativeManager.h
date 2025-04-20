#pragma once

#include "uml/uml-stable.h"
#include "metaManager.h"

namespace UML {

    class AbstractGenerativeManager;
    class ServerPersistencePolicy;
    
    class GenerativeSerializationPolicy : public EGM::JsonSerializationPolicy<UmlTypes> {
        protected:
            AbstractGenerativeManager* m_generative_manager = 0;
        public:
            std::vector<EGM::ManagedPtr<EGM::AbstractElement>> parseWhole(std::string data) override;
            std::string emitWhole(EGM::AbstractElement& el) override;
            void emit_set(YAML::Emitter& emitter, std::string set_name, EGM::AbstractSet& set) override;
            void parse_set(YAML::Node node, std::string set_name, EGM::AbstractSet& set) override;
    };
    
    class AbstractGenerativeManager : virtual public EGM::AbstractManager {
        friend class GenerativeSerializationPolicy;
        friend class ServerPersistencePolicy;
        template <class>
        friend class GenerativeManager;
        private:
            std::unordered_map<EGM::ID, MetaManager> m_meta_managers;
        public:
            virtual MetaManager& get_meta_manager(EGM::ID id) {
                return m_meta_managers.at(id);
            }
            virtual std::unordered_map<EGM::ID, MetaManager>& meta_managers() {
                return m_meta_managers;
            }
    };
 
    // Note: The BaseManager must have the UML::UmlTypes managed as well as use the 
    //       GenerativeSerializationPolicy defined above to be linked to it
    template <class BaseManager>
    class GenerativeManager : public BaseManager , virtual public AbstractGenerativeManager {
        friend class GenerativeSerializationPolicy;
        public:
            GenerativeManager() {
                // link to serialization policy, also causes compiler
                // error with improper config (no GenerativeSerializationPolicy in base hierarchy)
                BaseManager::m_generative_manager = this;
            }

            // generate a new MetaManager to manage at a higher level of abstraction
            // generation_root : root of uml elements to generate a manager from
            // return : the ID of the created meta_manager for use of the manager and its elements
            EGM::ID generate(UmlManager::Implementation<Package>& generation_root) {
                EGM::ID manager_id = EGM::ID::randomID();
                MetaManager& created_manager = m_meta_managers.emplace(manager_id, generation_root).first->second;
                // set storage root to correspond to manager id
                // this helps the generative manager quickly identify what meta manager an instance is part of
                created_manager.m_storage_root->setID(manager_id);            
                return manager_id;
            }

            void erase(EGM::AbstractElement& el) override {
                // make sure any stereotype data is freed first
                auto& uml_element = dynamic_cast<UmlManager::Implementation<Element>&>(el);
                for (auto& applied_stereotype : uml_element.getAppliedStereotypes()) {
                    // get meta_manager from applied_stereotype owning package (will be same id as manager)
                    auto& meta_manager = get_meta_manager(applied_stereotype.getOwningPackage().id());

                    // erase stereotype data so no hanging references to our base element
                    auto& meta_element = *meta_manager.get(applied_stereotype.getID());
                    meta_manager.erase(meta_element);        
                }

                // call super
                BaseManager::erase(el);
            }

            void release(EGM::AbstractElement& el) override {
                auto& uml_element = dynamic_cast<UmlManager::Implementation<Element>&>(el);
                for (auto& applied_stereotype: uml_element.getAppliedStereotypes()) {
                    // get meta_manager from applied stereotype owning package
                    auto& meta_manager = m_meta_managers.at(applied_stereotype.getOwningPackage().id());
                    
                    // release stereotype data so no hanging bad memory is still in use
                    auto& meta_element = *meta_manager.get(applied_stereotype.getID());
                    meta_manager.release(meta_element);
                }
            
                // call super
                BaseManager::release(el);
            }

            std::string dump_individual(EGM::AbstractElement& el) {
                return this->emitIndividual(el);
            }

            EGM::AbstractElementPtr parse_individual(std::string data) {
                return this->parseIndividual(data);
            }
    };

    using BasicGenerativeManager = GenerativeManager<EGM::Manager<UmlTypes, EGM::SerializedStoragePolicy<GenerativeSerializationPolicy, EGM::FilePersistencePolicy>>>;
}
