#include "uml-server/metaManager/metaManager.h"
#include "uml-server/constants.h"

using namespace UML;
using namespace EGM;

namespace UML {
UmlManager::Pointer<UML::Element> get_element_from_uml_manager(AbstractElementPtr ptr, ID id) {
    MetaManager::Pointer<MetaElement> meta_ptr = ptr;
    return dynamic_cast<UML::MetaManager&>(meta_ptr->getManager()).getUmlManager().abstractGet(id); 
}
}
AbstractElementPtr MetaElementSerializationPolicy::parse_meta_element_node(YAML::Node node, std::function<EGM::AbstractElementPtr(std::size_t,EGM::ID)> f) {
    auto it = node.begin();
    while (it != node.end()) {
        const auto keyNode = it->first;
        const auto valNode = it->second;
        if (valNode.IsMap()) {
            // look up key
            try {
                EGM::ID el_id = EGM::ID::nullID();
                if (valNode["id"] && valNode["id"].IsScalar()) {
                    el_id = EGM::ID::fromString(valNode["id"].template as<std::string>());
                }
                auto el = f(this->m_meta_manager->m_name_to_type.at(keyNode.as<std::string>()), el_id);
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
    return parse_meta_element_node(node, [this](std::size_t element_type, EGM::ID element_id) -> AbstractElementPtr { 
            return this->m_meta_manager->create(element_type, element_id);
        });
}



MetaManager::MetaElementPtr MetaManager::parse_stereotype_node(UmlManager::Implementation<Element>& el, YAML::Node node) {
    return parse_meta_element_node(node, [this, &el](std::size_t element_type, EGM::ID element_id) -> AbstractElementPtr { 
            return apply(el, element_type, element_id); 
        });
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

MetaManager::MetaManager(UmlManager::Implementation<Element>& abstraction_root) : 
    m_uml_manager(abstraction_root.getManager())
{
    
    // link to serialization policy
    MetaElementSerializationPolicy::m_meta_manager = this;
    MetaElementStoragePolicy::m_meta_manager = this;

    m_generation_root = &abstraction_root;

    // set the storage root to be a floating package
    // (this is usually bad practice cause it won't be "saved" by the manager
    // but th storage roots of the meta managers are included specially within UmlServer's 
    // Storage policy TODO)
    m_storage_root = m_uml_manager.create<Package>();
   
    // set up types 
    std::list<UmlManager::Pointer<Element>> queue = { &abstraction_root };
    std::size_t next_type = 0;
    while (!queue.empty()) {
        auto front = queue.front();
        queue.pop_front();
        if (front->is<Classifier>()) {
            UmlManager::Pointer<Classifier> curr_classifier = front;
            if (m_id_to_type.count(front.id())) {
                continue;
            }
            m_uml_types.emplace(next_type, front);
            m_id_to_type.emplace(front.id(), next_type);
            m_name_to_type.emplace(front->as<NamedElement>().getName(), next_type);
            next_type++;
            for (auto prop : curr_classifier->getAttributes().ptrs()) {
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
            for (auto base : curr_classifier->getGenerals().ptrs()) {
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

using MetaElementImpl = MetaManager::Implementation<MetaElement>;

template <template <class> class Literal>
void create_literal_slot(ManagerTypes<UmlTypes>& uml_manager, UmlManager::Pointer<Property> property, UmlManager::Pointer<InstanceSpecification> inst) {
    auto slot = uml_manager.create<Slot>();
    slot->setDefiningFeature(property);
    auto default_value = property->getDefaultValue();
    if (default_value && default_value->template is<Literal>()) {
        auto slot_value = uml_manager.create<Literal>();
        slot_value->setValue(default_value->template as<Literal>().getValue());
        slot->getValues().add(slot_value);
    }
    inst->getSlots().add(slot); 
}

enum class PrimitivePolicyType {
    BOOLEAN,
    INTEGER,
    NULL_TYPE,
    REAL,
    STRING,
    UNLIMITED_NATURAL
};

struct AbstractPrimitivePolicy {
    UmlManager::Pointer<Property> defining_feature;
    virtual PrimitivePolicyType primitive() const = 0;
};

struct RealDataPolicy : public EGM::AbstractDataPolicy , public AbstractPrimitivePolicy {
    double m_val = 0;
    RealDataPolicy(double val) : m_val(val) {}
    std::string getData() override {
        return std::to_string(m_val);
    }
    void setData(std::string data) override {
        char* rest {};
        m_val = std::strtod(data.c_str(), &rest);
    }
    void setData(double val) {
        m_val = val;
    }
    PrimitivePolicyType primitive() const override { return PrimitivePolicyType::REAL; }
};

struct MetaManagerBooleanDataPolicy : public EGM::AbstractDataPolicy , public AbstractPrimitivePolicy {
    bool m_val;
    MetaManagerBooleanDataPolicy(bool val) : m_val(val) {}
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
    void setData(bool val) {
        m_val = val;
    }
    PrimitivePolicyType primitive() const override { return PrimitivePolicyType::BOOLEAN; }
};

struct IntegerDataPolicy : public EGM::AbstractDataPolicy , public AbstractPrimitivePolicy {
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
    void setData(int val) {
        m_val = val;
    }
    PrimitivePolicyType primitive() const override { return PrimitivePolicyType::INTEGER; }
};

struct StringDataPolicy : public EGM::AbstractDataPolicy , public AbstractPrimitivePolicy {
    std::string m_val = "";
    StringDataPolicy(std::string val) : m_val(val) {}
    std::string getData() override {
        return m_val;
    }
    void setData(std::string data) override {
        m_val = data;
    }
    PrimitivePolicyType primitive() const override { return PrimitivePolicyType::STRING; }
};

void MetaManager::create_uml_representation(MetaManager::Pointer<MetaElement> meta_element) {
    auto element_instance = m_uml_manager.create<InstanceSpecification>();
    meta_element->uml_representation = element_instance;
    element_instance->setID(meta_element.id());
    element_instance->getClassifiers().add(meta_element->meta_type);
    m_storage_root->getPackagedElements().add(element_instance);

    if (meta_element->applying_element) {
        meta_element->applying_element->getAppliedStereotypes().add(element_instance);
    }

    for (auto& set_pair : meta_element->sets) {
        // set up slot
        auto slot = m_uml_manager.create<Slot>();
        UmlManager::Pointer<Property> property = meta_element->meta_type->getAttributes().get(set_pair.first);
        slot->setDefiningFeature(property);

        auto& set_policy = dynamic_cast<MetaElementSetPolicy<MetaManager::GenBaseHierarchy<MetaElement>>&>(*set_pair.second);
        set_policy.uml_manager = &m_uml_manager;
        set_policy.uml_slot = slot;

        // default value
        auto default_value = property->getDefaultValue();
        if (default_value) {
            if (!default_value->is<InstanceValue>()) {
                throw ManagerStateException("default value error, must be instance value!");
            }
            auto value_instance = default_value->as<InstanceValue>().getInstance();
            if (!value_instance) {
                throw ManagerStateException("default value error, instance value does not have an instance!");
            }
            // auto instance_classifier = value_instance->getClassifiers().front();
            // if (!instance_classifier) {
            //     throw ManagerStateException("default value error, instance does not have a classifier!");
            // }
            // auto value = this->create(instance_classifier.id());
            auto value = this->abstractGet(value_instance.id());
            switch (set_pair.second->setType()) {
                case EGM::SetType::SINGLETON:
                    dynamic_cast<MetaElementImpl::Singleton*>(set_pair.second.get())->set(value);
                    break;
                case EGM::SetType::SET:
                    dynamic_cast<MetaElementImpl::Set*>(set_pair.second.get())->add(value); 
                    break;
                case EGM::SetType::ORDERED_SET:
                    dynamic_cast<MetaElementImpl::OrderedSet*>(set_pair.second.get())->add(value); 
                    break;
                default:
                    throw ManagerStateException("TODO!");
            }
            set_policy.default_value = value.id();
        }

        element_instance->getSlots().add(slot);
    }

    for (auto& data_pair : meta_element->data) {
        AbstractPrimitivePolicy& primitive_policy = dynamic_cast<AbstractPrimitivePolicy&>(*data_pair.second);
        auto property = primitive_policy.defining_feature;
        switch (primitive_policy.primitive()) {
            case PrimitivePolicyType::BOOLEAN:
                create_literal_slot<LiteralBoolean>(m_uml_manager, property, element_instance);
                break;
            case PrimitivePolicyType::INTEGER:
                create_literal_slot<LiteralInteger>(m_uml_manager, property, element_instance);
                break;
            case PrimitivePolicyType::STRING: 
                create_literal_slot<LiteralString>(m_uml_manager, property, element_instance);
                break;
            case PrimitivePolicyType::REAL:
                create_literal_slot<LiteralReal>(m_uml_manager, property, element_instance);
                break;
            default:
                throw ManagerStateException("Could not process primitive type!");
        }
    }
}

template <template <class> class UmlType, template <template <class> class, class, class> class SetType>
using UmlSetType = SetType<UmlType, MetaManager::Implementation<MetaElement>, EGM::DoNothingPolicy>; 

template <template <template <class> class, class, class> class SetType>
std::unique_ptr<AbstractSet> create_meta_set(EGM::ID type_id, MetaManager::Implementation<MetaElement>& meta_el) {
    // special sets to hold uml types
    if (uml_meta_types.contains(type_id)) {
        if (type_id == association_type_id) {
            return std::make_unique<UmlSetType<UML::Association, SetType>>(&meta_el);
        }
        if (type_id == class_type_id) {
            return std::make_unique<UmlSetType<UML::Class, SetType>>(&meta_el);
        }
        if (type_id == classifier_type_id) {
            return std::make_unique<UmlSetType<UML::Classifier, SetType>>(&meta_el);
        }
        if (type_id == comment_type_id) {
            return std::make_unique<UmlSetType<UML::Comment, SetType>>(&meta_el);
        }
        if (type_id == connectable_element_type_id) {
            return std::make_unique<UmlSetType<UML::Comment, SetType>>(&meta_el);
        }
        if (type_id == data_type_type_id) {
            return std::make_unique<UmlSetType<UML::DataType, SetType>>(&meta_el);
        }
        if (type_id == dependency_type_id) {
            return std::make_unique<UmlSetType<UML::Dependency, SetType>>(&meta_el);
        }
        if (type_id == directed_relationship_type_id) {
            return std::make_unique<UmlSetType<UML::DirectedRelationship, SetType>>(&meta_el);
        }
        if (type_id == element_type_id) {
            return std::make_unique<UmlSetType<UML::Element, SetType>>(&meta_el);
        }
        if (type_id == encapsulated_classifier_type_id) {
            return std::make_unique<UmlSetType<UML::EncapsulatedClassifier, SetType>>(&meta_el);
        }
        if (type_id == enumeration_type_id) {
            return std::make_unique<UmlSetType<UML::Enumeration, SetType>>(&meta_el);
        }
        if (type_id == enumeration_literal_type_id) {
            return std::make_unique<UmlSetType<UML::EnumerationLiteral, SetType>>(&meta_el);
        }
        if (type_id == extension_type_id) {
            return std::make_unique<UmlSetType<UML::Extension, SetType>>(&meta_el);
        }
        if (type_id == extension_end_type_id) {
            return std::make_unique<UmlSetType<UML::ExtensionEnd, SetType>>(&meta_el);
        }
        if (type_id == feature_type_id) {
            return std::make_unique<UmlSetType<UML::Feature, SetType>>(&meta_el);
        }
        if (type_id == generalization_type_id) {
            return std::make_unique<UmlSetType<UML::Generalization, SetType>>(&meta_el);
        }
        if (type_id == instance_specification_type_id) {
            return std::make_unique<UmlSetType<UML::InstanceSpecification, SetType>>(&meta_el);
        }
        if (type_id == instance_value_type_id) {
            return std::make_unique<UmlSetType<UML::InstanceValue, SetType>>(&meta_el);
        }
        if (type_id == literal_boolean_type_id) {
            return std::make_unique<UmlSetType<UML::LiteralBoolean, SetType>>(&meta_el);
        }
        if (type_id == literal_integer_type_id) {
            return std::make_unique<UmlSetType<UML::LiteralInteger, SetType>>(&meta_el);
        }
        if (type_id == literal_null_type_id) {
            return std::make_unique<UmlSetType<UML::LiteralNull, SetType>>(&meta_el);
        }
        if (type_id == literal_real_type_id) {
            return std::make_unique<UmlSetType<UML::LiteralReal, SetType>>(&meta_el);
        }
        if (type_id == literal_specification_type_id) {
            return std::make_unique<UmlSetType<UML::LiteralSpecification, SetType>>(&meta_el);
        }
        if (type_id == literal_string_type_id) {
            return std::make_unique<UmlSetType<UML::LiteralString, SetType>>(&meta_el);
        }
        if (type_id == literal_unlimited_natural_type_id) {
            return std::make_unique<UmlSetType<UML::LiteralUnlimitedNatural, SetType>>(&meta_el);
        }
        if (type_id == multiplicity_element_type_id) {
            return std::make_unique<UmlSetType<UML::MultiplicityElement, SetType>>(&meta_el);
        }
        if (type_id == named_element_type_id) {
            return std::make_unique<UmlSetType<UML::NamedElement, SetType>>(&meta_el);
        }
        if (type_id == namespace_type_id) {
            return std::make_unique<UmlSetType<UML::Namespace, SetType>>(&meta_el);
        }
        if (type_id == package_type_id) {
            return std::make_unique<UmlSetType<UML::Package, SetType>>(&meta_el);
        }
        if (type_id == packageable_element_type_id) {
            return std::make_unique<UmlSetType<UML::Package, SetType>>(&meta_el);
        }
        // if (type_id == parameterable_element_type_id) {
        //     return std::make_unique<UmlSetType<UML::ParameterableElement, SetType>>(&meta_el);
        // }
        if (type_id == primitive_type_element_type_id) {
            return std::make_unique<UmlSetType<UML::PrimitiveType, SetType>>(&meta_el);
        }
        if (type_id == profile_type_id) {
            return std::make_unique<UmlSetType<UML::Profile, SetType>>(&meta_el);
        }
        if (type_id == property_type_id) {
            return std::make_unique<UmlSetType<UML::Property, SetType>>(&meta_el);
        }
        if (type_id == redefinable_element_type_id) {
            return std::make_unique<UmlSetType<UML::RedefinableElement, SetType>>(&meta_el);
        }
        if (type_id == relationship_type_id) {
            return std::make_unique<UmlSetType<UML::Relationship, SetType>>(&meta_el);
        }
        if (type_id == slot_type_id) {
            return std::make_unique<UmlSetType<UML::Slot, SetType>>(&meta_el);
        }
        if (type_id == stereotype_type_id) {
            return std::make_unique<UmlSetType<UML::Stereotype, SetType>>(&meta_el);
        }
        if (type_id == structural_feature_type_id) {
            return std::make_unique<UmlSetType<UML::StructuralFeature, SetType>>(&meta_el);
        }
        if (type_id == structured_classifier_type_id) {
            return std::make_unique<UmlSetType<UML::StructuredClassifier, SetType>>(&meta_el);
        }
        if (type_id == type_type_id) {
            return std::make_unique<UmlSetType<UML::Type, SetType>>(&meta_el);
        }
        if (type_id == typed_element_type_id) {
            return std::make_unique<UmlSetType<UML::TypedElement, SetType>>(&meta_el);
        }
        if (type_id == value_specification_type_id) {
            return std::make_unique<UmlSetType<UML::ValueSpecification, SetType>>(&meta_el);
        }
        throw ManagerStateException("unhandled type! " + type_id.string());
    }

    // default handling for meta_elements
    // return std::make_unique<SetType<MetaElement, MetaManager::Implementation<MetaElement>>>(&meta_el);
    return std::make_unique<MetaElementSet<MetaManager::GenBaseHierarchy<MetaElement>, SetType>>(&meta_el);
}

MetaManager::Pointer<MetaElement> MetaManager::create_meta_element_object(std::size_t element_type, UmlManager::Pointer<Element> applying_element) {
    auto meta_element = BaseManager::create<MetaElement>();

    if (next_id != EGM::ID::nullID()) {
        meta_element->setID(next_id);
        next_id = EGM::ID::nullID();
    }

    m_meta_elements.insert(meta_element.id());
    
    // get representing classifier
    auto meta_type = m_uml_types.at(element_type);
    meta_element->meta_type = meta_type;

    // set name
    meta_element->name = meta_type->getName();

    // mark applying element (stereotyped element) if exists
    meta_element->applying_element = applying_element;

    // set bases
    for (auto base : meta_type->getGenerals().ptrs()) {
        meta_element->m_bases.push_back(m_id_to_type.at(base.id()));
    }

    // reusable lambda for creating a property corresponding to a set
    // returns ptr to set
    std::function<EGM::AbstractSet*(UmlManager::Pointer<Property>)> create_property_set;
    create_property_set = [meta_element, applying_element, this, &create_property_set](UmlManager::Pointer<Property> property) -> EGM::AbstractSet* {
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
            created_set = &*meta_element->sets.emplace(
                    property.id(), 
                    create_meta_set<EGM::Singleton>(property->getType().id(), *meta_element)
                ).first->second;
        } else if (property->isOrdered()) {
            // ordered set
            created_set = &*meta_element->sets.emplace(
                    property.id(),
                    create_meta_set<EGM::OrderedSet>(property->getType().id(), *meta_element)
                ).first->second;
        } else {
            // default to set
            created_set = &*meta_element->sets.emplace(
                    property.id(),
                    create_meta_set<EGM::Set>(property->getType().id(), *meta_element)
                ).first->second;
        }

        for (auto subsetted_property : property->getSubsettedProperties().ptrs()) {
            auto subset = create_property_set(subsetted_property);
            created_set->subsets(*subset);
        }

        for (auto redefined_property : property->getRedefinedProperties().ptrs()) {
            auto redefined_set = create_property_set(redefined_property);
            created_set->redefines(*redefined_set);
        }

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

                auto data_policy = std::make_unique<MetaManagerBooleanDataPolicy>(initial_value);
                data_policy->defining_feature = property;
                meta_element->data.emplace(property.id(), std::move(data_policy));
            } else if (type_id == integer_type_id) {
                int initial_value = 0;
                auto default_value = property->getDefaultValue();
                if (default_value && default_value->is<LiteralInteger>()) {
                    initial_value = default_value->as<LiteralInteger>().getValue();
                }

                
                auto data_policy = std::make_unique<IntegerDataPolicy>(initial_value);
                data_policy->defining_feature = property;
                meta_element->data.emplace(property.id(), std::move(data_policy));
            } else if (type_id == real_type_id) {
                double initial_value = 0;
                auto default_value = property->getDefaultValue();
                if (default_value && default_value->is<LiteralReal>()) {
                    initial_value = default_value->as<LiteralReal>().getValue();
                }

                
                auto data_policy = std::make_unique<RealDataPolicy>(initial_value);
                data_policy->defining_feature = property;
                meta_element->data.emplace(property.id(), std::move(data_policy));
            } else if (type_id == string_type_id) {
                std::string initial_value = "";
                auto default_value = property->getDefaultValue();
                if (default_value && default_value->is<LiteralString>()) {
                    initial_value = default_value->as<LiteralString>().getValue();
                }

                
                auto data_policy = std::make_unique<StringDataPolicy>(initial_value);
                data_policy->defining_feature = property;
                meta_element->data.emplace(property.id(), std::move(data_policy));
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

// create_meta_element
// element_type - element_type of meta_element
// applying_element - pointer (can be null) to element applying meta_element to as stereotype
// return - the meta_element created as a ptr
EGM::AbstractElementPtr MetaManager::create_meta_element(std::size_t element_type, UmlManager::Pointer<Element> applying_element) {
    auto meta_element = create_meta_element_object(element_type, applying_element); 
    create_uml_representation(meta_element); 

    if (applying_element) {
        m_stereotyped_elements.emplace(applying_element.id(), applying_element);
    }

    return meta_element; 
}

AbstractElementPtr MetaElementStoragePolicy::loadElement(ID id) {
    UmlManager::Pointer<Element> uml_element = m_meta_manager->m_uml_manager.abstractGet(id);

    if (uml_element->is<InstanceSpecification>()) {
        auto& instance = uml_element->as<InstanceSpecification>();
        auto classifier = instance.getClassifiers().front();
        if (!classifier) {
            return uml_element;
            // throw ManagerStateException("no classifier for instance when restoring meta element");
        }

        auto match = m_meta_manager->m_id_to_type.find(classifier.id());
        if (match == m_meta_manager->m_id_to_type.end()) {
            return uml_element;
            // throw ManagerStateException("not tracking classifier " + classifier.id().string());
        }

        m_meta_manager->next_id = instance.getID();
        auto meta_element = m_meta_manager->create_meta_element_object(match->second, 0);

        for (auto& slot : instance.getSlots()) {
            auto defining_feature = slot.getDefiningFeature();
            if (!defining_feature) {
                throw ManagerStateException("bad slot for type representation " + classifier.id().string());
            }

            auto set_match = meta_element->sets.find(defining_feature.id());

            if (set_match != meta_element->sets.end()) {
                // the slot is a match to one of the meta_element's set
                auto& set = *set_match->second;
                auto get_val_meta_element = [this] (UmlManager::Pointer<ValueSpecification> uml_val) -> MetaManager::Pointer<MetaElement> {
                    if (!uml_val->is<InstanceValue>()) {
                        throw ManagerStateException("Expected an instance value for slot value!");
                    }
                    auto uml_val_instance = uml_val->as<InstanceValue>().getInstance();
                    if (!uml_val_instance) {
                        throw ManagerStateException("Instance value must have an instance !");
                    }
                    return m_meta_manager->get(uml_val_instance.id()); 
                };
                if (set.setType() == SetType::SINGLETON) {
                    if (slot.getValues().size() > 1) {
                        throw ManagerStateException("bad slot, has more than one value but is a singleon! " + instance.getID().string());
                    }
                    auto uml_val = slot.getValues().front();
                    if (uml_val) {
                        auto val = get_val_meta_element(uml_val);
                        meta_element->getSingleton(defining_feature.id()).set(val);
                    }
                } else {
                    for (auto uml_val : slot.getValues().ptrs()) {
                        auto val = get_val_meta_element(uml_val);
                        switch (set.setType()) {
                            case SetType::SET : {
                                meta_element->getSet(defining_feature.id()).add(val);
                                break;
                            }
                            case SetType::ORDERED_SET: {
                                meta_element->getOrderedSet(defining_feature.id()).add(val);
                                break;                           
                            }
                            default:
                                throw ManagerStateException("TODO");
                        }
                    }
                }
                continue;
            }

            auto data_match = meta_element->data.find(defining_feature.id());

            if (data_match != meta_element->data.end()) {
                auto& data_policy = dynamic_cast<AbstractPrimitivePolicy&>(*data_match->second);
                if (slot.getValues().size() > 1) {
                    throw ManagerStateException("Too many values for primitive type");
                }
                auto uml_val = slot.getValues().front();
                if (!uml_val->is<LiteralSpecification>()) {
                    throw ManagerStateException("Must be a literal specification!");
                }
                switch (data_policy.primitive()) {
                    case PrimitivePolicyType::BOOLEAN:
                        dynamic_cast<MetaManagerBooleanDataPolicy&>(data_policy).setData(uml_val->as<LiteralBoolean>().getValue());
                        break;
                    case PrimitivePolicyType::INTEGER:
                        dynamic_cast<IntegerDataPolicy&>(data_policy).setData(uml_val->as<LiteralInteger>().getValue());
                        break;
                    case PrimitivePolicyType::STRING:
                        dynamic_cast<StringDataPolicy&>(data_policy).setData(uml_val->as<LiteralString>().getValue());
                        break;
                    case PrimitivePolicyType::REAL:
                        dynamic_cast<RealDataPolicy&>(data_policy).setData(uml_val->as<LiteralReal>().getValue());
                        break;
                    default:
                        throw ManagerStateException("TODO");
                }
                continue;
            }
        }

        return meta_element;
    }

    return uml_element;
    // throw ManagerStateException("Could not load meta element");
}
void MetaElementStoragePolicy::saveElement(EGM::AbstractElement& el) {
    // do nothing cause our policies take care of it
}
void MetaElementStoragePolicy::eraseEl(EGM::ID id) {
    // handled by overriden method
}
