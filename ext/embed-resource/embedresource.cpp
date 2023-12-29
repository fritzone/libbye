#include <cstring>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <fpaq0.h>
#include <boost/program_options.hpp>

int main(int argc, char** argv)
{
    for(int i=0; i<argc; i++)
    {
        std::cerr << argv[i] << " ";
    }
    std::cerr << std::endl;

    namespace po = boost::program_options;

    po::options_description desc("libby-resourcer(c) 2023 fritzone.\n\nWill create a symbol.cpp file from the given resource.\n\nUsage");
    desc.add_options()
        ("help", po::value<std::string>(), "produce this help message")
        ("symbol", po::value<std::string>(), "the name of the symbol to be used")
        ("output", po::value<std::string>(), "the name of the output file")
        ("resource", po::value<std::string>(), "the name of the file to be embedded as a resource")
    ;

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }

    if(vm.count("symbol") != 1)
    {
        std::cerr << "Missing the name of the symbol to use (--symbol) " << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    if(vm.count("output") != 1)
    {
        std::cerr << "Missing the name of the output file to use (--output) " << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    if(vm.count("resource") != 1)
    {
        std::cerr << "Missing the resoruce file name to read into the generated symbols variable (--resource) " << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    std::string symbol_name =  vm.at("symbol").as<std::string>();
    std::string resource_file =  vm.at("resource").as<std::string>();
    std::string output_file_name =  vm.at("output").as<std::string>();

    std::cerr << "symbol_name:" << vm.at("symbol").as<std::string>() << std::endl << "resource_file:" << resource_file << std::endl << "output_file_name:" << output_file_name << std::endl;

    std::replace(symbol_name.begin(), symbol_name.end(), '.', '_');
    std::replace(symbol_name.begin(), symbol_name.end(), '-', '_');
    std::replace(symbol_name.begin(), symbol_name.end(), '/', '_');
    std::replace(symbol_name.begin(), symbol_name.end(), '\\', '_');

    std::ifstream ifs(resource_file, std::ios::binary);
    std::stringstream buffer_in;

    if(!ifs.is_open())
    {
        std::cerr << "failed to open " << resource_file << ": " << strerror(errno) << '\n';
        return 1;
    }
    buffer_in << ifs.rdbuf();

    std::string content = buffer_in.str();

    std::ofstream ofs;
    ofs.open(output_file_name);

    ofs << "#include <cstddef>" << std::endl;
    ofs << "namespace resourcer {" << std::endl;
    ofs << "const unsigned char resource_" << symbol_name << "[] = {" << std::endl << "    ";



    auto cr = compressor::compress(content);

    std::cout << "Original: "<<content.length() << " Compressed:" << cr.size();


    std::ofstream out("output.2", std::ios::out | std::ios::binary);
    out.write((char*)&cr[0], cr.size());
    out.close();

    size_t lineCount = 0;
    char c = 0;
    std::stringstream buffer(std::string(reinterpret_cast<const char*>(cr.data()), cr.size()));

    buffer.get(c);

    while (true)
    {
        if (buffer.eof())
        {
            break;
        }

        ofs << "0x" << std::hex << std::setw(2) << std::setfill('0') << (c & 0xff);

        buffer.get(c);
        if (buffer.eof())
        {
            break;
        }
        else
        {
            ofs << ", ";
        }

        if (++lineCount == 10)
        {
            ofs << std::endl << "    ";
            lineCount = 0;
        }
    }

    ofs << std::endl << "};" << std::endl;
    ofs << "const std::size_t resource_" << symbol_name << "_len = sizeof(resource_" << symbol_name << ");";
    ofs << std::endl << "}" << std::endl;

    ofs.close();
    ifs.close();

    return EXIT_SUCCESS;
}
