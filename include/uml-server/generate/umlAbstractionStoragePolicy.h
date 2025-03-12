#pragma once

#include "uml/uml-stable.h"
#include "optional"

namespace UML {

    template <std::size_t Type, std::size_t TotalTypes>
    struct GeneratedType;

    template <class Types> 
    class UmlAbstractionStoragePolicy : public UmlCafeJsonSerializationPolicy<Types> {

        template <std::size_t Type, std::size_t TotalTypes>
        friend struct GeneratedType;

        private:
            std::optional<std::reference_wrapper<AbstractManager>> m_manager = std::nullopt;
            std::unordered_map<ID, std::size_t> m_typesByID;
            std::unordered_map<std::size_t, ID> m_umlTypeIds;
        protected:
            AbstractElementPtr loadElement(ID id) {
                ElementPtr umlEl = m_manager->get().abstractGet(id);
                if (umlEl->is<InstanceSpecification>()) {
                    InstanceSpecificationPtr umlInstance = umlEl;

                    // look at instace's classifiers
                    // for now just using first, if more than 2 error
                    if (umlInstance->getClassifiers().size() != 1) {
                        throw ManagerStateException("Invalid number of classifiers, contact dev if you need this feature!");
                    }
                    ClassifierPtr classifier = umlInstance->getClassifiers().ptrs().front();
                    AbstractElementPtr ret = this->create(m_typesByID.at(classifier.id()));

                    // fill out 
                    throw ManagerStateException("TODO");
                } else {
                    throw ManagerStateException("TODO");
                }
            }
            void saveElement(AbstractElement& el) {
                throw ManagerStateException("TODO");
            }
            AbstractElementPtr loadAll(std::string path) {
                throw ManagerStateException("TODO");
            }
            AbstractElementPtr loadAll() {
                throw ManagerStateException("TODO");
            }
            void saveAll(AbstractElementPtr root, std::string location) {
                throw ManagerStateException("TODO");
            }
            void eraseEl(ID id) {
                throw ManagerStateException("TODO");
            }
            void link(AbstractManager& umlManager) {
                m_manager = std::optional<std::reference_wrapper<AbstractManager>> {umlManager};
            }
        public:
            void loadTypes(std::unordered_set<ClassifierPtr>& types) {
                std::size_t i = 0;
                for (auto type : types) {
                    if (!m_manager.has_value()) {
                        link(type->getManager());
                    }
                    m_typesByID.emplace(type.id(), i);
                    m_umlTypeIds.emplace(i, type.id());
                    i++;
                }
            }
    };
}