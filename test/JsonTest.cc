#include "json.h"

#include <cppunit/extensions/HelperMacros.h>

#include "RecoverableException.h"
#include "util.h"
#include "array_fun.h"
#include "Base64.h"

namespace aria2 {

class JsonTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JsonTest);
  CPPUNIT_TEST(testDecode);
  CPPUNIT_TEST(testDecode_error);
  CPPUNIT_TEST(testEncode);
  CPPUNIT_TEST(testDecodeGetParams);
  CPPUNIT_TEST_SUITE_END();
private:

public:
  void testDecode();
  void testDecode_error();
  void testEncode();
  void testDecodeGetParams();
};

CPPUNIT_TEST_SUITE_REGISTRATION( JsonTest );

void JsonTest::testDecode()
{
  {
    // empty object
    SharedHandle<ValueBase> r = json::decode("{}");
    const Dict* dict = asDict(r);
    CPPUNIT_ASSERT(dict);
  }
  {
    // empty object
    SharedHandle<ValueBase> r = json::decode("{  }");
    const Dict* dict = asDict(r);
    CPPUNIT_ASSERT(dict);
  }
  {
    // empty array
    SharedHandle<ValueBase> r = json::decode("[]");
    const List* list = asList(r);
    CPPUNIT_ASSERT(list);
  }
  {
    // empty array
    SharedHandle<ValueBase> r = json::decode("[ ]");
    const List* list = asList(r);
    CPPUNIT_ASSERT(list);
  }
  {
    // empty string
    SharedHandle<ValueBase> r = json::decode("[\"\"]");
    const List* list = asList(r);
    CPPUNIT_ASSERT(list);
    const String* s = asString(list->get(0));
    CPPUNIT_ASSERT_EQUAL(std::string(), s->s());
  }
  {
    // string
    SharedHandle<ValueBase> r = json::decode("[\"foobar\"]");
    const List* list = asList(r);
    CPPUNIT_ASSERT(list);
    const String* s = asString(list->get(0));
    CPPUNIT_ASSERT_EQUAL(std::string("foobar"), s->s());
  }
  {
    // string with escape
    SharedHandle<ValueBase> r = json::decode("[\"\\\\foo\\\"\\\"bar\"]");
    const List* list = asList(r);
    CPPUNIT_ASSERT(list);
    const String* s = asString(list->get(0));
    CPPUNIT_ASSERT_EQUAL(std::string("\\foo\"\"bar"), s->s());
  }
  {
    // string with escape
    SharedHandle<ValueBase> r = json::decode("[\"foo\\\"\"]");
    const List* list = asList(r);
    CPPUNIT_ASSERT(list);
    const String* s = asString(list->get(0));
    CPPUNIT_ASSERT_EQUAL(std::string("foo\""), s->s());
  }
  {
    // string: utf-8 1 to 3 bytes.
    SharedHandle<ValueBase> r = json::decode("[\"\\u0024\\u00A2\\u20AC\"]");
    const List* list = asList(r);
    CPPUNIT_ASSERT(list);
    const String* s = asString(list->get(0));
    CPPUNIT_ASSERT_EQUAL(std::string("$¢€"), s->s());
  }
  {
    // string: utf-8 4 bytes
    SharedHandle<ValueBase> r = json::decode("[\"\\uD852\\uDF62\"]");
    const List* list = asList(r);
    CPPUNIT_ASSERT(list);
    const String* s = asString(list->get(0));
    const char arr[] = { 0xF0u, 0xA4u, 0xADu, 0xA2u };
    CPPUNIT_ASSERT_EQUAL(std::string(vbegin(arr), vend(arr)), s->s());
  }
  {
    // null
    SharedHandle<ValueBase> r = json::decode("[null]");
    const List* list = asList(r);
    CPPUNIT_ASSERT(list);
    const Null* s = asNull(list->get(0));
    CPPUNIT_ASSERT(s);
  }
  {
    // true, false
    SharedHandle<ValueBase> r = json::decode("[true, false]");
    const List* list = asList(r);
    CPPUNIT_ASSERT(list);
    const Bool* trueValue = asBool(list->get(0));
    CPPUNIT_ASSERT(trueValue);
    CPPUNIT_ASSERT(trueValue->val());
    const Bool* falseValue = asBool(list->get(1));
    CPPUNIT_ASSERT(falseValue);
    CPPUNIT_ASSERT(!falseValue->val());
  }
  {
    // object: 1 member
    SharedHandle<ValueBase> r = json::decode("{\"foo\":[\"bar\"]}");
    const Dict* dict = asDict(r);
    CPPUNIT_ASSERT(dict);
    const List* list = asList(dict->get("foo"));
    CPPUNIT_ASSERT(list);
    const String* s = asString(list->get(0));
    CPPUNIT_ASSERT_EQUAL(std::string("bar"), s->s());
  }
  {
    // object: 2 members
    SharedHandle<ValueBase> r = json::decode("{\"\":[\"bar\"], "
                                             "\"alpha\" : \"bravo\"}");
    const Dict* dict = asDict(r);
    CPPUNIT_ASSERT(dict);
    const List* list = asList(dict->get(""));
    CPPUNIT_ASSERT(list);
    const String* s = asString(list->get(0));
    CPPUNIT_ASSERT_EQUAL(std::string("bar"), s->s());
    const String* str = asString(dict->get("alpha"));
    CPPUNIT_ASSERT_EQUAL(std::string("bravo"), str->s());
  }
  {
    // array: 2 values
    SharedHandle<ValueBase> r = json::decode("[\"foo\", {}]");
    const List* list = asList(r);
    CPPUNIT_ASSERT(list);
    const String* s = asString(list->get(0));
    CPPUNIT_ASSERT_EQUAL(std::string("foo"), s->s());
    const Dict* dict = asDict(list->get(1));
    CPPUNIT_ASSERT(dict);
  }
  {
    // Number: currently we handle floating point number as string
    SharedHandle<ValueBase> r = json::decode("[0,-1,1.2,-1.2e-10,-1e10]");
    const List* list = asList(r);
    CPPUNIT_ASSERT(list);
    const Integer* i = asInteger(list->get(0));
    CPPUNIT_ASSERT_EQUAL((Integer::ValueType)0, i->i());
    const Integer* i1 = asInteger(list->get(1));
    CPPUNIT_ASSERT_EQUAL((Integer::ValueType)-1, i1->i());
    const String* s2 = asString(list->get(2));
    CPPUNIT_ASSERT_EQUAL(std::string("1.2"), s2->s());
    const String* s3 = asString(list->get(3));
    CPPUNIT_ASSERT_EQUAL(std::string("-1.2e-10"), s3->s());
    const String* s4 = asString(list->get(4));
    CPPUNIT_ASSERT_EQUAL(std::string("-1e10"), s4->s());
  }
}

void JsonTest::testDecode_error()
{
  {
    try {
      // object
      SharedHandle<ValueBase> r = json::decode("{");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // object
      SharedHandle<ValueBase> r = json::decode("}");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // object
      SharedHandle<ValueBase> r = json::decode("{\"\":");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // object
      SharedHandle<ValueBase> r = json::decode("{\"\":\"\",");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // array
      SharedHandle<ValueBase> r = json::decode("[");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // array
      SharedHandle<ValueBase> r = json::decode("]");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // array
      SharedHandle<ValueBase> r = json::decode("[\"\"");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // array
      SharedHandle<ValueBase> r = json::decode("[\"\",");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // string
      SharedHandle<ValueBase> r = json::decode("[\"foo]");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // string
      SharedHandle<ValueBase> r = json::decode("[\"\\u\"]");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // string
      SharedHandle<ValueBase> r = json::decode("[\"\\u");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // string
      SharedHandle<ValueBase> r = json::decode("[\"\\u000\"]");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // string
      SharedHandle<ValueBase> r = json::decode("[\"\\u000");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // string
      SharedHandle<ValueBase> r = json::decode("[\"\\uD852foo\"]");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // string
      SharedHandle<ValueBase> r = json::decode("[\"\\uD852");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // string
      SharedHandle<ValueBase> r = json::decode("[\"\\uD852\\u\"]");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // string
      SharedHandle<ValueBase> r = json::decode("[\"\\uD852\\u");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // string
      SharedHandle<ValueBase> r = json::decode("[\"\\uD852\\u0000\"]");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // string
      SharedHandle<ValueBase> r = json::decode("[\"\\uD852\\uDF62");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // object
      SharedHandle<ValueBase> r = json::decode("{:\"\"}");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // object
      SharedHandle<ValueBase> r = json::decode("{\"foo\":}");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // number
      SharedHandle<ValueBase> r = json::decode("{00}");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // number
      SharedHandle<ValueBase> r = json::decode("{1.}");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // number
      SharedHandle<ValueBase> r = json::decode("{1.1e}");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
  {
    try {
      // bool
      SharedHandle<ValueBase> r = json::decode("{t");
      CPPUNIT_FAIL("exception must be thrown.");
    } catch(RecoverableException& e) {
      // success
    }
  }
}

void JsonTest::testEncode()
{
  {
    Dict dict;
    dict["name"] = String::g("aria2");
    dict["loc"] = Integer::g(80000);
    SharedHandle<List> files = List::g();
    files->append(String::g("aria2c"));
    dict["files"] = files;
    SharedHandle<Dict> attrs = Dict::g();
    attrs->put("license", String::g("GPL"));
    dict["attrs"] = attrs;

    CPPUNIT_ASSERT_EQUAL(std::string("{\"attrs\":{\"license\":\"GPL\"},"
                                     "\"files\":[\"aria2c\"],"
                                     "\"loc\":80000,"
                                     "\"name\":\"aria2\"}"),
                         json::encode(&dict));
  }
  {
    List list;
    list.append("\"\\/\b\f\n\r\t");
    CPPUNIT_ASSERT_EQUAL(std::string("[\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"]"),
                         json::encode(&list));
  }
  {
    List list;
    std::string s;
    s += 0x1Fu;
    list.append(s);
    CPPUNIT_ASSERT_EQUAL(std::string("[\"\\u001F\"]"),
                         json::encode(&list));
  }
  {
    List list;
    list.append(Bool::gTrue());
    list.append(Bool::gFalse());
    list.append(Null::g());
    CPPUNIT_ASSERT_EQUAL(std::string("[true,false,null]"),
                         json::encode(&list));
  }
}

void JsonTest::testDecodeGetParams()
{
  {
    std::string param = util::percentEncode(Base64::encode("[1,2,3]"));
    std::string query = "?params=";
    query += param;
    query += '&';
    query += "method=sum&";
    query += "id=300&";
    query += "jsoncallback=cb";
    json::JsonGetParam gparam = json::decodeGetParams(query);
    CPPUNIT_ASSERT_EQUAL(std::string("{\"method\":\"sum\","
                                     "\"id\":\"300\","
                                     "\"params\":[1,2,3]}"),
                         gparam.request);
    CPPUNIT_ASSERT_EQUAL(std::string("cb"), gparam.callback);
  }
  {
    std::string query = "?params=";
    query += util::percentEncode(Base64::encode("[{}]"));
    query += '&';
    query += "jsoncallback=cb";
    json::JsonGetParam gparam = json::decodeGetParams(query);
    CPPUNIT_ASSERT_EQUAL(std::string("[{}]"), gparam.request);
    CPPUNIT_ASSERT_EQUAL(std::string("cb"), gparam.callback);
  }
  {
    std::string query = "?method=sum&id=300";
    json::JsonGetParam gparam = json::decodeGetParams(query);
    CPPUNIT_ASSERT_EQUAL(std::string("{\"method\":\"sum\","
                                     "\"id\":\"300\"}"),
                         gparam.request);
    CPPUNIT_ASSERT_EQUAL(std::string(), gparam.callback);
  }
}

} // namespace aria2
