#pragma once

#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_USE_STL_CONTAINERS 0
#define VMA_USE_STL_VECTOR 0
#define VMA_USE_STL_UNORDERED_MAP 0
#define VMA_USE_STL_LIST 0
#define VMA_IMPLEMENTATION

#define VK_NO_PROTOTYPES
#define VOLK_IMPLEMENTATION
#define CHECK_RESULT(fn, statement)                                                                                    \
    if ((fn) != VK_SUCCESS)                                                                                            \
        printf(statement "\n");

#ifdef DEBUG
#define VMA_DEBUG_MARGIN 16
#define VMA_DEBUG_DETECT_CORRUPTION 1
#endif

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>
#include <volk.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                    void *pUserData)
{

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                      const VkAllocationCallbacks *pAllocator,
                                      VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}

#include "vkd.h"
#include "vkb.h"
#include "vkc.h"

namespace vkrt
{

class Instance
{
    VkInstance m_instance;
    uint32_t m_pd_count = 0;
    std::vector<Device> m_devices;
    std::vector<const char *> m_validation_layers = {"VK_LAYER_KHRONOS_validation"};
    std::vector<const char *> m_extension_names;
    VkDebugUtilsMessengerEXT m_debugMessenger;

    bool checkValidationLayerSupport()
    {
#ifdef _DEBUG
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName : m_validation_layers)
        {
            bool layerFound = false;
            for (const auto &layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
                return false;
        }

        m_extension_names.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        return true;
#else
        return false;
#endif
    }

  public:
    Instance()
    {
        CHECK_RESULT(volkInitialize(), "Failed to initialize volk");
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello ML";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "vkmlrt";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        if (checkValidationLayerSupport())
        {
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugCreateInfo.pfnUserCallback = debugCallback;
            debugCreateInfo.pUserData = nullptr; // Optional

            createInfo.enabledLayerCount = static_cast<uint32_t>(m_validation_layers.size());
            createInfo.ppEnabledLayerNames = m_validation_layers.data();
            createInfo.enabledExtensionCount = static_cast<uint32_t>(m_extension_names.size());
            createInfo.ppEnabledExtensionNames = m_extension_names.data();
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;

            CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_instance), "failed to create instance");
            volkLoadInstance(m_instance);

            CreateDebugUtilsMessengerEXT(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger);
        }
        else
        {
            CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_instance), "failed to create instance");
            volkLoadInstance(m_instance);
        }

        vkEnumeratePhysicalDevices(m_instance, &m_pd_count, nullptr);
        std::vector<VkPhysicalDevice> pds(m_pd_count);
        vkEnumeratePhysicalDevices(m_instance, &m_pd_count, pds.data());

        uint32_t i = 0;
        for (auto &pd : pds)
        {
            m_devices.emplace_back(m_instance, pd);
            ++i;
        }
    }

    auto &get_device(uint32_t i = 0)
    {
        return m_devices.at(i);
    }

    ~Instance()
    {
        for (auto &dev : m_devices)
            dev.destory();
        DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        vkDestroyInstance(m_instance, nullptr);
        volkFinalize();
    }
};

}; // namespace vkrt