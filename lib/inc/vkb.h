#include <vk_mem_alloc.h>


namespace vkrt {

class StorageBuffer
{
  protected:
    union storage {
        VkBuffer m_buffer;
        VkImage m_image;
        VkBufferView m_buffer_view;
        VkImageView m_image_view;
    } m_storage;

    VmaAllocation m_allocation = nullptr;
    VmaAllocationInfo m_allocation_info = {};
    VmaAllocationCreateInfo m_allocation_create_info;

    union createinfo {
        VkBufferCreateInfo m_buffer_create_info;
        VkImageCreateInfo m_image_create_info;
        VkBufferViewCreateInfo m_buffer_view_create_info;
        VkImageViewCreateInfo m_image_view_create_info;
    } m_createinfo;

    uint32_t m_id = UINT32_MAX;
    Device *m_dev = nullptr;

    int m_flags = 0;
    bool m_is_mapped = false;
    void *m_data = nullptr;

    union DescriptorInfo {
        VkDescriptorBufferInfo desc_buffer_info;
        VkDescriptorImageInfo desc_image_info;
    } m_desc_info;

    enum class BUFFERTYPE
    {
        BUFFER,
        IMAGE,
        BUFFER_VIEW,
        IMAGE_VIEW
    } m_type;

    void setFlags(int flags)
    {
        if (flags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
            m_allocation_create_info.flags |=
                VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        else if (m_is_mapped || flags & VK_BUFFER_USAGE_TRANSFER_DST_BIT ||
                 flags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT || flags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
            m_allocation_create_info.flags |=
                VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        else
            m_allocation_create_info.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        m_allocation_create_info.priority = 0.9f;
        m_allocation_create_info.pool = nullptr;
        m_allocation_create_info.requiredFlags = 0;
        m_allocation_create_info.preferredFlags = 0;
        m_allocation_create_info.memoryTypeBits = UINT32_MAX;
    }

    void map()
    {
        if (m_is_mapped && m_allocation_info.pMappedData != nullptr)
        {
            m_data = m_allocation_info.pMappedData;
        }
        else if (m_is_mapped)
        {
            m_dev->map_buffer(m_allocation, &m_data);
        }
    }

    void lazy_load()
    {
        if (m_type == BUFFERTYPE::BUFFER && m_storage.m_buffer == nullptr)
        {
            m_dev->allocate_buffer(&m_createinfo.m_buffer_create_info, &m_allocation_create_info, &m_storage.m_buffer,
                                   &m_allocation, &m_allocation_info, &m_id);
            if (isLive())
                map();
        }
        if (m_type == BUFFERTYPE::IMAGE && m_storage.m_image == nullptr)
        {
            m_dev->allocate_image(&m_createinfo.m_image_create_info, &m_allocation_create_info, &m_storage.m_image,
                                  &m_allocation, &m_allocation_info, &m_id);
            if (isLive())
                map();
        }

        if (m_type == BUFFERTYPE::BUFFER_VIEW && m_storage.m_buffer_view == nullptr)
        {
            m_dev->allocate_buffer_view(&m_createinfo.m_buffer_view_create_info, &m_storage.m_buffer_view, &m_id);
        }
        if (m_type == BUFFERTYPE::IMAGE_VIEW && m_storage.m_image_view == nullptr)
        {
            m_dev->allocate_image_view(&m_createinfo.m_image_view_create_info, &m_storage.m_image_view, &m_id);
        }
    }

  public:
    StorageBuffer(Device &dev, size_t size, VkBufferUsageFlags flags, bool is_mapped = false)
        : m_flags(static_cast<int>(flags)), m_is_mapped(is_mapped), m_type(BUFFERTYPE::BUFFER)
    {
        m_storage.m_buffer = nullptr;
        m_dev = &dev;
        m_createinfo.m_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        m_createinfo.m_buffer_create_info.pNext = nullptr;
        m_createinfo.m_buffer_create_info.size = size;
        m_createinfo.m_buffer_create_info.flags = 0;
        m_createinfo.m_buffer_create_info.usage = flags;
        m_createinfo.m_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        m_createinfo.m_buffer_create_info.pQueueFamilyIndices = 0;
        m_createinfo.m_buffer_create_info.queueFamilyIndexCount = 0;
        m_allocation_create_info.flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;
        m_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        m_type = BUFFERTYPE::BUFFER;
        setFlags(static_cast<int>(flags));
        // if (is_mapped)
        lazy_load();
    }

    StorageBuffer &operator=(const StorageBuffer &other)
    {
        m_dev = other.m_dev;
        m_storage = other.m_storage;
        m_allocation = other.m_allocation;
        m_allocation_info = other.m_allocation_info;
        m_createinfo = other.m_createinfo;
        m_allocation_create_info = other.m_allocation_create_info;
        m_id = other.m_id;
        m_flags = other.m_flags;
        m_desc_info = other.m_desc_info;
        m_type = other.m_type;
        return *this;
    }

    StorageBuffer(const StorageBuffer &other)
    {
        m_dev = other.m_dev;
        m_storage = other.m_storage;
        m_allocation = other.m_allocation;
        m_allocation_info = other.m_allocation_info;
        m_createinfo = other.m_createinfo;
        m_allocation_create_info = other.m_allocation_create_info;
        m_id = other.m_id;
        m_flags = other.m_flags;
        m_desc_info = other.m_desc_info;
        m_type = other.m_type;
    }

    auto getMemoryProperties()
    {
        VkMemoryPropertyFlagBits memFlags{};
        return m_dev->getMemoryProperty(m_allocation);
    }

    auto getSize() const {
        return m_createinfo.m_buffer_create_info.size;
    }

    void *data()
    {
        return m_data;
    }

    //auto GetAccessFlags() const
    //{
    //    if (m_createinfo.m_buffer_create_info.usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
    //        return VK_ACCESS_TRANSFER_READ_BIT;
    //    if (m_createinfo.m_buffer_create_info.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
    //        return VK_ACCESS_TRANSFER_WRITE_BIT;
    //    if (m_createinfo.m_buffer_create_info.usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
    //        return VK_ACCESS_SHADER_READ_BIT;
    //    if (m_createinfo.m_buffer_create_info.usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
    //        return VK_ACCESS_SHADER_READ_BIT;
    //    if (m_createinfo.m_buffer_create_info.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
    //        return VK_ACCESS_UNIFORM_READ_BIT;
    //    if (m_createinfo.m_buffer_create_info.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    //        return VK_ACCESS_SHADER_READ_BIT;
    //    if (m_createinfo.m_buffer_create_info.usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
    //        return VK_ACCESS_INDEX_READ_BIT;
    //    if (m_createinfo.m_buffer_create_info.usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
    //        return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    //    if (m_createinfo.m_buffer_create_info.usage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
    //        return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    //    if (m_createinfo.m_buffer_create_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    //        return VK_ACCESS_SHADER_READ_BIT;
    //    if (m_createinfo.m_buffer_create_info.usage & VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR)
    //        return VK_ACCESS_SHADER_READ_BIT;
    //    return VK_ACCESS_HOST_WRITE_BIT;
    //}

    bool isLive(void) const
    {
        return m_id != UINT32_MAX;
    }

    void setWriteDescriptorSet(VkWriteDescriptorSet &write)
    {
        lazy_load();
        if (m_storage.m_buffer != nullptr && m_type == BUFFERTYPE::BUFFER)
        {
            m_desc_info.desc_buffer_info.buffer = m_storage.m_buffer;
            m_desc_info.desc_buffer_info.offset = 0;
            m_desc_info.desc_buffer_info.range = m_allocation_info.size;
            write.pBufferInfo = &(m_desc_info.desc_buffer_info);
        }
        else if (m_storage.m_image != nullptr && (m_type == BUFFERTYPE::IMAGE || m_type == BUFFERTYPE::IMAGE_VIEW))
        {
            m_desc_info.desc_image_info.imageLayout = m_createinfo.m_image_create_info.initialLayout;
            if (m_type == BUFFERTYPE::IMAGE_VIEW)
                m_desc_info.desc_image_info.imageView = m_storage.m_image_view;
            write.pImageInfo = &(m_desc_info.desc_image_info);
        }
    }
};


}