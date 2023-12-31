#include <algorithm>
#include <clang-c/CXErrorCode.h>
#include <iostream>
#include <clang-c/Index.h>
#include <fstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <unistd.h>
#include <dlfcn.h>
#include <link.h>
#include <inja/inja.hpp>
#include "colors.h"
#include "inja/template.hpp"
#include "nlohmann/json_fwd.hpp"
#include <filesystem>

#include <boost/program_options.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>


using namespace inja;
using json = nlohmann::json;

// the structure holding a function declaration
struct function_decl
{
    std::string return_type;
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::string> parameter_names;
    std::string mangled_name;
    std::string filename;
    int linenumber;
};

// a structure declaration (struct or class)
struct struct_declaration
{
    std::string type;       // struct or class
    std::string name;       // the name of the struct
    std::string headerfile; // the headerfile in which it is declared
};

namespace
{
std::string fn  = "";                                   // the filename
std::vector<std::string> funs;                          // the functions as loaded by libelf
std::vector<struct_declaration> forward_declarations;   // forward declarations for proxy classes of classes that will be needed in case some header declares a class
std::vector<function_decl> declarations;                // these will be worked on in the end
}

template <typename T>
std::string join(const T& v, const std::string& delim)
{
    std::ostringstream s;
    for (const auto& i : v)
    {
        if (&i != &v[0])
        {
            s << delim;
        }
        s << i;
    }
    return s.str();
}

std::string to_string(CXLinkageKind k)
{
    switch(k)
    {
    case CXLinkage_Invalid: return "invalid";
    case CXLinkage_NoLinkage: return "nolinkage";
    case CXLinkage_Internal: return "internal";
    case CXLinkage_UniqueExternal: return "uniqext";
    case CXLinkage_External: return "external";
    default: return "unknown";
    };
}

std::string to_string(const CXString &s)
{
    std::string result = clang_getCString(s);
    clang_disposeString(s);
    return result;
}

function_decl resolve_function_declaration(CXCursor cursor, const std::string& file, int line)
{
    auto type = clang_getCursorType(cursor);

    auto cs = clang_getCursorSpelling(cursor);
    auto function_name = to_string(cs);
    auto rt = clang_getResultType(type);
    auto ts = clang_getTypeSpelling(rt);
    auto return_type = to_string(ts);
    auto linkage = clang_getCursorLinkage(cursor);

    if(linkage != CXLinkage_External)
    {
        return {};
    }

    int num_args = clang_Cursor_getNumArguments(cursor);
    int arg_ctr = 0;
    std::vector<std::string> f_args, nm_args;
    for (int i = 0; i < num_args; ++i)
    {
        auto arg_cursor = clang_Cursor_getArgument(cursor, i);
        auto arg_name = to_string(clang_getCursorSpelling(arg_cursor));
        if (arg_name.empty())
        {
            arg_name = "arg_" + std::to_string(++arg_ctr);
        }

        auto arg_data_type = to_string(clang_getTypeSpelling(clang_getArgType(type, i)));

        f_args.push_back(arg_data_type);
        nm_args.push_back(arg_name);
    }

    auto it = std::find_if(funs.begin(), funs.end(), [function_name](const std::string& n) -> bool {
        if( n.find(function_name) != std::string::npos )
        {
            return true;
        }
        return false;
    });

    std::string mangled;

    if(it != funs.end())
    {
        mangled = *it;
    }

    return {return_type, function_name, f_args, nm_args, mangled, file, line};
}

void print_diagnostics(CXTranslationUnit translationUnit)
{
    int nbDiag = clang_getNumDiagnostics(translationUnit);

    bool foundError = false;
    for (unsigned int currentDiag = 0; currentDiag < nbDiag; ++currentDiag)
    {
        CXDiagnostic diagnotic = clang_getDiagnostic(translationUnit, currentDiag);
        CXString errorString = clang_formatDiagnostic(diagnotic,clang_defaultDiagnosticDisplayOptions());
        std::string tmp{clang_getCString(errorString)};
        clang_disposeString(errorString);
        if (tmp.find("error:") != std::string::npos)
        {
            foundError = true;
        }
        std::cerr << tmp << std::endl;
    }
    if (foundError)
    {
        std::cerr << "Please resolve these issues and try again." <<std::endl;
        exit(-1);
    }
}

std::ostream& operator<<(std::ostream& stream, const CXString& str)
{
    stream << clang_getCString(str);
    clang_disposeString(str);
    return stream;
}

auto ClangVisitor = [](CXCursor cursor, CXCursor parent, CXClientData client_data) -> CXChildVisitResult
{
    auto kind = clang_getCursorKindSpelling(clang_getCursorKind(cursor));
    auto ck = clang_getCursorKind(cursor);

    auto d = clang_getCursorLocation(cursor);
    CXFile file;
    unsigned line;
    unsigned column;
    unsigned offset;
    clang_getSpellingLocation(d, &file, &line, &column, &offset);

    if (CXCursor_FunctionDecl == ck)
    {
        if(to_string(clang_File_tryGetRealPathName(file)) == fn)
        {
            function_decl fdc = resolve_function_declaration(cursor, to_string(clang_getFileName(file)), line);
            if(!fdc.name.empty())
            {
                declarations.push_back(fdc);
            }
        }
    }

    if(CXCursor_ClassDecl == ck)
    {
        if(to_string(clang_File_tryGetRealPathName(file)) == fn)
        {
            auto cs = clang_getCursorSpelling(cursor);
            auto name = to_string(cs);

            forward_declarations.push_back({"class", name});
        }
    }

    if(CXCursor_StructDecl == ck)
    {
        if(to_string(clang_File_tryGetRealPathName(file)) == fn)
        {
            auto cs = clang_getCursorSpelling(cursor);
            auto name = to_string(cs);

            forward_declarations.push_back({"struct", name, fn});
        }
    }

    return CXChildVisit_Recurse;
};

std::vector<std::string> list_functions(const char *libName)
{
    std::vector<std::string> result;

    auto library = dlopen(libName, RTLD_LAZY | RTLD_GLOBAL);
    if(!library)
    {
        std::cerr << "Cannot open " << libName << ". " << dlerror() << std::endl;
        return result;
    }

    struct link_map * symbol_map = nullptr;
    int dli_result = dlinfo(library, RTLD_DI_LINKMAP, &symbol_map);
    if(dli_result == -1 || symbol_map == nullptr)
    {
        std::cerr << "Cannot obtain library information from " << libName << ". " << dlerror() << std::endl;
        return result;
    }

    Elf64_Sym * symtab = nullptr;
    char * strtab = nullptr;
    int symentries = 0;
    for (auto section = symbol_map->l_ld; section->d_tag != DT_NULL; ++section)
    {
        if (section->d_tag == DT_SYMTAB)
        {
            symtab = (Elf64_Sym *)section->d_un.d_ptr;
        }
        if (section->d_tag == DT_STRTAB)
        {
            strtab = (char*)section->d_un.d_ptr;
        }
        if (section->d_tag == DT_SYMENT)
        {
            symentries = section->d_un.d_val;
        }
    }

    if(symentries == 0)
    {
        std::cerr << "Cannot obtain library information from " << libName << ". " << dlerror() << std::endl;
        return result;
    }

    unsigned long size = strtab - (char *)symtab;
    for (int k = 0; k < size / symentries; ++k)
    {
        auto sym = &symtab[k];
        if(sym && ELF64_ST_TYPE(symtab[k].st_info) == STT_FUNC)
        {
            auto str = &strtab[sym->st_name];
            if(str != nullptr)
            {
                result.push_back(std::string {str});
            }
        }
    }

    return result;
}

int main(int argc, char *argv[])
{
    for(int i=0; i<argc; i++)
    {
        std::cerr << argv[i] << " ";
    }
    std::cerr << std::endl;

    namespace po = boost::program_options;

    po::options_description desc("libby-parser (c) 2023 fritzone.\n\nUsage");
    desc.add_options()
        ("help", "produce this help message")
        ("library", po::value<std::string>(), "the compiled library")
        ("libname", po::value<std::string>(), "the name of the library")
        ("compilationdb", po::value<std::string>(), "the compilation library")
        ("proxy_header", po::value<std::string>(), "the location where the header template for the proxy class is read from")
        ("proxy_source", po::value<std::string>(), "the location where the source template for the proxy class is read from")
        ("target_dir", po::value<std::string>(), "the location where the generated proxy header and source files will be written to")
    ;

    po::variables_map vm;

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("input-file", po::value< std::vector<std::string> >(), "input file")
        ;

    po::options_description cmdline_options;
    cmdline_options.add(desc).add(hidden);
    po::positional_options_description p;
    p.add("input-file", -1);
    desc.add(hidden);

    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }

    std::vector<std::string> input_files;   // these will be the source files that will be parsed
    if (vm.count("input-file"))
    {
        input_files = vm["input-file"].as< std::vector<std::string> >();
    }

    if(vm.count("library") != 1)
    {
        std::cerr << "Missing the library to read the symbols from (--library) " << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    if(vm.count("compilationdb") != 1)
    {
        std::cerr << "Missing the compilation database (cmake genereated compile_commands.json) to read the compile options from (--compilationdb)" << std::endl;
        std::cerr << "Make sure the line "<<BOLD(FBLU("set(CMAKE_EXPORT_COMPILE_COMMANDS ON)")) << "is in your main CMakeLists.txt" << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    if(vm.count("libname") != 1)
    {
        std::cerr << "Missing the library name to read the symbols from (--libname)" << std::endl;
        std::cout << desc;
        return 1;
    }

    if(vm.count("proxy_header") != 1)
    {
        std::cerr << "Missing the location of the proxy header template (--proxy_header)" << std::endl;
        std::cout << desc;
        return 1;
    }

    if(vm.count("proxy_source") != 1)
    {
        std::cerr << "Missing the location of the proxy source template (--proxy_souirce)" << std::endl;
        std::cout << desc;
        return 1;
    }

    if(vm.count("target_dir") != 1)
    {
        std::cerr << "Missing the location of the target directory (--target_dir)" << std::endl;
        std::cout << desc;
        return 1;
    }

    std::string libname = vm.at("libname").as<std::string>();
    funs = list_functions(vm.at("library").as<std::string>().c_str());
    auto compile_db = vm["compilationdb"].as<std::string>();
    std::string header_location = vm["proxy_header"].as<std::string>();
    std::string source_location = vm["proxy_source"].as<std::string>();
    std::string target_dir = vm["target_dir"].as<std::string>();

    for(const auto& input_file : input_files)
    {

        std::ifstream f(compile_db);
        nlohmann::json data = json::parse(f);

        for (json::iterator it = data.begin(); it != data.end(); ++it)
        {
            fn = (*it)["file"];

            if(fn != input_file)
            {
                continue;
            }

            std::string scmdline = (*it)["command"];
            std::vector<std::string> words;

            boost::split(words, scmdline, boost::is_any_of(", "), boost::token_compress_on);
            words.erase(words.begin()); // the first one is the compile command

            // remove "-c" and "-o" because they seem to mess up life
            std::vector<std::string> finalWords;
            auto wit = words.begin();
            while(wit != words.end())
            {
                if(*wit == "-c" || *wit == "-o")
                {
                    // skip the option
                    wit++;

                    // skip the value
                    wit ++;
                }
                else
                {
                    finalWords.push_back(*wit);
                    wit ++;
                }
            }

            std::vector<const char*> cstrings;
            cstrings.reserve(finalWords.size());

            std::transform(
                finalWords.begin(),
                finalWords.end(),
                std::back_inserter(cstrings),
                [] (const auto& string) { return string.c_str();}
                );

            CXIndex index = clang_createIndex(0, 0);

            CXTranslationUnit unit;
            CXErrorCode ec = clang_parseTranslationUnit2(
                index,
                fn.c_str(),
                cstrings.data(),
                cstrings.size(),
                nullptr,
                0,
                CXTranslationUnit_KeepGoing,
                &unit
            );


            if (unit == nullptr)
            {
                std::cerr << "Unable to parse:" << fn <<". ("  << ec << ")" <<  std::endl;
                exit(-1);
            }

            if(ec != CXError_Success)
            {
                std::cerr << "Error while parsing:" << fn << ec << std::endl;
                print_diagnostics(unit);
                exit(-1);
            }

            CXCursor cursor = clang_getTranslationUnitCursor(unit);
            clang_visitChildren(cursor, ClangVisitor, nullptr);

            clang_disposeTranslationUnit(unit);
            clang_disposeIndex(index);
        }
    }


    nlohmann::json j;

    j["functions"] = {};

    // format the parameters for calling the function pointer

    for(const auto& d : declarations)
    {
        nlohmann::json j_f;
        j_f["name"] = d.name;
        j_f["mangled"] = d.mangled_name;
        j_f["return_type"] = d.return_type;
        j_f["parameters"] = join(d.parameters, ",");
        j_f["call"] = join(d.parameter_names, ",");
        j_f["filename"] = d.filename;
        j_f["line"] = d.linenumber;

        if(d.parameters.size() != d.parameter_names.size())
        {
            std::cerr << "Some error in function declaration " << d.name << std::endl;
            return 1;
        }

        std::string f_def = "";
        for(size_t i=0; i<d.parameters.size(); i++)
        {
            f_def = f_def + d.parameters.at(i) + " " + d.parameter_names.at(i);
            if(i < d.parameters.size() - 1)
            {
                f_def += ", ";
            }
        }
        j_f["fundef"] = f_def;


        j["functions"].push_back(j_f);
    }

    j["libname"] = libname;

    j["forwards"] = {};
    for(const auto&[t,n,h] : forward_declarations)
    {
        nlohmann::json j_fd;
        j_fd["name"] = n;
        j_fd["type"] = t;
        j_fd["header"] = h;

        j["forwards"].push_back(j_fd);
    }

    // render the header file
    Environment env;
    Template header_temp = env.parse_template(header_location);
    std::string header_result = env.render(header_temp, j);

    // create the directory
    boost::filesystem::path dir(target_dir);
    if(boost::filesystem::create_directories(dir))
    {
        std::cout<< "Directory Created: " << target_dir <<std::endl;
    }

    // write the header file
    std::string h_dir = target_dir + "/" + libname +"_proxy_header.h";
    std::cout << h_dir << std::endl;
    std::ofstream h_out(h_dir, std::ios::out | std::ios::in | std::ios::trunc);
    h_out << header_result;

    // render the CPP file
    Template source_temp = env.parse_template(source_location);
    std::string source_result = env.render(source_temp, j);

    std::string s_dir = target_dir + "/" + libname + "_proxy_source.cpp";
    std::cout << s_dir << std::endl;
    std::ofstream s_out(s_dir, std::ios::out | std::ios::in | std::ios::trunc);
    s_out << source_result;
}
