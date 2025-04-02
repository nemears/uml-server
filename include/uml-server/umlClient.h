#pragma once

#include "uml/uml-stable.h"
#include "serverPersistencePolicy.h"
#include "generativeManager.h"

namespace UML {
    class UmlClient : public GenerativeManager<EGM::Manager<UmlTypes, EGM::SerializedStoragePolicy<GenerativeSerializationPolicy, ServerPersistencePolicy>>> {
        using BaseManager = GenerativeManager<EGM::Manager<UmlTypes, EGM::SerializedStoragePolicy<GenerativeSerializationPolicy, ServerPersistencePolicy>>>; 
        public:
            BaseManager::Pointer<Element> get(std::string qualifiedName);
            BaseManager::Pointer<Element> get(EGM::ID id) {
                return BaseManager::get(id);
            }
            void setRoot(EGM::AbstractElementPtr root) override;
    };
}
