#pragma once
#include "egm/id.h"
#include "generativeManager.h"

#define UML_PORT 8652
#define UML_CLIENT_MSG_SIZE 200

namespace YAML {
    class Emitter;
}

namespace UML {
    class ServerPersistencePolicy : virtual public AbstractGenerativeManager {
        protected:
            std::string m_address;
            int m_port = UML_PORT;
            int m_socketD = 0;
            const EGM::ID clientID = EGM::ID::randomID();

            std::function<void()> m_initialization_procedure;

            void sendEmitter(int socket, YAML::Emitter& emitter);
            std::string loadElementData(EGM::ID id);
            void saveElementData(std::string data, EGM::ID id);
            std::string getProjectData(std::string path);
            std::string getProjectData();
            void saveProjectData(std::string data, std::string path);
            void saveProjectData(std::string data);
            void eraseEl(EGM::ID id);
            EGM::AbstractElementPtr reindex(EGM::ID oldID, EGM::ID newID) override;
            ServerPersistencePolicy();
        public:
            void mount(std::string mountPath);
            virtual ~ServerPersistencePolicy();
    };
}
