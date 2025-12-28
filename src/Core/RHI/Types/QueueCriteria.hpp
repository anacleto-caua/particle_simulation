#pragma once

#include <vector>
#include <algorithm>

#include <vulkan/vulkan.h>

#include "Core/RHI/Types/AppTypes.hpp"

class QueueCriteria {
private:
    struct QueueScoreBoard {
        uint32_t queueFamilyIndex = -1;
        int32_t score = 0;
    };

public:
    int32_t evaluateQueues(std::vector<VkQueueFamilyProperties> queueFamilies) {
        std::vector<QueueScoreBoard> scores;
        scores.reserve(queueFamilies.size());

        uint32_t index = 0;
        for(const auto& queueFamily : queueFamilies) {
            scores.push_back(
                { 
                    index,
                    evaluateQueue(queueFamilies[index], index)
                }
            );
            index++;
        }

        std::sort(
            scores.begin(), scores.end(), 
            [](const QueueScoreBoard& a, const QueueScoreBoard& b) {
                return a.score > b.score; 
            }
        );

        if(scores[0].score >= 0) {
            return scores[0].queueFamilyIndex;
        }

        return -1;
    }

    static QueueCriteria startCriteria() {
        QueueCriteria criteria;
        return criteria;
    }

    static QueueCriteria startCriteria(const QueueCriteria& copyCriteria) {
        return copyCriteria;
    }

    QueueCriteria& addRequiredFlags(VkQueueFlags flag) {
        m_requiredFlags |= flag;
        return *this;
    }

    QueueCriteria& addAvoidedFlags(VkQueueFlags flag) {
        m_avoidedFlags |= flag;
        return *this;
    }

    QueueCriteria& requireSurfaceSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
        m_device = device;
        m_surface = surface;
        m_requiresSurfaceSupport = true;
        return  *this;
    }

    QueueCriteria& desireExclusivenessAgainst(const QueueContext& queueCtx) {
        m_uniqueAgainst.push_back(queueCtx);
        return *this;
    }

private:
    VkQueueFlags m_requiredFlags = 0;
    VkQueueFlags m_avoidedFlags = 0;
    // VkQueueFlags forbiddenFlags = 0;
    // VkQueueFlags desiredFlags = 0;

    std::vector<QueueContext> m_uniqueAgainst;

    bool m_requiresSurfaceSupport = false;
    VkPhysicalDevice m_device = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    // It'll break if you pass an invalid surface or device, so don't do that...
    bool checkForSurfaceSupport(uint32_t queueFamilyIndex) {
        if(!m_requiresSurfaceSupport || m_device == VK_NULL_HANDLE || m_surface == VK_NULL_HANDLE) {
            return true;
        }
        
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_device, queueFamilyIndex, m_surface, &presentSupport);
        return presentSupport;
    }

    int32_t evaluateQueue(const VkQueueFamilyProperties &candidateFamily, uint32_t queueFamilyIndex) {
        if(!checkForSurfaceSupport(queueFamilyIndex)) {
            return -1;
        }

        VkQueueFlags candidateFlags = candidateFamily.queueFlags;
        if((candidateFlags & m_requiredFlags) != m_requiredFlags) {
            return -1;
        }

        int score = 0;
        if((candidateFlags & m_avoidedFlags) == 0) {
            score += 100; 
        }

        for(QueueContext queueCtx : m_uniqueAgainst) {
            if(queueCtx.queueFamilyIndex != queueFamilyIndex) {
                score+=10;
            }
        }

        return score;
    }
};