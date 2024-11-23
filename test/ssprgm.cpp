#include <stdint.h>
#include "square.h"
#include "vkrt.h"
#include <iostream>
#include <vector>

// glslangValidator.exe --target-env vulkan1.3 --vn square -o .\test\square.h .\test\square.comp 

int main()
{
    vkrt::Instance inst;
    auto &dev0 = inst.get_device(0);
    auto &dev1 = inst.get_device(1);
    size_t size = static_cast<size_t>(4096) * 4096 * 32; 
    auto buf = vkrt::StorageBuffer(dev1, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
    auto buf1 = vkrt::StorageBuffer(dev1, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);

    //auto buf2 = vkrt::StorageBuffer(dev1, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, false);
    //auto buf3 = vkrt::StorageBuffer(dev1, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, false);
    size_t total_elems = size / sizeof(float);
    std::vector<float> input_data(total_elems, 2.f);
    std::memcpy(buf.data(), input_data.data(), size );
    std::vector<uint32_t> code(square, square + (sizeof(square) / sizeof(uint32_t)));
      
    std::vector<vkrt::StorageBuffer> bufs = {buf, buf1};

    auto cpt = vkrt::ComputeProgram(dev1, code, bufs);
    cpt.record(total_elems / 1024, 1, 1);
    
    input_data.clear();
    
    cpt.wait();

    std::vector<float> output_data(total_elems, 0.f);  
    std::memcpy(output_data.data(), buf1.data(), size);
    size_t i;
    for ( i = 0; i < output_data.size(); ++i)
    {
        if (output_data[i] != 4.f)
        {
            std::cout << "ERROR: " << i << " " << output_data[i] << std::endl;            
        }
    }



    return 0;
}
