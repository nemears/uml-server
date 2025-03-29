#include "gtest/gtest.h"
#include "uml-server/metaManager.h"
#include "uml-server/constants.h"

using namespace UML;
using namespace EGM;

using MetaElementPtr = MetaManager::Pointer<MetaElement>;

class MetaManagerTest : public ::testing::Test {};

TEST_F(MetaManagerTest, basicTypeTest) {
    UmlManager m;
    auto pckg = m.create<Package>();
    auto clazz = m.create<Class>();
    pckg->setName("root");
    clazz->setName("test");
    pckg->getPackagedElements().add(clazz);

    MetaManager mm(*pckg);
    MetaManager::Pointer<MetaElement> meta_type = mm.create(0);
    ASSERT_EQ(meta_type->name, "test");
}

TEST_F(MetaManagerTest, basicSetTest) {
    UmlManager m;
    auto root = m.create<Package>();
    auto clazz = m.create<Class>();
    auto type = m.create<Class>();
    auto property = m.create<Property>();
    root->setName("root");
    clazz->setName("test");
    type->setName("type");
    property->setName("prop");
    root->getPackagedElements().add(clazz);
    root->getPackagedElements().add(type);
    property->setType(type);
    clazz->getOwnedAttributes().add(property);
    
    MetaManager mm(*root);
    MetaElementPtr el = mm.create(clazz.id());
    ASSERT_EQ(el->sets.size(), 1);
    ASSERT_EQ(el->sets.count(property.id()), 1);
    ASSERT_EQ(el->sets.at(property.id())->setType(), SetType::SET);
}

TEST_F(MetaManagerTest, dataTypeTest) {
    UmlManager m;
    auto root = m.create<Package>();
    auto clazz = m.create<Class>();
    auto bool_type = m.create<PrimitiveType>();
    auto int_type = m.create<PrimitiveType>();
    auto real_type = m.create<PrimitiveType>();
    auto string_type = m.create<PrimitiveType>();
    auto unlimited_natural_type = m.create<PrimitiveType>();

    bool_type->setID(boolean_type_id);
    int_type->setID(integer_type_id);
    real_type->setID(real_type_id);
    string_type->setID(string_type_id);
    unlimited_natural_type->setID(unlimited_natural_type_id);
    
    bool_type->setName("Boolean");
    int_type->setName("Integer");
    real_type->setName("Real");
    string_type->setName("String");
    unlimited_natural_type->setName("UnlimitedNatural");

    clazz->setName("Class");

    root->getPackagedElements().add(clazz);
    root->getPackagedElements().add(bool_type);
    root->getPackagedElements().add(int_type);
    root->getPackagedElements().add(real_type);
    root->getPackagedElements().add(string_type);
    root->getPackagedElements().add(unlimited_natural_type);

    auto bool_property = m.create<Property>();
    bool_property->setName("bool");
    bool_property->setType(bool_type);
    auto bool_default = m.create<LiteralBoolean>();
    bool_default->setValue(true);
    bool_property->setDefaultValue(bool_default);
    clazz->getOwnedAttributes().add(bool_property);

    auto int_property = m.create<Property>();
    int_property->setName("int");
    int_property->setType(int_type);
    auto int_default = m.create<LiteralInteger>();
    int_default->setValue(420);
    int_property->setDefaultValue(int_default);
    clazz->getOwnedAttributes().add(int_property);

    auto real_property = m.create<Property>();
    real_property->setName("real");
    real_property->setType(real_type);
    auto real_default = m.create<LiteralReal>();
    real_default->setValue(2.71828);
    real_property->setDefaultValue(real_default);
    clazz->getOwnedAttributes().add(real_property);

    auto string_property = m.create<Property>();
    string_property->setName("string");
    string_property->setType(string_type);
    auto string_default = m.create<LiteralString>();
    string_default->setValue("cat");
    string_property->setDefaultValue(string_default);
    clazz->getOwnedAttributes().add(string_property);

    MetaManager meta_manager(*root);
    auto clazz_impl = meta_manager.create(clazz.id());
    ASSERT_TRUE(clazz_impl->data.count(bool_property.id()));
    ASSERT_EQ(clazz_impl->data.at(bool_property.id())->getData(), "true");
    clazz_impl->data.at(bool_property.id())->setData("false");
    ASSERT_EQ(clazz_impl->data.at(bool_property.id())->getData(), "false");
    ASSERT_THROW(clazz_impl->data.at(bool_property.id())->setData("obviously not a bool"), ManagerStateException);

    ASSERT_TRUE(clazz_impl->data.count(int_property.id()));
    ASSERT_EQ(clazz_impl->data.at(int_property.id())->getData(), "420");
    clazz_impl->data.at(int_property.id())->setData("69");
    ASSERT_EQ(clazz_impl->data.at(int_property.id())->getData(), "69");
    ASSERT_THROW(clazz_impl->data.at(int_property.id())->setData("obviously not an int"), ManagerStateException);

    ASSERT_TRUE(clazz_impl->data.count(real_property.id()));
    char* rem = {};
    ASSERT_EQ(std::strtod(clazz_impl->data.at(real_property.id())->getData().c_str(), &rem), 2.71828);
    clazz_impl->data.at(real_property.id())->setData("3.14159");
    ASSERT_EQ(std::strtod(clazz_impl->data.at(real_property.id())->getData().c_str(), &rem), 3.14159);
    // ASSERT_THROW(clazz_impl->data.at(real_property.id())->setData("yeah try making this a double >:)"), ManagerStateException);

    ASSERT_TRUE(clazz_impl->data.count(string_property.id()));
    ASSERT_EQ(clazz_impl->data.at(string_property.id())->getData(), "cat");
    clazz_impl->data.at(string_property.id())->setData("gatita");
    ASSERT_EQ(clazz_impl->data.at(string_property.id())->getData(), "gatita");
}

TEST_F(MetaManagerTest, different_set_types) {
    UmlManager m;
    auto root = m.create<Package>();
    auto clazz = m.create<Class>();
    auto type = m.create<Class>();
    auto set_property = m.create<Property>();
    auto ordered_property = m.create<Property>();
    auto singleton_property = m.create<Property>();
    auto singleton_upper = m.create<LiteralInteger>();

    root->getPackagedElements().add(clazz);
    root->getPackagedElements().add(type);
    root->setName("root");
    clazz->setName("clazz");
    type->setName("type");
    set_property->setName("set");
    ordered_property->setName("ordered");
    singleton_property->setName("singleton");
    set_property->setType(type);
    ordered_property->setType(type);
    singleton_property->setType(type);
    ordered_property->setIsOrdered(true);
    singleton_property->setUpperValue(singleton_upper);
    singleton_upper->setValue(1);
    clazz->getOwnedAttributes().add(set_property);
    clazz->getOwnedAttributes().add(ordered_property);
    clazz->getOwnedAttributes().add(singleton_property);

    MetaManager meta_manager(*root);
    auto meta_element = meta_manager.create(clazz.id());
    ASSERT_EQ(meta_element->sets.size(), 3);
    ASSERT_EQ(meta_element->sets.at(set_property.id())->setType(), SetType::SET);
    ASSERT_EQ(meta_element->sets.at(ordered_property.id())->setType(), SetType::ORDERED_SET);
    ASSERT_EQ(meta_element->sets.at(singleton_property.id())->setType(), SetType::SINGLETON);
}
