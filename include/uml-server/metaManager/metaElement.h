#pragma once

#include "uml/uml-stable.h"
#include "proxyElement.h"

namespace UML {
    template <class>
    struct MetaElementSetPolicy;

    
    template <class, template <template<class> class, class, class> class>
    class MetaElementSet;

    template <class Policy, template <template <class> class, class, class> class SetType>
    class ProxyElementSet;

    template <class ManagerPolicy>
    struct MetaElement : public ManagerPolicy {
        template <class, template <template <class> class, class, class> class>
        friend class ProxyElementSet;
        using Info = EGM::TypeInfo<MetaElement>;
        MANAGED_ELEMENT_CONSTRUCTOR(MetaElement);
        std::unordered_map<EGM::ID, std::unique_ptr<EGM::AbstractSet>> sets;
        std::unordered_map<EGM::ID, std::unique_ptr<EGM::AbstractDataPolicy>> data;
        std::vector<std::size_t> m_bases;
        std::string name = "";
        UmlManager::Pointer<InstanceSpecification> uml_representation;
        UmlManager::Pointer<Classifier> meta_type;
        UmlManager::Pointer<Element> applying_element;
        using Set = MetaElementSet<ManagerPolicy, EGM::Set>;
        using OrderedSet = MetaElementSet<ManagerPolicy, EGM::OrderedSet>;
        using Singleton = MetaElementSet<ManagerPolicy, EGM::Singleton>;
        using ProxySet = ProxyElementSet<ManagerPolicy, EGM::Set>;
        using ProxySingleton = ProxyElementSet<ManagerPolicy, EGM::Singleton>;
        using ProxyOrderedSet = ProxyElementSet<ManagerPolicy, EGM::OrderedSet>;
        Set& getSet(EGM::ID id) const {
            return dynamic_cast<Set&>(*sets.at(id));
        }
        OrderedSet& getOrderedSet(EGM::ID id) const {
            return dynamic_cast<OrderedSet&>(*sets.at(id));
        }
        Singleton& getSingleton(EGM::ID id) const {
            return dynamic_cast<Singleton&>(*sets.at(id));
        }
        ProxySet& getProxySet(EGM::ID id) const {
            return dynamic_cast<ProxySet&>(*sets.at(id));
        }
        ProxySingleton& getProxySingleton(EGM::ID id) const {
            return dynamic_cast<ProxySingleton&>(*sets.at(id));
        }
        ProxyOrderedSet& getProxyOrderedSet(EGM::ID id) const {
            return dynamic_cast<ProxyOrderedSet&>(*sets.at(id));
        }
        private:
            void init() {}
    };
}

namespace EGM {
    template <>
    struct ElementInfo<UML::MetaElement> {
        static std::string name() {
            return "MetaElement";
        }
        template <class Policy>
        static SetList sets(UML::MetaElement<Policy>& el) {
            SetList ret;
            ret.reserve(el.sets.size());
            for (auto& pair : el.sets) {
                UML::UmlManager::Pointer<UML::Property> prop = get_element_from_uml_manager(&el, pair.first);
                ret.push_back(make_set_pair(prop->getName().c_str(), *pair.second));
            }
            return ret;
        }
        using MetaElementDataList = std::vector<std::pair<std::string, AbstractDataPolicy*>>; 
        template <class Policy>
        static  MetaElementDataList data(UML::MetaElement<Policy>& el) {
            MetaElementDataList ret;
            ret.reserve(el.data.size());
            for (auto& pair : el.data) {
                UML::UmlManager::Pointer<UML::Property> prop = get_element_from_uml_manager(&el, pair.first);
                ret.push_back(std::make_pair<std::string, AbstractDataPolicy*>(prop->getName(), pair.second.get()));
            }
            return ret;
        }
    };
}
