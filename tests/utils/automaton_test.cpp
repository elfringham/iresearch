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

#include "tests_shared.hpp"

#include "utils/automaton_utils.hpp"

TEST(automaton_test, match_wildcard) {
  // nil string
  {
    auto a = irs::from_wildcard<char>(irs::string_ref::NIL);
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_TRUE(irs::accept<char>(a, ""));
    ASSERT_TRUE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_FALSE(irs::accept<char>(a, "a"));
  }

  // empty string
  {
    auto a = irs::from_wildcard<char>(irs::string_ref::EMPTY);
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_TRUE(irs::accept<char>(a, ""));
    ASSERT_TRUE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_FALSE(irs::accept<char>(a, "a"));
  }

  // any or empty string
  {
    auto a = irs::from_wildcard<char>("%");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_TRUE(irs::accept<char>(a, ""));
    ASSERT_TRUE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "a"));
    ASSERT_TRUE(irs::accept<char>(a, "abc"));
  }

  // any or empty string
  {
    auto a = irs::from_wildcard<char>("%%");

    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_TRUE(irs::accept<char>(a, ""));
    ASSERT_TRUE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "a"));
    ASSERT_TRUE(irs::accept<char>(a, "aa"));
    ASSERT_TRUE(irs::accept<char>(a, "azbce1d"));
    ASSERT_TRUE(irs::accept<char>(a, "azbce1d1"));
    ASSERT_TRUE(irs::accept<char>(a, "azbce11d"));
  }

  // any char
  {
    auto a = irs::from_wildcard<char>("_");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_FALSE(irs::accept<char>(a, ""));
    ASSERT_FALSE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "a"));
    ASSERT_FALSE(irs::accept<char>(a, "abc"));
  }

  // two any chars
  {
    auto a = irs::from_wildcard<char>("__");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_FALSE(irs::accept<char>(a, ""));
    ASSERT_FALSE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_FALSE(irs::accept<char>(a, "a"));
    ASSERT_TRUE(irs::accept<char>(a, "ba"));
    ASSERT_FALSE(irs::accept<char>(a, "azbce1d"));
    ASSERT_FALSE(irs::accept<char>(a, "azbce1d1"));
    ASSERT_FALSE(irs::accept<char>(a, "azbce11d"));
  }

  // any char (suffix)
  {
    auto a = irs::from_wildcard<char>("a_");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_FALSE(irs::accept<char>(a, ""));
    ASSERT_FALSE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "a_"));
    ASSERT_FALSE(irs::accept<char>(a, "a"));
    ASSERT_TRUE(irs::accept<char>(a, "ab"));
  }

  // any char (prefix)
  {
    auto a = irs::from_wildcard<char>("_a");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_FALSE(irs::accept<char>(a, ""));
    ASSERT_FALSE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "_a"));
    ASSERT_FALSE(irs::accept<char>(a, "a"));
    ASSERT_TRUE(irs::accept<char>(a, "ba"));
  }

  // escaped '_'
  {
    auto a = irs::from_wildcard<char>("\\_a");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_FALSE(irs::accept<char>(a, ""));
    ASSERT_FALSE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "_a"));
    ASSERT_FALSE(irs::accept<char>(a, "a"));
    ASSERT_FALSE(irs::accept<char>(a, "ba"));
  }

  // escaped '\'
  {
    auto a = irs::from_wildcard<char>("\\\\\\_a");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_FALSE(irs::accept<char>(a, ""));
    ASSERT_FALSE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "\\_a"));
    ASSERT_FALSE(irs::accept<char>(a, "a"));
    ASSERT_FALSE(irs::accept<char>(a, "ba"));
  }

  // nonterminated '\'
  {
    auto a = irs::from_wildcard<char>("a\\");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_FALSE(irs::accept<char>(a, ""));
    ASSERT_FALSE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "a\\"));
    ASSERT_FALSE(irs::accept<char>(a, "a"));
    ASSERT_FALSE(irs::accept<char>(a, "ba"));
  }

  // escaped '%'
  {
    auto a = irs::from_wildcard<char>("\\\\\\%a");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_FALSE(irs::accept<char>(a, ""));
    ASSERT_FALSE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "\\%a"));
    ASSERT_FALSE(irs::accept<char>(a, "a"));
    ASSERT_FALSE(irs::accept<char>(a, "ba"));
  }

  // prefix
  {
    auto a = irs::from_wildcard<char>("foo%");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_FALSE(irs::accept<char>(a, ""));
    ASSERT_FALSE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "foo"));
    ASSERT_TRUE(irs::accept<char>(a, "foobar"));
    ASSERT_FALSE(irs::accept<char>(a, "foa"));
    ASSERT_FALSE(irs::accept<char>(a, "foabar"));
  }

  // suffix
  {
    auto a = irs::from_wildcard<char>("%foo");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_FALSE(irs::accept<char>(a, ""));
    ASSERT_FALSE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "foo"));
    ASSERT_TRUE(irs::accept<char>(a, "bfoo"));
    ASSERT_FALSE(irs::accept<char>(a, "foa"));
    ASSERT_FALSE(irs::accept<char>(a, "bfoa"));
  }

  // mixed
  {
    auto a = irs::from_wildcard<char>("a%bce_d");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_FALSE(irs::accept<char>(a, ""));
    ASSERT_FALSE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "azbce1d"));
    ASSERT_FALSE(irs::accept<char>(a, "azbce1d1"));
    ASSERT_FALSE(irs::accept<char>(a, "azbce11d"));
  }

  // mixed
  {
    auto a = irs::from_wildcard<char>("%_");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_FALSE(irs::accept<char>(a, ""));
    ASSERT_FALSE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "a"));
    ASSERT_TRUE(irs::accept<char>(a, "aa"));
    ASSERT_TRUE(irs::accept<char>(a, "azbce1d"));
    ASSERT_TRUE(irs::accept<char>(a, "azbce1d1"));
    ASSERT_TRUE(irs::accept<char>(a, "azbce11d"));
  }

  // mixed
  {
    auto a = irs::from_wildcard<char>("%%_");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_FALSE(irs::accept<char>(a, ""));
    ASSERT_FALSE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "a"));
    ASSERT_TRUE(irs::accept<char>(a, "aa"));
    ASSERT_TRUE(irs::accept<char>(a, "azbce1d"));
    ASSERT_TRUE(irs::accept<char>(a, "azbce1d1"));
    ASSERT_TRUE(irs::accept<char>(a, "azbce11d"));
  }

  // mixed
  {
    auto a = irs::from_wildcard<char>("_%");
    ASSERT_EQ(fst::kAcceptor | fst::kUnweighted, a.Properties(fst::kAcceptor | fst::kUnweighted, true));
    ASSERT_FALSE(irs::accept<char>(a, ""));
    ASSERT_FALSE(irs::accept<char>(a, irs::string_ref::NIL));
    ASSERT_TRUE(irs::accept<char>(a, "a"));
    ASSERT_TRUE(irs::accept<char>(a, "aa"));
    ASSERT_TRUE(irs::accept<char>(a, "azbce1d"));
    ASSERT_TRUE(irs::accept<char>(a, "azbce1d1"));
    ASSERT_TRUE(irs::accept<char>(a, "azbce11d"));
  }
}

//TEST(automaton_test, test) {
//  auto a0 = irs::from_wildcard<char>("%");
////  auto a1 = irs::from_wildcard<char>("a");
//  auto a2 = irs::from_wildcard<char>("_");
//
// // fst::Concat(&a0, a1);
//  fst::Concat(&a0, a2);
//  fst::RmEpsilon(&a0);
//  fst::fsa::Automaton a;
//  fst::Determinize(a0, &a);
//
//  {
//    std::fstream fff("myfuck1", std::fstream::out);
//    fst::drawFst(a, fff, "");
//  }
//
//  ASSERT_FALSE(fst::kError == a0.Properties(fst::kError, true));
//}