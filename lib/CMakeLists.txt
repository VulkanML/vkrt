

if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()


add_library( rt STATIC vkrt.cpp  inc/vkrt.h  inc/vka.h inc/vkb.h inc/vkc.h inc/vkd.h inc/vkt.h)
target_include_directories(rt PUBLIC inc)
add_library(rt::rt ALIAS rt)
target_include_directories( rt PRIVATE ext/volk)
target_include_directories( rt PRIVATE ext/VulkanMemoryAllocator/include)
target_include_directories( rt PUBLIC "${Vulkan_INCLUDE_DIRS}")
target_link_libraries( rt "${Vulkan_LIBRARIES}")
target_link_libraries( rt volk)
target_link_libraries( rt VulkanMemoryAllocator)
target_link_libraries( rt spirv-reflect-static)