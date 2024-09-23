#pragma once

#include "uml/managers/umlManager.h"
#include "uml/managers/serialization/uml-cafe/umlCafeSerializationPolicy.h"
#include "serverPersistencePolicy.h"

namespace UML {
    class UmlClient : public Manager<UmlTypes, UmlCafeJsonSerializationPolicy<UmlTypes>, ServerPersistencePolicy> {
        public:
            ElementPtr get(std::string qualifiedName);
            ElementPtr get(ID id) {
                return Manager<UmlTypes, UmlCafeJsonSerializationPolicy<UmlTypes>, ServerPersistencePolicy>::get(id);
            }
            void setRoot(AbstractElementPtr root) override;
    };
}