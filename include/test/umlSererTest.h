#pragma once

namespace UML {
    template <template <class> class V, template <class> class W, template <class> class U, class S>
    void setIntegrationTestClientServer(S& (U<UmlClient::GenBaseHierarchy<U>>::*acessor)()) { // TODO test this one a lil more thouroghly
        UmlClient m;
        auto u = m.create<W>();
        auto t = m.create<V>();
        auto t2 = m.create<V>();
        ASSERT_NO_THROW(m.get(u.id()));
        ASSERT_NO_THROW(m.get(t.id()));
        ASSERT_NO_THROW(((*u).*acessor)().add(*t));
        u.release();
        ASSERT_FALSE(u.loaded());
        ASSERT_EQ(((*u).*acessor)().front(), t);
        t.release();
        ASSERT_FALSE(t.loaded());
        ASSERT_EQ(((*u).*acessor)().front(), t);
        u.release();
        t.release();
        ASSERT_FALSE(u.loaded());
        ASSERT_FALSE(t.loaded());
        ASSERT_EQ(((*u).*acessor)().front(), t);
        u.release();
        t.release();
        ASSERT_FALSE(u.loaded());
        ASSERT_FALSE(t.loaded());
        ASSERT_EQ(t, ((*u).*acessor)().front());
        ASSERT_NO_THROW(((*u).*acessor)().remove(*t));
        ASSERT_NO_THROW(((*u).*acessor)().add(*t2));
        ASSERT_NO_THROW(((*u).*acessor)().add(*t));
        ASSERT_EQ(((*u).*acessor)().size(), 2);
        ASSERT_TRUE(((*u).*acessor)().contains(*t2));
        ASSERT_TRUE(((*u).*acessor)().contains(*t));
        ASSERT_EQ(((*u).*acessor)().size(), 2);
    }

    template <template <class> class V, template <class> class W, template <class> class U, class S>
    void singletonIntegrationTestClientServer(S& (U<UmlClient::GenBaseHierarchy<U>>::*acessor)()) {
        UmlClient m;
        auto u = m.create<W>();
        auto t = m.create<V>();
        ASSERT_NO_THROW(m.get(u.id()));
        ASSERT_NO_THROW(m.get(t.id()));
        ASSERT_NO_THROW(((*u).*acessor)().set(t));
        ASSERT_EQ(((*u).*acessor)().get(), *t);
        u.release();
        ASSERT_EQ(((*u).*acessor)().get(), *t);
        t.release();
        ASSERT_EQ(((*u).*acessor)().get(), *t);
    }

    #define UML_SERVER_SINGLETON_INTEGRATION_TEST(TEST_NAME, T, U, Type, acessor) \
    class TEST_NAME ## Method : public ::testing::Test { \
        void SetUp() override { \
            std::hash<std::string> hasher; \
            srand(static_cast<unsigned int>(time(0)) ^ hasher(std::string(#TEST_NAME))); \
        }; \
    }; \
    TEST_F(TEST_NAME ## Method, clientServerSingletonIntegrationTest) { \
        ASSERT_NO_FATAL_FAILURE((singletonIntegrationTestClientServer<T,U,Type>(&UmlClient::Implementation<Type>::acessor))); \
    }

    #define UML_SERVER_SET_INTEGRATION_TEST(TEST_NAME, T, U, Type, signature) \
    class TEST_NAME ## Method : public ::testing::Test { \
        void SetUp() override { \
            std::hash<std::string> hasher; \
            srand(static_cast<unsigned int>(time(0)) ^ hasher(std::string(#TEST_NAME))); \
        }; \
    }; \
    TEST_F( TEST_NAME ## Method , clientServerSetIntegrationTest ) { \
        ASSERT_NO_FATAL_FAILURE((setIntegrationTestClientServer<T , U>(&UmlClient::Implementation<Type>::signature)));\
    }
}
