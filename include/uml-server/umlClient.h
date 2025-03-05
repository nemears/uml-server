#pragma once

#include "uml/uml-stable.h"
#include "serverPersistencePolicy.h"

namespace UML {
    class UmlClient : public EGM::Manager<UmlTypes, EGM::SerializedStoragePolicy<EGM::JsonSerializationPolicy<UmlTypes>, ServerPersistencePolicy>> {
        using BaseManager = EGM::Manager<UmlTypes, EGM::SerializedStoragePolicy<EGM::JsonSerializationPolicy<UmlTypes>, ServerPersistencePolicy>>; 
        public:
            BaseManager::Pointer<Element> get(std::string qualifiedName);
            BaseManager::Pointer<Element> get(EGM::ID id) {
                return BaseManager::get(id);
            }
            void setRoot(EGM::AbstractElementPtr root) override;
    };
}
