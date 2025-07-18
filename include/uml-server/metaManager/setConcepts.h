#pragma once

#include "egm/egm-basic.h"

namespace UML {
    template <class Policy, template <template <class> class, class, class> class T>
    concept HasSetMethod = requires(
        T<ProxyElement, MetaElement<Policy>, EGM::DoNothingPolicy>& set_impl, 
        ProxyElement<typename Policy::manager::template GenBaseHierarchy<ProxyElement>>& proxy_el
    ) { 
        set_impl.set(proxy_el);
    };

    template <class Policy, template <template <class> class, class, class> class T>
    concept HasAddMethod = requires(
        T<ProxyElement, MetaElement<Policy>, EGM::DoNothingPolicy>& set_impl,
        ProxyElement<typename Policy::manager::template GenBaseHierarchy<ProxyElement>>& proxy_el
    ) {
        set_impl.add(proxy_el);
    };

    template <class Policy, template <template <class> class, class, class> class T>
    concept HasFrontMethod = requires(
        T<ProxyElement, MetaElement<Policy>, EGM::DoNothingPolicy>& set_impl,
        ProxyElement<typename Policy::manager::template GenBaseHierarchy<ProxyElement>>& proxy_el
    ) {
        set_impl.front();
    }; 
}
