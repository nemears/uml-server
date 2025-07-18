#pragma once

#include "uml/uml-stable.h"

namespace UML {
    template <class ManagerPolicy>
    struct ProxyElement : public ManagerPolicy {
        UmlManager::Pointer<Element> m_uml_element;
        using Info = EGM::TypeInfo<ProxyElement>;
        MANAGED_ELEMENT_CONSTRUCTOR(ProxyElement);
        private:
            void init() {}
    };
}

namespace EGM {
    template <>
    struct ElementInfo<UML::ProxyElement> {
        static std::string name() { return "ProxyElement"; };
    };
}
