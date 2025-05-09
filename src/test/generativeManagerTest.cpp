#include "gtest/gtest.h"
#include "uml-server/generativeManager.h"
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
