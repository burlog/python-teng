/*
 * FILE             $Id: $
 *
 * LICENCE          MIT
 *
 * DESCRIPTION      Teng C++ wrapper.
 *
 * AUTHOR           Michal Bukovsky <burlog@seznam.cz>
 */

#include <teng/teng.h>
#include <teng/udf.h>
#include <teng/structs.h>
#include <boost/python.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

using Teng::Teng_t;
using Teng::Fragment_t;
using Teng::FragmentList_t;
using Teng::FragmentValue_t;
using Teng::IntType_t;
using Teng::Error_t;
using Teng::Writer_t;
using Teng::FileWriter_t;
using Teng::Value_t;
using Teng::StringWriter_t;
using GenPageArgs_t = Teng::Teng_t::GenPageArgs_t;

namespace bp = boost::python;

namespace {

std::string get_string(const bp::object &obj) {
    if (obj.is_none()) return "";
    return bp::extract<std::string>(obj)();
}

std::string extract_exc() {
    PyObject *exc, *value, *traceback;
    PyErr_Fetch(&exc, &value, &traceback);
    bp::handle<> exc_handle(exc);
    bp::handle<> value_handle(bp::allow_null(value));
    return exc_handle
        ? bp::extract<std::string>(bp::str(exc_handle))()
          + ": " + bp::extract<std::string>(bp::str(value_handle))()
        : __PRETTY_FUNCTION__;
}

struct fragment_pair_to_tuple_t {
    using pair_t = std::pair<const std::string, FragmentValue_t>;
    static PyObject *convert(const pair_t &p) {
        return bp::incref(bp::make_tuple(
            p.first,
            p.second
        ).ptr());
    }
    static const PyTypeObject *get_pytype() {return &PyTuple_Type;}
};

struct fragment_value_to_python_t {
    using value_t = FragmentValue_t;
    static PyObject *convert(const value_t &p) {
        switch (p.type()) {
        case value_t::tag::string:
            return bp::incref(bp::object(*p.string()).ptr());
        case value_t::tag::integral:
            return bp::incref(bp::object(*p.integral()).ptr());
        case value_t::tag::real:
            return bp::incref(bp::object(*p.real()).ptr());
        case value_t::tag::frag:
            return bp::incref(bp::object(boost::ref(*p.fragment())).ptr());
        case value_t::tag::list:
            return bp::incref(bp::object(boost::ref(*p.list())).ptr());
        case value_t::tag::frag_ptr:
            return bp::incref(bp::object(boost::ref(*p.fragment())).ptr());
        }
        return bp::incref(bp::object().ptr());
    }
};

class UDFWrapper_t {
public:
    UDFWrapper_t(const std::string &name, bp::object callback)
        : name(name), callback(new bp::object(callback))
    {}

    Value_t operator()(const std::vector<Value_t> &args) const {
        try {
            bp::list python_args;
            for (auto &arg: args) {
                switch (arg.type()) {
                case Value_t::tag::string:
                    python_args.append(arg.as_string());
                    break;
                case Value_t::tag::string_ref:
                    python_args.append(arg.as_string_ref().str());
                    break;
                case Value_t::tag::integral:
                    python_args.append(arg.as_int());
                    break;
                case Value_t::tag::real:
                    python_args.append(arg.as_real());
                    break;
                case Value_t::tag::frag_ref:
                    python_args.append(
                        bp::object(boost::ref(*arg.as_frag_ref().ptr))
                    );
                    break;
                case Value_t::tag::list_ref:
                    python_args.append(boost::ref(arg.as_list_ref().ptr));
                    break;
                case Value_t::tag::regex:
                    throw std::runtime_error(
                        "regex values are not supported yet"
                    );
                    break;
                case Value_t::tag::undefined:
                    python_args.append(bp::object());
                    break;
                }
            }
            python_args.reverse();
            auto python_result = (*callback)(*bp::tuple(python_args));
            if (python_result.is_none())
                return Value_t();
            bp::extract<Teng::IntType_t> int_value(python_result);
            if (int_value.check())
                return Value_t(int_value());
            bp::extract<double> real_value(python_result);
            if (real_value.check())
                return Value_t(real_value());
            bp::extract<std::string> string_value(python_result);
            if (string_value.check())
                return Value_t(string_value());
            throw std::runtime_error(
                "result type must be one of {int, float, string}"
            );

        } catch (const bp::error_already_set &) {
            throw std::runtime_error(extract_exc());
        }
    }

protected:
    std::string name;
    bp::object *callback;
};

class PyWriter_t: public Writer_t {
public:
    PyWriter_t(bp::object writer)
        : writer(writer),
          write_string(writer.attr("write")),
          write_slice(writer.attr("write_slice")),
          flush_writer(writer.attr("flush"))
    {}

    int write(const std::string &str) override {
        try {
            return bp::extract<int>(write_string(str));
        } catch (const bp::error_already_set &) {
            throw std::runtime_error(extract_exc());
        }
    }

    int write(const char *str) override {
        try {
            return bp::extract<int>(write_string(str));
        } catch (const bp::error_already_set &) {
            throw std::runtime_error(extract_exc());
        }
    }

    int write(const char *str, std::size_t size) override {
        return write(std::string(str, size));
    }

    int write(
        const std::string &str,
        std::pair<std::string::const_iterator, std::string::const_iterator> intv
    ) override {
        try {
            auto from = intv.first - str.begin();
            auto to = intv.second - str.begin();
            return bp::extract<int>(write_slice(str, from, to));
        } catch (const bp::error_already_set &) {
            throw std::runtime_error(extract_exc());
        }
    }

    int flush() override {
        try {
            return bp::extract<int>(flush_writer());
        } catch (const bp::error_already_set &) {
            throw std::runtime_error(extract_exc());
        }
    }

    bp::object writer;
    bp::object write_string;
    bp::object write_slice;
    bp::object flush_writer;
};

class PyStringWriter_t: public StringWriter_t {
public:
    PyStringWriter_t(const PyStringWriter_t &) = delete;
    PyStringWriter_t &operator=(const PyStringWriter_t &) = delete;
    PyStringWriter_t(): StringWriter_t(output) {}
    std::string output;
};

static bp::object Teng__init__(bp::tuple args, bp::dict kwargs) {
    auto self = args[0];

    // parse args
    std::string root
        = bp::extract<std::string>(bp::len(args) > 1?
            args[1]: kwargs.get("root", ""));
    // int64_t log_to_output
    //     = bp::extract<int64_t>(bp::len(args) > 4?
    //         args[4]: kwargs.get("logToOutput", 0));
    // int64_t error_fragment
    //     = bp::extract<int64_t>(bp::len(args) > 5?
    //         args[5]: kwargs.get("errorFragment", 0));
    // int64_t validate
    //     = bp::extract<int64_t>(bp::len(args) > 6?
    //         args[6]: kwargs.get("validate", 0));
    uint32_t temp_cache_size
        = bp::extract<int32_t>(bp::len(args) > 7?
            args[7]: kwargs.get("templateCacheSize", 0));
    uint32_t dict_cache_size
        = bp::extract<int32_t>(bp::len(args) > 8?
            args[8]: kwargs.get("dictionaryCacheSize", 0));

    // set some defaults
    self.attr("defaultEncoding")
        = bp::len(args) > 2? args[2]: kwargs.get("encoding", "utf-8");
    self.attr("defaultContentType")
        = bp::len(args) > 3? args[3]: kwargs.get("contentType", "");

    // call the real constructor
    Teng_t::Settings_t settings(temp_cache_size, dict_cache_size);
    return self.attr("__init__")(root, settings);
}

bp::object Teng_createDataRoot(Teng_t &) {
    return bp::object(std::make_shared<Fragment_t>());
}

bp::object Teng_dictionaryLookup(
    Teng_t &self,
    const std::string &params_file,
    const std::string &dic_file,
    const std::string &language,
    const std::string &key
) {
    if (auto *s = self.dictionaryLookup(params_file, dic_file, language, key))
        return bp::object(*s);
    return bp::object();
}

int Teng_generatePage_filename(
    Teng_t &self,
    const std::string &templateFilename,
    const bp::object &skin,
    const bp::object &dict,
    const bp::object &lang,
    const bp::object &param,
    const bp::object &contentType,
    const bp::object &encoding,
    const Fragment_t &data,
    bp::object pywriter,
    Error_t &err
) {
    GenPageArgs_t args;
    args.templateFilename = templateFilename;
    args.skin = get_string(skin);
    args.dictFilename = get_string(dict);
    args.lang = get_string(lang);
    args.paramsFilename = get_string(param);
    args.encoding = get_string(encoding);
    args.contentType = get_string(contentType);

    auto string_writer = bp::extract<PyStringWriter_t &>(pywriter);
    if (string_writer.check())
        return self.generatePage(args, data, string_writer(), err);

    auto file_writer = bp::extract<FileWriter_t &>(pywriter);
    if (file_writer.check())
        return self.generatePage(args, data, file_writer(), err);

    PyWriter_t writer(pywriter);
    return self.generatePage(args, data, writer, err);
}

int Teng_generatePage_string(
    Teng_t &self,
    const std::string &templateString,
    const bp::object &dict,
    const bp::object &lang,
    const bp::object &param,
    const bp::object &contentType,
    const bp::object &encoding,
    const Fragment_t &data,
    bp::object pywriter,
    Error_t &err
) {
    GenPageArgs_t args;
    args.templateString = templateString;
    args.dictFilename = get_string(dict);
    args.lang = get_string(lang);
    args.paramsFilename = get_string(param);
    args.encoding = get_string(encoding);
    args.contentType = get_string(contentType);

    auto string_writer = bp::extract<PyStringWriter_t &>(pywriter);
    if (string_writer.check())
        return self.generatePage(args, data, string_writer(), err);

    auto file_writer = bp::extract<FileWriter_t &>(pywriter);
    if (file_writer.check())
        return self.generatePage(args, data, file_writer(), err);

    PyWriter_t writer(pywriter);
    return self.generatePage(args, data, writer, err);
}

void fragmentlist_add_variable(
    FragmentList_t &self,
    bp::object obj
) {
    if (obj.is_none()) self.addValue("");
    else self.addValue(bp::extract<std::string>(bp::str(obj)));
}

void fragment_add_variable(
    Fragment_t &self,
    const std::string &name,
    bp::object obj
) {
    if (obj.is_none()) self.addVariable(name, "");
    else self.addVariable(name, bp::extract<std::string>(bp::str(obj)));
}

std::string fragment_to_string(const Fragment_t &self) {
    std::stringstream os;
    self.dump(os);
    return os.str();
}

std::string fragment_list_to_string(const FragmentList_t &self) {
    std::stringstream os;
    self.dump(os);
    return os.str();
}

std::string error_to_string(const Error_t &self) {
    std::stringstream os;
    self.dump(os);
    return os.str();
}

bp::object fragment_get_item(const Fragment_t &self, bp::object name) {
    bp::extract<std::string> string_value(name);
    if (string_value.check()) {
        auto iitem = self.find(string_value());
        if (iitem != self.end())
            return bp::object(iitem->second);
    }
    PyErr_SetString(PyExc_KeyError, "key not found");
    bp::throw_error_already_set();
    throw std::runtime_error(__PRETTY_FUNCTION__);
}

bp::object fragment_list_get_item(const FragmentList_t &self, std::size_t i) {
    if (i < self.size())
        return bp::object(self[i]);
    PyErr_SetString(PyExc_IndexError, "index is out of range");
    bp::throw_error_already_set();
    throw std::runtime_error(__PRETTY_FUNCTION__);
}

bool fragment_has_item(const Fragment_t &self, bp::object name) {
    bp::extract<std::string> string_value(name);
    if (string_value.check()) {
        auto iitem = self.find(string_value());
        if (iitem != self.end())
            return true;
    }
    return false;
}

static bp::list listSupportedContentTypes() {
    bp::list content_types;
    auto results = Teng_t::listSupportedContentTypes();
    for (auto &result: results)
        content_types.append(bp::make_tuple(result.first, result.second));
    return content_types;
}

static bool registerUdf(const std::string &name, bp::object callback) {
    if (!PyCallable_Check(callback.ptr())) {
        PyErr_SetString(PyExc_ValueError, "Second param must be callable");
        return false;
    }
    if (Teng::udf::findFunction("udf." + name)) return false;
    Teng::udf::registerFunction(name, UDFWrapper_t(name, callback));
    return true;
}

} // namespace

namespace Teng {

bool
operator==(const Error_t::ErrorPos_t &lhs, const Error_t::ErrorPos_t &rhs) {
    return lhs.lineno == rhs.lineno
        && lhs.colno == rhs.colno
        && lhs.filename == rhs.filename;
}

} // namespace Teng

BOOST_PYTHON_MODULE(rawteng) {
    // teng settings class
    bp::class_<Teng_t::Settings_t>
        ("Settings", bp::no_init)
        ;

    // teng error class
    bp::class_<Error_t, boost::noncopyable>
        ("Error", bp::init<>())
        .def("entries", &Error_t::getEntries)
        .def("__repr__", &error_to_string)
        ;

    bp::class_<Error_t::Entry_t>
        ("ErrorEntry", bp::no_init)
        .add_property("level",
                +[] (const Error_t::Entry_t &self) { return int(self.level);})
        .add_property("filename",
                +[] (const Error_t::Entry_t &self) { return self.pos.filename;})
        .add_property("line",
                +[] (const Error_t::Entry_t &self) { return self.pos.lineno;})
        .add_property("column",
                +[] (const Error_t::Entry_t &self) { return self.pos.colno;})
        .add_property("message",
                +[] (const Error_t::Entry_t &self) { return self.msg;})
        .def("__repr__",
             &Error_t::Entry_t::getLogLine)
        ;

    // list of errors
    bp::class_<std::vector<Error_t::Entry_t>>
        ("ErrorEntries", bp::no_init)
        .def(bp::vector_indexing_suite<std::vector<Error_t::Entry_t>, false>())
        ;

    // teng writers class
    bp::class_<FileWriter_t, boost::noncopyable>
        ("FileWriter", bp::init<std::string>())
        .def("dump", +[] (const FileWriter_t &) {return std::string();})
        ;
    bp::class_<PyStringWriter_t, boost::noncopyable>
        ("StringWriter", bp::init<>())
        .def("dump", +[] (const PyStringWriter_t &self) {return self.output;})
        ;

    // teng fragment class
    bp::class_<FragmentList_t, boost::noncopyable>
        ("FragmentList", bp::no_init)
        .def("_addFragment",
            &FragmentList_t::addFragment, bp::return_internal_reference<>())
        .def("_addFragmentList",
            &FragmentList_t::addFragmentList, bp::return_internal_reference<>())
        .def("addVariable",
            &FragmentList_t::addIntValue)
        .def("addVariable",
            &FragmentList_t::addRealValue)
        .def("addVariable",
            &FragmentList_t::addStringValue)
        .def("addVariable",
            &fragmentlist_add_variable)
        .def("__getitem__",
             &fragment_list_get_item)
        .def("__len__",
             &FragmentList_t::size)
        .def("__iter__",
             bp::iterator<FragmentList_t>())
        .def("__repr__",
            &fragment_list_to_string)
        ;

    // teng fragment class
    bp::class_<Fragment_t, std::shared_ptr<Fragment_t>, boost::noncopyable>
        ("Fragment", bp::no_init)
        .def("_addFragment",
            &Fragment_t::addFragment, bp::return_internal_reference<>())
        .def("_addFragmentList",
            &Fragment_t::addFragmentList, bp::return_internal_reference<>())
        .def("addVariable",
            &Fragment_t::addStringVariable)
        .def("addVariable",
            &Fragment_t::addRealVariable)
        .def("addVariable",
            &Fragment_t::addIntVariable)
        .def("addVariable",
            &fragment_add_variable)
        .def("__getitem__",
             &fragment_get_item)
        .def("__contains__",
             &fragment_has_item)
        .def("__len__",
             &Fragment_t::size)
        .def("__iter__",
             bp::iterator<Fragment_t>())
        .def("__repr__",
            &fragment_to_string)
        ;

    // std::pair<const std::string, FragmentValue_t> -> tuple
    bp::to_python_converter<
        fragment_pair_to_tuple_t::pair_t,
        fragment_pair_to_tuple_t,
        true
    >();

    // FragmentValue_t -> python
    bp::to_python_converter<
        fragment_value_to_python_t::value_t,
        fragment_value_to_python_t
    >();

    // teng class
    bp::class_<Teng_t, boost::noncopyable>
        ("Teng", bp::no_init)
        .def("__init__", bp::raw_function(Teng__init__))
        .def(bp::init<std::string, Teng_t::Settings_t>())
        .def("_createDataRoot", Teng_createDataRoot)
        .def("_dictionaryLookup", Teng_dictionaryLookup)
        .def("_generatePage", Teng_generatePage_filename)
        .def("_generatePage", Teng_generatePage_string)
        ;

    // functions
    bp::def("listSupportedContentTypes", &listSupportedContentTypes);
    bp::def("registerUdf", &registerUdf);
}

