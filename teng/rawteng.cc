/*
 * FILE             $Id: $
 *
 * LICENCE          MIT
 *
 * DESCRIPTION      Teng C++ wrapper.
 *
 * AUTHOR           Michal Bukovsky <burlog@seznam.cz>
 */

#include <teng.h>
#include <tengudf.h>
#include <tengstructs.h>
#include <boost/python.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

using Teng::Teng_t;
using Teng::Fragment_t;
using Teng::IntType_t;
using Teng::Error_t;
using Teng::Writer_t;
using Teng::FileWriter_t;
using Teng::StringWriter_t;
using Teng::UDFValue_t;

namespace bp = boost::python;

namespace {

class UDFWrapper_t {
public:
    UDFWrapper_t(const std::string &name, bp::object callback)
        : name(name), callback(new bp::object(callback))
    {}

    UDFValue_t operator()(const std::vector<UDFValue_t> &args) const {
        bp::list python_args;
        for (auto &arg: args) {
            switch (arg.getType()) {
            case UDFValue_t::Integer:
                python_args.append(arg.getInt());
                break;
            case UDFValue_t::Real:
                python_args.append(arg.getReal());
                break;
            case UDFValue_t::String:
                python_args.append(bp::str(arg.getString()));
                break;
            default:
                python_args.append(0);
                break;
            }
        }
        try {
            auto python_result = (*callback)(*bp::tuple(python_args));
            bp::extract<Teng::IntType_t> int_value(python_result);
            if (int_value.check())
                return UDFValue_t(int_value());
            bp::extract<double> real_value(python_result);
            if (real_value.check())
                return UDFValue_t(real_value());
            bp::extract<std::string> string_value(python_result);
            if (string_value.check())
                return UDFValue_t(string_value());
            throw std::runtime_error(
                "result type must be one of {int, float, string}"
            );

        } catch (const bp::error_already_set &) {
            PyObject *exc, *value, *traceback;
            PyErr_Fetch(&exc, &value, &traceback);
            bp::handle<> exc_handle(exc);
            bp::handle<> value_handle(bp::allow_null(value));
            if (!exc_handle) throw std::runtime_error(__PRETTY_FUNCTION__);
            throw std::runtime_error(
                bp::extract<std::string>(bp::str(exc_handle))()
                + ": " + bp::extract<std::string>(bp::str(value_handle))()
            );
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
        return bp::extract<int>(write_string(str));
    }

    int write(const char *str) override {
        return bp::extract<int>(write_string(str));
    }

    int write(
        const std::string &str,
        std::pair<std::string::const_iterator,
        std::string::const_iterator> interval
    ) override {
        auto begin = interval.first - str.begin();
        auto end = interval.second - str.begin();
        return bp::extract<int>(write_slice(str, begin, end));
    }

    int flush() override {
        return bp::extract<int>(flush_writer());
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
            args[1]: kwargs["root"]);
    int64_t log_to_output
        = bp::extract<int64_t>(bp::len(args) > 4?
            args[4]: kwargs.get("logToOutput", 0));
    int64_t error_fragment
        = bp::extract<int64_t>(bp::len(args) > 5?
            args[5]: kwargs.get("errorFragment", 0));
    int64_t validate
        = bp::extract<int64_t>(bp::len(args) > 6?
            args[6]: kwargs.get("validate", 0));
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
    Teng_t::Settings_t settings(0, false, temp_cache_size, dict_cache_size);
    return self.attr("__init__")(root, settings);
}

bp::object Teng_createDataRoot(Teng_t &) {
    return bp::object(std::make_shared<Fragment_t>());
}

bp::object Teng_dictionaryLookup(Teng_t &self,
                                 const std::string &configFilename,
                                 const std::string &dictFilename,
                                 const std::string &language,
                                 const std::string &key)
{
    std::string s;
    if (self.dictionaryLookup(configFilename, dictFilename, language, key, s))
        return bp::object();
    return bp::object(s);
}

int Teng_generatePage_filename(Teng_t &self,
                               const std::string &templateFilename,
                               const std::string &skin,
                               const std::string &dict,
                               const std::string &lang,
                               const std::string &param,
                               const std::string &contentType,
                               const std::string &encoding,
                               const Fragment_t &data,
                               bp::object pywriter,
                               Error_t &err)
{
    auto generate_page = [&] (Writer_t &writer) {
        return self.generatePage(templateFilename, skin, dict, lang, param,
                                 contentType, encoding, data, writer, err);
    };
    auto string_writer = bp::extract<PyStringWriter_t &>(pywriter);
    if (string_writer.check()) return generate_page(string_writer());
    auto file_writer = bp::extract<FileWriter_t &>(pywriter);
    if (file_writer.check()) return generate_page(file_writer());
    PyWriter_t writer(pywriter);
    return generate_page(writer);
}

int Teng_generatePage_string(Teng_t &self,
                             const std::string &templateString,
                             const std::string &dict,
                             const std::string &lang,
                             const std::string &param,
                             const std::string &contentType,
                             const std::string &encoding,
                             const Fragment_t &data,
                             bp::object pywriter,
                             Error_t &err)
{
    auto generate_page = [&] (Writer_t &writer) {
        return self.generatePage(templateString, dict, lang, param,
                                 contentType, encoding, data, writer, err);
    };
    auto string_writer = bp::extract<PyStringWriter_t &>(pywriter);
    if (string_writer.check()) return generate_page(string_writer());
    auto file_writer = bp::extract<FileWriter_t &>(pywriter);
    if (file_writer.check()) return generate_page(file_writer());
    PyWriter_t writer(pywriter);
    return generate_page(writer);
}

void fragment_add_variable(Fragment_t &self,
                           const std::string &name,
                           bp::object obj)
{
    if (obj.is_none()) self.addVariable(name, "");
    else self.addVariable(name, bp::extract<std::string>(bp::str(obj)));
}

std::string fragment_to_string(const Fragment_t &self) {
    std::stringstream os;
    self.dump(os);
    return os.str();
}

std::string error_to_string(const Error_t &self) {
    std::stringstream os;
    self.dump(os);
    return os.str();
}

static bp::list listSupportedContentTypes() {
    bp::list content_types;
    std::vector<std::pair<std::string, std::string>> results;
    Teng_t::listSupportedContentTypes(results);
    for (auto &result: results)
        content_types.append(bp::make_tuple(result.first, result.second));
    return content_types;
}

static bool registerUdf(const std::string &name, bp::object callback) {
    if (!PyCallable_Check(callback.ptr())) {
        PyErr_SetString(PyExc_ValueError, "Second param must be callable");
        throw bp::error_already_set();
    }
    if (Teng::findUDF("udf." + name)) return false;
    Teng::registerUDF(name, UDFWrapper_t(name, callback));
    return true;
}

template <typename var_t>
using av_t = void (Fragment_t::*)(const std::string &, var_t);

} // namespace

namespace Teng {

bool
operator==(const Error_t::Position_t &lhs, const Error_t::Position_t &rhs) {
    return lhs.lineno == rhs.lineno
        && lhs.col == rhs.col
        && lhs.filename == rhs.filename;
}

bool
operator==(const Error_t::Entry_t &lhs, const Error_t::Entry_t &rhs) {
    return lhs.level == rhs.level
        && lhs.pos == rhs.pos
        && lhs.message == rhs.message;
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
        .def("entries",
            &Error_t::getEntries, bp::return_internal_reference<>())
        .def("__repr__",
            &error_to_string)
        ;

    bp::class_<Error_t::Entry_t, boost::noncopyable>
        ("ErrorEntry", bp::no_init)
        .add_property("level",
                +[] (const Error_t::Entry_t &self) { return int(self.level);})
        .add_property("filename",
                +[] (const Error_t::Entry_t &self) { return self.pos.filename;})
        .add_property("line",
                +[] (const Error_t::Entry_t &self) { return self.pos.lineno;})
        .add_property("column",
                +[] (const Error_t::Entry_t &self) { return self.pos.col;})
        .add_property("message",
                +[] (const Error_t::Entry_t &self) { return self.message;})
        .def("__repr__",
                &Error_t::Entry_t::getLogLine)
        ;

    // list of errors
    bp::class_<std::vector<Error_t::Entry_t>, boost::noncopyable>
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
    bp::class_<Fragment_t, std::shared_ptr<Fragment_t>, boost::noncopyable>
        ("Fragment", bp::no_init)
        .def("_addFragment",
            &Fragment_t::addFragment, bp::return_internal_reference<>())
        .def("addVariable",
            &fragment_add_variable)
        .def("addVariable",
            static_cast<av_t<const std::string &>>(&Fragment_t::addVariable))
        .def("addVariable",
            static_cast<av_t<double>>(&Fragment_t::addVariable))
        .def("addVariable",
            static_cast<av_t<IntType_t>>(&Fragment_t::addVariable))
        .def("__repr__",
            &fragment_to_string)
        ;

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

