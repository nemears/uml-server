#include "uml-server/metaManager.h"
#include "uml-server/constants.h"

using namespace UML;
using namespace EGM;

namespace UML {
UmlManager::Pointer<UML::Element> get_element_from_uml_manager(AbstractElementPtr ptr, ID id) {
    MetaManager::Pointer<MetaElement> meta_ptr = ptr;
    return dynamic_cast<UML::MetaManager&>(meta_ptr->getManager()).getUmlManager().get(id); 
}
}
AbstractElementPtr MetaElementSerializationPolicy::parse_meta_element_node(YAML::Node node, std::function<EGM::AbstractElementPtr(std::size_t)> f) {
    auto it = node.begin();
    while (it != node.end()) {
        const auto keyNode = it->first;
        const auto valNode = it->second;
        if (valNode.IsMap()) {
            // look up key
            try {
                auto el = f(m_meta_manager->m_name_to_type.at(keyNode.as<std::string>()));
                if (valNode["id"] && valNode["id"].IsScalar()) {
                    el->setID(EGM::ID::fromString(valNode["id"].template as<std::string>()));
                }
                
                auto serialization_policy = m_serializationByType.at(0); // get meta manager
                serialization_policy->parseBody(valNode, el);
                serialization_policy->parseScope(node, el);
                return el;
            } catch (std::exception& e) {
                throw EGM::ManagerStateException("Could not find proper type to parse! line number " + std::to_string(keyNode.Mark().line));
            }
        }
        it++;
    }
    throw EGM::ManagerStateException("could not find property type to parse! line number " + std::to_string(node.Mark().line));
}

AbstractElementPtr MetaElementSerializationPolicy::parseNode(YAML::Node node) {
    return parse_meta_element_node(node, [this](std::size_t element_type) -> AbstractElementPtr { return m_meta_manager->create(element_type); });
}



MetaManager::MetaElementPtr MetaManager::parse_stereotype_node(UmlManager::Implementation<Element>& el, YAML::Node node) {
    return parse_meta_element_node(node, [this, &el](std::size_t element_type) -> AbstractElementPtr { return m_meta_manager->apply(el, element_type); });
}


void MetaElementSerializationPolicy::emitIndividual(YAML::Emitter& emitter, EGM::AbstractElement& el) {
    auto serialization_policy = m_serializationByType.at(0);

    MetaManager::Pointer<MetaElement> meta_el = AbstractElementPtr(&el);

    // emit scope
    serialization_policy->emitScope(emitter, &el);

    // emit body
    std::string elementName = meta_el->name;
    emitter << YAML::Key << elementName << YAML::Value << YAML::BeginMap;
    emitter << YAML::Key << "id" << YAML::Value << meta_el.id().string();
    serialization_policy->emitBody(emitter, &el);
    emitter << YAML::EndMap;
}

MetaManager::MetaManager(UmlManager::Implementation<Package>& abstraction_root) : 
    m_uml_manager(dynamic_cast<UmlManager&>(abstraction_root.getManager()))
{
    
    // link to serialization policy
    m_meta_manager = this;

    m_generation_root = &abstraction_root;

    // set the storage root to be a floating package
    // (this is usually bad practice cause it won't be "saved" by the manager
    // but th storage roots of the meta managers are included specially within UmlServer's 
    // Storage policy TODO)
    m_storage_root = m_uml_manager.create<Package>();
   
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
                        type_id == boolean_type_id ||
                        type_id == integer_type_id ||
                        type_id == real_type_id ||
                        type_id == string_type_id ||
                        type_id == unlimited_natural_type_id
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

template <template <class> class Literal>
void create_literal_slot(UmlManager& uml_manager, UmlManager::Pointer<Property> property, UmlManager::Pointer<InstanceSpecification> inst, UmlManager::Pointer<Literal> default_value) {
    auto slot = uml_manager.create<Slot>();
    slot->setDefiningFeature(property);
    if (default_value && default_value->template is<Literal>()) {
        auto slot_value = uml_manager.create<Literal>();
        slot_value->setValue(default_value->template as<Literal>().getValue());
        slot->getValues().add(slot_value);
    }
    inst->getSlots().add(slot); 
}

// create_meta_element
// element_type - element_type of meta_element
// applying_element - pointer (can be null) to element applying meta_element to as stereotype
// return - the meta_element created as a ptr
EGM::AbstractElementPtr MetaManager::create_meta_element(std::size_t element_type, UmlManager::Pointer<Element> applying_element) {
    auto meta_element = BaseManager::create<MetaElement>();

    m_meta_elements.insert(meta_element.id());
    
    // get representing classifier
    auto meta_type = m_uml_types.at(element_type);

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

    // reusable lambda for creating a property corresponding to a set
    // returns ptr to set
    std::function<EGM::AbstractSet*(UmlManager::Pointer<Property>)> create_property_set;
    create_property_set = [meta_element, element_instance, applying_element, this, &create_property_set](UmlManager::Pointer<Property> property) -> EGM::AbstractSet* {
        // check if already created
        auto property_sets_it = meta_element->sets.find(property.id());
        if (property_sets_it != meta_element->sets.end()) {
            return property_sets_it->second.get();
        }
        
        // check if the set is from the base uml meta model
        if (applying_element) {
            auto uml_property_match = uml_meta_model_property_ids.find(property.id());
            if (uml_property_match != uml_meta_model_property_ids.end()) {
                return &uml_property_match->second(applying_element);
            }
        }
        
        auto upper_value_spec = property->getUpperValue();
        std::optional<int> upper_value = std::nullopt;
        if (upper_value_spec && upper_value_spec->is<LiteralInteger>()) {
            upper_value = upper_value_spec->as<LiteralInteger>().getValue();
        }

        EGM::AbstractSet* created_set = 0;

        if (upper_value && *upper_value == 1) {
            // singleton
            created_set = &*meta_element->sets.emplace(property.id(), std::make_unique<EGM::Singleton<MetaElement, MetaElementImpl>>(&*meta_element)).first->second;
        } else if (property->isOrdered()) {
            created_set = &*meta_element->sets.emplace(property.id(), std::make_unique<EGM::OrderedSet<MetaElement, MetaElementImpl>>(&*meta_element)).first->second;
        } else {
            // default to set
            created_set = &*meta_element->sets.emplace(property.id(), std::make_unique<EGM::Set<MetaElement, MetaElementImpl>>(&*meta_element)).first->second;
        }

        for (auto subsetted_property : property->getSubsettedProperties().ptrs()) {
            auto subset = create_property_set(subsetted_property);
            created_set->subsets(*subset);
        }

        for (auto redefined_property : property->getRedefinedProperties().ptrs()) {
            auto redefined_set = create_property_set(redefined_property);
            created_set->redefines(*redefined_set);
        }

        // set up slot
        auto slot = m_uml_manager.create<Slot>();
        slot->setDefiningFeature(property);

        // TODO default value

        element_instance->getSlots().add(slot);
        return created_set;
    };

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
            if (type_id == boolean_type_id) {
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
                create_literal_slot<LiteralBoolean>(m_uml_manager, property, element_instance, default_value);
            } else if (type_id == integer_type_id) {
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
                create_literal_slot<LiteralInteger>(m_uml_manager, property, element_instance, default_value);
            } else if (type_id == real_type_id) {
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
                create_literal_slot<LiteralReal>(m_uml_manager, property, element_instance, default_value);
            } else if (type_id == string_type_id) {
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
                create_literal_slot<LiteralString>(m_uml_manager, property, element_instance, default_value);
            } else if (type_id == unlimited_natural_type_id) {
                throw EGM::ManagerStateException("TODO Unlimited Natural");
            } else {
                create_property_set(property);
            }
        }

        for (auto base : front->getGenerals().ptrs()) {
            queue.push_back(base);
        }
    }
    
    return meta_element; 
}
