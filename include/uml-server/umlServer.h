#pragma once

#include "generativeManager.h"

#include <atomic>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <optional>
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

    void send_message(int socket, std::string& data);
    std::optional<std::string> receive_message(int socket);

    class UmlServer : public GenerativeManager<EGM::Manager<UmlTypes, EGM::SerializedStoragePolicy<GenerativeSerializationPolicy, EGM::FilePersistencePolicy>>> {

        private:
            friend struct UmlServerSerializationPolicy;
            using BaseManager = GenerativeManager<EGM::Manager<UmlTypes, EGM::SerializedStoragePolicy<GenerativeSerializationPolicy, EGM::FilePersistencePolicy>>>;

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
