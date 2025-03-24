#pragma once

#include "uml/uml-stable.h"
#include "metaManager.h"
#include <atomic>
#include <iostream>
#include <mutex>
#include <condition_variable>
#ifdef WIN32
#include "winsock2.h"
#include <ws2tcpip.h>
#include <stdio.h>
typedef SOCKET socketType;
#else
typedef int socketType;
#endif

#define UML_PORT 8652
#define UML_SERVER_MSG_SIZE 200
#define UML_SERVER_NUM_ELS 200

namespace std {
    class thread;
}

namespace UML {

    class UmlServer;

    struct UmlServerSerializationPolicy : public EGM::JsonSerializationPolicy<UmlTypes> {
        std::vector<EGM::ManagedPtr<EGM::AbstractElement>> parseWhole(std::string data) override;
        std::string emitWhole(EGM::AbstractElement& el) override;
        protected:
            UmlServer* m_uml_server = 0;
    };

    class UmlServer : public EGM::Manager<UmlTypes, EGM::SerializedStoragePolicy<UmlServerSerializationPolicy, EGM::FilePersistencePolicy>> {

        private:
            friend struct UmlServerSerializationPolicy;
            using BaseManager = EGM::Manager<UmlTypes, EGM::SerializedStoragePolicy<UmlServerSerializationPolicy, EGM::FilePersistencePolicy>>; 

            struct ClientInfo {
                socketType socket;
                std::thread* thread;
                std::thread* handler;
                std::mutex handlerMtx;
                std::condition_variable handlerCv;
                std::list<std::string> threadQueue;
            };

            int m_port = UML_PORT;

            //data
            const EGM::ID m_shutdownID = EGM::ID::randomID();
            socketType m_socketD = 
            #ifndef WIN32
            0;
            #else
            INVALID_SOCKET;
            WSADATA m_wsaData;
            #endif
            std::unordered_map<EGM::ID, ClientInfo> m_clients;
            std::unordered_map<std::string, EGM::ID> m_urls;
            std::list<EGM::ID> m_releaseQueue;
            long unsigned int m_numEls = 0;
            long unsigned int m_maxEls = UML_SERVER_NUM_ELS;

            // threading
            static void acceptNewClients(UmlServer* me);
            static void receiveFromClient(UmlServer* me, EGM::ID id);
            static void clientSubThreadHandler(UmlServer* me, EGM::ID id);
            static void garbageCollector(UmlServer* me);
            static void zombieKiller(UmlServer* me);
            void handleMessage(EGM::ID id, std::string buff);
            void sendMessage(ClientInfo& info, std::string& data);
            std::thread* m_acceptThread = 0;
            std::thread* m_garbageCollectionThread = 0;
            std::thread* m_zombieKillerThread = 0;
            std::atomic<bool> m_running = false;
            std::mutex m_runMtx;
            std::condition_variable m_runCv;
            std::mutex m_logMtx;
            std::mutex m_acceptMtx;
            std::mutex m_shutdownMtx;
            std::condition_variable m_shutdownCv;
            bool m_shutdownV = false;
            std::mutex m_garbageMtx;
            std::condition_variable m_garbageCv;
            std::list<EGM::ID> m_zombies;
            std::mutex m_zombieMtx;
            std::condition_variable m_zombieCv;
            std::mutex m_messageHandlerMtx;
            
            std::unordered_map<std::string, std::size_t> names_to_element_type;

            template <class List, class Dummy = void>
            struct fill_names_to_element_type;

            template <template <class> class First, template <class> class ... Rest, class Dummy>
            struct fill_names_to_element_type<EGM::TemplateTypeList<First, Rest...>, Dummy> {
                static void fill(UmlServer& server) {
                    if constexpr (!UmlServer::IsAbstract<First>{})
                        server.names_to_element_type.emplace(EGM::ElementInfo<First>::name(), UmlServer::ElementType<First>::result);
                    
                    fill_names_to_element_type<EGM::TemplateTypeList<Rest...>>::fill(server);
                }
            };

            template <class Dummy>
            struct fill_names_to_element_type<EGM::TemplateTypeList<>, Dummy> {
                static void fill(UmlServer&) {}
            };

            std::unordered_map<EGM::ID, MetaManager> m_meta_managers;

        protected:
            void closeClientConnections(ClientInfo& client);
        public:
            UmlServer();
            UmlServer(int port);
            UmlServer(int port, bool deferStart);
            UmlServer(bool deferStart);
            virtual ~UmlServer();
            void start();
            int numClients();
            void log(std::string msg);
            size_t count(EGM::ID id);
            void reset();
            void shutdownServer();
            void setMaxEls(int maxEls);
            int getMaxEls();
            int getNumElsInMemory();
            int waitTillShutDown(int ms);
            int waitTillShutDown();
            void setRoot(EGM::AbstractElementPtr el) override;
            void setRoot(UmlManager::Implementation<Element>& el);
    };
}
