#include "uml/uml-stable.h"
#include "uml-server/umlServer.h"
#include <expected>
#ifndef WIN32
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <poll.h>
#include <netinet/tcp.h>
#else
#include <ws2tcpip.h>
#include <stdio.h>
#endif
#include <thread>
#include <yaml-cpp/yaml.h>
#include "uml/uml-stable.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <errno.h>
#include <string.h>
#include <format>

#ifdef WIN32
typedef size_t ssize_t;
#endif

using namespace UML;
using namespace EGM;

using ElementPtr = UmlServer::Pointer<Element>;
using NamedElementPtr = UmlServer::Pointer<NamedElement>;

namespace UML {
void send_message(int socket, std::string& data) {
    uint64_t dataSize = data.size();
    uint64_t dataSizeNetwork = htobe64(dataSize);
    send(socket, &dataSizeNetwork, sizeof(uint64_t), 0);
    uint64_t total_bytes_sent = 0;
    const char* data_buffer = data.c_str();
    while (total_bytes_sent < dataSize) {
        int bytesSent = send(socket, data_buffer + total_bytes_sent, dataSize, 0);
        if (bytesSent <= 0) {
            throw ManagerStateException();
        }
        total_bytes_sent += bytesSent;
        dataSize -= bytesSent;
    }
}

std::optional<std::string> receive_message(int socket) {
    uint64_t message_size_buffer;
    uint64_t bytes_read = recv(socket, &message_size_buffer, sizeof(uint64_t), 0);
    if (bytes_read <= 0) {
        return std::nullopt;
    }

    // TODO check bytes_read == sizeof(uint64_t) and try to read rest
    
    // get rest of message 
    message_size_buffer = be64toh(message_size_buffer);

    if (message_size_buffer > SIZE_MAX) {
        return std::nullopt;
    }

    char* message_buffer = (char*) calloc(message_size_buffer + 1, sizeof(char));
    message_buffer[message_size_buffer] = '\0';
    bytes_read = 0;
    while ((bytes_read += recv(socket, message_buffer + bytes_read, message_size_buffer - bytes_read, 0)) < message_size_buffer) {}
    std::string message_string = std::move(message_buffer);
    free(message_buffer);
    return message_string;
}
}

using ParameterPair = std::pair<std::string, std::string>;
using RequestInfo = std::pair<std::variant<ID, std::string>, std::vector<ParameterPair>>;

/**
 * This function takes a request string of the form 28_char_ID?parameter1=value1&parameter2=value2 etc...
 * It returns either a RequestInfo object, or an error string wrapped in an expected
 **/
static std::expected<RequestInfo, std::string> parse_id_and_parms(std::string request_string) {
    auto has_question_mark = request_string.find("?");
    if (has_question_mark != std::string::npos) {
        ID parsed_id = ID::fromString(request_string.substr(0, has_question_mark));
        std::vector<std::pair<std::string, std::string>> parsed_parameters;
        auto get_request_parameters_string = request_string.substr(has_question_mark + 1);
        std::stringstream parameters_stream(get_request_parameters_string);
        std::string current_parameter;
        while (std::getline(parameters_stream, current_parameter, '&')) {
            auto equal_sign_pos = current_parameter.find("=");
            if (equal_sign_pos == std::string::npos) {
                std::string msg = "{\"error\":\"get request invalid parameter, no '=' found in parameter\"}";
                return std::unexpected(msg);
            }
            
            auto parameter_name = current_parameter.substr(0, equal_sign_pos);
            auto parameter_value = current_parameter.substr(equal_sign_pos + 1);
            parsed_parameters.push_back(std::pair<std::string, std::string>(parameter_name, parameter_value));
        }
        return std::pair<std::variant<ID, std::string>, std::vector<ParameterPair>>(parsed_id, parsed_parameters);
    } else {
        if (!ID::isValid(request_string)) {
            return std::pair<std::variant<ID, std::string>, std::vector<ParameterPair>>(request_string, std::vector<ParameterPair>{}); 
        }
        return std::pair<std::variant<ID, std::string>, std::vector<ParameterPair>>(ID::fromString(request_string), std::vector<ParameterPair>{});
    }
}

#define NOT_SCALAR 1
#define NOT_ID 2

int check_id(YAML::Node id_node) {
    if (!id_node.IsScalar()) {
        return NOT_SCALAR;
    }

    if (!ID::isValid(id_node.as<std::string>())) {
        return NOT_ID;
    }

    return 0;
}

void UmlServer::handleMessage(ID id, std::string buff) {
    std::lock_guard<std::mutex> handleLock(m_messageHandlerMtx);
    ClientInfo& info = m_clients[id];
    log("server got message from client(" + id.string() + "):\n" + std::string(buff));

    if (buff == "KILL") {
        std::string kill_response = "{\"shutdown\":\"success\"}";
        log(kill_response);
        send_message(info.socket, kill_response);
        info.handlerCv.notify_one();
        shutdownServer();
        return;
    }
    
    YAML::Node node;
    try {
        node = YAML::Load(buff);
    } catch (std::exception& e) {
        log(e.what());
        std::string msg = std::string("{\"error\": ") + std::string(e.what()) + std::string("}");
        log(msg);
        send_message(info.socket, msg);
        return;
    }   
    
    if (!node.IsMap()) {
        log("ERROR receiving message from client, invalid format!\nMessage:\n" + buff);
        std::string msg = std::string("{\"error\":\"") + std::string("ERROR receiving message from client, invalid format!\nMessage:\n" + buff) + std::string("\"}");
        log(msg);
        send_message(info.socket, msg);
        return;
    }
    if (node["DELETE"] || node["delete"]) {

        auto delete_node = node["DELETE"] ? node["DELETE"] : node["delete"];

        if (!delete_node.IsScalar()) {
            log("bad formatting for delete request!");
            std::string error_message = "{\"error\":\"Delete requests need to be in the format {\"delete\":id}\"}";
            log(error_message);
            send_message(info.socket, error_message);
            return;
        }

        std::string delete_request_string = delete_node.as<std::string>();

        auto parse_result = parse_id_and_parms(delete_request_string);

        if (!parse_result) {
            log(parse_result.error());
            return;
        }
        
        ID meta_manager_id;
        if (parse_result->second.size() > 0) {
            for (auto& parameter_pair : parse_result->second) {
                if (parameter_pair.first == "manager") {
                    meta_manager_id = ID::fromString(parameter_pair.second);
                }
            }
        }
        
        ID elID;
        // std::optional<std::string> url;
        if (parse_result->first.index() == 0) {
            elID = std::get<ID>(parse_result->first);
        } else {
            log("bad delete request, must specify an id!");
            std::string error_message = "{\"error\":\"Could not parse id in delete request\"}";
            log(error_message);
            send_message(info.socket, error_message);
            return;
        }


        if (meta_manager_id != ID::nullID()) {
            MetaManager& meta_manager = get_meta_manager(meta_manager_id);
            auto el_to_erase = meta_manager.get(elID);
            meta_manager.erase(*el_to_erase);
            log("erased element " + elID.string() + " from meta manager " + meta_manager_id.string());
        } else {
            try {
                ElementPtr elToErase = get(elID);
                erase(*elToErase);
                log("erased element " + elID.string());
                std::lock_guard<std::mutex> garbageLck(m_garbageMtx);
                m_releaseQueue.remove(elID);
                m_numEls--;
            } catch (std::exception& e) {
                log("exception encountered when trying to delete element: " + std::string(e.what()));
                std::string error_message = std::format("{{\"error\":\"{}\"}}", e.what());
                log(error_message);
                send_message(info.socket, error_message);
                return;
            }
        }

        // send reply
        std::string reply_message = "{\"status\":\"success\"}";
        log(reply_message);
        send_message(info.socket, reply_message);
    } else if (node["DUMP"] || node["dump"]) {
        std::string dump = this->dumpYaml();
        send_message(info.socket, dump);
        log("dumped server data to client, data: " + dump);
    } else if (node["generate"]) {
        if (!node["generate"].IsScalar()) {
            std::string msg = "{\"error\":\"invalid generate request, must be a scalar of an id to generate!\"}"; 
            send_message(info.socket, msg);
            log(msg);
            return;
        } else {
            // generate the meta manager, send id of manager back
            ID generation_root_id = ID::fromString(node["generate"].as<std::string>());
            ID manager_id = generate(get(generation_root_id)->as<Element>());

            std::ostringstream oss;
            oss << "{\"manager\":\"" << manager_id.string() << "\"}";
            std::string msg = oss.str();
            send_message(info.socket, msg);
            log("generated manager with id " + manager_id.string());
        }
    } else if (node["GET"] || node["get"]) {
        ID elID;
        ID manager_id;
        YAML::Node getNode = (node["GET"] ? node["GET"] : node["get"]);
        if (!getNode.IsScalar()) {
            std::string msg = "{\"error\":\"invalid format for get request! Must be formatted as a scalar string!\"}";
            send_message(info.socket, msg);
            log(msg);
            return;
        } else {
            // parse id and parameters from request
            auto parse_result = parse_id_and_parms(getNode.as<std::string>());

            if (!parse_result) {
                std::string msg = "{\"error\":\"problem while parsing get request parameters: " + parse_result.error() + "\"}";
                log(msg);
                send_message(info.socket, msg);
                return;
            }

            for (auto& parameter_pair : parse_result->second) {
                if (parameter_pair.first == "manager") {
                    manager_id = ID::fromString(parameter_pair.second);
                } else {
                    std::string msg = "{\"error\":\"invalid parameter in get request: " + parameter_pair.first + "\"}";
                    log(msg);
                    send_message(info.socket, msg);
                    return;
                }
            }

            std::optional<std::string> url;
            if (parse_result->first.index() == 0) {
                elID = std::get<ID>(parse_result->first);
            } else {
                url = std::get<std::string>(parse_result->first);
            } 


            // process request
            try {
                if (manager_id == ID::nullID()) {
                    if (elID == ID::nullID()) {
                        // TODO check url
                        elID = m_urls.at(*url);
                    }
                    ElementPtr el = abstractGet(elID);
                    std::string msg = this->emitIndividual(*el);
                    send_message(info.socket, msg);
                    log("server got element " +  elID.string() + " for client " + id.string() + ":\n" + msg);
                } else {
                    MetaManager& meta_manager = get_meta_manager(manager_id);

                    // first check if we have a stereotyped element of this
                    auto stereotype_match = meta_manager.get_stereotyped_element(elID);
                    std::string msg;
                    if (stereotype_match) {
                        msg = this->emitIndividual(*stereotype_match);
                        send_message(info.socket, msg);
                    } else {
                        MetaManager::Pointer<MetaElement> el = meta_manager.get(elID);
                        msg = meta_manager.emit_meta_element(*el);
                        send_message(info.socket, msg);
                    }
                    log("server got element " + elID.string() + " from manager " + manager_id.string() + " for client " + id.string() + " :\n" + msg);
                }
            } catch (std::exception& e) {
                log(e.what());
                std::string msg = std::string("{\"ERROR\":\"") + std::string(e.what()) + std::string("\"}");
                log(msg);
                send_message(info.socket, msg);
                return;
            } 
        }
    } else if (node["POST"] || node["post"]) {
        log("server handling post request from client " + id.string());
        try {
            ID id;
            auto postNode = node["POST"] ? node["POST"] : node["post"];
            if (postNode.IsScalar()) {
                std::size_t type = names_to_element_type.at((node["POST"] ? node["POST"] : node["post"]).as<std::string>());
                id = ID::fromString(node["id"].as<std::string>());
                ElementPtr ret = 0;
                ret = create(type);
                ret->setID(id);
            } else if (postNode.IsMap()) {
                ManagedPtr<AbstractElement> created_element;
                auto manager_node = postNode["manager"];
                if (manager_node) {
                    switch (check_id(manager_node)) {
                        case NOT_SCALAR: {
                            std::string msg = "{\"error\":\"post request improperly formatted, manager must be a scalar!\"}";
                            log(msg);
                            send_message(info.socket, msg);
                            return;
                        }
                        case NOT_ID: {
                            std::string msg = "{\"error\":\"post request manager not a valid id!\"}";
                            log(msg);
                            send_message(info.socket, msg);
                            return;
                        }
                    }
                    ID manager_id = ID::fromString(manager_node.as<std::string>());

                    MetaManager& meta_manager = get_meta_manager(manager_id);
                    
                    if (!postNode["type"]) {
                        std::string msg = "{\"error\":\"post request did not specify type!\"}";
                        log(msg);
                        return;
                    }
                    
                    std::optional<std::size_t> element_type_option;

                    switch (check_id(postNode["type"])) {
                        case 0: {
                             element_type_option = meta_manager.get_type_with_id(ID::fromString(postNode["type"].as<std::string>()));
                             break;
                        }
                        case NOT_SCALAR: {
                            std::string msg = "{\"error\":\"type must be a scalar value for post requests!\"}";
                            log(msg);
                            send_message(info.socket, msg);
                            return;
                        }
                             
                    }


                    if (!element_type_option) {
                        element_type_option = meta_manager.get_type_by_name(postNode["type"].as<std::string>());
                    }

                    if (!element_type_option) {
                        std::string msg = std::format(
                            "{{\"error\":\"could not determine type provided <{}>\"}}",
                            postNode["type"].as<std::string>()        
                        );
                        log(msg);
                        send_message(info.socket, msg);
                        return;
                    }

                    EGM::ID id_specified = EGM::ID::nullID();

                    if (postNode["id"]) {
                        id_specified = ID::fromString(postNode["id"].as<std::string>());
                        log("setting id of posted element to " + created_element.id().string());
                    } 

                    auto applying_element_node = postNode["applying_element"];
                    if (applying_element_node) {
                        // post request is asking us to apply a stereotype from this manager
                        switch (check_id(applying_element_node)) {
                            case NOT_SCALAR: {
                                std::string msg = "{\"error\":\"post request improperly formatted, manager must be a scalar!\"}";
                                log(msg);
                                send_message(info.socket, msg);
                                return;
                            }
                            case NOT_ID: {
                                std::string msg = "{\"error\":\"post request manager not a valid id!\"}";
                                log(msg);
                                send_message(info.socket, msg);
                                return;
                            }
                        }

                        UmlManager::Pointer<Element> uml_element = get(ID::fromString(applying_element_node.as<std::string>()));
                        created_element = meta_manager.apply(*uml_element, *element_type_option, id_specified);
                    } else {
                        created_element = meta_manager.create(*element_type_option, id_specified);
                    }
                } else {
                    if (postNode["type"]) {
                        auto type = names_to_element_type.at(postNode["type"].as<std::string>());
                        ElementPtr ret = create(type);
                        
                        if (postNode["url"]) {
                            auto urlVal = postNode["url"].as<std::string>();
                            if (ID::isValid(urlVal)) {
                                ret->setID(ID::fromString(urlVal));
                            } else {
                                // break up url into path and find owner
                                log("TODO url specified for POST request!");
                            }                        
                        }
                        if (postNode["name"] && ret->is<NamedElement>()) {
                            NamedElementPtr namedPtr = ret;
                            namedPtr->setName(postNode["name"].as<std::string>());
                        }
                        if (postNode["owner"]) {
                            if (postNode["set"]) {
                                auto owner = get(ID::fromString(postNode["owner"].as<std::string>()));
                                std::string setName = postNode["set"].as<std::string>();
                                this->m_types.at(owner->getElementType())->forEachSet(*owner, [this, setName, ret](std::string name , AbstractSet& set) {
                                    if (name == setName) {
                                        this->addToSet(set, *ret);
                                    }
                                });
                            } else {
                                // TODO find appropriate set by type
                                log("TODO! owner specified but not the set, TODO automatically infer best set for owner");
                            }
                        }
                        created_element = ret;
                        if (postNode["id"]) {
                            id = ID::fromString(postNode["id"].as<std::string>());
                            created_element->setID(id);
                            log("set id of posted element to " + created_element.id().string());
                        }
                    } else {
                        std::string msg = "{\"error\":\"Must specify type when posting a uml element\"}";
                        log(msg);
                        send_message(info.socket, msg);
                        return;
                    }
                }
            }
            std::string reply_message = "{\"status\":\"success\"}";
            send_message(info.socket, reply_message);
            log(reply_message);
            std::lock_guard<std::mutex> garbageLck(m_garbageMtx);
            m_releaseQueue.push_front(id);
            m_garbageCv.notify_one();
        } catch (std::exception& e) {
            std::string error_message = std::format(
                    "{{\"error\":\"server could not create new element for client {} exception with request: {}\"}}",
                    id.string(),
                    e.what()
                );
            log(error_message);
            send_message(info.socket, error_message);
            return;
        }
    } else if (node["PUT"] || node["put"]) {
        YAML::Node putNode = (node["PUT"] ? node["PUT"] : node["put"]);
        if (!putNode.IsMap()) {
            std::string msg = "{\"error\":\"Improper formatting for put request! Must be a map!\"}";
            log(msg);
            send_message(info.socket, msg);
            return;
        }

        auto manager_node = putNode["manager"];
        if (manager_node) {
            if (!manager_node.IsScalar()) {
                std::string error_msg = "{\"error\":\"Bad format for put request manager field! Must be a scalar id!\"}";
                log(error_msg);
                send_message(info.socket, error_msg);
                return;
            }
            
            if (!ID::isValid(manager_node.as<std::string>())) {
                std::string error_msg = "{\"error\":\"Bad format for put request manager field! Improper id format!\"}";
                log(error_msg);
                send_message(info.socket, error_msg);
                return;
            }

            auto element_node = putNode["element"];
            if (!element_node.IsMap()) {
                std::string error_msg = "{\"error\":\"Bad format for put request element field! Field must be a map!\"}";
                log(error_msg);
                send_message(info.socket, error_msg);
                return;
            }

            MetaManager& meta_manager = get_meta_manager(ID::fromString(manager_node.as<std::string>()));

            auto el = meta_manager.parse_node(element_node);
            if (el) {
                meta_manager.restoreElAndOpposites(el);
            }
            log("put element " + el.id().string() + " to meta manager " + manager_node.as<std::string>() + " for client " + id.string() + " succesfully!");
        } else {
            try {
                ElementPtr el = parseNode(putNode["element"]);
                if (el) {
                    // run add policies we skipped over
                    restoreElAndOpposites(el);
                }
                bool isRoot = false;
                if (putNode["qualifiedName"]) {
                    if (putNode["qualifiedName"].as<std::string>().compare("") == 0) {
                        isRoot = true;
                    }
                    m_urls[putNode["qualifiedName"].as<std::string>()] = el.id();
                }
                if (isRoot) {
                    setRoot(*el);
                }
                log("server put element " + el.id().string() + " successfully for client " + id.string());
            } catch (std::exception& e) {
                log("Error parsing PUT request: " + std::string(e.what()));
                std::string error_message = std::format(
                        "{{\"error\":\"Error parsing put request {}\"}}",
                        e.what()    
                    );
                send_message(info.socket, error_message);
                return;
            }
        }
        std::string reply_message = "{\"status\":\"success\"}";
        log(reply_message);
        send_message(info.socket, reply_message);
    } else if (node["SAVE"] || node["save"]) {
        YAML::Node saveNode = (node["SAVE"] ? node["SAVE"] : node["save"]);
        std::string path = saveNode.as<std::string>();
        try {
            if (path.empty()) {
                save();
            } else {
                save(path);
            }
        } catch (std::exception& e) {
            log("ERROR saving element, error: " + std::string(e.what()));
            std::string error_message = std::format(
                    "{{\"error\":\"error saving element: {}\"}}",
                    e.what()    
                );
            send_message(info.socket, error_message);
            return;
        }
        log("saved element to " + path);
        std::string reply_message = "{\"status\":\"success\"}";
        log(reply_message);
        send_message(info.socket, reply_message);
    } else {
        log("ERROR receiving message from client, invalid format!\nMessage:\n" + buff);
        std::string msg = "{\"error\":\"ERROR receiving message from client, invalid format!\"}";
        send_message(info.socket, msg);
        return;
    }
    log("Done processing message");
}

void UmlServer::receiveFromClient(UmlServer* me, ID id) {

    /**
     * TODO: lossless receive, client can rapidfire messages
     **/

    me->log("server set up thread to listen to client " + id.string());
    ClientInfo& info = me->m_clients[id];
    while (me->m_running) {
        // receive message
        auto message_option = receive_message(info.socket);
        if (!message_option) {
            me->log(std::format("ERROR: fatal client error for client {}", id.string()));
            return;
        }

        // dispatch message
        std::lock_guard<std::mutex> lck(info.handlerMtx);
        info.threadQueue.push_back(*message_option);
        me->log("receive from client thread added new message to threadQueue");
        info.handlerCv.notify_one();
    }
}

void UmlServer::acceptNewClients(UmlServer* me) {
    struct pollfd pfds[1] = {{me->m_socketD, POLLIN}};
    {
        std::lock_guard<std::mutex> rLck(me->m_runMtx);
        while (me->m_running) {
            socketType newSocketD = 
            #ifndef WIN32
            0;
            #else
            INVALID_SOCKET;
            #endif
            struct addrinfo* clientAddress;
            socklen_t addr_size = sizeof clientAddress;
            #ifndef WIN32
            if (!poll(pfds, 1, 1000)) {
            #else
            if (!WSAPoll(pfds, 1,1000)) {
            #endif
                continue;
            }
            if (!me->m_running) {
                break;
            }
            std::lock_guard<std::mutex> aLck(me->m_acceptMtx);
            me->log("server aquired acceptance lock");
            #ifndef WIN32
            newSocketD = accept(me->m_socketD, (struct sockaddr *)&clientAddress, &addr_size);
            if (newSocketD == -1) {
                if (me->m_running) {
                    me->log("bad socket accepted, error: " + std::string(strerror(errno)));
                    throw ManagerStateException("bad socket accepted");
                } else {
                    continue;
                }
            }
            #else
            newSocketD = accept(me->m_socketD, 0, 0);
            if (newSocketD == INVALID_SOCKET) {
                if (me->m_running) {
                    closesocket(newSocketD);
                    WSACleanup();
                    me->log("bad socket accepted, error: " + std::string(strerror(errno)));
                    throw ManagerStateException("bad socket accepted");
                } else {
                    closesocket(newSocketD);
                    WSACleanup();
                    continue;
                }
            }
            #endif
           
            YAML::Emitter emitter;
            emitter << YAML::DoubleQuoted << YAML::Flow; // emit json

            // info to give is just the present metaManagers in a list
            emitter << YAML::BeginSeq;
            for (auto& meta_manager_pair : me->meta_managers()) {
                emitter << YAML::BeginMap;
                emitter << YAML::Key << "id";
                emitter << YAML::Value << meta_manager_pair.first.string();
                emitter << YAML::Key << "uml_root";
                emitter << YAML::Value << meta_manager_pair.second.get_generation_root().id().string();
                emitter << YAML::EndMap;
            }
            emitter << YAML::EndSeq;

            // send to client
            std::string json_dump = emitter.c_str();
            send_message(newSocketD, json_dump);

            // client will send back an id
            uint64_t size_buffer;
            int bytes_received =  recv(newSocketD, &size_buffer, sizeof(uint64_t), 0);
            if (bytes_received != sizeof(uint64_t)) {
                throw ManagerStateException("Did not read full uint64_t size");
            }

            size_buffer = be64toh(size_buffer);

            if (size_buffer != 28) {
                throw ManagerStateException("Clients reported message size for id is of improper size!");
            }

            char id_buffer[29];
            bytes_received = recv(newSocketD, id_buffer, 28, 0);
            if (bytes_received != 28) {
                throw ManagerStateException("Did not receive enough bytes from client for id!");
            }

            id_buffer[28] = '\0';

            auto client_id = ID::fromString(id_buffer);

            if (client_id == me->m_shutdownID)
                break;

            // add to client map setup threads
            me->log("got id from client: " + client_id.string());
            ClientInfo& client_info = me->m_clients[client_id];
            client_info.socket = newSocketD;
            client_info.thread = new std::thread(receiveFromClient, me, client_id);
            client_info.handler = new std::thread(clientSubThreadHandler, me, client_id);
           
            auto id_buffer_string = client_id.string();
            send_message(newSocketD, id_buffer_string);
            me->log("sent id back to client: " + client_id.string());
        }
        me->m_running = false;
    }
    me->m_runCv.notify_all();
}

void UmlServer::clientSubThreadHandler(UmlServer* me, ID id) {
    ClientInfo& info = me->m_clients[id];
    while (me->m_running && info.thread) {
        std::unique_lock<std::mutex> lck(info.handlerMtx);
        info.handlerCv.wait(lck, [&info] { return !info.threadQueue.empty(); });
        for (std::string buff : info.threadQueue) {
            if (!buff.empty()) {
                me->handleMessage(id, buff);
            }
        }
        info.threadQueue.clear();
    }
}

void UmlServer::garbageCollector(UmlServer* me) {
    while(me->m_running) {
        std::unique_lock<std::mutex> garbageLck(me->m_garbageMtx);
        me->m_garbageCv.wait(garbageLck, [me] { return me->m_releaseQueue.size() != me->m_numEls; });
        if (me->m_numEls == me->m_maxEls) {
            ID releasedID = me->m_releaseQueue.back();
            ElementPtr elToErase = me->get(releasedID);
            me->release(*elToErase);
            me->m_releaseQueue.pop_back();
        } else {
            me->m_numEls++;
        }
    }
}

void UmlServer::closeClientConnections(ClientInfo& client) {
    client.thread->join();
    #ifndef WIN32
    close(client.socket);
    #else
    int result = shutdown(client.socket, SD_SEND);
    if (result == SOCKET_ERROR) {
        closesocket(client.socket);
        WSACleanup();
    }
    #endif
    delete client.thread;
    client.thread = 0;
    client.threadQueue.push_back("");
    client.handlerCv.notify_all();
    client.handler->join();
    delete client.handler;
}

void UmlServer::zombieKiller(UmlServer* me) {
    while(me->m_running) {
        std::unique_lock<std::mutex> zombieLck(me->m_zombieMtx);
        me->m_zombieCv.wait(zombieLck, [me] { return !me->m_zombies.empty() || !me->m_running; });
        for (const ID id : me->m_zombies) {
            ClientInfo& client = me->m_clients[id];
            me->closeClientConnections(client);
            me->m_clients.erase(id);
        }
        me->m_zombies.clear();
    }
}

#ifdef UML_DEBUG
std::string time_in_HH_MM_SS_MMM()
{
    using namespace std::chrono;

    // get current time
    auto now = system_clock::now();

    // get number of milliseconds for the current second
    // (remainder after division into seconds)
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    // convert to std::time_t in order to convert to std::tm (broken time)
    auto timer = system_clock::to_time_t(now);

    // convert to broken time
    std::tm bt = *std::localtime(&timer);

    std::ostringstream oss;

    oss << std::put_time(&bt, "%H:%M:%S"); // HH:MM:SS
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
}

#endif

// std::vector<std::unique_lock<std::mutex>> UmlServer::lockReferences(ManagerNode& node) {
//     std::vector<std::unique_lock<std::mutex>> ret;
//     ret.reserve(node.m_references.size());
//     for (auto& referencePair : node.m_references) {
//         if (!referencePair.second.node) {
//             continue;
//         }
//         ret.push_back(std::unique_lock<std::mutex>(referencePair.second.node->m_mtx));
//     }
//     return ret;
// }

void UmlServer::log(std::string msg) {
    std::lock_guard<std::mutex> lck(m_logMtx);
    #if UML_DEBUG
    std::cout << "[" << time_in_HH_MM_SS_MMM() << "]:" << msg << std::endl;
    #else
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::string nowStr = std::ctime(&nowTime);
    std::cout << "[" + nowStr.substr(0, nowStr.size() - 1) + "]:" + msg << std::endl;
    #endif
}

UmlServer::UmlServer(int port, bool deferStart) {
    m_port = port;
    if (!deferStart) {
        start();
    }
}

UmlServer::UmlServer(int port) : UmlServer(port, false) {

}

UmlServer::UmlServer(bool deferStart) : UmlServer(UML_PORT, deferStart) {

}

UmlServer::UmlServer() : UmlServer(UML_PORT, false) {
    
}

UmlServer::~UmlServer() {
    if (m_running) {
        shutdownServer();
    }
}

void UmlServer::start() {
    int status;
    #ifndef WIN32
    struct addrinfo hints;
    struct addrinfo* m_address;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me
    const char* portStr = std::to_string(m_port).c_str();
    if ((status = getaddrinfo(NULL, portStr, &hints, &m_address)) != 0) {
        throw ManagerStateException("Server could not get address info! error: " + std::string(strerror(errno)));
    }

    // get socket descriptor
    if ((m_socketD = socket(m_address->ai_family, m_address->ai_socktype, m_address->ai_protocol)) == -1) {
        throw ManagerStateException("Server could not get socket from addressinfo, error: " + std::string(strerror(errno)));
    }

    // allow address reuse
    int enable_reuse_addr = 1;
    if (setsockopt(m_socketD, SOL_SOCKET, SO_REUSEADDR, &enable_reuse_addr, sizeof(int)) < 0) {
        throw ManagerStateException("Server could not set socket options, error: " + std::string(strerror(errno)));
    }

    // disable NAGLES algorithm cause we tend to send small bits of data instead of large messages
    int enable_tcp_nodelay = 1;
    if (setsockopt(m_socketD, IPPROTO_TCP, TCP_NODELAY, &enable_tcp_nodelay, sizeof(int)) < 0) {
        throw ManagerStateException("Server could not set socket options, error: " + std::string(strerror(errno)));
    }

    if ((status = bind(m_socketD, m_address->ai_addr, m_address->ai_addrlen)) == -1) {
        throw ManagerStateException("Server could not bind to socket, error: " + std::string(strerror(errno)));
    }
    if ((status = listen(m_socketD, 10)) == -1) {
        throw ManagerStateException("Server could not listen to socker, error:" + std::string(strerror(errno)));
    }
    freeaddrinfo(m_address);
    #else
    status = WSAStartup(MAKEWORD(2,2), &m_wsaData);
    if (status != 0) {
        throw ManagerStateException("TODO Winsock, WSAStartup");
    }
    struct addrinfo *result = 0, *ptr = 0, hints;
    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    status = getaddrinfo(0, std::to_string(m_port).c_str(), &hints, &result);
    if (status != 0) {
        WSACleanup();
        throw ManagerStateException("TODO winsock getaddrinfo");
    }

    m_socketD = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_socketD == INVALID_SOCKET) {
        freeaddrinfo(result);
        WSACleanup();
        throw ManagerStateException("TODO winsock socket");
    }
    
    status = bind(m_socketD, result->ai_addr, (int) result->ai_addrlen);
    freeaddrinfo(result);
    if (status == SOCKET_ERROR) {
        closesocket(m_socketD);
        WSACleanup();
        throw ManagerStateException("TODO winsock bind");
    }
    status = listen(m_socketD, SOMAXCONN);
    if (status == SOCKET_ERROR) {
        closesocket(m_socketD);
        WSACleanup();
        throw ManagerStateException("TODO winsock listen");
    }
    #endif

    m_running = true;
    m_acceptThread = new std::thread(acceptNewClients, this);
    m_garbageCollectionThread = new std::thread(garbageCollector, this);
    m_zombieKillerThread = new std::thread(zombieKiller, this);
    log("server set up thread to accept new clients");
}

int UmlServer::numClients() {
    std::lock_guard<std::mutex> lck(m_acceptMtx);
    return m_clients.size();
}

size_t UmlServer::count(ID id) {
    // std::lock_guard<std::mutex> graphLock(m_graphMtx);
    return loaded(id) ? 1 : 0;
}

void UmlServer::reset() {
    log("server resetting");
    // clear();
}

void UmlServer::shutdownServer() {
    log("server shutting down");
    bool fail = false;
    struct addrinfo hints;
    struct addrinfo* myAddress;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me
    int status;
    if ((status = getaddrinfo(NULL, std::to_string(m_port).c_str(), &hints, &myAddress)) != 0) {
        // TODO warn on improper shutdown
        log("server could not get address info!");
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        std::cerr << stderr << std::endl;
        fail = true;
    }
    socketType tempSocket;
    if (!fail) {
        tempSocket = socket(myAddress->ai_family, myAddress->ai_socktype, myAddress->ai_protocol);
        if (tempSocket == -1) {
            /** TODO: failure message **/
            log("server could not get socket descriptor!");
            fail = true;
        }
    }
    if (!fail) {
        if (connect(tempSocket, myAddress->ai_addr, myAddress->ai_addrlen) == -1) {
            /** TODO: failure message **/
            log("server could not connect to self!");
            fail = true;
        }
    }
    
    if (!fail) {
        // send terminate message
        char* idMsg = new char[29];
        std::string strBuff = m_shutdownID.string();
        std::copy(strBuff.begin(), strBuff.end(), idMsg);
        idMsg[28] = '\0';
        send(tempSocket, idMsg, 29, 0);
        freeaddrinfo(myAddress);
        #ifndef WIN32
        close(tempSocket);
        #else
        int result = shutdown(tempSocket, SD_SEND);
        if (result == SOCKET_ERROR) {
            closesocket(tempSocket);
            WSACleanup();
        }
        #endif
        delete[] idMsg;

        // wait for thread to stop
        std::unique_lock<std::mutex> rLck(m_runMtx);
        m_runCv.wait(rLck, [this]{ return !m_running; });
        m_acceptThread->join();
    }

    // close everything
    #ifndef WIN32
    close(m_socketD);
    #else
    int result = shutdown(m_socketD, SD_SEND);
    if (result == SOCKET_ERROR) {
        closesocket(m_socketD);
        WSACleanup();
    }
    #endif
    for (auto& client : m_clients) {
        closeClientConnections(client.second);
    }
    delete m_acceptThread;

    m_releaseQueue.clear();
    m_numEls = -1;
    m_garbageCv.notify_one();
    m_garbageCollectionThread->join();
    delete m_garbageCollectionThread;

    m_zombieCv.notify_one();
    m_zombieKillerThread->join();
    delete m_zombieKillerThread;

    m_shutdownV = true;
    m_shutdownCv.notify_all();
    log("server succesfully shut down");
}

void UmlServer::setMaxEls(int maxEls) {
    std::lock_guard<std::mutex> garbageLck(m_garbageMtx);
    m_maxEls = maxEls;
}

int UmlServer::getMaxEls() {
    return m_maxEls;
}

int UmlServer::getNumElsInMemory() {
    return m_numEls;
}

using namespace std::chrono_literals;

int UmlServer::waitTillShutDown(int ms) {
    std::unique_lock<std::mutex> sLck(m_shutdownMtx);
    m_shutdownCv.wait_for(sLck, ms * 1ms, [this] { return m_shutdownV; });
    return 1;
}

int UmlServer::waitTillShutDown() {
    std::unique_lock<std::mutex> sLck(m_shutdownMtx);
    m_shutdownCv.wait(sLck, [this] { return m_shutdownV; });
    return 1;
}

void UmlServer::setRoot(AbstractElementPtr el) {
    BaseManager::setRoot(el);
    if (!el) {
        return;
    }
    m_urls[""] = el->getID();
}

void UmlServer::setRoot(UmlServer::Implementation<Element>& el) {
    setRoot(&el);
}
