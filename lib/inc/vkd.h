#pragma once
#include "vka.h"
#include <vk_mem_alloc.h>
#include <thread>

namespace vkrt
{

class Device
{

    VkDevice m_dev;
    VkPipelineCache m_pipeline_cache;
    VkPhysicalDevice m_pDev;

    VmaAllocator m_allocator;
    std::vector<std::shared_ptr<QueueDispatcher>> m_queues;
    std::multimap<VkQueueFlags, uint32_t> m_queue_map;
    std::map<uint32_t, std::pair<VkBuffer, VmaAllocation>> m_buffers;
    std::map<uint32_t, std::pair<VkImage, VmaAllocation>> m_images;
    std::map<uint32_t, VkBufferView> m_buffer_views;
    std::map<uint32_t, VkImageView> m_image_views;
    std::map<uint32_t, VkShaderModule> m_shader_modules;
    std::map<uint32_t, VkPipelineLayout> m_pipeline_layout;
    std::map<uint32_t, VkPipeline> m_pipelines;
    // TODO make a descriptor pool per thread; and have thread access each one independentaly

    DescriptorAllocator *m_desc_pool_alloc;
    DescriptorLayoutCache *m_desc_layout_cache;

    void setupQueues(std::vector<VkQueueFamilyProperties> &queueFamilies)
    {
        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
        {
            m_queues.emplace_back(std::make_shared<QueueDispatcher>(i, m_dev, queueFamilies[i]));
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                m_queue_map.emplace(std::make_pair(VK_QUEUE_GRAPHICS_BIT, i));
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                m_queue_map.emplace(std::make_pair(VK_QUEUE_COMPUTE_BIT, i));
            if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
                m_queue_map.emplace(std::make_pair(VK_QUEUE_TRANSFER_BIT, i));
            if (queueFamilies[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                m_queue_map.emplace(std::make_pair(VK_QUEUE_SPARSE_BINDING_BIT, i));
            if (queueFamilies[i].queueFlags & VK_QUEUE_PROTECTED_BIT)
                m_queue_map.emplace(std::make_pair(VK_QUEUE_PROTECTED_BIT, i));
            if (queueFamilies[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)
                m_queue_map.emplace(std::make_pair(VK_QUEUE_VIDEO_DECODE_BIT_KHR, i));
            if (queueFamilies[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)
                m_queue_map.emplace(std::make_pair(VK_QUEUE_VIDEO_ENCODE_BIT_KHR, i));
        }
    }

    size_t findQueueFamilies(std::vector<VkQueueFamilyProperties> &queueFamilies,
                             std::vector<VkDeviceQueueCreateInfo> &queueCreateInfos) const
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount, nullptr);
        queueFamilies.resize(queueFamilyCount);
        queueCreateInfos.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount, queueFamilies.data());

        int i = 0, j = 0;

        for (const auto &queueFamily : queueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = i;
            queueCreateInfo.queueCount = queueFamily.queueCount;
            float *priorites = new float[queueCreateInfo.queueCount];
            std::fill(priorites, priorites + queueCreateInfo.queueCount, 0.9f);
            queueCreateInfo.pQueuePriorities = priorites;
            queueCreateInfos[i++] = queueCreateInfo;
            j += queueFamily.queueCount;
        }
        return j;
    }

    struct features
    {
        VkPhysicalDeviceFeatures2 features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        VkPhysicalDeviceVulkan11Features features11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
        VkPhysicalDeviceVulkan12Features features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        VkPhysicalDeviceVulkan13Features features13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceCooperativeMatrixFeaturesKHR coo_matrix_features = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR};
    } m_feats;

    struct properties
    {
        VkPhysicalDeviceProperties2 device_properties_2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        VkPhysicalDeviceSubgroupProperties subgroup_properties = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
        VkPhysicalDeviceVulkan11Properties device_vulkan11_properties = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES};
        VkPhysicalDeviceVulkan12Properties device_vulkan12_properties = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
        VkPhysicalDeviceVulkan13Properties device_vulkan13_properties = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
    } m_props;

    void getFeaturesAndProperties(VkPhysicalDevice &pd)
    {
        m_feats.features2.pNext = &m_feats.features11;
        m_feats.features11.pNext = &m_feats.features12;
        m_feats.features12.pNext = &m_feats.features13;
        m_feats.features13.pNext = &m_feats.coo_matrix_features;
        vkGetPhysicalDeviceFeatures2(pd, &m_feats.features2);

        m_props.device_properties_2.pNext = &m_props.device_vulkan11_properties;
        m_props.device_vulkan11_properties.pNext = &m_props.device_vulkan12_properties;
        m_props.device_vulkan12_properties.pNext = &m_props.device_vulkan13_properties;
        m_props.device_vulkan13_properties.pNext = &m_props.subgroup_properties;
        vkGetPhysicalDeviceProperties2(pd, &m_props.device_properties_2);
    }

    uint32_t supported_data_types() const
    {
        m_feats.features2.features.shaderFloat64;
        m_feats.features2.features.shaderInt16;
        m_feats.features2.features.shaderInt64;
        m_feats.features12.shaderFloat16;
        m_feats.features12.shaderInt8;
        return 0;
    }

    void map_buffer(VmaAllocation alloc, void **data) const
    {
        vmaMapMemory(m_allocator, alloc, data);
    }

    void unmap_buffer(VmaAllocation alloc) const
    {
        vmaUnmapMemory(m_allocator, alloc);
    }

    void allocate_buffer(VkBufferCreateInfo *bufferCreateInfo, VmaAllocationCreateInfo *vmaAllocCreateInfo,
                         VkBuffer *buffer, VmaAllocation *allocation, VmaAllocationInfo *allocInfo, uint32_t *buffer_id)
    {
        uint32_t memTypeIdx = 0;
        VkResult res{};

        if (vmaFindMemoryTypeIndexForBufferInfo(m_allocator, bufferCreateInfo, vmaAllocCreateInfo, &memTypeIdx) ==
            VK_SUCCESS)
        {
            vmaAllocCreateInfo->memoryTypeBits = 1u << memTypeIdx;
            res = vmaCreateBuffer(m_allocator, bufferCreateInfo, vmaAllocCreateInfo, buffer, allocation, allocInfo);
        }
        else
        {
            printf("Failed to find memory type index\n");
            return;
        }

        if (buffer != nullptr && res == VK_SUCCESS)
        {
            *buffer_id = m_buffers.size();
            m_buffers.emplace(*buffer_id, std::make_pair(*buffer, *allocation));
        }
    }

    void destroy_buffer(VkBuffer buffer, VmaAllocation allocation) const
    {
        vkDestroyBuffer(m_dev, buffer, nullptr);
        vmaFreeMemory(m_allocator, allocation);
    }

    void allocate_image(const VkImageCreateInfo *imageCreateInfo, VmaAllocationCreateInfo *vmaAllocCreateInfo,
                        VkImage *image, VmaAllocation *allocation, VmaAllocationInfo *alloc_info, uint32_t *buffer_id)
    {
        uint32_t memTypeIdx = 0;
        if (vmaFindMemoryTypeIndexForImageInfo(m_allocator, imageCreateInfo, vmaAllocCreateInfo, &memTypeIdx) ==
            VK_SUCCESS)
        {
            vmaAllocCreateInfo->memoryTypeBits = 1u << memTypeIdx;
            CHECK_RESULT(
                vmaCreateImage(m_allocator, imageCreateInfo, vmaAllocCreateInfo, image, allocation, alloc_info),
                "failure to allocate image");
            *buffer_id = m_images.size();
            m_images.emplace(*buffer_id, std::make_pair(*image, *allocation));
        }
        else
        {
            printf("Failed to find memory type index\n");
        }
    }

    void destroy_image(VkImage image, VmaAllocation allocation) const
    {
        vmaDestroyImage(m_allocator, image, allocation);
    }

    void allocate_buffer_view(const VkBufferViewCreateInfo *bufferViewCreateInfo, VkBufferView *bufferView,
                              uint32_t *buffer_id)
    {
        *buffer_id = m_buffer_views.size();
        CHECK_RESULT(vkCreateBufferView(m_dev, bufferViewCreateInfo, nullptr, bufferView),
                     "failure to allocate buffer view");
        m_buffer_views.emplace(*buffer_id, *bufferView);
    }

    void destroy_buffer_view(VkBufferView bufferView) const
    {
        vkDestroyBufferView(m_dev, bufferView, nullptr);
    }

    void allocate_image_view(const VkImageViewCreateInfo *imageViewCreateInfo, VkImageView *imageView,
                             uint32_t *buffer_id)
    {
        *buffer_id = m_image_views.size();
        CHECK_RESULT(vkCreateImageView(m_dev, imageViewCreateInfo, nullptr, imageView),
                     "failure to allocate image view");
        m_image_views.emplace(*buffer_id, *imageView);
    }

    void destroy_image_view(VkImageView imageView) const
    {
        vkDestroyImageView(m_dev, imageView, nullptr);
    }

    auto get_shader_module(const std::vector<uint32_t> &code, VkShaderModule *shaderModule, uint32_t *shader_id)
    {
        *shader_id = m_shader_modules.size();
        VkShaderModuleCreateInfo moduleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        moduleCreateInfo.pCode = code.data();
        moduleCreateInfo.codeSize = code.size() * sizeof(uint32_t);
        moduleCreateInfo.flags = 0;
        moduleCreateInfo.pNext = nullptr;
        CHECK_RESULT(vkCreateShaderModule(m_dev, &moduleCreateInfo, nullptr, shaderModule),
                     " failed to create shaderModule");
        m_shader_modules.emplace(*shader_id, *shaderModule);
    }

    void destroy_shader_module(VkShaderModule shaderModule) const
    {
        vkDestroyShaderModule(m_dev, shaderModule, nullptr);
    }

    void get_pipeline_layout(VkPipelineLayoutCreateInfo *layoutInfo, VkPipelineLayout *layout, uint32_t *layout_id)
    {
        *layout_id = m_pipeline_layout.size();
        vkCreatePipelineLayout(m_dev, layoutInfo, nullptr, layout);
        m_pipeline_layout.emplace(*layout_id, *layout);
    }

    void destroy_pipeline_layout(VkPipelineLayout pipelineLayout) const
    {
        vkDestroyPipelineLayout(m_dev, pipelineLayout, nullptr);
    }

    void get_pipeline(VkComputePipelineCreateInfo *StageInfo, VkPipeline *pipeline, uint32_t *pipeline_id)
    {
        *pipeline_id = m_pipelines.size();
        vkCreateComputePipelines(m_dev, m_pipeline_cache, 1, StageInfo, nullptr, pipeline);
        m_pipelines.emplace(*pipeline_id, *pipeline);
    }

    void destroy_pipeline(VkPipeline pipeline) const
    {
        vkDestroyPipeline(m_dev, pipeline, nullptr);
    }

    size_t getMaxAllocationSize() const
    {
        return std::max<uint64_t>(0, // m_props.device_vulkan13_properties.maxBufferSize,
                                  m_props.device_vulkan11_properties.maxMemoryAllocationSize);
    }

    size_t getSparseAllocationSize() const
    {
        return std::max<uint64_t>(m_props.device_properties_2.properties.limits.maxStorageBufferRange,
                                  m_props.device_properties_2.properties.limits.sparseAddressSpaceSize);
    }

    std::vector<uint32_t> getSparseQueueFamily() const
    {
        std::vector<uint32_t> queues;
        for (auto &q : m_queues)
        {
            if (q->isSparseBindingFamily())
            {
                queues.push_back(q->get_idx());
            }
        }
        return queues;
    }

    
    auto getDescriptorSet(uint32_t set_number, VkDescriptorSetLayoutCreateInfo *create_info,
                          std::vector<VkWriteDescriptorSet> &writes, VkDescriptorSet &set,
                          VkDescriptorSetLayout &layout)
    {
        layout = m_desc_layout_cache->create_descriptor_layout(m_dev, set_number, create_info);
        bool success = m_desc_pool_alloc->allocate(m_dev, &set, &layout);
        if (!success)
            return false;
        for (VkWriteDescriptorSet &w : writes)
            w.dstSet = set;
        vkUpdateDescriptorSets(m_dev, writes.size(), writes.data(), 0, nullptr);
        return true;
    }

    std::shared_future<void> submit(struct ComputePacket &packet)
    {
        auto qit = m_queue_map.equal_range(VK_QUEUE_COMPUTE_BIT);
        for (auto &qidx = qit.first; qidx != qit.second; ++qidx)
        {
            std::shared_ptr<QueueDispatcher> &q = m_queues.at(qidx->second);
            if (!q->isBusy())
            {
                return q->submit(packet);
                break;
            }
        }
        throw std::runtime_error("No compute queue available");
    }

    friend class ComputeProgram;
    friend class StorageBuffer;

  public:
    Device(const Device &other)
    {
        m_dev = other.m_dev;
        m_pipeline_cache = other.m_pipeline_cache;
        m_pDev = other.m_pDev;
        m_allocator = other.m_allocator;
        m_queues = other.m_queues;
        m_buffers = other.m_buffers;
        m_images = other.m_images;
        m_buffer_views = other.m_buffer_views;
        m_image_views = other.m_image_views;
        m_feats = other.m_feats;
        m_props = other.m_props;
        m_desc_pool_alloc = other.m_desc_pool_alloc;
        m_desc_layout_cache = other.m_desc_layout_cache;
    }

    Device &operator=(const Device &other)
    {
        m_dev = other.m_dev;
        m_pipeline_cache = other.m_pipeline_cache;
        m_pDev = other.m_pDev;
        m_allocator = other.m_allocator;
        m_queues = other.m_queues;
        m_buffers = other.m_buffers;
        m_images = other.m_images;
        m_buffer_views = other.m_buffer_views;
        m_image_views = other.m_image_views;
        m_feats = other.m_feats;
        m_props = other.m_props;
        m_desc_pool_alloc = other.m_desc_pool_alloc;
        m_desc_layout_cache = other.m_desc_layout_cache;
        return *this;
    }

    Device(VkInstance &inst, VkPhysicalDevice &pd) : m_pDev(pd)
    {
        m_desc_pool_alloc = new DescriptorAllocator({{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10}});
        m_desc_layout_cache = new DescriptorLayoutCache;
        getFeaturesAndProperties(pd);
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount, queueFamilies.data());

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        size_t max_queues = findQueueFamilies(queueFamilies, queueCreateInfos);
        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = queueCreateInfos.size();
        deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
        VkPhysicalDeviceFeatures2 features2 = m_feats.features2;
        VkPhysicalDeviceVulkan11Features features11 = m_feats.features11;
        VkPhysicalDeviceVulkan12Features features12 = m_feats.features12;
        VkPhysicalDeviceVulkan13Features features13 = m_feats.features13;
        features13.pNext = nullptr;
        features2.pNext = &features11;
        features11.pNext = &features12;
        features12.pNext = &features13;
        deviceInfo.pNext = &features2;

        vkCreateDevice(pd, &deviceInfo, nullptr, &m_dev);
        volkLoadDevice(m_dev);
        setupQueues(queueFamilies);

        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        vkCreatePipelineCache(m_dev, &pipelineCacheCreateInfo, nullptr, &m_pipeline_cache);

        VolkDeviceTable dt;
        volkLoadDeviceTable(&dt, m_dev);

        VmaVulkanFunctions vmaFuncs = {};
        vmaFuncs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vmaFuncs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
        vmaFuncs.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
        vmaFuncs.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
        vmaFuncs.vkAllocateMemory = dt.vkAllocateMemory;
        vmaFuncs.vkFreeMemory = dt.vkFreeMemory;
        vmaFuncs.vkMapMemory = dt.vkMapMemory;
        vmaFuncs.vkUnmapMemory = dt.vkUnmapMemory;
        vmaFuncs.vkFlushMappedMemoryRanges = dt.vkFlushMappedMemoryRanges;
        vmaFuncs.vkInvalidateMappedMemoryRanges = dt.vkInvalidateMappedMemoryRanges;
        vmaFuncs.vkBindBufferMemory = dt.vkBindBufferMemory;
        vmaFuncs.vkBindImageMemory = dt.vkBindImageMemory;
        vmaFuncs.vkGetBufferMemoryRequirements = dt.vkGetBufferMemoryRequirements;
        vmaFuncs.vkGetImageMemoryRequirements = dt.vkGetImageMemoryRequirements;
        vmaFuncs.vkCreateBuffer = dt.vkCreateBuffer;
        vmaFuncs.vkDestroyBuffer = dt.vkDestroyBuffer;
        vmaFuncs.vkCreateImage = dt.vkCreateImage;
        vmaFuncs.vkDestroyImage = dt.vkDestroyImage;
        vmaFuncs.vkCmdCopyBuffer = dt.vkCmdCopyBuffer;

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = pd;
        allocatorInfo.device = m_dev;
        allocatorInfo.pVulkanFunctions = &vmaFuncs;
        allocatorInfo.instance = inst;
        allocatorInfo.preferredLargeHeapBlockSize = getMaxAllocationSize();
        allocatorInfo.vulkanApiVersion = volkGetInstanceVersion();
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT |
                              VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
                              VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT | VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT;

        CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &m_allocator), "Failed to create allocator");
    }

    void destory()
    {
        for (auto &id_buf : m_buffers)
            destroy_buffer(id_buf.second.first, id_buf.second.second);
        for (auto &id_img : m_images)
            destroy_image(id_img.second.first, id_img.second.second);
        for (auto &id_buf_view : m_buffer_views)
            destroy_buffer_view(id_buf_view.second);
        for (auto &id_img_view : m_image_views)
            destroy_image_view(id_img_view.second);
        for (auto &id_shader_module : m_shader_modules)
            destroy_shader_module(id_shader_module.second);
        for (auto &id_pipeline_layout : m_pipeline_layout)
            destroy_pipeline_layout(id_pipeline_layout.second);
        for (auto &id_pipeline : m_pipelines)
            destroy_pipeline(id_pipeline.second);
        for (auto &q : m_queues)
            q->destroy(m_dev);
        m_desc_layout_cache->destory(m_dev);
        m_desc_pool_alloc->destory(m_dev);
        delete m_desc_layout_cache;
        delete m_desc_pool_alloc;
        vmaDestroyAllocator(m_allocator);
        vkDestroyPipelineCache(m_dev, m_pipeline_cache, nullptr);
        vkDestroyDevice(m_dev, nullptr);
    }

    auto getMemoryProperty(VmaAllocation &alloc) const
    {
        VkMemoryPropertyFlags flags;
        vmaGetAllocationMemoryProperties(m_allocator, alloc, &flags);
        return flags;
    }

       
};

} // namespace vkrt

/*
    auto &q = getSparseQueueFamily();    
    if (bufferCreateInfo->size >= getSparseAllocationSize())
    {
        return;
    }

    if (m_feats.features2.features.sparseBinding)
    {
        bufferCreateInfo->sharingMode = VK_SHARING_MODE_CONCURRENT;
        bufferCreateInfo->flags = VK_BUFFER_CREATE_SPARSE_BINDING_BIT;
        bufferCreateInfo->pQueueFamilyIndices = q.data();
        bufferCreateInfo->queueFamilyIndexCount = q.size();
    }
    if (m_feats.features2.features.sparseResidencyAliased)
    {
        bufferCreateInfo->flags |= VK_BUFFER_CREATE_SPARSE_ALIASED_BIT;
        vmaAllocCreateInfo->flags |= VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;
    }
    if (m_feats.features2.features.sparseResidencyBuffer)
    {
        bufferCreateInfo->flags |= VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;
    }

    res = vkCreateBuffer(m_dev, bufferCreateInfo, nullptr, buffer);
    CHECK_RESULT(res, "failed to create buffer");

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(m_dev, *buffer, &mem_reqs);

    vmaAllocCreateInfo->requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    vmaAllocCreateInfo->preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vmaAllocCreateInfo->usage = VMA_MEMORY_USAGE_UNKNOWN;
    VkSparseMemoryBind binds[MAX_CHUNKS] = {};
    if (vmaFindMemoryTypeIndexForBufferInfo(m_allocator, bufferCreateInfo, vmaAllocCreateInfo, &memTypeIdx) ==
        VK_SUCCESS)
    {
        auto total_size = bufferCreateInfo->size;
        size_t num_blocks = std::ceil<size_t>(bufferCreateInfo->size / getMaxAllocationSize());
        vmaAllocCreateInfo->memoryTypeBits = 1u << memTypeIdx;

        mem_reqs.size = getMaxAllocationSize();
        vmaAllocateMemoryPages(m_allocator, &mem_reqs, vmaAllocCreateInfo, num_blocks, allocation.data(),
                                allocInfo.data());
        size_t bindCount = 0;
        size_t offset = 0;
        for (auto bindCount = 0; bindCount < allocInfo.size(); ++bindCount)
        {
            VkSparseMemoryBind *bind = &binds[bindCount];
            bind->memory = allocInfo[bindCount].deviceMemory;
            bind->memoryOffset = offset;
            bind->resourceOffset = allocInfo[bindCount].offset;
            bind->size = std::min<uint64_t>(allocInfo[bindCount].size, total_size);
            total_size -= bind->size;
            offset += bind->size;
        }
    }
    else
    {
        printf("Failed to find memory type index\n");
    }
        



*/