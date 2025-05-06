#include "uml/uml-stable.h"
#include "uml-server/umlClient.h"
#include "uml-server/umlServer.h"
#include "uml/uml-stable.h"
#include "yaml-cpp/yaml.h"
#include <sys/socket.h>
#include <sys/types.h>

using namespace std;
using namespace EGM;

namespace UML {

UmlClient::Pointer<Element> UmlClient::get(std::string qualifiedName) {
    // TODO check if one is in memory?
    
    // request
    YAML::Emitter emitter;
    emitter << YAML::DoubleQuoted  << YAML::Flow << YAML::BeginMap << 
        YAML::Key << "GET" << YAML::Value << qualifiedName << 
    YAML::EndMap;
    sendEmitter(m_socketD, emitter);

    // receive and parse
    UmlClient::Pointer<Element> ret = JsonSerializationPolicy<UmlTypes>::parseIndividual(*receive_message(m_socketD));
    
    // Todo run restoration


    return ret;
}

void UmlClient::setRoot(AbstractElementPtr root) {
    BaseManager::setRoot(root);
    if (!root) {
        throw new ManagerStateException("TODO set root to null on server");
    }
    YAML::Emitter emitter;
    emitter << YAML::DoubleQuoted << YAML::Flow << YAML::BeginMap << 
        YAML::Key << "PUT" << YAML::Value << YAML::BeginMap << YAML::Key << 
        "id" << YAML::Value << root->getID().string() << YAML::Key << "qualifiedName" << YAML::Value << "" << 
        YAML::Key << "element" << YAML::Value << YAML::Load(emitIndividual(dynamic_cast<UmlClient::BaseElement&>(*root))) << YAML::EndMap << YAML::EndMap;
    sendEmitter(m_socketD, emitter);
    receive_and_check_reply();
}

}
