#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <regex>

#include "oi/SymbolService.h"
// TODO needed?:
#include "oi/ContainerInfo.h"
#include "oi/OIParser.h"
#include "oi/type_graph/DrgnParser.h"
#include "oi/type_graph/Printer.h"
#include "oi/type_graph/TypeGraph.h"
#include "oi/type_graph/Types.h"

using namespace type_graph;
using ::testing::HasSubstr;

// TODO setup google logging for tests so it doesn't appear on terminal by
// default

class DrgnParserTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    symbols_ = new SymbolService{TARGET_EXE_PATH};
  }

  static void TearDownTestSuite() {
    delete symbols_;
  }

  std::string run(std::string_view function, bool chaseRawPointers);
  void test(std::string_view function,
            std::string_view expected,
            bool chaseRawPointers = true);
  void testContains(std::string_view function,
                    std::string_view expected,
                    bool chaseRawPointers = true);
  void testMultiCompiler(std::string_view function,
                         std::string_view expectedClang,
                         std::string_view expectedGcc,
                         bool chaseRawPointers = true);
  void testMultiCompilerContains(std::string_view function,
                                 std::string_view expectedClang,
                                 std::string_view expectedGcc,
                                 bool chaseRawPointers = true);

  static SymbolService* symbols_;
};

SymbolService* DrgnParserTest::symbols_ = nullptr;

std::string DrgnParserTest::run(std::string_view function,
                                bool chaseRawPointers) {
  irequest req{"entry", std::string{function}, "arg0"};
  auto drgnRoot = symbols_->getRootType(req);

  TypeGraph typeGraph;
  // TODO more container types, with various template parameter options
  ContainerInfo std_vector{"std::vector", SEQ_TYPE, "vector"};
  std_vector.stubTemplateParams = {1};

  std::vector<ContainerInfo> containers;
  containers.emplace_back(std::move(std_vector));

  DrgnParser drgnParser(typeGraph, containers, chaseRawPointers);
  Type& type = drgnParser.parse(drgnRoot->type.type);

  std::stringstream out;
  Printer printer{out, typeGraph.size()};
  printer.print(type);

  return out.str();
}

void DrgnParserTest::test(std::string_view function,
                          std::string_view expected,
                          bool chaseRawPointers) {
  auto actual = run(function, chaseRawPointers);

  expected.remove_prefix(1);  // Remove initial '\n'
  EXPECT_EQ(expected, actual);
}

void DrgnParserTest::testContains(std::string_view function,
                                  std::string_view expected,
                                  bool chaseRawPointers) {
  auto actual = run(function, chaseRawPointers);

  expected.remove_prefix(1);  // Remove initial '\n'
  EXPECT_THAT(actual, HasSubstr(expected));
}

void DrgnParserTest::testMultiCompiler(
    std::string_view function,
    [[maybe_unused]] std::string_view expectedClang,
    [[maybe_unused]] std::string_view expectedGcc,
    bool chaseRawPointers) {
#if defined(__clang__)
  test(function, expectedClang, chaseRawPointers);
#else
  test(function, expectedGcc, chaseRawPointers);
#endif
}

void DrgnParserTest::testMultiCompilerContains(
    std::string_view function,
    [[maybe_unused]] std::string_view expectedClang,
    [[maybe_unused]] std::string_view expectedGcc,
    bool chaseRawPointers) {
#if defined(__clang__)
  testContains(function, expectedClang, chaseRawPointers);
#else
  testContains(function, expectedGcc, chaseRawPointers);
#endif
}

TEST_F(DrgnParserTest, SimpleStruct) {
  test("oid_test_case_simple_struct", R"(
[0] Pointer
[1]   Struct: SimpleStruct (size: 16)
        Member: a (offset: 0)
          Primitive: int32_t
        Member: b (offset: 4)
          Primitive: int8_t
        Member: c (offset: 8)
          Primitive: int64_t
)");
}

TEST_F(DrgnParserTest, SimpleClass) {
  test("oid_test_case_simple_class", R"(
[0] Pointer
[1]   Class: SimpleClass (size: 16)
        Member: a (offset: 0)
          Primitive: int32_t
        Member: b (offset: 4)
          Primitive: int8_t
        Member: c (offset: 8)
          Primitive: int64_t
)");
}

TEST_F(DrgnParserTest, SimpleUnion) {
  test("oid_test_case_simple_union", R"(
[0] Pointer
[1]   Union: SimpleUnion (size: 8)
        Member: a (offset: 0)
          Primitive: int32_t
        Member: b (offset: 0)
          Primitive: int8_t
        Member: c (offset: 0)
          Primitive: int64_t
)");
}

TEST_F(DrgnParserTest, Inheritance) {
  test("oid_test_case_inheritance_access_public", R"(
[0] Pointer
[1]   Class: Public (size: 8)
        Parent (offset: 0)
[2]       Class: Base (size: 4)
            Member: base_int (offset: 0)
              Primitive: int32_t
        Member: public_int (offset: 4)
          Primitive: int32_t
)");
}

TEST_F(DrgnParserTest, InheritanceMultiple) {
  test("oid_test_case_inheritance_multiple_a", R"(
[0] Pointer
[1]   Struct: Derived_2 (size: 24)
        Parent (offset: 0)
[2]       Struct: Base_1 (size: 4)
            Member: a (offset: 0)
              Primitive: int32_t
        Parent (offset: 4)
[3]       Struct: Derived_1 (size: 12)
            Parent (offset: 0)
[4]           Struct: Base_2 (size: 4)
                Member: b (offset: 0)
                  Primitive: int32_t
            Parent (offset: 4)
[5]           Struct: Base_3 (size: 4)
                Member: c (offset: 0)
                  Primitive: int32_t
            Member: d (offset: 8)
              Primitive: int32_t
        Parent (offset: 16)
[6]       Struct: Base_4 (size: 4)
            Member: e (offset: 0)
              Primitive: int32_t
        Member: f (offset: 20)
          Primitive: int32_t
)");
}

TEST_F(DrgnParserTest, Container) {
  testMultiCompiler("oid_test_case_std_vector_int_empty", R"(
[0] Pointer
[1]   Container: std::vector (size: 24)
        Param
          Primitive: int32_t
        Param
[2]       Class: allocator<int> (size: 1)
            Param
              Primitive: int32_t
            Parent (offset: 0)
[3]           Typedef: __allocator_base<int>
[4]             Class: new_allocator<int> (size: 1)
                  Param
                    Primitive: int32_t
                  Function: new_allocator
                  Function: new_allocator
                  Function: allocate
                  Function: deallocate
                  Function: _M_max_size
            Function: allocator
            Function: allocator
            Function: operator=
            Function: ~allocator
            Function: allocate
            Function: deallocate
)",
                    R"(
[0] Pointer
[1]   Container: std::vector (size: 24)
        Param
          Primitive: int32_t
        Param
[2]       Class: allocator<int> (size: 1)
            Parent (offset: 0)
[3]           Class: new_allocator<int> (size: 1)
                Param
                  Primitive: int32_t
                Function: new_allocator
                Function: new_allocator
                Function: allocate
                Function: deallocate
                Function: _M_max_size
            Function: allocator
            Function: allocator
            Function: operator=
            Function: ~allocator
            Function: allocate
            Function: deallocate
)");
}
// TODO test vector with custom allocator

TEST_F(DrgnParserTest, Enum) {
  test("oid_test_case_enums_scoped", R"(
    Enum: ScopedEnum (size: 4)
)");
}

TEST_F(DrgnParserTest, EnumInt8) {
  test("oid_test_case_enums_scoped_int8", R"(
    Enum: ScopedEnumInt8 (size: 1)
)");
}

TEST_F(DrgnParserTest, UnscopedEnum) {
  test("oid_test_case_enums_unscoped", R"(
    Enum: UNSCOPED_ENUM (size: 4)
)");
}

TEST_F(DrgnParserTest, Typedef) {
  test("oid_test_case_typedefs_c_style", R"(
[0] Typedef: TdUInt64
[1]   Typedef: uint64_t
[2]     Typedef: __uint64_t
          Primitive: uint64_t
)");
}

TEST_F(DrgnParserTest, Using) {
  test("oid_test_case_typedefs_using", R"(
[0] Typedef: UsingUInt64
[1]   Typedef: uint64_t
[2]     Typedef: __uint64_t
          Primitive: uint64_t
)");
}

TEST_F(DrgnParserTest, ArrayMember) {
  test("oid_test_case_arrays_member_int10", R"(
[0] Pointer
[1]   Struct: Foo10 (size: 40)
        Member: arr (offset: 0)
[2]       Array: (length: 10)
            Primitive: int32_t
)");
}

TEST_F(DrgnParserTest, ArrayRef) {
  test("oid_test_case_arrays_ref_int10", R"(
[0] Pointer
[1]   Array: (length: 10)
        Primitive: int32_t
)");
}

TEST_F(DrgnParserTest, ArrayDirect) {
  test("oid_test_case_arrays_direct_int10", R"(
[0] Pointer
      Primitive: int32_t
)");
}

TEST_F(DrgnParserTest, Pointer) {
  test("oid_test_case_pointers_struct_primitive_ptrs", R"(
[0] Pointer
[1]   Struct: PrimitivePtrs (size: 24)
        Member: a (offset: 0)
          Primitive: int32_t
        Member: b (offset: 8)
[2]       Pointer
            Primitive: int32_t
        Member: c (offset: 16)
[3]       Pointer
            Primitive: void
)");
}

TEST_F(DrgnParserTest, PointerNoFollow) {
  test("oid_test_case_pointers_struct_primitive_ptrs", R"(
[0] Pointer
[1]   Struct: PrimitivePtrs (size: 24)
        Member: a (offset: 0)
          Primitive: int32_t
        Member: b (offset: 8)
          Primitive: uintptr_t
        Member: c (offset: 16)
          Primitive: uintptr_t
)",
       false);
}

TEST_F(DrgnParserTest, PointerIncomplete) {
  test("oid_test_case_pointers_incomplete_raw", R"(
[0] Pointer
      Primitive: void
)");
}

TEST_F(DrgnParserTest, Cycle) {
  test("oid_test_case_cycles_raw_ptr", R"(
[0] Pointer
[1]   Struct: RawNode (size: 16)
        Member: value (offset: 0)
          Primitive: int32_t
        Member: next (offset: 8)
[2]       Pointer
            [1]
)");
}

TEST_F(DrgnParserTest, ClassTemplateInt) {
  test("oid_test_case_templates_int", R"(
[0] Pointer
[1]   Class: TemplatedClass1<int> (size: 4)
        Param
          Primitive: int32_t
        Member: val (offset: 0)
          Primitive: int32_t
)");
}

TEST_F(DrgnParserTest, ClassTemplateVector) {
  testMultiCompiler("oid_test_case_templates_vector", R"(
[0] Pointer
[1]   Class: TemplatedClass1<std::vector<int, std::allocator<int> > > (size: 24)
        Param
[2]       Container: std::vector (size: 24)
            Param
              Primitive: int32_t
            Param
[3]           Class: allocator<int> (size: 1)
                Param
                  Primitive: int32_t
                Parent (offset: 0)
[4]               Typedef: __allocator_base<int>
[5]                 Class: new_allocator<int> (size: 1)
                      Param
                        Primitive: int32_t
                      Function: new_allocator
                      Function: new_allocator
                      Function: allocate
                      Function: deallocate
                      Function: _M_max_size
                Function: allocator
                Function: allocator
                Function: operator=
                Function: ~allocator
                Function: allocate
                Function: deallocate
        Member: val (offset: 0)
          [2]
        Function: ~TemplatedClass1
        Function: TemplatedClass1
)",
                    R"(
[0] Pointer
[1]   Class: TemplatedClass1<std::vector<int, std::allocator<int> > > (size: 24)
        Param
[2]       Container: std::vector (size: 24)
            Param
              Primitive: int32_t
            Param
[3]           Class: allocator<int> (size: 1)
                Parent (offset: 0)
[4]               Class: new_allocator<int> (size: 1)
                    Param
                      Primitive: int32_t
                    Function: new_allocator
                    Function: new_allocator
                    Function: allocate
                    Function: deallocate
                    Function: _M_max_size
                Function: allocator
                Function: allocator
                Function: operator=
                Function: ~allocator
                Function: allocate
                Function: deallocate
        Member: val (offset: 0)
          [2]
        Function: TemplatedClass1
        Function: ~TemplatedClass1
)");
}

TEST_F(DrgnParserTest, ClassTemplateTwo) {
  test("oid_test_case_templates_two", R"(
[0] Pointer
[1]   Class: TemplatedClass2<ns_templates::Foo, int> (size: 12)
        Param
[2]       Struct: Foo (size: 8)
            Member: a (offset: 0)
              Primitive: int32_t
            Member: b (offset: 4)
              Primitive: int32_t
        Param
          Primitive: int32_t
        Member: tc1 (offset: 0)
[3]       Class: TemplatedClass1<ns_templates::Foo> (size: 8)
            Param
              [2]
            Member: val (offset: 0)
              [2]
        Member: val2 (offset: 8)
          Primitive: int32_t
)");
}

TEST_F(DrgnParserTest, ClassTemplateValue) {
  test("oid_test_case_templates_value", R"(
[0] Pointer
[1]   Struct: TemplatedClassVal<3> (size: 12)
        Param
          Value: 3
        Member: arr (offset: 0)
[2]       Array: (length: 3)
            Primitive: int32_t
)");
}

TEST_F(DrgnParserTest, TemplateEnumValue) {
  testMultiCompilerContains("oid_test_case_enums_params_scoped_enum_val",
                            R"(
[0] Pointer
[1]   Class: MyClass<ns_enums_params::MyNS::ScopedEnum::One> (size: 4)
        Param
          Value: ns_enums_params::MyNS::ScopedEnum::One
)",
                            R"(
[0] Pointer
[1]   Class: MyClass<(ns_enums_params::MyNS::ScopedEnum)1> (size: 4)
        Param
          Value: ns_enums_params::MyNS::ScopedEnum::One
)");
}

TEST_F(DrgnParserTest, TemplateEnumValueGaps) {
  testMultiCompilerContains("oid_test_case_enums_params_scoped_enum_val_gaps",
                            R"(
[0] Pointer
[1]   Class: ClassGaps<ns_enums_params::MyNS::EnumWithGaps::Twenty> (size: 4)
        Param
          Value: ns_enums_params::MyNS::EnumWithGaps::Twenty
)",
                            R"(
[0] Pointer
[1]   Class: ClassGaps<(ns_enums_params::MyNS::EnumWithGaps)20> (size: 4)
        Param
          Value: ns_enums_params::MyNS::EnumWithGaps::Twenty
)");
}

TEST_F(DrgnParserTest, TemplateEnumValueNegative) {
  testMultiCompilerContains(
      "oid_test_case_enums_params_scoped_enum_val_negative", R"(
[0] Pointer
[1]   Class: ClassGaps<ns_enums_params::MyNS::EnumWithGaps::MinusTwo> (size: 4)
        Param
          Value: ns_enums_params::MyNS::EnumWithGaps::MinusTwo
)",
      R"(
[0] Pointer
[1]   Class: ClassGaps<(ns_enums_params::MyNS::EnumWithGaps)-2> (size: 4)
        Param
          Value: ns_enums_params::MyNS::EnumWithGaps::MinusTwo
)");
}

// TODO
// TEST_F(DrgnParserTest, ClassFunctions) {
//  test("TestClassFunctions", R"(
//[0] Pointer
//[1]   Class: ClassFunctions (size: 4)
//        Member: memberA (offset: 0)
//          Primitive: int32_t
//        Function: foo (virtuality: 0)
//        Function: bar (virtuality: 0)
//)");
//}

TEST_F(DrgnParserTest, StructAlignment) {
  GTEST_SKIP() << "Alignment not reported by drgn yet";
  test("oid_test_case_alignment_struct", R"(
[0] Pointer
[1]   Struct: Align16 (size: 16, align: 16)
        Member: c (offset: 0)
          Primitive: int8_t
)");
}

TEST_F(DrgnParserTest, MemberAlignment) {
  GTEST_SKIP() << "Alignment not reported by drgn yet";
  test("oid_test_case_alignment_member_alignment", R"(
[0] Pointer
[1]   Struct: MemberAlignment (size: 64)
        Member: c (offset: 0)
          Primitive: int8_t
        Member: c32 (offset: 32, align: 32)
          Primitive: int8_t
)");
}

TEST_F(DrgnParserTest, VirtualFunctions) {
  testMultiCompiler("oid_test_case_inheritance_polymorphic_a_as_a", R"(
[0] Pointer
[1]   Class: A (size: 16)
        Member: _vptr$A (offset: 0)
[2]       Pointer
[3]         Pointer
              Primitive: void
        Member: int_a (offset: 8)
          Primitive: int32_t
        Function: ~A (virtual)
        Function: myfunc (virtual)
        Function: A
        Function: A
)",
                    R"(
[0] Pointer
[1]   Class: A (size: 16)
        Member: _vptr.A (offset: 0)
[2]       Pointer
[3]         Pointer
              Primitive: void
        Member: int_a (offset: 8)
          Primitive: int32_t
        Function: operator=
        Function: A
        Function: A
        Function: ~A (virtual)
        Function: myfunc (virtual)
)");
}

TEST_F(DrgnParserTest, BitfieldsSingle) {
  test("oid_test_case_bitfields_single", R"(
[0] Pointer
[1]   Struct: Single (size: 4)
        Member: bitfield (offset: 0, bitsize: 3)
          Primitive: int32_t
)");
}

TEST_F(DrgnParserTest, BitfieldsWithinBytes) {
  test("oid_test_case_bitfields_within_bytes", R"(
[0] Pointer
[1]   Struct: WithinBytes (size: 2)
        Member: a (offset: 0, bitsize: 3)
          Primitive: int8_t
        Member: b (offset: 0.375, bitsize: 5)
          Primitive: int8_t
        Member: c (offset: 1, bitsize: 7)
          Primitive: int8_t
)");
}

TEST_F(DrgnParserTest, BitfieldsStraddleBytes) {
  test("oid_test_case_bitfields_straddle_bytes", R"(
[0] Pointer
[1]   Struct: StraddleBytes (size: 3)
        Member: a (offset: 0, bitsize: 7)
          Primitive: int8_t
        Member: b (offset: 1, bitsize: 7)
          Primitive: int8_t
        Member: c (offset: 2, bitsize: 2)
          Primitive: int8_t
)");
}

TEST_F(DrgnParserTest, BitfieldsMixed) {
  test("oid_test_case_bitfields_mixed", R"(
[0] Pointer
[1]   Struct: Mixed (size: 12)
        Member: a (offset: 0)
          Primitive: int32_t
        Member: b (offset: 4, bitsize: 4)
          Primitive: int8_t
        Member: c (offset: 4.5, bitsize: 12)
          Primitive: int16_t
        Member: d (offset: 6)
          Primitive: int8_t
        Member: e (offset: 8, bitsize: 22)
          Primitive: int32_t
)");
}

TEST_F(DrgnParserTest, BitfieldsMoreBitsThanType) {
  GTEST_SKIP() << "drgn errors out";
  test("oid_test_case_bitfields_more_bits_than_type", R"(
[0] Pointer
[1]   Struct: MoreBitsThanType (size: 4)
        Member: a (offset: 0, bitsize: 8)
          Primitive: int8_t
)");
}

TEST_F(DrgnParserTest, BitfieldsZeroBits) {
  test("oid_test_case_bitfields_zero_bits", R"(
[0] Pointer
[1]   Struct: ZeroBits (size: 2)
        Member: b1 (offset: 0, bitsize: 3)
          Primitive: int8_t
        Member: b2 (offset: 1, bitsize: 2)
          Primitive: int8_t
)");
}

TEST_F(DrgnParserTest, BitfieldsEnum) {
  test("oid_test_case_bitfields_enum", R"(
[0] Pointer
[1]   Struct: Enum (size: 4)
        Member: e (offset: 0, bitsize: 2)
          Enum: MyEnum (size: 4)
        Member: f (offset: 0.25, bitsize: 4)
          Enum: MyEnum (size: 4)
)");
}

// TODO test virtual classes
