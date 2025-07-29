#pragma once

#include "proxyElement.h"
#include "metaElement.h"
#include "metaManager.h"
#include "setConcepts.h"

namespace UML {
   
    class AbstractProxySet {};

    template <class Policy, template <template <class> class, class, class> class SetType>
    class ProxyElementSet : public SetType<ProxyElement, MetaElement<Policy>, EGM::DoNothingPolicy> , public AbstractProxySet {
        protected:
            using BaseSet = SetType<ProxyElement, MetaElement<Policy>, EGM::DoNothingPolicy>;
            MetaManager::ProxyElementPtr get_proxy_element(UmlManager::Pointer<Element> el) {
                MetaManager& meta_manager = dynamic_cast<MetaManager&>(this->m_el.m_manager);
                auto proxy_element_match = meta_manager.m_proxy_elements.find(el.id());
                MetaManager::ProxyElementPtr proxy_el;
                if (proxy_element_match == meta_manager.m_proxy_elements.end()) {
                    proxy_el = meta_manager.create_proxy_element(el);
                } else {
                    proxy_el = proxy_element_match->second;
                } 
                return proxy_el;
            }
        public:
            using BaseSet::BaseSet;
            void set(UmlManager::Pointer<Element> el) requires HasSetMethod<Policy, SetType> {
                BaseSet::set(get_proxy_element(el));
            }

            void set(EGM::ID id) requires HasSetMethod<Policy, SetType> {
                auto& uml_manager = dynamic_cast<MetaManager&>(this->m_el.m_manager).m_uml_manager;
                set(uml_manager.createPtr(id));
            }

            void add(UmlManager::Pointer<Element> el) requires HasAddMethod<Policy, SetType> {
                BaseSet::add(get_proxy_element(el));
            }

            void add(EGM::ID& id) requires HasAddMethod<Policy, SetType> {
                auto& uml_manager = dynamic_cast<MetaManager&>(this->m_el.m_manager).m_uml_manager;
                add(uml_manager.createPtr(id));
            }

            using ElementPtr = UmlManager::Pointer<Element>;
            using ElementImpl = UmlManager::Implementation<Element>;

            struct Iterator : public EGM::AbstractSet::iterator {
                std::unique_ptr<EGM::AbstractSet::iterator> uml_iterator;
                Iterator() {}
                Iterator(const Iterator& rhs) {
                    uml_iterator = rhs.uml_iterator->clone();
                }
                EGM::AbstractElementPtr getCurr() const override {
                    return uml_iterator->getCurr();
                }
                void next() override {
                    uml_iterator->next();
                }
                protected:
                std::unique_ptr<EGM::AbstractSet::iterator> clone() const override {
                    return std::make_unique<Iterator>(*this);
                }
                ElementImpl& operator*() {
                    return dynamic_cast<ElementImpl&>(*getCurr());
                }
                ElementPtr operator->() {
                    return getCurr();
                }
                Iterator operator++() {
                    next();
                    return *this;
                }
                Iterator operator++(int) {
                    return ++(*this);
                }
            };

            Iterator begin() const {
                Iterator iterator;
                iterator.uml_iterator = BaseSet::beginPtr();
                return iterator;
            }
            Iterator end() const {
                Iterator iterator;
                iterator.uml_iterator = BaseSet::endPtr();
                return iterator;
            }
            std::unique_ptr<EGM::AbstractSet::iterator> endPtr() const override {
                return std::make_unique<Iterator>(std::move(begin()));
            }
            UmlManager::Pointer<Element> front() requires HasFrontMethod<Policy, SetType> {
                return std::dynamic_pointer_cast<MetaManager::Implementation<ProxyElement>>(begin().getCurr().ptr())->m_uml_element;
            }
    };
}
