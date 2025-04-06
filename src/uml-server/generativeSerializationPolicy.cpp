#include "uml-server/generativeManager.h"

using namespace std;
using namespace UML;
using namespace EGM;

static ID process_id_node(YAML::Node node) {
    string id_line = to_string(node.Mark().line);
    if (!node) {
        throw ManagerStateException("Invalid configuration for meta_manager, must have a uml_root id specified! line" + id_line);
    }

    if (!node.IsScalar()) {
        throw ManagerStateException("Invalid formatting uml_root must be a scalar id! line " + id_line);
    }

    if (!ID::isValid(node.as<std::string>())) {
        throw ManagerStateException("Invalid formatting, uml_root must be a valid uml id! line " + id_line);
    } 

    return ID::fromString(node.as<std::string>());
}

vector<ManagedPtr<AbstractElement>> GenerativeSerializationPolicy::parseWhole(std::string data) {
    disablePolicies();
    vector<YAML::Node> json_nodes = YAML::LoadAll(data);
    if (json_nodes.empty()) {
        enablePolicies();
        throw ManagerStateException("Could not parse whole data! Is it json?");
    }

    /** parse the base json object of 
     *  {
     *      uml: {
     *          regular uml emit ...
     *      }
     *      meta_managers: [
     *          {
     *              uml_root: "id_of_root_that_generated_this"
     *              id: "id_of_meta_manager"
     *              data: [ MetaManager::dump_all_data ]
     *          },
     *          ...
     *      ]
     *  }
     **/

    if (!json_nodes[0]["uml"]) {
        enablePolicies();
        throw ManagerStateException("Could not find uml data for UmlServer!");
    }

    auto uml_node = json_nodes[0]["uml"];
    if (!uml_node.IsMap()) {
        enablePolicies();
        throw ManagerStateException("Invalid type for uml data, must be a map!");
    }
    
    // parse uml data
    auto parsed_uml_root = parse_composite_node(uml_node);

    enablePolicies();

    // parse meta_managers
    if (json_nodes[0]["meta_managers"]) {
        auto meta_managers_nodes = json_nodes[0]["meta_managers"];

        if (!meta_managers_nodes.IsSequence()) {
            throw ManagerStateException("Invalid formatting for meta_managers! must be a sequence! line " + meta_managers_nodes.Mark().line);
        }

        for (auto meta_manager_node : meta_managers_nodes) {
            ID uml_generation_root_id = process_id_node(meta_manager_node["uml_root"]);
            ID meta_manager_id = process_id_node(meta_manager_node["id"]);
            MetaManager& meta_manager = m_generative_manager->m_meta_managers.emplace(meta_manager_id, ManagedPtr<BaseElement>(m_generative_manager->abstractGet(uml_generation_root_id))->as<Package>()).first->second;
            
            auto data_node = meta_manager_node["data"];
            string data_node_line = to_string(data_node.Mark().line);
            if (!data_node) {
                throw ManagerStateException("Invalid format for meta_manager, no data! line " + to_string(meta_manager_node.Mark().line));
            }

            if (!data_node.IsScalar()) {
                throw ManagerStateException("Invalid format for meta_manager data node, must be a sequence! line " + data_node_line);
            }

            for (auto meta_element_node : data_node) {
                meta_manager.parse_node(meta_element_node);
            }
        }
    }

    return vector<AbstractElementPtr> { parsed_uml_root };
}

string GenerativeSerializationPolicy::emitWhole(AbstractElement& el) {
    YAML::Emitter emitter;
    emitter << YAML::BeginMap;
    emitter << YAML::Key << "uml" << YAML::Value;
    m_serializationByType.at(el.getElementType())->emitComposite(emitter, AbstractElementPtr(&el));
    emitter << YAML::Key << "meta_managers" << YAML::Value;
    emitter << YAML::BeginSeq;
    for (auto& meta_manager_pair : m_generative_manager->m_meta_managers) {
        ID manager_id = meta_manager_pair.first;
        MetaManager& meta_manager = meta_manager_pair.second;
        emitter << YAML::Key << "uml_root" << YAML::Value << meta_manager.get_generation_root().id().string();
        emitter << YAML::Key << "id" << YAML::Value << manager_id.string();
        emitter << YAML::Key << "data";
        meta_manager.dump_all_data(emitter);
    }
    emitter << YAML::EndSeq;
    emitter << YAML::EndMap;

    return emitter.c_str();
}

void GenerativeSerializationPolicy::emit_set(YAML::Emitter& emitter, std::string set_name, AbstractSet& set) {
    if (set_name == "appliedStereotypes" && !set.empty()) {
        emitter << YAML::Key << set_name << YAML::Value << YAML::BeginSeq;
        for (auto it = set.beginPtr(); *it != *set.endPtr(); it->next()) {
            UmlManager::Pointer<InstanceSpecification> stereotype_instance = it->getCurr();
            MetaManager& meta_manager = m_generative_manager->m_meta_managers.at(stereotype_instance->getOwningPackage().id());
            emitter << YAML::BeginMap;
            emitter << YAML::Key << "manager" << YAML::Value << stereotype_instance->getOwningPackage().id().string();
            emitter << YAML::Key << "data" << YAML::Value << YAML::BeginMap;
            MetaManager::Implementation<MetaElement>& meta_element = meta_manager.get(stereotype_instance.id())->as<MetaElement>();
            meta_manager.emit_meta_element(emitter, meta_element);
            emitter << YAML::EndMap;
            emitter << YAML::EndMap;
        }
        emitter << YAML::EndSeq;
    } else {
        EGM::YamlSerializationPolicy<UmlTypes>::emit_set(emitter, set_name, set);
    }
}

void GenerativeSerializationPolicy::parse_set(YAML::Node node, std::string set_name, AbstractSet& set) {
    if (set_name == "appliedStereotypes") {
        auto stereotype_node = node["appliedStereotypes"];
        if (stereotype_node) {

            if (!stereotype_node.IsSequence()) {
                throw ManagerStateException("applied stereotypes must be set! line: " + std::to_string(stereotype_node.Mark().line));
            }

            for (auto applied_stereotype_node : stereotype_node) {

                auto manager_node = applied_stereotype_node["manager"];
                if (!manager_node) {
                    throw ManagerStateException("no manager given to stereotype_data! line: " + std::to_string(applied_stereotype_node.Mark().line)); 
                }

                auto manager_string = manager_node.as<std::string>();
                if (!ID::isValid(manager_string)) {
                    throw ManagerStateException("no valid id for manager given to stereotype_data! line: " + std::to_string(applied_stereotype_node.Mark().line)); 
                }

                auto meta_manager_id  = ID::fromString(manager_string);
                auto& meta_manager = m_generative_manager->m_meta_managers.at(meta_manager_id);

                // process data
                auto data_node = applied_stereotype_node["data"];
                if (!data_node) {
                    throw ManagerStateException("no data given for stereotype data! line: " + std::to_string(applied_stereotype_node.Mark().line));
                }

                if (!data_node.IsMap()) {
                    throw ManagerStateException("improper node for data given for stereotype data! line: " + std::to_string(data_node.Mark().line));
                }

                // parse data
                auto stereotype_data_element = meta_manager.parse_stereotype_node(dynamic_cast<UmlManager::Implementation<Element>&>(set.getOwner()), data_node);            
            }
        }
    } else {
        EGM::YamlSerializationPolicy<UmlTypes>::parse_set(node, set_name, set);
    }
}
