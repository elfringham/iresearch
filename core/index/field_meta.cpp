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
#include "field_meta.hpp"

NS_ROOT

// -----------------------------------------------------------------------------
// --SECTION--                                         field_meta implementation
// -----------------------------------------------------------------------------

/*static*/ const field_meta field_meta::EMPTY;

field_meta::field_meta(
    const string_ref& name,
    const flags& features,
    field_id norm /* = field_limits::invalid() */)
  : features(features),
    name(name.c_str(), name.size()),
    norm(norm) {
}

// -----------------------------------------------------------------------------
// --SECTION--                                        column_meta implementation
// -----------------------------------------------------------------------------

column_meta::column_meta(const string_ref& name, field_id id)
  : name(name.c_str(), name.size()), id(id) {
}

NS_END
