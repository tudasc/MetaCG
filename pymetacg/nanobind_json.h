// adapted from https://github.com/ianhbell/nanobind_json
/***************************************************************************
 * Copyright (c) 2019, Martin Renou                                         *
 *                                                                          *
 * Distributed under the terms of the BSD 3-Clause License.                 *
 *                                                                          *
 * The full license is in the file LICENSE, distributed with this software. *
 ****************************************************************************/

#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include <nanobind/nanobind.h>

namespace nb = nanobind;
namespace nl = nlohmann;

namespace pyjson {
inline nb::object from_json(const nl::json& j) {
  if (j.is_null()) {
    return nb::none();
  } else if (j.is_boolean()) {
    return nb::bool_(j.get<bool>());
  } else if (j.is_number_integer()) {
    return nb::int_(j.get<long>());
  } else if (j.is_number_float()) {
    return nb::float_(j.get<double>());
  } else if (j.is_string()) {
    return nb::str(j.get<std::string>().c_str());
  } else if (j.is_array()) {
    nb::list obj;
    for (const auto& el : j) {
      obj.append(from_json(el));
    }
    return std::move(obj);
  } else  // Object
  {
    nb::dict obj;
    for (nl::json::const_iterator it = j.cbegin(); it != j.cend(); ++it) {
      obj[nb::str(it.key().c_str())] = from_json(it.value());
    }
    return std::move(obj);
  }
}

inline nl::json to_json(const nb::handle& obj) {
  if (obj.ptr() == nullptr || obj.is_none()) {
    return nullptr;
  }
  if (nb::isinstance<nb::bool_>(obj)) {
    // return obj.cast<bool>();
    return nb::cast<bool>(obj);
  }
  if (nb::isinstance<nb::int_>(obj)) {
    return nb::cast<long>(obj);
  }
  if (nb::isinstance<nb::float_>(obj))

    return nb::cast<double>(obj);

  if (nb::isinstance<nb::bytes>(obj)) {
    nb::module_ base64 = nb::module_::import_("base64");
    return nb::cast<std::string>(base64.attr("b64encode")(obj).attr("decode")("utf-8"));
  }
  if (nb::isinstance<nb::str>(obj)) {
    return nb::cast<std::string>(obj);
  }
  if (nb::isinstance<nb::tuple>(obj) || nb::isinstance<nb::list>(obj)) {
    auto out = nl::json::array();
    for (const nb::handle value : obj) {
      out.push_back(to_json(value));
    }
    return out;
  }
  if (nb::isinstance<nb::dict>(obj)) {
    auto out = nl::json::object();
    for (const nb::handle key : obj) {
      out[nb::cast<std::string>(nb::str(key))] = to_json(obj[key]);
    }
    return out;
  }
  throw std::runtime_error("to_json not implemented for this type of object: " + nb::cast<std::string>(nb::repr(obj)));
}
}  // namespace pyjson

// nlohmann_json serializers
namespace nlohmann {
#define MAKE_NLJSON_SERIALIZER_DESERIALIZER(T)                                                                         \
  template <>                                                                                                          \
  struct adl_serializer<T> {                                                                                           \
    inline static void to_json(json& j, const T& obj) { j = pyjson::to_json(obj); }                                    \
                                                                                                                       \
    inline static T from_json(const json& j) { return nb::cast<T>(pyjson::from_json(j)); }                             \
  };

MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::object);

MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::bool_);
MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::int_);
MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::float_);
MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::str);
MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::list);
MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::tuple);
MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::dict);

#undef MAKE_NLJSON_SERIALIZER_DESERIALIZER
}  // namespace nlohmann

// nanobind caster
namespace nanobind {
namespace detail {
template <>
struct type_caster<nl::json> {
 public:
  static constexpr auto Name = const_name("JSON");

  bool from_python(handle src, bool) {
    // not implemented
    return false;
  }

  static handle from_cpp(nl::json src, rv_policy /* policy */, cleanup_list* /* parent */) {
    object obj = pyjson::from_json(src);
    return obj.release();
  }
};
}  // namespace detail
}  // namespace nanobind
