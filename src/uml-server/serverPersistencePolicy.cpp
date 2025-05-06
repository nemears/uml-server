#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <cstring>
#include <netinet/tcp.h>
#include <unistd.h>
#include "uml-server/serverPersistencePolicy.h"
#include "uml/uml-stable.h"
#include "yaml-cpp/yaml.h"
#include "uml-server/generativeManager.h"
#include "uml-server/umlServer.h"
#include <future>
#include <format>

using namespace std;
using namespace EGM;

namespace UML {

void ServerPersistencePolicy::create_storage(AbstractElement& el) {
    std::string post_request = std::format(
                "{{\"post\":{{\"type\":\"{}\",\"id\":\"{}\"}}}}",
                element_types_to_name.at(el.getElementType()),
                el.getID().string()
            );
    send_message(m_socketD, post_request);
    receive_and_check_reply();
}


void ServerPersistencePolicy::sendEmitter(int socket, YAML::Emitter& emitter) {
    string emitter_string(emitter.c_str());
    send_message(socket, emitter_string);
}

std::string ServerPersistencePolicy::loadElementData(ID id) {
    // request
    YAML::Emitter emitter;
    emitter << YAML::DoubleQuoted  << YAML::Flow << YAML::BeginMap << 
        YAML::Key << "GET" << YAML::Value << id.string() << 
    YAML::EndMap;
    sendEmitter(m_socketD, emitter);

    // receive
    return *receive_message(m_socketD);
}

void ServerPersistencePolicy::receive_and_check_reply() const {
    auto reply = *receive_message(m_socketD);
    YAML::Node reply_json = YAML::Load(reply);
    if (reply_json["error"]) {
        throw ManagerStateException(std::format("received error from server: {}", reply_json["error"].as<std::string>()));
    }
    if (!reply_json["status"]) {
        throw ManagerStateException("no success status in reply from server!");
    }
    if (reply_json["status"].as<std::string>() != "success") {
        throw ManagerStateException("status from server is not success!");
    }
}

void ServerPersistencePolicy::saveElementData(std::string data, ID id) {
    YAML::Emitter emitter;
    emitter << YAML::DoubleQuoted << YAML::Flow << YAML::BeginMap << 
        YAML::Key << "PUT" << YAML::Value << YAML::BeginMap << YAML::Key << 
        "id" << YAML::Value << id.string() << YAML::Key << "element" << YAML::Value << 
        YAML::Load(data) << YAML::EndMap << YAML::EndMap;
    sendEmitter(m_socketD, emitter);
    receive_and_check_reply();
}

std::string ServerPersistencePolicy::getProjectData(std::string path) {
    // TODO connect to new server and get head
    throw ManagerStateException("Do not try to open with a client, get individual elements");
}

std::string ServerPersistencePolicy::getProjectData() {
    // get head and construct ????
    throw ManagerStateException("Do not try to open with a client, get individual elements");
}

void ServerPersistencePolicy::saveProjectData(std::string data, std::string path) {
    // TODO this one is weird, maybe we connect to a different server ?
    YAML::Emitter emitter;
    emitter << YAML::DoubleQuoted << YAML::Flow << YAML::BeginMap << YAML::Key << "save" << YAML::Value << "." << YAML::EndMap;
    sendEmitter(m_socketD, emitter);
    receive_and_check_reply();
}

void ServerPersistencePolicy::saveProjectData(std::string data) {
    YAML::Emitter emitter;
    emitter << YAML::DoubleQuoted << YAML::Flow << YAML::BeginMap << YAML::Key << "save" << YAML::Value << "." << YAML::EndMap;
    sendEmitter(m_socketD, emitter);
    receive_and_check_reply();
}

void ServerPersistencePolicy::eraseEl(ID id) {
    YAML::Emitter emitter;
    emitter << YAML::DoubleQuoted  << YAML::Flow << YAML::BeginMap << 
        YAML::Key << "DELETE" << YAML::Value << id.string() << 
    YAML::EndMap;
    sendEmitter(m_socketD, emitter);
    receive_and_check_reply();
}

AbstractElementPtr ServerPersistencePolicy::reindex(ID oldID, ID newID) {
    // TODO
    return AbstractElementPtr();
}

ServerPersistencePolicy::ServerPersistencePolicy() {
    struct addrinfo hints;
    struct addrinfo* myAddress;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = m_address.empty() ? AI_PASSIVE : AI_CANONNAME; // fill in my IP for me
    int status = 0;
    if ((status = getaddrinfo(m_address.empty() ? 0 : m_address.c_str(), std::to_string(m_port).c_str(), &hints, &myAddress)) != 0) {
        throw ManagerStateException("client could not get address! " + std::string(strerror(errno)));
    }

    // get socket descriptor
    m_socketD = socket(myAddress->ai_family, myAddress->ai_socktype, myAddress->ai_protocol);
    if (m_socketD == -1) {
        freeaddrinfo(myAddress);
        throw ManagerStateException("client could not get socket!");
    }
    if (connect(m_socketD, myAddress->ai_addr, myAddress->ai_addrlen) == -1) {
        freeaddrinfo(myAddress);
        throw ManagerStateException("client could not connect to server! " + std::string(strerror(errno)));
    }
    freeaddrinfo(myAddress);
    
    // disable Nagle's on client side for no latency, client tends to write write read which is bad and it is hard to bundle writes together 
    // since is controlled by user of api
    int yes = 1;
    int result = setsockopt(m_socketD, IPPROTO_TCP, TCP_NODELAY, (char*) &yes, sizeof(int));
    if (result < 0) {
        throw ManagerStateException("could not disable Nagle's algorithm on client side!");
    }

    // receive server identification (lists of meta_managers)
    uint64_t server_message_size;
    int bytes_received = recv(m_socketD, &server_message_size, sizeof(uint64_t), 0);
    if (bytes_received != sizeof(uint64_t)) {
        throw ManagerStateException();
    }

    server_message_size = be64toh(server_message_size);
    char* server_message_buffer = (char*) malloc((server_message_size + 1) * sizeof(char));
    char* buffer_index = server_message_buffer;
    while ((bytes_received = recv(m_socketD, buffer_index, server_message_size, 0)) < (int) server_message_size) {
        if (bytes_received <= 0) {
            throw ManagerStateException("could not process server initial message!");
        }
        server_message_size -= bytes_received;
        buffer_index += bytes_received;
    }

    server_message_buffer[server_message_size] = '\0';

    auto server_message_node = YAML::Load(server_message_buffer);
    
    m_initialization_procedure = [server_message_node, this](){
        for (auto manager_info_node : server_message_node) {
            m_meta_managers.emplace(
                    ID::fromString(manager_info_node["id"].as<std::string>()), 
                    dynamic_cast<UmlManager::Implementation<Package>&>(*this->abstractGet(ID::fromString(manager_info_node["uml_root"].as<std::string>())))
            );
        }             
    };
    
    free(server_message_buffer);
    std::string id_string = clientID.string();
    send_message(m_socketD, id_string);

    // receive
    uint64_t receive_message_size;
    bytes_received = recv(m_socketD, &receive_message_size, sizeof(uint64_t), 0);
    if (bytes_received != sizeof(uint64_t)) {
        throw ManagerStateException("could not process entire size");
    }

    receive_message_size = be64toh(receive_message_size);
    if (receive_message_size != 28)
        throw ManagerStateException("wrong size for id sent to manager");

    char id_buffer[29];
    char* id_index = id_buffer;
    while ((bytes_received = recv(m_socketD, id_index, receive_message_size, 0)) < (int) receive_message_size) {
        receive_message_size -= bytes_received;
        id_index += bytes_received;
    }
    id_buffer[28] = '\0';

    ID id_from_server = ID::fromString(id_buffer);
    if (id_from_server != clientID) {
        throw ManagerStateException("wrong id from server!");
    }
}

void mount(string mountPath) {
    // TODO connect to another server
}

ServerPersistencePolicy::~ServerPersistencePolicy() {
    close(m_socketD);
}

}
