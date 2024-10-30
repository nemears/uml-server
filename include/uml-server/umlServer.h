#ifndef _UML_UML_SERVER_H_
#define _UML_UML_SERVER_H_

#include "uml/managers/umlManager.h"
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

    class UmlServer : public Manager<UmlTypes, UmlCafeJsonSerializationPolicy<UmlTypes>> {

        private:
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
            const ID m_shutdownID = ID::randomID();
            socketType m_socketD = 
            #ifndef WIN32
            0;
            #else
            INVALID_SOCKET;
            WSADATA m_wsaData;
            #endif
            std::unordered_map<ID, ClientInfo> m_clients;
            std::unordered_map<std::string, ID> m_urls;
            std::list<ID> m_releaseQueue;
            long unsigned int m_numEls = 0;
            long unsigned int m_maxEls = UML_SERVER_NUM_ELS;

            // threading
            static void acceptNewClients(UmlServer* me);
            static void receiveFromClient(UmlServer* me, ID id);
            static void clientSubThreadHandler(UmlServer* me, ID id);
            static void garbageCollector(UmlServer* me);
            static void zombieKiller(UmlServer* me);
            void handleMessage(ID id, std::string buff);
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
            std::list<ID> m_zombies;
            std::mutex m_zombieMtx;
            std::condition_variable m_zombieCv;

            std::mutex m_messageHandlerMtx;

            // helper methods
            template <class OwnerType, class OwneeType, std::size_t I = 0>
            bool findSetAndAddHelper2(std::string setName, OwnerType& owner, OwneeType& ownee) {
                if constexpr (I + 1 < std::tuple_size<typename OwnerType::Info::BaseList>{}) {
                    return findSetAndAddHelper2<std::tuple_element_t<I + 1, typename OwnerType::Info::BaseList>, OwneeType, 0>(setName, owner, ownee);
                } else {
                    for (auto set : OwnerType::Info::sets(owner)) {
                        if (set.first == setName) {
                            addToSet(*set.second, ownee);
                            return true;
                        }
                    }    
                }
                return false;
            }

            template <std::size_t I = 0, class Tuple, class OwnerType>
            void findSetAndAddHelper1(std::string setName, OwnerType& owner, BaseElement<Tuple>& el) {
                if (I == el.getElementType()) {
                    findSetAndAddHelper2(setName, owner, el. template as<std::tuple_element_t<I, Tuple>>());
                }
                if constexpr (I + 1 < std::tuple_size<Tuple>{}) {
                    findSetAndAddHelper1<I + 1, Tuple, OwnerType>(setName, owner, el);
                } else {
                    throw ManagerStateException("could not downcast element!");
                }
            }

            template <std::size_t I = 0, class Tuple>
            void findSetAndAdd(std::string setName, BaseElement<Tuple>& el, BaseElement<Tuple>& ownee) {
                if (I == el.getElementType()) {
                    findSetAndAddHelper1(setName, el. template as<std::tuple_element_t<I, Tuple>>(), ownee);
                }
                if constexpr (I + 1 < std::tuple_size<Tuple>{}) {
                    findSetAndAdd<I + 1, Tuple>(setName, el, ownee);
                } else {
                    throw ManagerStateException("could not downcast element!");
                }
            } 
        protected:
            void closeClientConnections(ClientInfo& client);
            // std::vector<std::unique_lock<std::mutex>> lockReferences(ManagerNode& node);
        public:
            UmlServer();
            UmlServer(int port);
            UmlServer(int port, bool deferStart);
            UmlServer(bool deferStart);
            virtual ~UmlServer();
            void start();
            int numClients();
            void log(std::string msg);
            size_t count(ID id);
            void reset();
            void shutdownServer();
            void setMaxEls(int maxEls);
            int getMaxEls();
            int getNumElsInMemory();
            int waitTillShutDown(int ms);
            int waitTillShutDown();
            void setRoot(AbstractElementPtr el) override;
            void setRoot(Element& el);
    };
}

#endif
