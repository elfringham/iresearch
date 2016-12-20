//
// IResearch search engine 
// 
// Copyright � 2016 by EMC Corporation, All Rights Reserved
// 
// This software contains the intellectual property of EMC Corporation or is licensed to
// EMC Corporation from third parties. Use of this software and the intellectual property
// contained therein is expressly limited to the terms and conditions of the License
// Agreement under which it is provided by or on behalf of EMC.
// 

#include "tests_shared.hpp"

#include "index/field_meta.hpp"
#include "store/memory_directory.hpp"
#include "store/fs_directory.hpp"
#include "formats/formats_10.hpp"
#include "formats_test_case_base.hpp"
#include "formats/format_utils.hpp"

class format_10_test_case : public tests::format_test_case_base {
 protected:
  ir::format::ptr get_codec() {
    return ir::formats::get("1_0");
  }

  void postings_read_write_single_doc() {
    ir::field_meta field;

    // docs & attributes for term0
    std::vector<ir::doc_id_t> docs0{ 3 };
    ir::attributes attrs0;

    // docs & attributes for term0
    std::vector<ir::doc_id_t> docs1{ 6 };
    ir::attributes attrs1;

    // write postings
    {
      ir::flush_state state;
      state.dir = &dir();
      state.doc_count = 100;
      state.fields_count = 1;
      state.name = "segment_name";
      state.features = &field.features;
      state.ver = IRESEARCH_VERSION;

      auto out = dir().create("attributes");
      ASSERT_FALSE(!out);

      // prepare writer
      ir::version10::postings_writer writer(false);
      writer.prepare(*out, state);

      // begin field
      writer.begin_field(field.features);

      // write postings for term0
      {
        postings docs(docs0.begin(), docs0.end());
        writer.write(docs, attrs0);
        ASSERT_TRUE(attrs0.contains<ir::version10::term_meta>());

        // check term_meta
        {
          auto& meta = *attrs0.get<ir::version10::term_meta>();
          ASSERT_EQ(1, meta.docs_count);
          ASSERT_EQ(2, meta.e_single_doc);
        }

        // write term0 attributes to out
        writer.encode(*out, attrs0);
      }

      // write postings for term0
      {
        postings docs(docs1.begin(), docs1.end());
        writer.write(docs, attrs1);
        ASSERT_TRUE(attrs1.contains<ir::version10::term_meta>());

        // check term_meta
        {
          auto& meta = *attrs1.get<ir::version10::term_meta>();
          ASSERT_EQ(1, meta.docs_count);
          ASSERT_EQ(5, meta.e_single_doc);
        }

        // write term0 attributes to out
        writer.encode(*out, attrs1);
      }

      // check doc positions for term0 & term1
      {
        auto& meta0 = *attrs0.get<ir::version10::term_meta>();
        auto& meta1 = *attrs1.get<ir::version10::term_meta>();
        ASSERT_EQ(meta0.docs_count, meta1.docs_count);
        ASSERT_EQ(meta0.doc_start, meta1.doc_start);
        ASSERT_EQ(meta0.pos_start, meta1.pos_start);
        ASSERT_EQ(meta0.pos_end, meta1.pos_end);
        ASSERT_EQ(meta0.pay_start, meta1.pay_start);
      }

      // finish writing
      writer.end();
    }

    // read postings
    {
      ir::fields_meta fields;
      {
        std::vector<ir::field_meta> src;
        src.push_back(field);
        fields = ir::fields_meta(std::move(src), ir::flags(field.features));
      }

      ir::segment_meta meta;
      meta.name = "segment_name";

      ir::reader_state state;
      state.docs_mask = nullptr;
      state.dir = &dir();
      state.meta = &meta;
      state.fields = &fields;

      auto in = dir().open("attributes");
      ASSERT_FALSE(!in);

      /* prepare reader */
      ir::version10::postings_reader reader;
      reader.prepare(*in, state);

      /* read term0 attributes & postings */
      {
        ir::attributes read_attrs;
        reader.decode(*in, field.features, read_attrs);
        ASSERT_TRUE(read_attrs.contains<ir::version10::term_meta>());

        /* check term_meta for term0 */
        {
          auto& meta = *attrs0.get<ir::version10::term_meta>();
          auto& read_meta = *read_attrs.get<ir::version10::term_meta>();
          ASSERT_EQ(meta.docs_count, read_meta.docs_count);
          ASSERT_EQ(meta.doc_start, read_meta.doc_start);
          ASSERT_EQ(meta.pos_start, read_meta.pos_start);
          ASSERT_EQ(meta.pos_end, read_meta.pos_end);
          ASSERT_EQ(meta.pay_start, read_meta.pay_start);
          ASSERT_EQ(meta.e_single_doc, read_meta.e_single_doc);
          ASSERT_EQ(meta.e_skip_start, read_meta.e_skip_start);
        }

        /* read documents */
        auto it = reader.iterator(field.features, read_attrs, ir::flags::empty_instance());
        for (size_t i = 0; it->next();) {
          ASSERT_EQ(docs0[i++], it->value());
        }
      }

      /* check term_meta for term1 */
      {
        ir::attributes read_attrs;
        reader.decode(*in, field.features, read_attrs);
        ASSERT_TRUE(read_attrs.contains<ir::version10::term_meta>());

        {
          auto& meta = *attrs1.get<ir::version10::term_meta>();
          auto& read_meta = *read_attrs.get<ir::version10::term_meta>();
          ASSERT_EQ(meta.docs_count, read_meta.docs_count);
          ASSERT_EQ(0, read_meta.doc_start); /* we don't read doc start in case of singleton */
          ASSERT_EQ(meta.pos_start, read_meta.pos_start);
          ASSERT_EQ(meta.pos_end, read_meta.pos_end);
          ASSERT_EQ(meta.pay_start, read_meta.pay_start);
          ASSERT_EQ(meta.e_single_doc, read_meta.e_single_doc);
          ASSERT_EQ(meta.e_skip_start, read_meta.e_skip_start);
        }

        /* read documents */
        auto it = reader.iterator(field.features, read_attrs, ir::flags::empty_instance());
        for (size_t i = 0; it->next();) {
          ASSERT_EQ(docs1[i++], it->value());
        }
      }
    }
  }

  void postings_read_write() {
    ir::field_meta field;

    // docs & attributes for term0
    ir::attributes attrs0;
    std::vector<ir::doc_id_t> docs0{ 1, 3, 5, 7, 79, 101, 124 };

    // docs & attributes for term1
    ir::attributes attrs1;
    std::vector<ir::doc_id_t> docs1{ 2, 7, 9, 19 };

    // write postings
    {
      ir::flush_state state;
      state.dir = &dir();
      state.doc_count = 150;
      state.fields_count = 1;
      state.name = "segment_name";
      state.ver = IRESEARCH_VERSION;
      state.features = &field.features;

      auto out = dir().create("attributes");
      ASSERT_FALSE(!out);

      /* prepare writer */
      ir::version10::postings_writer writer(false);
      writer.prepare(*out, state);

      /* begin field */
      writer.begin_field(field.features);

      /* write postings for term0 */
      {
        postings docs(docs0.begin(), docs0.end());
        writer.write(docs, attrs0);
        ASSERT_TRUE(attrs0.contains<ir::version10::term_meta>());

        /* write attributes to out */
        writer.encode(*out, attrs0);
      }
      
      /* write postings for term1 */
      {
        postings docs(docs1.begin(), docs1.end());
        writer.write(docs, attrs1);
        ASSERT_TRUE(attrs1.contains<ir::version10::term_meta>());

        /* write attributes to out */
        writer.encode(*out, attrs1);
      }

      /* check doc positions for term0 & term1 */
      {
        auto& meta0 = *attrs0.get<ir::version10::term_meta>();
        auto& meta1 = *attrs1.get<ir::version10::term_meta>();
        ASSERT_GT(meta1.doc_start, meta0.doc_start);
      }

      /* finish writing */
      writer.end();
    }

    /* read postings */
    {
      ir::fields_meta fields;
      {
        std::vector<ir::field_meta> src;
        src.push_back(field);
        fields = ir::fields_meta(std::move(src), ir::flags(field.features));
      }

      ir::segment_meta meta;
      meta.name = "segment_name";

      ir::reader_state state;
      state.docs_mask = nullptr;
      state.dir = &dir();
      state.meta = &meta;
      state.fields = &fields;

      auto in = dir().open("attributes");
      ASSERT_FALSE(!in);

      /* prepare reader */
      ir::version10::postings_reader reader;
      reader.prepare(*in, state);
      
      /* cumulative attributes */
      ir::attributes read_attrs;

      /* read term0 attributes */
      {
        reader.decode(*in, field.features, read_attrs);
        ASSERT_TRUE(read_attrs.contains<ir::version10::term_meta>());

        /* check term_meta */
        {
          auto& meta = *attrs0.get<ir::version10::term_meta>();
          auto& read_meta = *read_attrs.get<ir::version10::term_meta>();
          ASSERT_EQ(meta.docs_count, read_meta.docs_count);
          ASSERT_EQ(meta.doc_start, read_meta.doc_start);
          ASSERT_EQ(meta.pos_start, read_meta.pos_start);
          ASSERT_EQ(meta.pos_end, read_meta.pos_end);
          ASSERT_EQ(meta.pay_start, read_meta.pay_start);
          ASSERT_EQ(meta.e_single_doc, read_meta.e_single_doc);
          ASSERT_EQ(meta.e_skip_start, read_meta.e_skip_start);
        }

        /* read documents */
        auto it = reader.iterator(field.features, read_attrs, ir::flags::empty_instance());
        for (size_t i = 0; it->next();) {
          ASSERT_EQ(docs0[i++], it->value());
        }
      }

      /* read term1 attributes */
      {
        reader.decode(*in, field.features, read_attrs);
        ASSERT_TRUE(read_attrs.contains<ir::version10::term_meta>());

        /* check term_meta */
        {
          auto& meta = *attrs1.get<ir::version10::term_meta>();
          auto& read_meta = *read_attrs.get<ir::version10::term_meta>();
          ASSERT_EQ(meta.docs_count, read_meta.docs_count);
          ASSERT_EQ(meta.doc_start, read_meta.doc_start);
          ASSERT_EQ(meta.pos_start, read_meta.pos_start);
          ASSERT_EQ(meta.pos_end, read_meta.pos_end);
          ASSERT_EQ(meta.pay_start, read_meta.pay_start);
          ASSERT_EQ(meta.e_single_doc, read_meta.e_single_doc);
          ASSERT_EQ(meta.e_skip_start, read_meta.e_skip_start);
        }

        /* read documents */
        auto it = reader.iterator(field.features, read_attrs, ir::flags::empty_instance());
        for (size_t i = 0; it->next();) {
          ASSERT_EQ(docs1[i++], it->value());
        }
      }
    }
  }

  void format_compress_read_write() {
    // iterate over an empty index
    {
      ir::compressed_index<uint64_t> reader;
      ASSERT_EQ(reader.begin(), reader.end());
    }
    
    const size_t start_offset = 100;
    const ir::doc_id_t blocks_count = 5000;
    const ir::doc_id_t block_docs = 128;
    const ir::doc_id_t last_block_docs_count = 73;
      
    ir::compressing_index_writer writer(1024);

    // writer index
    {
      auto out = dir().create("_0.idx");
      ASSERT_FALSE(!out);
      writer.prepare(*out);

      ir::doc_id_t i = 0;
      ir::doc_id_t doc = 0;
      for (; i < blocks_count; ++i) {
        writer.write(doc, start_offset + i);
        doc += block_docs;
      }      
      writer.write(doc, start_offset + i); // write terminal, partially filled block

      writer.finish();
    }

    // read index
    {
      const auto max_doc = blocks_count*block_docs + last_block_docs_count + (ir::type_limits<ir::type_t::doc_id_t>::min)();
      auto in = dir().open("_0.idx");
      ASSERT_FALSE(!in);

      static auto visitor = [] (uint64_t& t, uint64_t v) { t = v; };

      ir::compressed_index<uint64_t> reader;
      ASSERT_TRUE(reader.read(*in, max_doc, visitor));
      ASSERT_EQ(reader.end(), reader.lower_bound(max_doc + 1));

      // check iterators equality
      {
        auto it0 = reader.begin();
        std::advance(it0, 1542);
        auto it1 = it0++;
        ASSERT_EQ(it0, ++it1);
        ASSERT_EQ((*it0).first, (*it1).first);
        ASSERT_EQ((*it0).second, (*it1).second);
      }

      ir::doc_id_t i = 0;
      for (auto& entry : reader) {
        ASSERT_EQ(block_docs*i, entry.first);
        ASSERT_EQ(start_offset + i, entry.second);
        ++i;
      }

      for (ir::doc_id_t i = 0; i < max_doc; ++i) {
        auto less_or_eq = reader.lower_bound(i);
        ASSERT_NE(reader.end(), less_or_eq);
        ASSERT_EQ(start_offset + i / block_docs, less_or_eq->second);

        auto exact = reader.find(i);
        if (0 == i % block_docs) {
          ASSERT_NE(reader.end(), less_or_eq);
          ASSERT_EQ(start_offset + i / block_docs, exact->second);
        } else {
          ASSERT_EQ(reader.end(), exact);
        }
      }
    }
  }

  void assert_positions(const ir::doc_iterator& expected, const ir::doc_iterator& actual) {
    ir::position* expected_pos = expected.attributes().get<ir::position>();
    ir::position* actual_pos = actual.attributes().get<ir::position>();
    ASSERT_EQ(nullptr == expected_pos, nullptr == actual_pos);
    if (!expected_pos) {
      return;
    }

    ir::offset* expected_offset = expected_pos->attributes().get<ir::offset>();
    ir::offset* actual_offset = actual_pos->attributes().get<ir::offset>();
    ASSERT_EQ(nullptr == expected_offset, nullptr == actual_offset);
    
    ir::payload* expected_payload = expected_pos->attributes().get<ir::payload>();
    ir::payload* actual_payload = actual_pos->attributes().get<ir::payload>();
    ASSERT_EQ(nullptr == expected_payload, nullptr == actual_payload);

    for (; expected_pos->next();) {
      ASSERT_TRUE(actual_pos->next());
      ASSERT_EQ(expected_pos->value(), actual_pos->value());

      if (expected_offset) {
        ASSERT_EQ(expected_offset->start, actual_offset->start);
        ASSERT_EQ(expected_offset->end, actual_offset->end);
      }

      if (expected_payload) {
        ASSERT_EQ(expected_payload->value, actual_payload->value);
      }
    }
    ASSERT_FALSE(actual_pos->next());
  }

  void postings_seek(const std::vector<ir::doc_id_t>& docs, const ir::flags& features) {
    ir::field_meta field;
    field.features = features;

    /* attributes for term */
    ir::attributes attrs;

    /* write postings for field*/
    {
      ir::flush_state state;
      state.dir = &dir();
      state.doc_count = docs.back()+1;
      state.fields_count = 1;
      state.name = "segment_name";
      state.ver = IRESEARCH_VERSION;
      state.features = &field.features;

      auto out = dir().create("attributes");
      ASSERT_FALSE(!out);
      ir::write_string(*out, ir::string_ref("file_header"));

      /* prepare writer */
      ir::version10::postings_writer writer(false);
      writer.prepare(*out, state);

      /* begin field */
      writer.begin_field(field.features);
      /* write postings for term */
      {
        postings it(docs.begin(), docs.end(), field.features);
        writer.write(it, attrs);
        ASSERT_TRUE(attrs.contains<ir::version10::term_meta>());

        /* write attributes to out */
//        writer.encode(*out, attrs);
      }
      
      attrs.clear();

      /* begin field */
      writer.begin_field(field.features);
      /* write postings for term */
      {
        postings it(docs.begin(), docs.end(), field.features);
        writer.write(it, attrs);
        ASSERT_TRUE(attrs.contains<ir::version10::term_meta>());

        /* write attributes to out */
        writer.encode(*out, attrs);
      }

      /* finish writing */
      writer.end();
    }

    /* read postings */
    {
      ir::fields_meta fields;
      {
        std::vector<ir::field_meta> src;
        src.push_back(field);
        fields = ir::fields_meta(std::move(src), ir::flags(field.features));
      }

      ir::segment_meta meta;
      meta.name = "segment_name";

      ir::reader_state state;
      state.docs_mask = nullptr;
      state.dir = &dir();
      state.meta = &meta;
      state.fields = &fields;

      auto in = dir().open("attributes");
      ASSERT_FALSE(!in);
      ir::read_string<std::string>(*in);

      /* prepare reader */
      ir::version10::postings_reader reader;
      reader.prepare(*in, state);

      /* cumulative attributes */
      ir::attributes read_attrs;
      if (field.features.check<ir::frequency>()) {
        read_attrs.add<ir::frequency>()->value = 10;
      }

      // read term attributes
      {
        reader.decode(*in, field.features, read_attrs);
      //  reader.decode(*in, field, read_attrs);
        ASSERT_TRUE(read_attrs.contains<ir::version10::term_meta>());

        // check term_meta
        {
          auto& meta = *attrs.get<ir::version10::term_meta>();
          auto& read_meta = *read_attrs.get<ir::version10::term_meta>();
          ASSERT_EQ(meta.docs_count, read_meta.docs_count);
          ASSERT_EQ(meta.doc_start, read_meta.doc_start);
          ASSERT_EQ(meta.pos_start, read_meta.pos_start);
          ASSERT_EQ(meta.pos_end, read_meta.pos_end);
          ASSERT_EQ(meta.pay_start, read_meta.pay_start);
          ASSERT_EQ(meta.e_single_doc, read_meta.e_single_doc);
          ASSERT_EQ(meta.e_skip_start, read_meta.e_skip_start);
        }

        // seek for every document 127th document in a block
        {
          const size_t inc = ir::version10::postings_writer::BLOCK_SIZE;
          const size_t seed = ir::version10::postings_writer::BLOCK_SIZE-1;
          auto it = reader.iterator(field.features, read_attrs, field.features);
          ASSERT_FALSE(ir::type_limits<ir::type_t::doc_id_t>::valid(it->value()));

          postings expected(docs.begin(), docs.end(), field.features);
          for (size_t i = seed, size = docs.size(); i < size; i += inc) {
            auto doc = docs[i];
            ASSERT_EQ(doc, it->seek(doc));
            ASSERT_EQ(doc, it->seek(doc)); // seek to the same doc
            ASSERT_EQ(doc, it->seek(ir::type_limits<ir::type_t::doc_id_t>::invalid())); // seek to the smaller doc

            ASSERT_EQ(doc, expected.seek(doc));
            assert_positions(expected, *it);
          }
        }

        // seek for every 128th document in a block
        {
          const size_t inc = ir::version10::postings_writer::BLOCK_SIZE;
          const size_t seed = ir::version10::postings_writer::BLOCK_SIZE;
          auto it = reader.iterator(field.features, read_attrs, field.features);
          ASSERT_FALSE(ir::type_limits<ir::type_t::doc_id_t>::valid(it->value()));

          postings expected(docs.begin(), docs.end(), field.features);
          for (size_t i = seed, size = docs.size(); i < size; i += inc) {
            auto doc = docs[i];
            ASSERT_EQ(doc, it->seek(doc));
            ASSERT_EQ(doc, it->seek(doc)); // seek to the same doc
            ASSERT_EQ(doc, it->seek(ir::type_limits<ir::type_t::doc_id_t>::invalid())); // seek to the smaller doc

            ASSERT_EQ(doc, expected.seek(doc));
            assert_positions(expected, *it);
          }
        }

        // seek for every document
        {
          auto it = reader.iterator(field.features, read_attrs, field.features);
          ASSERT_FALSE(ir::type_limits<ir::type_t::doc_id_t>::valid(it->value()));

          postings expected(docs.begin(), docs.end(), field.features);
          for (auto doc : docs) {
            ASSERT_EQ(doc, it->seek(doc));
            ASSERT_EQ(doc, it->seek(doc)); // seek to the same doc
            ASSERT_EQ(doc, it->seek(ir::type_limits<ir::type_t::doc_id_t>::invalid())); // seek to the smaller doc

            ASSERT_EQ(doc, expected.seek(doc));
            assert_positions(expected, *it);
          }
          ASSERT_FALSE(it->next());
          ASSERT_TRUE(ir::type_limits<ir::type_t::doc_id_t>::eof(it->value()));

          // seek after the existing documents
          ASSERT_TRUE(ir::type_limits<ir::type_t::doc_id_t>::eof(it->seek(docs.back() + 10)));
        }

        // seek for backwards && next
        {
          for (auto doc = docs.rbegin(), end = docs.rend(); doc != end; ++doc) {
            postings expected(docs.begin(), docs.end(), field.features);
            auto it = reader.iterator(field.features, read_attrs, field.features);
            ASSERT_FALSE(ir::type_limits<ir::type_t::doc_id_t>::valid(it->value()));
            ASSERT_EQ(*doc, it->seek(*doc));

            ASSERT_EQ(*doc, expected.seek(*doc));
            assert_positions(expected, *it);
            if (doc != docs.rbegin()) {
              ASSERT_TRUE(it->next());
              ASSERT_EQ(*(doc-1), it->value());

              ASSERT_TRUE(expected.next());
              ASSERT_EQ(*(doc - 1), expected.value());
              assert_positions(expected, *it);
            }
          }
        }

        // seek for every 5th document
        {
          const size_t inc = 5;
          const size_t seed = 0;
          auto it = reader.iterator(field.features, read_attrs, field.features);
          ASSERT_FALSE(ir::type_limits<ir::type_t::doc_id_t>::valid(it->value()));
            
          postings expected(docs.begin(), docs.end(), field.features);
          for (size_t i = seed, size = docs.size(); i < size; i += inc) {
            auto doc = docs[i];
            ASSERT_EQ(doc, it->seek(doc));
            ASSERT_EQ(doc, it->seek(doc)); // seek to the same doc
            ASSERT_EQ(doc, it->seek(ir::type_limits<ir::type_t::doc_id_t>::invalid())); // seek to the smaller doc

            ASSERT_EQ(doc, expected.seek(doc));
            assert_positions(expected, *it);
          }
        }

        // seek for INVALID_DOC
        {
          auto it = reader.iterator(field.features, read_attrs, ir::flags::empty_instance());
          ASSERT_FALSE(ir::type_limits<ir::type_t::doc_id_t>::valid(it->value()));
          ASSERT_FALSE(ir::type_limits<ir::type_t::doc_id_t>::valid(it->seek(ir::type_limits<ir::type_t::doc_id_t>::invalid())));
          ASSERT_TRUE(it->next());
          ASSERT_EQ(docs.front(), it->value());
        }

        // seek for NO_MORE_DOCS
        {
          auto it = reader.iterator(field.features, read_attrs, ir::flags::empty_instance());
          ASSERT_FALSE(ir::type_limits<ir::type_t::doc_id_t>::valid(it->value()));
          ASSERT_TRUE(ir::type_limits<ir::type_t::doc_id_t>::eof(it->seek(ir::type_limits<ir::type_t::doc_id_t>::eof())));
          ASSERT_FALSE(it->next());
          ASSERT_TRUE(ir::type_limits<ir::type_t::doc_id_t>::eof(it->value()));
        }
      }
    }
  }

  void postings_seek() {
    // bug: ires336
    {
      ir::directory::ptr dir(get_directory());
      const ir::string_ref segment_name = "bug";
      const ir::string_ref field = "sbiotype";
      const ir::bytes_ref term = ir::ref_cast<ir::byte_type>(ir::string_ref("protein_coding"));

      std::vector<ir::doc_id_t> docs;
      {
        std::string buf;
        std::ifstream in(resource("postings.txt"));
        char* pend;
        while (std::getline(in, buf)) {
          docs.push_back(strtol(buf.c_str(), &pend, 10));
        }
      }
      std::vector<ir::bytes_ref> terms{ term };
      tests::format_test_case_base::terms<decltype(terms.begin())> trms(terms.begin(), terms.end(), docs.begin(), docs.end());

      iresearch::flush_state flush_state;
      flush_state.dir = dir.get();
      flush_state.doc_count = 10000;
      flush_state.fields_count = 1;
      flush_state.features = &ir::flags::empty_instance();
      flush_state.name = segment_name;
      flush_state.ver = 0;

      ir::field_meta field_meta;
      field_meta.id = 0;
      field_meta.name = field;
      {
        auto fw = get_codec()->get_field_writer(true);
        fw->prepare(flush_state);
        fw->write(field_meta.id, field_meta.features, trms);
        fw->end();
      }

      ir::fields_meta fields;
      {
        std::vector<ir::field_meta> src;
        src.push_back(field_meta);
        fields = ir::fields_meta(std::move(src), ir::flags(field_meta.features));
      }

      ir::segment_meta meta;
      meta.name = segment_name;

      ir::reader_state state;
      state.docs_mask = nullptr;
      state.dir = dir.get();
      state.meta = &meta;
      state.fields = &fields;

      auto fr = get_codec()->get_field_reader();
      fr->prepare(state);

      auto it = fr->terms(field_meta.id)->iterator();
      ASSERT_TRUE(it->seek(term));
      
      // ires-336 sequence
      {
        auto docs = it->postings(ir::flags::empty_instance());
        ASSERT_EQ(4048, docs->seek(4048));
        ASSERT_EQ(6830, docs->seek(6829));
      }

      // ires-336 extended sequence
      {
        auto docs = it->postings(ir::flags::empty_instance());
        ASSERT_EQ(1068, docs->seek(1068));
        ASSERT_EQ(1875, docs->seek(1873));
        ASSERT_EQ(4048, docs->seek(4048));
        ASSERT_EQ(6830, docs->seek(6829));
      }

      // extended sequence
      {
        auto docs = it->postings(ir::flags::empty_instance());
        ASSERT_EQ(4048, docs->seek(4048));
        ASSERT_EQ(4400, docs->seek(4400));
        ASSERT_EQ(6830, docs->seek(6829));
      }
      
      // ires-336 full sequence
      {
        auto docs = it->postings(ir::flags::empty_instance());
        ASSERT_EQ(334, docs->seek(334));
        ASSERT_EQ(1046, docs->seek(1046));
        ASSERT_EQ(1068, docs->seek(1068));
        ASSERT_EQ(2307, docs->seek(2307));
        ASSERT_EQ(2843, docs->seek(2843));
        ASSERT_EQ(3059, docs->seek(3059));
        ASSERT_EQ(3564, docs->seek(3564));
        ASSERT_EQ(4048, docs->seek(4048));
        ASSERT_EQ(7773, docs->seek(7773));
        ASSERT_EQ(8204, docs->seek(8204));
        ASSERT_EQ(9353, docs->seek(9353));
        ASSERT_EQ(9366, docs->seek(9366));
      }
    }

    // short list (< postings_writer::BLOCK_SIZE)
    {
      std::vector<ir::doc_id_t> docs;
      {
        const size_t count = 117;
        docs.reserve(count);
        auto i = (ir::type_limits<ir::type_t::doc_id_t>::min)();
        std::generate_n(std::back_inserter(docs), count,[&i]() {return i++;});
      }
      postings_seek(docs, {ir::frequency::type() });
      postings_seek(docs, { ir::frequency::type(), ir::position::type() });
      postings_seek(docs, { ir::frequency::type(), ir::position::type(), ir::offset::type() });
      postings_seek(docs, { ir::frequency::type(), ir::position::type(), ir::payload::type() });
      postings_seek(docs, { ir::frequency::type(), ir::position::type(), ir::offset::type(), ir::payload::type() });
    }

    // equals to postings_writer::BLOCK_SIZE
    {
      std::vector<ir::doc_id_t> docs;
      {
        const size_t count = ir::version10::postings_writer::BLOCK_SIZE;
        docs.reserve(count);
        auto i = (ir::type_limits<ir::type_t::doc_id_t>::min)();
        std::generate_n(std::back_inserter(docs), count,[&i]() {return i++;});
      }
      postings_seek(docs, {});
      postings_seek(docs, { ir::frequency::type(), ir::position::type() });
      postings_seek(docs, { ir::frequency::type(), ir::position::type(), ir::offset::type() });
      postings_seek(docs, { ir::frequency::type(), ir::position::type(), ir::payload::type() });
      postings_seek(docs, { ir::frequency::type(), ir::position::type(), ir::offset::type(), ir::payload::type() });
    }

    // long list
    {
      std::vector<ir::doc_id_t> docs;
      {
        const size_t count = 10000;
        docs.reserve(count);
        auto i = (ir::type_limits<ir::type_t::doc_id_t>::min)();
        std::generate_n(std::back_inserter(docs), count,[&i]() {return i++;});
      }
      postings_seek(docs, {});
      postings_seek(docs, { ir::frequency::type(), ir::position::type() });
      postings_seek(docs, { ir::frequency::type(), ir::position::type(), ir::offset::type() });
      postings_seek(docs, { ir::frequency::type(), ir::position::type(), ir::payload::type() });
      postings_seek(docs, { ir::frequency::type(), ir::position::type(), ir::offset::type(), ir::payload::type() });
    }

    // 2^15 
    {
      std::vector<ir::doc_id_t> docs;
      {
        const size_t count = 32768;
        docs.reserve(count);
        auto i = (ir::type_limits<ir::type_t::doc_id_t>::min)();
        std::generate_n(std::back_inserter(docs), count,[&i]() {return i+=2;});
      }
      postings_seek(docs, {});
      postings_seek(docs, { ir::frequency::type(), ir::position::type() });
      postings_seek(docs, { ir::frequency::type(), ir::position::type(), ir::offset::type() });
      postings_seek(docs, { ir::frequency::type(), ir::position::type(), ir::payload::type() });
      postings_seek(docs, { ir::frequency::type(), ir::position::type(), ir::offset::type(), ir::payload::type() });
    }
  }
}; // format_10_test_case

// ----------------------------------------------------------------------------
// --SECTION--                           memory_directory + iresearch_format_10
// ----------------------------------------------------------------------------

class memory_format_10_test_case : public format_10_test_case {
 protected:
  virtual ir::directory* get_directory() override {
    return new ir::memory_directory();
  }
};

TEST_F(memory_format_10_test_case, directory_cleaner) {
  directory_artifact_cleaner();
}

TEST_F(memory_format_10_test_case, fields_rw) {
  fields_read_write();
}

TEST_F(memory_format_10_test_case, postings_rw) {
  postings_read_write_single_doc();
  postings_read_write();
}

TEST_F(memory_format_10_test_case, postings_seek) {
  postings_seek();
}

TEST_F(memory_format_10_test_case, segment_meta_rw) {
  segment_meta_read_write();
}

TEST_F(memory_format_10_test_case, field_meta_rw) {
  field_meta_read_write();
}

TEST_F(memory_format_10_test_case, columns_rw) {
  columns_read_write_empty();
  columns_read_write();
}

TEST_F(memory_format_10_test_case, columns_rw_reuse) {
  columns_big_document_read_write();
  columns_read_write_reuse();
  columns_read_write_typed();
  format_compress_read_write();
}

TEST_F(memory_format_10_test_case, columns_meta_rw) {
  columns_meta_read_write();
}

TEST_F(memory_format_10_test_case, document_mask_rw) {
  document_mask_read_write();
}

// ----------------------------------------------------------------------------
// --SECTION--                               fs_directory + iresearch_format_10
// ----------------------------------------------------------------------------

class fs_format_10_test_case : public format_10_test_case {
protected:
  virtual ir::directory* get_directory() override {
    const fs::path dir = fs::path(test_dir()).append("index");
    ir::fs_directory::create_directory(dir.string());
    return new ir::fs_directory(dir.string());
  }
};

TEST_F(fs_format_10_test_case, test_load) {
  auto format = iresearch::formats::get("1_0");

  ASSERT_NE(nullptr, format);
}

TEST_F(fs_format_10_test_case, directory_cleaner) {
  directory_artifact_cleaner();
}

TEST_F(fs_format_10_test_case, fields_rw) {
  fields_read_write();
}

TEST_F(fs_format_10_test_case, postings_seek) {
  postings_seek();
}

TEST_F(fs_format_10_test_case, postings_rw) {
  postings_read_write();
  postings_read_write_single_doc();
}

TEST_F(fs_format_10_test_case, segment_meta_rw) {
  segment_meta_read_write();
}

TEST_F(fs_format_10_test_case, field_meta_rw) {
  field_meta_read_write();
}

TEST_F(fs_format_10_test_case, columns_rw) {
  columns_read_write_empty();
  columns_read_write();
}

TEST_F(fs_format_10_test_case, columns_rw_reuse) {
  columns_big_document_read_write();
  columns_read_write_reuse();
  columns_read_write_typed();
  format_compress_read_write();
}

TEST_F(fs_format_10_test_case, columns_meta_rw) {
  columns_meta_read_write();
}

TEST_F(fs_format_10_test_case, document_mask_rw) {
  document_mask_read_write();
}