#pragma once

namespace UML {
    template <class V, class W, class T = Element, class U = Element, class S = Set<T,U>>
    void setIntegrationTestClientServer(S& (U::*acessor)()) { // TODO test this one a lil more thouroghly
        UmlClient m;
        UmlPtr<W> u = m.create<W>();
        UmlPtr<V> t = m.create<V>();
        UmlPtr<V> t2 = m.create<V>();
        ASSERT_NO_THROW(m.get(u.id()));
        ASSERT_NO_THROW(m.get(t.id()));
        ASSERT_NO_THROW(((*u).*acessor)().add(*t));
        u.release();
        ASSERT_FALSE(u.loaded());
        ASSERT_EQ(((*u).*acessor)().front(), *t);
        ASSERT_EQ(&((*u).*acessor)().front(), t.ptr());
        t.release();
        ASSERT_FALSE(t.loaded());
        ASSERT_EQ(((*u).*acessor)().front(), *t);
        u.release();
        t.release();
        ASSERT_FALSE(u.loaded());
        ASSERT_FALSE(t.loaded());
        ASSERT_EQ(((*u).*acessor)().front(), *t);
        u.release();
        t.release();
        ASSERT_FALSE(u.loaded());
        ASSERT_FALSE(t.loaded());
        ASSERT_EQ(*t, ((*u).*acessor)().front());
        ASSERT_NO_THROW(((*u).*acessor)().remove(*t));
        ASSERT_NO_THROW(((*u).*acessor)().add(*t2));
        ASSERT_NO_THROW(((*u).*acessor)().add(*t));
        ASSERT_EQ(((*u).*acessor)().size(), 2);
        ASSERT_TRUE(((*u).*acessor)().contains(*t2));
        ASSERT_TRUE(((*u).*acessor)().contains(*t));
        ASSERT_EQ(((*u).*acessor)().size(), 2);
    }

    template <class V, class W, class T = Element, class U = Element>
    void singletonIntegrationTestClientServer(UmlPtr<T> (U::*acessor)() const, void (U::*mutator)(T*)) {
        UmlClient m;
        UmlPtr<W> u = m.create<W>();
        UmlPtr<V> t = m.create<V>();
        ASSERT_NO_THROW(m.get(u.id()));
        ASSERT_NO_THROW(m.get(t.id()));
        ASSERT_NO_THROW(((*u).*mutator)(t.ptr()));
        ASSERT_EQ(((*u).*acessor)(), t);
        u.release();
        ASSERT_EQ(((*u).*acessor)(), t);
        t.release();
        ASSERT_EQ(((*u).*acessor)(), t);
    }

    #define UML_SERVER_SINGLETON_INTEGRATION_TEST(TEST_NAME, T, U, acessor, mutator) \
    class TEST_NAME ## Method : public ::testing::Test { \
        void SetUp() override { \
            std::hash<std::string> hasher; \
            srand(static_cast<unsigned int>(time(0)) ^ hasher(std::string(#TEST_NAME))); \
        }; \
    }; \
    TEST_F(TEST_NAME ## Method, clientServerSingletonIntegrationTest) { \
        ASSERT_NO_FATAL_FAILURE((singletonIntegrationTestClientServer<T,U>(acessor, mutator))); \
    }

    #define UML_SERVER_SET_INTEGRATION_TEST(TEST_NAME, T, U, signature) \
    class TEST_NAME ## Method : public ::testing::Test { \
        void SetUp() override { \
            std::hash<std::string> hasher; \
            srand(static_cast<unsigned int>(time(0)) ^ hasher(std::string(#TEST_NAME))); \
        }; \
    }; \
    TEST_F( TEST_NAME ## Method , clientServerSetIntegrationTest ) { \
        ASSERT_NO_FATAL_FAILURE((setIntegrationTestClientServer<T , U>(signature)));\
    }
}