#include <mlopen/hipoc_program.hpp>
#include <mlopen/kernel.hpp>
#include <mlopen/errors.hpp>
#include <mlopen/replace.hpp>

#include <sstream>

#include <unistd.h>

namespace mlopen {

std::string quote(std::string s)
{
    return '"' + s + '"';
}

std::string get_thread_id()
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

void execute(std::string exe, std::string args)
{
    std::string cmd = exe + " " + args + " > /dev/null";
    // std::cout << cmd << std::endl;
    if (std::system(cmd.c_str()) != 0) MLOPEN_THROW("Can't execute " + cmd);
}

template<std::size_t N>
std::array<char, N> make_str_array(const char(&a)[N])
{
    std::array<char, N> result;
    std::copy(a, a+N, result.begin());
    return result;
}

struct tmp_dir
{
    std::string name;
    tmp_dir()
    {
        auto t = make_str_array("miopen-XXXXXX");
        name = mkdtemp(t.data());
    }

    void execute(std::string exe, std::string args)
    {
        std::string cd = "cd " + this->name + "; ";
        std::string cmd = cd + exe + " " + args + " > /dev/null";
        // std::cout << cmd << std::endl;
        if (std::system(cmd.c_str()) != 0) MLOPEN_THROW("Can't execute " + cmd);
    }

    ~tmp_dir()
    {
        if(!name.empty())
        {
            std::string cmd = "rm -rf " + name;
            if (std::system(cmd.c_str()) != 0) MLOPEN_THROW("Can't execute " + cmd);
        }
    }
};

struct tmp_file
{
    std::string name;
    tmp_file() = delete;
    tmp_file(const tmp_file&)=delete;
    tmp_file(std::string s)
    : name("/tmp/" + s + "." + get_thread_id() + "." + std::to_string(std::time(nullptr)))
    {
    }

    ~tmp_file()
    {
        std::remove(name.c_str());
    }
};

void WriteFile(const std::string& content, const std::string& name)
{
    HIPOCProgram::FilePtr f{std::fopen(name.c_str(), "w")};
    if (std::fwrite(content.c_str(), 1, content.size(), f.get()) != content.size()) 
        MLOPEN_THROW("Failed to write to src file");
}

hipModulePtr CreateModule(const std::string& program_name, std::string params)
{   
    tmp_file src_file{program_name};
    std::string src = GetKernelSrc(program_name);

    WriteFile(src, src_file.name);
        
    std::string bin_file = src_file.name + ".bin";
    std::string hsaco_file = src_file.name + ".hsaco";
    std::string obj_file = src_file.name + ".obj";

#if 0
    execute(HIP_OC_COMPILER, 
        "-march=hsail64 -mdevice=Fiji -output=" + bin_file +  
        params + " " +
        src_file.name);

    execute("/usr/bin/objcopy", 
        "-I elf32-little -j .text -O binary -S " + bin_file +
        " " + hsaco_file
    );
#else
    execute(HIP_OC_COMPILER, 
        "-march=hsail64 -mdevice=Fiji -save-temps=dump -nobin " +  
        params + " " +
        src_file.name);
    execute(HIP_OC_FINALIZER,
        "-target=8:0:3 -hsail dump_0_Fiji.hsail -output=" + hsaco_file
    );
    std::remove("dump_0_Fiji.hsail");
#endif

    hipModule_t raw_m;
    auto status = hipModuleLoad(&raw_m, hsaco_file.c_str());
    hipModulePtr m{raw_m};
    if (status != hipSuccess) MLOPEN_THROW_HIP_STATUS(status, "Failed creating module");
    // TODO: Remove file on destruction
    // std::remove(obj_file.c_str());
    return m;
}

HIPOCProgram::HIPOCProgram()
{}
HIPOCProgram::HIPOCProgram(const std::string &program_name, std::string params)
{
    this->module = CreateModule(program_name, params);
}
}
