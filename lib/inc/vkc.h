#pragma once
#include <spirv_reflect.h>
#include <vector>

namespace vkrt {


class ComputeProgram
{
    VkPipeline m_pipeline = nullptr;
    VkPipelineLayout m_pipeline_layout = nullptr;

    VkShaderModule m_module;
    SpvReflectShaderModule m_spv_module = {};
    struct set
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{};
        std::vector<VkWriteDescriptorSet> writes{};
        VkDescriptorSetLayout layout = nullptr;
    };
    std::vector<VkDescriptorSet> m_descriptor_sets;
    std::shared_future<void> m_wait;
    std::vector<struct set> m_sets;
    Device *m_dev = nullptr;
    uint32_t m_program_id = UINT32_MAX;
    uint32_t m_pipeline_layout_id = UINT32_MAX;
    uint32_t m_pipeline_id = UINT32_MAX;

  public:
    ComputeProgram(Device &dev, const std::vector<uint32_t> &code, std::vector<StorageBuffer> &bufs);
    void record(uint32_t x, uint32_t y, uint32_t z);

    void wait()
    {
        m_wait.wait();
    }

};


ComputeProgram::ComputeProgram(Device &dev, const std::vector<uint32_t> &code, std::vector<StorageBuffer> &bufs)
{
    m_dev = &dev;
    m_dev->get_shader_module(code, &m_module, &m_program_id);

    SpvReflectResult result = spvReflectCreateShaderModule(code.size() * sizeof(uint32_t), code.data(), &m_spv_module);
    uint32_t count = 0;
    result = spvReflectEnumerateDescriptorSets(&m_spv_module, &count, NULL);
    std::vector<SpvReflectDescriptorSet *> sets(count);
    m_descriptor_sets.resize(count);
    m_sets.resize(count);
    result = spvReflectEnumerateDescriptorSets(&m_spv_module, &count, sets.data());
    size_t l = 0;
    for (size_t i = 0; i < sets.size(); ++i)
    {
        const SpvReflectDescriptorSet &refl_set = *(sets[i]);
        auto &set = m_sets[i];
        set.bindings.resize(refl_set.binding_count);
        set.writes.resize(refl_set.binding_count);
        for (size_t j = 0; j < refl_set.binding_count; ++j)
        {
            auto &binding = set.bindings[j];
            const SpvReflectDescriptorBinding &refl_binding = *(refl_set.bindings[j]);
            binding.binding = refl_binding.binding;
            binding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
            binding.descriptorCount = 1;
            for (uint32_t k = 0; k < refl_binding.array.dims_count; ++k)
                binding.descriptorCount *= refl_binding.array.dims[k];
            binding.stageFlags = static_cast<VkShaderStageFlagBits>(m_spv_module.shader_stage);

            auto &write = set.writes[j];
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.pNext = nullptr;
            write.descriptorCount = binding.descriptorCount;
            write.descriptorType = binding.descriptorType;
            write.dstBinding = binding.binding;
            bufs[l++].setWriteDescriptorSet(write);
        }
        VkDescriptorSetLayoutCreateInfo create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        create_info.bindingCount = static_cast<uint32_t>(set.bindings.size());
        create_info.pBindings = set.bindings.data();
        m_dev->getDescriptorSet(i, &create_info, set.writes, m_descriptor_sets[i], set.layout);
    }



    uint32_t input_count = 0;
    result = spvReflectEnumerateInputVariables(&m_spv_module, &input_count, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        throw std::runtime_error("Failed to enumerate input variables!");
    }
    std::vector<SpvReflectInterfaceVariable *> input_vars(input_count);
    result = spvReflectEnumerateInputVariables(&m_spv_module, &input_count, input_vars.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        throw std::runtime_error("Failed to enumerate input variables!");
    }
    std::cout << "Inputs:" << std::endl;
    for (const auto *input : input_vars)
    {
        std::cout << " - " << input->name << " (Location: " << input->location << ")" << std::endl;
    }

    uint32_t output_count = 0;
    result = spvReflectEnumerateOutputVariables(&m_spv_module, &output_count, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        throw std::runtime_error("Failed to enumerate output variables!");
    }
    std::vector<SpvReflectInterfaceVariable *> output_vars(output_count);
    result = spvReflectEnumerateOutputVariables(&m_spv_module, &output_count, output_vars.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        throw std::runtime_error("Failed to enumerate output variables!");
    }
    std::cout << "Outputs:" << std::endl;
    for (const auto *output : output_vars)
    {
        std::cout << " - " << output->name << " (Location: " << output->location << ")" << std::endl;
    }



    VkPipelineShaderStageCreateInfo StageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    StageInfo.pNext = nullptr;
    StageInfo.stage = static_cast<VkShaderStageFlagBits>(m_spv_module.shader_stage);
    StageInfo.module = m_module;
    StageInfo.pName = m_spv_module.entry_point_name;

    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.pNext = nullptr;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(m_sets.size());
    std::vector<VkDescriptorSetLayout> layouts(m_sets.size());
    for (size_t i = 0; i < m_sets.size(); ++i)
        layouts[i] = m_sets[i].layout;
    layoutInfo.pSetLayouts = layouts.data();
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = nullptr;
    m_dev->get_pipeline_layout(&layoutInfo, &m_pipeline_layout, &m_pipeline_layout_id);

    VkComputePipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pipelineInfo.pNext = nullptr;
    pipelineInfo.stage = StageInfo;
    pipelineInfo.layout = m_pipeline_layout;
    m_dev->get_pipeline(&pipelineInfo, &m_pipeline, &m_pipeline_id);

}

inline void ComputeProgram::record(uint32_t x, uint32_t y, uint32_t z)
{
    ComputePacket packet;
    packet.flags = VK_SHADER_STAGE_COMPUTE_BIT;
    packet.x = x;
    packet.y = y;
    packet.z = z;
    packet.pipeline = m_pipeline;
    packet.layout = m_pipeline_layout;
    packet.sets = m_descriptor_sets;

    
    m_wait = m_dev->submit(packet);
}

} // namespace vkrt