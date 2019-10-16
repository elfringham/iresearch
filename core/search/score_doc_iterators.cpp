////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2016 by EMC Corporation, All Rights Reserved
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
/// Copyright holder is EMC Corporation
///
/// @author Andrey Abramov
/// @author Vasiliy Nabatchikov
////////////////////////////////////////////////////////////////////////////////

#include "shared.hpp"
#include "score_doc_iterators.hpp"

NS_ROOT

#if defined(_MSC_VER)
  #pragma warning( disable : 4706 )
#elif defined (__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wparentheses"
#endif

basic_doc_iterator::basic_doc_iterator(
    const sub_reader& segment,
    const term_reader& field,
    const byte_type* stats,
    doc_iterator::ptr&& it,
    const order::prepared& ord,
    cost::cost_t estimation,
    boost_t boost) noexcept
  : it_(std::move(it)),
    stats_(stats) {
  assert(it_);

  // set estimation value
  estimate(estimation);

  // make document attribute accessible
  doc_ = (attrs_.emplace<irs::document>()
            = it_->attributes().get<irs::document>()).get();
  assert(doc_);

  // set scorers
  prepare_score(
    ord,
    ord.prepare_scorers(segment, field, stats_, it_->attributes(), boost
  ));
}

#if defined(_MSC_VER)
  #pragma warning( default : 4706 )
#elif defined (__GNUC__)
  #pragma GCC diagnostic pop
#endif

NS_END // ROOT
