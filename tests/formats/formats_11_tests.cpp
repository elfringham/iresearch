////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2019 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Andrey Abramov
/// @author Vasiliy Nabatchikov
////////////////////////////////////////////////////////////////////////////////

#include "tests_shared.hpp"
#include "formats_test_case_base.hpp"
#include "store/directory_attributes.hpp"

NS_LOCAL

// -----------------------------------------------------------------------------
// --SECTION--                                          format 11 specific tests
// -----------------------------------------------------------------------------

class format_11_test_case : public tests::directory_test_case_base {
};

TEST_P(format_11_test_case, read_zero_block_encryption) {
  tests::json_doc_generator gen(
    resource("simple_sequential.json"),
    &tests::generic_json_field_factory
  );

  tests::document const* doc1 = gen.next();

  // replace encryption
  ASSERT_TRUE(dir().attributes().contains<tests::rot13_encryption>());

  // write segment with format10
  {
    auto codec = irs::formats::get("1_1");
    ASSERT_NE(nullptr, codec);
    auto writer = irs::index_writer::make(dir(), codec, irs::OM_CREATE);
    ASSERT_NE(nullptr, writer);

    ASSERT_TRUE(insert(*writer,
      doc1->indexed.begin(), doc1->indexed.end(),
      doc1->stored.begin(), doc1->stored.end()
    ));

    writer->commit();
  }

  // replace encryption
  ASSERT_TRUE(dir().attributes().remove<tests::rot13_encryption>());
  dir().attributes().emplace<tests::rot13_encryption>(6);

  // can't open encrypted index without encryption
  ASSERT_THROW(irs::directory_reader::open(dir()), irs::index_error);
}

TEST_P(format_11_test_case, write_zero_block_encryption) {
  tests::json_doc_generator gen(
    resource("simple_sequential.json"),
    &tests::generic_json_field_factory
  );

  tests::document const* doc1 = gen.next();

  // replace encryption
  ASSERT_TRUE(dir().attributes().remove<tests::rot13_encryption>());
  dir().attributes().emplace<tests::rot13_encryption>(0);

  // write segment with format10
  auto codec = irs::formats::get("1_1");
  ASSERT_NE(nullptr, codec);
  auto writer = irs::index_writer::make(dir(), codec, irs::OM_CREATE);
  ASSERT_NE(nullptr, writer);

  ASSERT_TRUE(insert(*writer,
    doc1->indexed.begin(), doc1->indexed.end(),
    doc1->stored.begin(), doc1->stored.end()
  ));

  ASSERT_THROW(writer->commit(), irs::index_error);
}

TEST_P(format_11_test_case, open_ecnrypted_with_wrong_encryption) {
  tests::json_doc_generator gen(
    resource("simple_sequential.json"),
    &tests::generic_json_field_factory
  );

  tests::document const* doc1 = gen.next();

  ASSERT_TRUE(dir().attributes().contains<tests::rot13_encryption>());

  // write segment with format10
  {
    auto codec = irs::formats::get("1_1");
    ASSERT_NE(nullptr, codec);
    auto writer = irs::index_writer::make(dir(), codec, irs::OM_CREATE);
    ASSERT_NE(nullptr, writer);

    ASSERT_TRUE(insert(*writer,
      doc1->indexed.begin(), doc1->indexed.end(),
      doc1->stored.begin(), doc1->stored.end()
    ));

    writer->commit();
  }

  // replace encryption
  ASSERT_TRUE(dir().attributes().remove<tests::rot13_encryption>());
  dir().attributes().emplace<tests::rot13_encryption>(6);

  // can't open encrypted index without encryption
  ASSERT_THROW(irs::directory_reader::open(dir()), irs::index_error);
}

TEST_P(format_11_test_case, open_ecnrypted_with_non_encrypted) {
  tests::json_doc_generator gen(
    resource("simple_sequential.json"),
    &tests::generic_json_field_factory
  );

  tests::document const* doc1 = gen.next();

  ASSERT_TRUE(dir().attributes().contains<tests::rot13_encryption>());

  // write segment with format10
  {
    auto codec = irs::formats::get("1_1");
    ASSERT_NE(nullptr, codec);
    auto writer = irs::index_writer::make(dir(), codec, irs::OM_CREATE);
    ASSERT_NE(nullptr, writer);

    ASSERT_TRUE(insert(*writer,
      doc1->indexed.begin(), doc1->indexed.end(),
      doc1->stored.begin(), doc1->stored.end()
    ));

    writer->commit();
  }

  // remove encryption
  dir().attributes().remove<tests::rot13_encryption>();

  // can't open encrypted index without encryption
  ASSERT_THROW(irs::directory_reader::open(dir()), irs::index_error);
}

TEST_P(format_11_test_case, open_non_ecnrypted_with_encrypted) {
  tests::json_doc_generator gen(
    resource("simple_sequential.json"),
    &tests::generic_json_field_factory
  );

  tests::document const* doc1 = gen.next();

  ASSERT_TRUE(dir().attributes().remove<tests::rot13_encryption>());

  // write segment with format10
  {
    auto codec = irs::formats::get("1_1");
    ASSERT_NE(nullptr, codec);
    auto writer = irs::index_writer::make(dir(), codec, irs::OM_CREATE);
    ASSERT_NE(nullptr, writer);

    ASSERT_TRUE(insert(*writer,
      doc1->indexed.begin(), doc1->indexed.end(),
      doc1->stored.begin(), doc1->stored.end()
    ));

    writer->commit();
  }

  // add cipher
  dir().attributes().emplace<tests::rot13_encryption>(7);

  // check index
  auto index = irs::directory_reader::open(dir());
  ASSERT_TRUE(index);
  ASSERT_EQ(1, index->size());
  ASSERT_EQ(1, index->docs_count());
  ASSERT_EQ(1, index->live_docs_count());

  // check segment 0
  {
    auto& segment = index[0];
    ASSERT_EQ(1, segment.size());
    ASSERT_EQ(1, segment.docs_count());
    ASSERT_EQ(1, segment.live_docs_count());

    std::unordered_set<irs::string_ref> expectedName = { "A" };
    const auto* column = segment.column_reader("name");
    ASSERT_NE(nullptr, column);
    auto values = column->values();
    ASSERT_EQ(expectedName.size(), segment.docs_count()); // total count of documents
    auto terms = segment.field("same");
    ASSERT_NE(nullptr, terms);
    auto termItr = terms->iterator();
    ASSERT_TRUE(termItr->next());

    irs::bytes_ref actual_value;
    for (auto docsItr = termItr->postings(iresearch::flags()); docsItr->next();) {
      ASSERT_TRUE(values(docsItr->value(), actual_value));
      ASSERT_EQ(1, expectedName.erase(irs::to_string<irs::string_ref>(actual_value.c_str())));
    }

    ASSERT_TRUE(expectedName.empty());
  }
}

TEST_P(format_11_test_case, open_10_with_11) {
  tests::json_doc_generator gen(
    resource("simple_sequential.json"),
    &tests::generic_json_field_factory
  );

  tests::document const* doc1 = gen.next();

  // write segment with format10
  {
    auto codec = irs::formats::get("1_0");
    ASSERT_NE(nullptr, codec);
    auto writer = irs::index_writer::make(dir(), codec, irs::OM_CREATE);
    ASSERT_NE(nullptr, writer);

    ASSERT_TRUE(insert(*writer,
      doc1->indexed.begin(), doc1->indexed.end(),
      doc1->stored.begin(), doc1->stored.end()
    ));

    writer->commit();
  }

  // check index
  auto codec = irs::formats::get("1_1");
  ASSERT_NE(nullptr, codec);
  auto index = irs::directory_reader::open(dir(), codec);
  ASSERT_TRUE(index);
  ASSERT_EQ(1, index->size());
  ASSERT_EQ(1, index->docs_count());
  ASSERT_EQ(1, index->live_docs_count());

  // check segment 0
  {
    auto& segment = index[0];
    ASSERT_EQ(1, segment.size());
    ASSERT_EQ(1, segment.docs_count());
    ASSERT_EQ(1, segment.live_docs_count());

    std::unordered_set<irs::string_ref> expectedName = { "A" };
    const auto* column = segment.column_reader("name");
    ASSERT_NE(nullptr, column);
    auto values = column->values();
    ASSERT_EQ(expectedName.size(), segment.docs_count()); // total count of documents
    auto terms = segment.field("same");
    ASSERT_NE(nullptr, terms);
    auto termItr = terms->iterator();
    ASSERT_TRUE(termItr->next());

    irs::bytes_ref actual_value;
    for (auto docsItr = termItr->postings(iresearch::flags()); docsItr->next();) {
      ASSERT_TRUE(values(docsItr->value(), actual_value));
      ASSERT_EQ(1, expectedName.erase(irs::to_string<irs::string_ref>(actual_value.c_str())));
    }

    ASSERT_TRUE(expectedName.empty());
  }
}

TEST_P(format_11_test_case, formats_10_11) {
  tests::json_doc_generator gen(
    resource("simple_sequential.json"),
    &tests::generic_json_field_factory
  );

  tests::document const* doc1 = gen.next();
  tests::document const* doc2 = gen.next();

  // write segment with format10
  {
    auto codec = irs::formats::get("1_0");
    ASSERT_NE(nullptr, codec);
    auto writer = irs::index_writer::make(dir(), codec, irs::OM_CREATE);
    ASSERT_NE(nullptr, writer);

    ASSERT_TRUE(insert(*writer,
      doc1->indexed.begin(), doc1->indexed.end(),
      doc1->stored.begin(), doc1->stored.end()
    ));

    writer->commit();
  }

  // write segment with format11
  {
    auto codec = irs::formats::get("1_1");
    ASSERT_NE(nullptr, codec);
    auto writer = irs::index_writer::make(dir(), codec, irs::OM_APPEND);
    ASSERT_NE(nullptr, writer);

    ASSERT_TRUE(insert(*writer,
      doc2->indexed.begin(), doc2->indexed.end(),
      doc2->stored.begin(), doc2->stored.end()
    ));

    writer->commit();
  }

  // check index
  auto index = irs::directory_reader::open(dir());
  ASSERT_TRUE(index);
  ASSERT_EQ(2, index->size());
  ASSERT_EQ(2, index->docs_count());
  ASSERT_EQ(2, index->live_docs_count());

  // check segment 0
  {
    auto& segment = index[0];
    ASSERT_EQ(1, segment.size());
    ASSERT_EQ(1, segment.docs_count());
    ASSERT_EQ(1, segment.live_docs_count());

    std::unordered_set<irs::string_ref> expectedName = { "A" };
    const auto* column = segment.column_reader("name");
    ASSERT_NE(nullptr, column);
    auto values = column->values();
    ASSERT_EQ(expectedName.size(), segment.docs_count()); // total count of documents
    auto terms = segment.field("same");
    ASSERT_NE(nullptr, terms);
    auto termItr = terms->iterator();
    ASSERT_TRUE(termItr->next());

    irs::bytes_ref actual_value;
    for (auto docsItr = termItr->postings(iresearch::flags()); docsItr->next();) {
      ASSERT_TRUE(values(docsItr->value(), actual_value));
      ASSERT_EQ(1, expectedName.erase(irs::to_string<irs::string_ref>(actual_value.c_str())));
    }

    ASSERT_TRUE(expectedName.empty());
  }

  // check segment 1
  {
    auto& segment = index[1];
    ASSERT_EQ(1, segment.size());
    ASSERT_EQ(1, segment.docs_count());
    ASSERT_EQ(1, segment.live_docs_count());

    std::unordered_set<irs::string_ref> expectedName = { "B" };
    const auto* column = segment.column_reader("name");
    ASSERT_NE(nullptr, column);
    auto values = column->values();
    ASSERT_EQ(expectedName.size(), segment.docs_count()); // total count of documents
    auto terms = segment.field("same");
    ASSERT_NE(nullptr, terms);
    auto termItr = terms->iterator();
    ASSERT_TRUE(termItr->next());

    irs::bytes_ref actual_value;
    for (auto docsItr = termItr->postings(iresearch::flags()); docsItr->next();) {
      ASSERT_TRUE(values(docsItr->value(), actual_value));
      ASSERT_EQ(1, expectedName.erase(irs::to_string<irs::string_ref>(actual_value.c_str())));
    }

    ASSERT_TRUE(expectedName.empty());
  }
}

INSTANTIATE_TEST_CASE_P(
  format_11_test,
  format_11_test_case,
  ::testing::Values(
    &tests::rot13_cipher_directory<&tests::memory_directory, 16>,
    &tests::rot13_cipher_directory<&tests::fs_directory, 16>,
    &tests::rot13_cipher_directory<&tests::mmap_directory, 16>
  ),
  tests::directory_test_case_base::to_string
);

// -----------------------------------------------------------------------------
// --SECTION--                                                     generic tests
// -----------------------------------------------------------------------------

using tests::format_test_case;

INSTANTIATE_TEST_CASE_P(
  format_11_test,
  format_test_case,
  ::testing::Combine(
    ::testing::Values(
      &tests::rot13_cipher_directory<&tests::memory_directory, 16>,
      &tests::rot13_cipher_directory<&tests::fs_directory, 16>,
      &tests::rot13_cipher_directory<&tests::mmap_directory, 16>,
      &tests::rot13_cipher_directory<&tests::memory_directory, 7>,
      &tests::rot13_cipher_directory<&tests::fs_directory, 7>,
      &tests::rot13_cipher_directory<&tests::mmap_directory, 7>
//      &tests::rot13_cipher_directory<&tests::memory_directory, 0>,
//      &tests::rot13_cipher_directory<&tests::fs_directory, 0>,
//      &tests::rot13_cipher_directory<&tests::mmap_directory, 0>
    ),
    ::testing::Values("1_1")
  ),
  tests::to_string
);

NS_END