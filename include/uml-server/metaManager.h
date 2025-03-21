#pragma once

#include "uml/uml-stable.h"

namespace UML {
    
    template <class ManagerPolicy>
    struct MetaElement : public ManagerPolicy {
        using Info = EGM::TypeInfo<MetaElement>;
        MANAGED_ELEMENT_CONSTRUCTOR(MetaElement);
        std::unordered_map<EGM::ID, std::unique_ptr<EGM::AbstractSet>> sets;
        std::unordered_map<EGM::ID, std::unique_ptr<EGM::AbstractDataPolicy>> data;
        std::vector<std::size_t> m_bases;
        std::string name = "";
        private:
            void init() {}
    };


    class MetaManager;

    UmlManager::Pointer<UML::Element> get_element_from_uml_manager(EGM::AbstractElementPtr ptr, EGM::ID id);
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
    };
}

namespace UML {
    struct MetaElementSerializationPolicy : public EGM::YamlSerializationPolicy<EGM::TemplateTypeList<MetaElement>> {
        protected:
            MetaManager& m_meta_manager;
            EGM::AbstractElementPtr parseNode(YAML::Node node) override;  
            std::string emitIndividual(EGM::AbstractElement& el) override;
        public:
            MetaElementSerializationPolicy(MetaManager* meta_manager) : m_meta_manager(*meta_manager) {}
    };

    class MetaManagerStoragePolicy {
        protected:
            EGM::AbstractElementPtr loadElement(EGM::ID id) {
                
            }
            void saveElement(EGM::AbstractElement& el) {
                
            }
            void eraseEl(EGM::ID id) {
                // TODO
            }
    };

    class MetaManager : public EGM::Manager<EGM::TemplateTypeList<MetaElement>, MetaManagerStoragePolicy>, public MetaElementSerializationPolicy {
        friend struct MetaElementSerializationPolicy;
        private:
            using BaseManager = EGM::Manager<EGM::TemplateTypeList<MetaElement>, MetaManagerStoragePolicy>;
            using MetaElementPtr = BaseManager::Pointer<MetaElement>;
            using MetaElementImpl = BaseManager::Implementation<MetaElement>;
            UmlManager& m_uml_manager;
            UmlManager::Pointer<Package> m_storage_root;
            std::unordered_map<std::size_t, UmlManager::Pointer<Classifier>> m_uml_types;
            std::unordered_map<EGM::ID, std::size_t> m_id_to_type;
            std::unordered_map<std::string, std::size_t> m_name_to_type;

        public:

            // Ids relevant to a uml meta manager
            static const EGM::ID boolean_type_id() { return EGM::ID::fromString("bool_bzkcabSy3CiFd&HmJOtnVRK"); }
            static const EGM::ID integer_type_id() { return EGM::ID::fromString("int_r9nNbBukx47IomXrT2raqtc4"); }
            static const EGM::ID real_type_id() {return EGM::ID::fromString("real_aZG&w6yl61bXVWutgeyScN9"); }
            static const EGM::ID string_type_id() { return EGM::ID::fromString("string_L&R5eAEq6f3LUNtUmzHzT"); }
            static const EGM::ID unlimited_natural_type_id() { return EGM::ID::fromString("qlCO1PwnQkJ4kg7DLifFEs0OSv9e"); }

            MetaManager(UmlManager::Implementation<Package>& abstraction_root, UmlManager::Implementation<Package>& storage_root) : 
                MetaElementSerializationPolicy(this),
                m_uml_manager(dynamic_cast<UmlManager&>(abstraction_root.getManager()))
            {
                
                // link to serialization policy
                // set storage_root
                m_storage_root = &storage_root;
               
                // set up types 
                std::list<UmlManager::Pointer<PackageableElement>> queue = { &abstraction_root };
                std::size_t next_type = 0;
                while (!queue.empty()) {
                    auto front = queue.front();
                    queue.pop_front();
                    if (front->is<Classifier>()) {
                        if (m_id_to_type.count(front.id())) {
                            continue;
                        }
                        m_uml_types.emplace(next_type, front);
                        m_id_to_type.emplace(front.id(), next_type);
                        m_name_to_type.emplace(front->as<NamedElement>().getName(), next_type);
                        next_type++;
                        for (auto prop : front->as<Classifier>().getAttributes().ptrs()) {
                            if (prop->getType()) {
                                
                                // check if built in data type
                                const EGM::ID& type_id = prop->getType().id();
                                if (
                                    type_id == boolean_type_id() ||
                                    type_id == integer_type_id() ||
                                    type_id == real_type_id() ||
                                    type_id == string_type_id() ||
                                    type_id == unlimited_natural_type_id()
                                ) 
                                    continue;
                               
                                 queue.push_back(prop->getType());
                            }
                        }
                        for (auto base : front->as<Classifier>().getGenerals().ptrs()) {
                            queue.push_back(base);
                        }
                    }
                    if (front->is<Package>()) {
                        UmlManager::Pointer<Package> package = front;
                        for (auto packagedEl : package->getPackagedElements().ptrs()) {
                            queue.push_back(packagedEl);
                        }
                    }
                }
            }

        private:
            template <template <class> class Literal>
            void create_literal_slot(UmlManager::Pointer<Property> property, UmlManager::Pointer<InstanceSpecification> inst, UmlManager::Pointer<Literal> default_value) {
                auto slot = m_uml_manager.create<Slot>();
                slot->setDefiningFeature(property);
                if (default_value && default_value->template is<Literal>()) {
                    auto slot_value = m_uml_manager.create<Literal>();
                    slot_value->setValue(default_value->template as<Literal>().getValue());
                    slot->getValues().add(slot_value);
                }
                inst->getSlots().add(slot); 
            }
        public:

            EGM::AbstractElementPtr create(std::size_t elementType) override {
                
                auto meta_element = BaseManager::template create<MetaElement>();
                
                // get representing classifier
                auto meta_type = m_uml_types.at(elementType);

                // set name
                meta_element->name = meta_type->getName();

                // set bases
                for (auto base : meta_type->getGenerals().ptrs()) {
                    meta_element->m_bases.push_back(m_id_to_type.at(base.id()));
                }
                
                // add to UmlManager
                auto element_instance = m_uml_manager.create<InstanceSpecification>();
                element_instance->setID(meta_element.id());
                element_instance->getClassifiers().add(meta_type);
                m_storage_root->getPackagedElements().add(element_instance);

                // set sets
                std::list<UmlManager::Pointer<Classifier>> queue = { meta_type };
                while (!queue.empty()) {
                    auto front = queue.front();
                    queue.pop_front();
                    for (auto property : front->getAttributes().ptrs()) {
                        
                        auto property_type = property->getType();
                        if (!property_type) {
                            // TODO log error
                            continue;
                        }

                        // see if the type is primitive type or not to figure out whether to map
                        // the property to a set or to data
                        const EGM::ID& type_id = property_type.id();
                        if (type_id == boolean_type_id()) {
                            bool initial_value = false;
                            auto default_value = property->getDefaultValue();
                            if (default_value && default_value->is<LiteralBoolean>()) {
                                initial_value = default_value->as<LiteralBoolean>().getValue();
                            }

                            struct BooleanDataPolicy : public EGM::AbstractDataPolicy {
                                bool m_val;
                                BooleanDataPolicy(bool val) : m_val(val) {}
                                std::string getData() override {
                                    if (m_val) {
                                        return "true";
                                    } else {
                                        return "false";
                                    }
                                }
                                void setData(std::string data) override {
                                    if (data == "true") {
                                        m_val = true;
                                    } else if (data == "false") {
                                        m_val = false;
                                    } else {
                                        throw EGM::ManagerStateException("could not determine boolean value from string");
                                    }
                                }
                            };

                            meta_element->data.emplace(property.id(), std::make_unique<BooleanDataPolicy>(initial_value));

                            // add to uml_manager
                            create_literal_slot<LiteralBoolean>(property, element_instance, default_value);
                        } else if (type_id == integer_type_id()) {
                            int initial_value = 0;
                            auto default_value = property->getDefaultValue();
                            if (default_value && default_value->is<LiteralInteger>()) {
                                initial_value = default_value->as<LiteralInteger>().getValue();
                            }

                            struct IntegerDataPolicy : public EGM::AbstractDataPolicy {
                                int m_val = 0;
                                IntegerDataPolicy(int val) : m_val(val) {}
                                std::string getData() override {
                                    return std::to_string(m_val);
                                }
                                void setData(std::string data) override {
                                    try {
                                        m_val = std::stoi(data);
                                    } catch (std::invalid_argument& e) {
                                        throw EGM::ManagerStateException("invalid integer");
                                    }
                                }
                            };

                            meta_element->data.emplace(property.id(), std::make_unique<IntegerDataPolicy>(initial_value));

                            // add to uml_manager
                            create_literal_slot<LiteralInteger>(property, element_instance, default_value);
                        } else if (type_id == real_type_id()) {
                            double initial_value = 0;
                            auto default_value = property->getDefaultValue();
                            if (default_value && default_value->is<LiteralReal>()) {
                                initial_value = default_value->as<LiteralReal>().getValue();
                            }

                            struct RealDataPolicy : public EGM::AbstractDataPolicy {
                                double m_val = 0;
                                RealDataPolicy(double val) : m_val(val) {}
                                std::string getData() override {
                                    return std::to_string(m_val);
                                }
                                void setData(std::string data) override {
                                    char* rest {};
                                    m_val = std::strtod(data.c_str(), &rest);
                                }
                            };

                            meta_element->data.emplace(property.id(), std::make_unique<RealDataPolicy>(initial_value));
                            create_literal_slot<LiteralReal>(property, element_instance, default_value);
                        } else if (type_id == string_type_id()) {
                            std::string initial_value = "";
                            auto default_value = property->getDefaultValue();
                            if (default_value && default_value->is<LiteralString>()) {
                                initial_value = default_value->as<LiteralString>().getValue();
                            }

                            struct StringDataPolicy : public EGM::AbstractDataPolicy {
                                std::string m_val = "";
                                StringDataPolicy(std::string val) : m_val(val) {}
                                std::string getData() override {
                                    return m_val;
                                }
                                void setData(std::string data) override {
                                    m_val = data;
                                }
                            };

                            meta_element->data.emplace(property.id(), std::make_unique<StringDataPolicy>(initial_value));
                            create_literal_slot<LiteralString>(property, element_instance, default_value);
                        } else if (type_id == unlimited_natural_type_id()) {
                            throw EGM::ManagerStateException("TODO Unlimited Natural");
                        } else {

                            // all other types
                            // TODO make proper set type based on multiplicity
                            meta_element->sets.emplace(property.id(), std::make_unique<EGM::Set<MetaElement, MetaElementImpl>>(&*meta_element));

                            // set up slot
                            auto slot = m_uml_manager.create<Slot>();
                            slot->setDefiningFeature(property);

                            // TODO default value

                            element_instance->getSlots().add(slot);
                        }
                    }

                    for (auto base : front->getGenerals().ptrs()) {
                        queue.push_back(base);
                    }
                }

                
                return meta_element; 

            }

            MetaElementPtr create(EGM::ID id) {
                return create(m_id_to_type.at(id));
            } 

            UmlManager& getUmlManager() const {
                return m_uml_manager;
            }

            std::string emit_meta_element(MetaManager::Implementation<MetaElement>& el) {
                return emitIndividual(el);
            }
    };
}
