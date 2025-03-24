#include "uml-server/metaManager.h"

using namespace UML;
using namespace EGM;

namespace UML {
UmlManager::Pointer<UML::Element> get_element_from_uml_manager(AbstractElementPtr ptr, ID id) {
    MetaManager::Pointer<MetaElement> meta_ptr = ptr;
    return dynamic_cast<UML::MetaManager&>(meta_ptr->getManager()).getUmlManager().get(id); 
}
}

AbstractElementPtr MetaElementSerializationPolicy::parseNode(YAML::Node node) {
    auto it = node.begin();
    while (it != node.end()) {
        const auto keyNode = it->first;
        const auto valNode = it->second;
        if (valNode.IsMap()) {
            // look up key
            try {
                auto el = m_meta_manager->create(m_meta_manager->m_name_to_type.at(keyNode.as<std::string>()));
                if (valNode["id"] && valNode["id"].IsScalar()) {
                    el->setID(EGM::ID::fromString(valNode["id"].template as<std::string>()));
                }
                
                auto serialization_policy = m_serializationByType.at(0); // get meta manager
                serialization_policy->parseBody(valNode, el);
                serialization_policy->parseScope(node, el);
                return el;
            } catch (std::exception& e) {
                throw EGM::ManagerStateException("Could not find proper type to parse! line number " + std::to_string(keyNode.Mark().line));
            }
        }
        it++;
    }
    throw EGM::ManagerStateException("could not find property type to parse! line number " + std::to_string(node.Mark().line));
}

std::string MetaElementSerializationPolicy::emitIndividual(EGM::AbstractElement& el) {
    auto serialization_policy = m_serializationByType.at(0);

    MetaManager::Pointer<MetaElement> meta_el = AbstractElementPtr(&el);

    YAML::Emitter emitter;
    primeEmitter(emitter);
    emitter << YAML::BeginMap;

    // emit scope
    serialization_policy->emitScope(emitter, &el);

    // emit body
    std::string elementName = meta_el->name;
    emitter << YAML::Key << elementName << YAML::Value << YAML::BeginMap;
    emitter << YAML::Key << "id" << YAML::Value << meta_el.id().string();
    serialization_policy->emitBody(emitter, &el);
    emitter << YAML::EndMap;
    emitter << YAML::EndMap;

    return emitter.c_str();
}
