#pragma once

#include "uml/uml-stable.h"

namespace UML {
    // Ids relevant to a uml meta manager
    const EGM::ID boolean_type_id = EGM::ID::fromString("bool_bzkcabSy3CiFd&HmJOtnVRK");
    const EGM::ID integer_type_id = EGM::ID::fromString("int_r9nNbBukx47IomXrT2raqtc4");
    const EGM::ID real_type_id = EGM::ID::fromString("real_aZG&w6yl61bXVWutgeyScN9");
    const EGM::ID string_type_id = EGM::ID::fromString("string_L&R5eAEq6f3LUNtUmzHzT");
    const EGM::ID unlimited_natural_type_id = EGM::ID::fromString("qlCO1PwnQkJ4kg7DLifFEs0OSv9e");

    // map of ids to functions that return to the corresponding set on a uml element
    std::unordered_map<EGM::ID, std::function<EGM::AbstractSet&(UmlManager::Pointer<Element>)>> uml_meta_model_property_ids = {
        // Association
        { EGM::ID::fromString("d8zBcyATDRBFGVmH5iDnhcPK75Az"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Association>().getMemberEnds(); }},
        { EGM::ID::fromString("5C0yj0wfSbiWvILuj8OS68&CqLJE"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Association>().getNavigableOwnedEnds();}},
        { EGM::ID::fromString("JlEgzt8KZtjezaH4EOmSR8Va1uJz"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Association>().getOwnedEnds();}},

        // Class
        { EGM::ID::fromString("N0AP6gsEA_lVyI3CsenzzBChuZ0x"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Class>().getOwnedAttributes(); }},
        
        // Classifier
        { EGM::ID::fromString("qFjM5899B6bmuxuplcin9csEUa_g"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Classifier>().getAttributes(); }},
        { EGM::ID::fromString("5pvS7Q1wlyZbihnK1eqOnOMLgLS&"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Classifier>().getFeatures(); }},
        { EGM::ID::fromString("xgmvbUmIrJ9jZG&HKOw0tOKhN9eS"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Classifier>().getGeneralizations(); }},

        // Comment
        { EGM::ID::fromString("LZ2gyp2q4IVggFXMhkbNe1bp24FI"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Comment>().getAnnotatedElements(); }},

        // DataType
        { EGM::ID::fromString("YRfW6j4vEoAIs5IZxAhzL&Xrfi4E"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<DataType>().getOwnedAttributes(); }},

        // Dependency
        { EGM::ID::fromString("NK0caSsNdziSo64RKhGUuv7Ev&Bn"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Dependency>().getClients();}},
        { EGM::ID::fromString("lcCuXfAev7UJtHr7eQDbYh7ldN7K"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Dependency>().getSuppliers();}},

        // DirectedRelationship
        { EGM::ID::fromString("_FHdhz9m77&6D0ezEAIz1wTHosuY"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<DirectedRelationship>().getSources(); }},
        { EGM::ID::fromString("y3jO9&br6vLo7t50PMinNTMu4wHv"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<DirectedRelationship>().getTargets(); }},

        // Element
        { EGM::ID::fromString("d8boa7uurAj2JNoQMbCe69k89d1M"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->getOwnedComments(); }},
        { EGM::ID::fromString("XtmmRpH294S3m9x9Bp8FT63LuCMl"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->getOwnedElements(); }},
        { EGM::ID::fromString("XIKAlIqrt4NOzZBgAKzoXuLIH2DS"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->getOwnerSingleton(); }},

        // Enumeration
        { EGM::ID::fromString("0bbShzyDGBoiPzK0yL7mlNf74vTb"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Enumeration>().getOwnedLiterals(); }},

        // EnumerationLiteral
        { EGM::ID::fromString("VL8ZmpG7YGf8VqW_H44oqD_R5Zll"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<EnumerationLiteral>().getEnumerationSingleton(); }},

        // Extension
        { EGM::ID::fromString("igEp5i15i4BoE&2swNhF6w84BF&I"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Extension>().getOwnedEndSingleton(); }},

        // Feature
        { EGM::ID::fromString("YrJ4n1R3EqvXEESg4y6KMeakK2EL"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Feature>().getFeaturingClassifierSingleton(); }},

        // Generalization
        { EGM::ID::fromString("wEgOiXpA9Tw&JquWlj4qO3&cWot9"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Generalization>().getGeneralSingleton(); }},
        { EGM::ID::fromString("jT&SRPuPxWJ0bJic11xKhaM87ghd"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Generalization>().getSpecificSingleton(); }},

        // InstanceSpecification
        { EGM::ID::fromString("YpZ9&oJJ6ovW5&6If5lTmik&RQQj"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<InstanceSpecification>().getClassifiers(); }},
        { EGM::ID::fromString("PKeMhq71InnxiqcJgmoWNe0Kl&1N"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<InstanceSpecification>().getSlots(); }},
        { EGM::ID::fromString("bRYJdS3KkJwyt9oSd_NV4FFhIfKE"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<InstanceSpecification>().getSpecificationSingleton(); }},

        // InstanceValue
        { EGM::ID::fromString("PS7IdeknPVzfxqJ8OTgQzohKd7Rl"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<InstanceValue>().getInstanceSingleton(); }},

        // MultiplicityElement
        { EGM::ID::fromString("Iwnqqcp1OKg3dYryp9Ckoagw_44c"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<MultiplicityElement>().getLowerValueSingleton(); }},
        { EGM::ID::fromString("Cfl8pdxVKDi27IekVEmJ6gyRLmXD"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<MultiplicityElement>().getUpperValueSingleton(); }},

        // NamedElement
        { EGM::ID::fromString("XjBtcuD1gdltfGHP3NqzlXg61zbh"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<NamedElement>().getNamespaceSingleton();}},
        { EGM::ID::fromString("KDgwtlUDTBZuGIAAed8D5blGk98f"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<NamedElement>().getClientDependencies();}},

        // Namespace
        { EGM::ID::fromString("ypBZodwaaRcE7gld4WuSbrsav39p"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Namespace>().getMembers(); }},
        { EGM::ID::fromString("RFhcUcft&ayqEN8IOPI1ftyO5gsx"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Namespace>().getOwnedMembers(); }},

        // Package
        { EGM::ID::fromString("f3emF_P7lUQCj92wICwZqIVIcw9E"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Package>().getOwnedStereotypes(); }},
        { EGM::ID::fromString("F628ncQKADxo6FLtAlDOdlJfewLy"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Package>().getPackagedElements(); }},

        // PackageableElement
        { EGM::ID::fromString("PL0LXBVcYZKI2s3zYLO4d_2R9_tV"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<PackageableElement>().getOwningPackageSingleton(); }},

        // Property
        { EGM::ID::fromString("gCWcKwE5Kw2AKY5znPDleBTp_Nid"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Property>().getAssociationSingleton(); }},
        { EGM::ID::fromString("1p269YwWkH9HxehdVjQ9NBL_YVvR"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Property>().getClassSingleton(); }},
        { EGM::ID::fromString("1QgH6QgvtOGBrDU6WQy&UY9sQZ_4"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Property>().getDataTypeSingleton(); }},
        { EGM::ID::fromString("pQ_GGmgHjaCJJWVEUPGaqJm938m9"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Property>().getDefaultValueSingleton(); }},
        { EGM::ID::fromString("2WwfKSv8p8cxwJNjCsUReaBSVvJJ"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Property>().getOwningAssociationSingleton(); }},
        { EGM::ID::fromString("wD9hC3sTeNAcNyGh3RnW7SWIImL4"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Property>().getRedefinedProperties(); }},
        { EGM::ID::fromString("cWJ3pzQMLpI56xhvmb33j&&1V_39"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Property>().getSubsettedProperties(); }},

        // RedefinableElement
        { EGM::ID::fromString("Z&YvjYeWesI&xJ_j5vHwiAtvZD_f"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<RedefinableElement>().getRedefinedElements();} },
        { EGM::ID::fromString("kcSfJsip&b8_Eg2UWxLwb0MaR4sP"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<RedefinableElement>().getRedefinitionContext();}},
        
        // Relationship
        { EGM::ID::fromString("QUowokvKRwjZYrwmci655pJh1OeW"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Relationship>().getRelatedElements(); }},
       
        // Slot 
        { EGM::ID::fromString("4Z6EqaCMszp8Jm3pfJdLvLGpQaCY"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Slot>().getDefiningFeatureSingleton(); }},
        { EGM::ID::fromString("M_u0KJZqq2ol7Qo4ewruqV2B_vcp"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Slot>().getOwningInstanceSingleton(); }},
        { EGM::ID::fromString("jT301V4UEMD76BrZ7HPK9r3SaWhI"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Slot>().getValues(); }},

        // Stereotype
        // { EGM::ID::fromString("2bqeQwbxvqiPN5EguSWTmLhFHc8s"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<Stereotype>().getProfileSingleton(); }},

        // StructuredClassifier
        { EGM::ID::fromString("WrQFN_WEBkkbwBshG76_qGRsh0Lm"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<StructuredClassifier>().getOwnedAttributes(); }},
        { EGM::ID::fromString("kD2EJ5rI0ejL7W1npwzC373Ws1ys"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<StructuredClassifier>().getParts(); }},
        { EGM::ID::fromString("k6BecMare3f49MVVcdCxtjU&EkN1"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<StructuredClassifier>().getRoles(); }},

        // TypedElement
        { EGM::ID::fromString("Uxf3ayap8ZKqATyFZDo2ImJqDW1F"), [](UmlManager::Pointer<Element> el) -> EGM::AbstractSet& { return el->as<TypedElement>().getTypeSingleton(); }}
    };
}

