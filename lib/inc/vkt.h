#pragma once
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <volk.h>


//
//class cmd
//{
//    VkCommandPool m_pool;
//    VkCommandBuffer m_primary_buffer = nullptr;
//    VkQueue m_queue;
//    VkFence m_fence;
//    std::vector<VkCommandBuffer> m_cmd_buffers;
//    std::queue<std::future<VkCommandBuffer>> m_futures;
//    std::condition_variable m_cv;
//    std::mutex m_cmd_mutex;
//    bool start, stop;
//    std::condition_variable m_busy_flag;
//    std::mutex m_busy_mutex;
//    std::future<void> m_future;
//
//    std::future<void> m_wait_thread;
//
//    std::bitset<31> m_non_working_map;
//
//    int i = -1;
//
//    void run()
//    {
//        m_future = std::async(std::launch::async, [this]() {
//            while (true)
//            {
//                {
//                    std::unique_lock<std::mutex> lock(m_busy_mutex);
//                    m_busy_flag.wait(lock, [this]() { return start && !m_futures.empty(); });
//                }
//                std::vector<VkCommandBuffer> cmdBuffers;
//                while (!m_futures.empty())
//                {
//                    auto cmd = m_futures.front().get();
//                    cmdBuffers.push_back(cmd);
//                    m_futures.pop();
//                }
//
//                if (stop)
//                    break;
//
//                VkCommandBufferBeginInfo primaryBeginInfo = {};
//                primaryBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//                primaryBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
//                primaryBeginInfo.pNext = nullptr;
//                vkBeginCommandBuffer(m_primary_buffer, &primaryBeginInfo);
//                vkCmdExecuteCommands(m_primary_buffer, cmdBuffers.size(), cmdBuffers.data());
//                vkEndCommandBuffer(m_primary_buffer);
//                VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
//                submitInfo.waitSemaphoreCount = 0;
//                submitInfo.pWaitSemaphores = nullptr;
//                submitInfo.commandBufferCount = 1;
//                submitInfo.pCommandBuffers = &m_primary_buffer;
//                submitInfo.signalSemaphoreCount = 0;
//                submitInfo.pSignalSemaphores = nullptr;
//                vkQueueWaitIdle(m_queue);
//                vkQueueSubmit(m_queue, 1, &submitInfo, m_fence);
//                m_wait_thread = std::async(std::launch::async,
//                                           [this]() { vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, UINT64_MAX); });
//                for (auto buffer : cmdBuffers)
//                    vkResetCommandBuffer(buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
//            }
//        });
//    }
//
//    std::shared_future<void> submit(ComputePacket &packet)
//    {
//        std::shared_future<void> sf;
//        VkCommandBuffer buffer = m_cmd_buffers.at(++i);
//        auto fut = std::async(
//            std::launch::async,
//            [this](ComputePacket &packet, VkCommandBuffer buffer) {
//                start = true;
//                m_busy_flag.notify_one();
//
//                VkCommandBufferInheritanceRenderingInfo inheritanceRenderingInfo = {};
//                inheritanceRenderingInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
//                inheritanceRenderingInfo.colorAttachmentCount = 0;
//                inheritanceRenderingInfo.pColorAttachmentFormats = nullptr;
//                inheritanceRenderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
//                inheritanceRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
//                inheritanceRenderingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//
//                VkCommandBufferInheritanceInfo inheritanceInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
//                inheritanceInfo.renderPass = VK_NULL_HANDLE;
//                inheritanceInfo.framebuffer = VK_NULL_HANDLE;
//                inheritanceInfo.pNext = &inheritanceRenderingInfo;
//                VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
//
//                beginInfo.pNext = nullptr;
//                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
//                beginInfo.pInheritanceInfo = &inheritanceInfo;
//                vkBeginCommandBuffer(buffer, &beginInfo);
//                vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, packet.pipeline);
//                vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, packet.layout, 0, packet.sets.size(),
//                                        packet.sets.data(), 0, nullptr);
//
//                vkCmdDispatch(buffer, packet.x, packet.y, packet.z);
//                vkEndCommandBuffer(buffer);
//                return buffer;
//            },
//            packet, buffer);
//        m_futures.push(std::move(fut));
//
//        return sf;
//    }
//};