#include "gtest/gtest.h"
#include "uml-server/generativeManager.h"
#include "uml-server/constants.h"
#include <format>

using namespace UML;
using namespace EGM;

class GenerativeManagerTest : public ::testing::Test {};

using MetaElementImpl = BasicGenerativeManager::Implementation<MetaElement>;

BasicGenerativeManager::Pointer<Profile> profile;
BasicGenerativeManager::Pointer<Stereotype> stereotype;
BasicGenerativeManager::Pointer<Property> foo_property;
BasicGenerativeManager::Pointer<Class> foo_type;

void setup_foo_bar_profile(BasicGenerativeManager& m) {
    profile = m.create<Profile>();
    stereotype = m.create<Stereotype>();
    foo_property = m.create<Property>();
    foo_type = m.create<Class>();
    profile->setName("profile");
    stereotype->setName("Bar");
    foo_property->setName("foo");
    foo_type->setName("Foo");
    foo_property->setType(foo_type);
    profile->getPackagedElements().add(stereotype);
    stereotype->getOwnedAttributes().add(foo_property);
}

TEST_F(GenerativeManagerTest, basicEmitString) {
    BasicGenerativeManager m;
    m.open(std::format("{}", PROJECT_TEMPLATE));
    auto clazz = m.create<Class>();
    auto string_property = m.create<Property>();
    clazz->setName("foo");
    string_property->setName("blob");
    string_property->setType(m.get(string_type_id));
    clazz->getOwnedAttributes().add(string_property);
    ID meta_manager_id = m.generate(*clazz);
    auto& meta_manager = m.get_meta_manager(meta_manager_id);
    auto foo = meta_manager.create(clazz.id());
    foo->data.at(string_property.id())->setData("fox");
    auto expected_emit = std::format("{{\"foo\":{{\"id\":{},\"blob\":\"fox\"}}}}", foo.id().string());
}

TEST_F(GenerativeManagerTest, basicParseString) {
    BasicGenerativeManager m;
    m.open(std::format("{}", PROJECT_TEMPLATE));
    auto clazz = m.create<Class>();
    auto string_property = m.create<Property>();
    clazz->setName("foo");
    string_property->setName("blob");
    string_property->setType(m.get(string_type_id));
    clazz->getOwnedAttributes().add(string_property);
    ID meta_manager_id = m.generate(*clazz);
    auto& meta_manager = m.get_meta_manager(meta_manager_id); 
    auto parse_data = "{\"foo\":{\"blob\":\"fox\"}}";
    auto parse_data_json = YAML::Load(parse_data);
    auto foo = meta_manager.parse_node(parse_data_json);
    ASSERT_EQ(foo->data.at(string_property.id())->getData(), "fox");
}

TEST_F(GenerativeManagerTest, basicEmitStereotypedElement) {
    BasicGenerativeManager m;
    setup_foo_bar_profile(m);

    ID meta_manager_id = m.generate(*profile);
    auto& meta_manager = m.get_meta_manager(meta_manager_id);
    
    auto stereotyped_element = m.create<Class>();
    stereotyped_element->setName("meow");
    auto stereotype_data = meta_manager.apply(*stereotyped_element, stereotype.id());
    auto foo_inst = meta_manager.create(foo_type.id());
    stereotype_data->getSet(foo_property.id()).add(foo_inst);
    auto stereotyped_element_dump = m.dump_individual(*stereotyped_element);
    auto expected_emit = std::format(
            R"""({{"Class": {{"id": "{}", "appliedStereotypes": [{{"manager": "{}", "data": {{"Bar": {{"id": "{}", "foo": ["{}"]}}}}}}], "name": "meow"}}}})""",
            stereotyped_element.id().string(), 
            meta_manager_id.string(),
            stereotype_data.id().string(), 
            foo_inst.id().string()
    );
    ASSERT_EQ(stereotyped_element_dump, expected_emit);
}

TEST_F(GenerativeManagerTest, basicParseStereotypedElement) {
    BasicGenerativeManager m;
    setup_foo_bar_profile(m);
    ID meta_manager_id = m.generate(*profile);
    auto& meta_manager = m.get_meta_manager(meta_manager_id);
    auto foo_inst = meta_manager.create(foo_type.id());
    auto data_to_parse = std::format(
            R"""({{"Class": {{"id": "9RdVAhZvBM47eAupfq2VlFQ6xrN1", "appliedStereotypes": [{{"manager": "{}", "data": {{"Bar": {{"id": "B2cK0xejNdMUwnC8XcY4HVvnt1c-", "foo": ["{}"]}}}}}}], "name": "meow"}}}})""",
            meta_manager_id.string(),
            foo_inst.id().string()
    ); 
    BasicGenerativeManager::Pointer<Element> parsed_element = m.parse_individual(data_to_parse);
    ASSERT_TRUE(parsed_element->is<Class>());
    ASSERT_EQ(parsed_element->as<NamedElement>().getName(), "meow");
    ASSERT_EQ(parsed_element->getAppliedStereotypes().size(), 1);
    MetaManager::Pointer<MetaElement> stereotype_data = meta_manager.get(parsed_element->getAppliedStereotypes().ids().front());
    ASSERT_EQ(stereotype_data->name, "Bar");
    ASSERT_EQ(stereotype_data->getSet(foo_property.id()).size(), 1);
    ASSERT_EQ(stereotype_data->getSet(foo_property.id()).ids().front(), foo_inst.id());
}

TEST_F(GenerativeManagerTest, subsetUmlProperty) {
    BasicGenerativeManager m;

    // load uml-cafe because it holds most up to date uml profile
    m.open(std::format("{}", PROJECT_TEMPLATE));

    // create profile that subsets packagedElements
    auto profile = m.create<Profile>();
    auto squirell = m.create<Stereotype>();
    auto nut = m.create<Stereotype>();
    auto nut_property = m.create<Property>();
    
    profile->setName("squirell profile");
    squirell->setName("Squirell");
    nut->setName("Nut");
    nut_property->setName("nuts");

    squirell->getOwnedAttributes().add(nut_property);
    nut_property->setType(nut);
    nut_property->getSubsettedProperties().add(m.get(ID::fromString("F628ncQKADxo6FLtAlDOdlJfewLy"))); // subsets packagedElements
    profile->getPackagedElements().add(squirell);
    profile->getPackagedElements().add(nut);

    ID meta_manager_id = m.generate(*profile);
    auto& meta_manager = m.get_meta_manager(meta_manager_id);

    auto squirell_inst = m.create<Package>();
    auto squirell_data = meta_manager.apply(*squirell_inst, squirell.id());
    auto nut_inst = m.create<Package>();
    auto nut_data = meta_manager.apply(*nut_inst, nut.id());
    squirell_data->getSet(nut_property.id()).add(nut_data);

    ASSERT_EQ(squirell_inst->getPackagedElements().size(), 1);
}

TEST_F(GenerativeManagerTest, referenceUmlElement) {
    BasicGenerativeManager m;
    
    m.open(std::format("{}", PROJECT_TEMPLATE));

    // create types
    auto the_type = m.create<Class>();
    auto uml_typed_property = m.create<Property>();

    the_type->setName("TestType");
    uml_typed_property->setType(element_type_id);
    uml_typed_property->setName("umlElement");
    the_type->getOwnedAttributes().add(uml_typed_property);

    ID meta_manager_id = m.generate(*the_type);
    auto& meta_manager = m.get_meta_manager(meta_manager_id);

    auto type_inst = meta_manager.create(the_type.id());
    auto uml_package = m.create<Package>();
    type_inst->getProxySet(uml_typed_property.id()).add(uml_package);
    
    ASSERT_TRUE(type_inst->getProxySet(uml_typed_property.id()).front());
    ASSERT_EQ(type_inst->getProxySet(uml_typed_property.id()).front().id(), uml_package.id());
}
