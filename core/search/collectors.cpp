////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2020 ArangoDB GmbH, Cologne, Germany
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
////////////////////////////////////////////////////////////////////////////////

#include "collectors.hpp"

NS_LOCAL

using namespace irs;

struct noop_field_collector final : sort::field_collector {
  virtual void collect(const sub_reader&, const term_reader&) override {

  }
  virtual void reset() override { }
  virtual void collect(const bytes_ref&) override { }
  virtual void write(data_output&) const override { }
};

struct noop_term_collector final : sort::term_collector {
  virtual void collect(const sub_reader&,
                       const term_reader&,
                       const attribute_view&) override {
  }
  virtual void reset() override { }
  virtual void collect(const bytes_ref&) override { }
  virtual void write(data_output&) const override { }
};

static noop_field_collector NOOP_FIELD_STATS;
static noop_term_collector NOOP_TERM_STATS;

NS_END

NS_ROOT

// -----------------------------------------------------------------------------
// --SECTION--                                           field_collector_wrapper
// -----------------------------------------------------------------------------

field_collector_wrapper::field_collector_wrapper() noexcept
  : base_type(&NOOP_FIELD_STATS) {
}

field_collector_wrapper::field_collector_wrapper(collector_type::ptr&& collector) noexcept
  : base_type(!collector ? &NOOP_FIELD_STATS : collector.release()) {
}

field_collector_wrapper& field_collector_wrapper::operator=(collector_type::ptr&& collector) noexcept {
  reset(!collector ? &NOOP_FIELD_STATS : collector.release());
  return *this;
};

field_collector_wrapper::~field_collector_wrapper() {
  reset(nullptr);
}

void field_collector_wrapper::reset(sort::field_collector* collector) noexcept {
  if (collector_.get() == &NOOP_FIELD_STATS) {
    collector_.release();
  }
  collector_.reset(collector);
}

field_collectors::field_collectors(const order::prepared& buckets)
  : collectors_base<field_collector_wrapper>(buckets.size(), buckets) {
  auto begin = collectors_.begin();
  for (auto& bucket : buckets) {
    *begin = bucket.bucket->prepare_field_collector();
    assert(*begin); // ensured by wrapper
    ++begin;
  }
  assert(begin == collectors_.end());
}

// -----------------------------------------------------------------------------
// --SECTION--                                                  field_collectors
// -----------------------------------------------------------------------------

void field_collectors::collect(const sub_reader& segment,
                               const term_reader& field) const {
  switch (collectors_.size()) {
    case 0:
      return;
    case 1:
      collectors_.front()->collect(segment, field);
      return;
    case 2:
      collectors_.front()->collect(segment, field);
      collectors_.back()->collect(segment, field);
      return;
    default:
      for (auto& collector : collectors_) {
        collector->collect(segment, field);
      }
  }
}

void field_collectors::finish(byte_type* stats_buf, const index_reader& index) const {
  // special case where term statistics collection is not applicable
  // e.g. by_column_existence filter
  assert(buckets_->size() == collectors_.size());

  for (size_t i = 0, count = collectors_.size(); i < count; ++i) {
    auto& sort = (*buckets_)[i];
    assert(sort.bucket); // ensured by order::prepare

    sort.bucket->collect(
      stats_buf + sort.stats_offset, // where stats for bucket start
      index,
      collectors_[i].get(),
      nullptr
    );
  }
}

// -----------------------------------------------------------------------------
// --SECTION--                                            term_collector_wrapper
// -----------------------------------------------------------------------------

term_collector_wrapper::term_collector_wrapper() noexcept
  : base_type(&NOOP_TERM_STATS) {
}

term_collector_wrapper::term_collector_wrapper(collector_type::ptr&& collector) noexcept
  : base_type(!collector ? &NOOP_TERM_STATS : collector.release()) {
}

term_collector_wrapper& term_collector_wrapper::operator=(collector_type::ptr&& collector) noexcept {
  reset(!collector ? &NOOP_TERM_STATS : collector.release());
  return *this;
};

term_collector_wrapper::~term_collector_wrapper() {
  reset(nullptr);
}

void term_collector_wrapper::reset(collector_type* collector) noexcept {
  if (collector_.get() == &NOOP_TERM_STATS) {
    collector_.release();
  }
  collector_.reset(collector);
}

// -----------------------------------------------------------------------------
// --SECTION--                                                   term_collectors
// -----------------------------------------------------------------------------

term_collectors::term_collectors(const order::prepared& buckets, size_t size)
  : collectors_base<sort::term_collector::ptr>(buckets.size()*size, buckets) {
  // add term collectors from each bucket
  // layout order [t0.b0, t0.b1, ... t0.bN, t1.b0, t1.b1 ... tM.BN]
  auto begin = collectors_.begin();
  for (size_t i = 0; i < size; ++i) {
    for (auto& entry: buckets) {
      assert(entry.bucket); // ensured by order::prepare

      *begin = entry.bucket->prepare_term_collector();
      assert(*begin); // ensured by wrapper
      ++begin;
    }
  }
  assert(begin == collectors_.end());
}


void term_collectors::collect(
    const sub_reader& segment, const term_reader& field,
    size_t term_idx, const attribute_view& attrs) const {
  // collector may be null if prepare_term_collector() returned nullptr
  const size_t count = buckets_->size();

  switch (count) {
    case 0:
      return;
    case 1: {
      assert(term_idx < collectors_.size());
      assert(collectors_[term_idx]); // enforced by wrapper
      collectors_[term_idx]->collect(segment, field, attrs);
      return;
    }
    case 2: {
      assert(term_idx + 1 < collectors_.size());
      assert(collectors_[term_idx]); // enforced by wrapper
      assert(collectors_[term_idx+1]); // enforced by wrapper
      collectors_[term_idx]->collect(segment, field, attrs);
      collectors_[term_idx+1]->collect(segment, field, attrs);
      return;
    }
    default: {
      const size_t term_offset_count = term_idx * count;
      for (size_t i = 0; i < count; ++i) {
        const auto idx = term_offset_count + i;
        assert(idx < collectors_.size()); // enforced by allocation in the constructor
        assert(collectors_[idx]); // enforced by wrapper

        collectors_[idx]->collect(segment, field, attrs);
      }
      return;
    }
  }
}


size_t term_collectors::push_back() {
  const size_t size = buckets_->size();

  switch (size) {
    case 0:
      return 0;
    case 1: {
      const auto term_offset = collectors_.size();
      assert((*buckets_)[0].bucket); // ensured by order::prepare
      collectors_.emplace_back((*buckets_)[0].bucket->prepare_term_collector());
      return term_offset;
    }
    case 2: {
      const auto term_offset = collectors_.size() / 2;
      assert((*buckets_)[0].bucket); // ensured by order::prepare
      collectors_.emplace_back((*buckets_)[0].bucket->prepare_term_collector());
      assert((*buckets_)[1].bucket); // ensured by order::prepare
      collectors_.emplace_back((*buckets_)[1].bucket->prepare_term_collector());
      return term_offset;
    }
    default: {
      const auto term_offset = collectors_.size() / size;
      collectors_.reserve(collectors_.size() + size);
      for (auto& entry: (*buckets_)) {
        assert(entry.bucket); // ensured by order::prepare
        collectors_.emplace_back(entry.bucket->prepare_term_collector());
      }
      return term_offset;
    }
  }
}

void term_collectors::finish(byte_type* stats_buf,
                             const field_collectors& field_collectors,
                             const index_reader& index) const {
  auto bucket_count = buckets_->size();
  assert(collectors_.size() % bucket_count == 0); // enforced by allocation in the constructor

  for (size_t i = 0, count = collectors_.size(); i < count; ++i) {
    auto bucket_offset = i % bucket_count;
    auto& sort = (*buckets_)[bucket_offset];
    assert(sort.bucket); // ensured by order::prepare

    assert(i % bucket_count < field_collectors.size());
    sort.bucket->collect(
      stats_buf + sort.stats_offset, // where stats for bucket start
      index,
      field_collectors[bucket_offset],
      collectors_[i].get()
    );
  }
}

NS_END