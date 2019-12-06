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
////////////////////////////////////////////////////////////////////////////////

#include "regex_filter.hpp"

#include "shared.hpp"
#include "limited_sample_scorer.hpp"
#include "disjunction.hpp"
#include "multiterm_query.hpp"
#include "term_query.hpp"
#include "bitset_doc_iterator.hpp"
#include "index/index_reader.hpp"
#include "utils/automaton_utils.hpp"
#include "utils/hash_utils.hpp"

NS_ROOT

DEFINE_FILTER_TYPE(by_regex)
DEFINE_FACTORY_DEFAULT(by_regex)

filter::prepared::ptr by_regex::prepare(
    const index_reader& index,
    const order::prepared& order,
    boost_t boost,
    const attribute_view& /*ctx*/) const {
  boost *= this->boost();
  const string_ref field = this->field();

//  switch (wildcard_type(term())) {
//    case WildcardType::TERM:
//      return term_query::make(index, order, boost, field, term());
//    case WildcardType::MATCH_ALL:
//      return by_prefix::prepare(index, order, boost, field,
//                                bytes_ref::EMPTY, // empty prefix == match all
//                                scored_terms_limit());
//    case WildcardType::PREFIX: {
//      assert(!term().empty());
//      const auto pos = term().find(wildcard_traits_t::MATCH_ANY_STRING);
//      assert(pos != irs::bstring::npos);
//
//      return by_prefix::prepare(index, order, boost, field,
//                                bytes_ref(term().c_str(), pos), // remove trailing '%'
//                                scored_terms_limit());
//    }
//
//    case WildcardType::WILDCARD:
//      return prepare_automaton_filter(field, from_wildcard<byte_type, wildcard_traits_t>(term()),
//                                      scored_terms_limit(), index, order, boost);
//  }

  assert(false);
  return prepared::empty();
}

by_regex::by_regex() noexcept
  : by_prefix(by_regex::type()) {
}

NS_END
