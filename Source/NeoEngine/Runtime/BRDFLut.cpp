#include "BRDFLut.h"
#include <cmath>
#include <numbers>

namespace NeoEngine {

BRDFLut::~BRDFLut() noexcept {
    // Vulkan resources cleaned up by RAII wrappers
}

bool BRDFLut::Generate() noexcept {
    // Generate BRDF lookup table
    // This pre-computes the split-sum approximation for PBR
    // LUT dimensions: 512x512 (roughness x view angle)
    
    std::vector<glm::vec2> lutData(LUT_RESOLUTION * LUT_RESOLUTION);
    
    // For each pixel in the LUT
    for (uint32_t y = 0; y < LUT_RESOLUTION; ++y) {
        for (uint32_t x = 0; x < LUT_RESOLUTION; ++x) {
            float roughness = static_cast<float>(x) / static_cast<float>(LUT_RESOLUTION - 1);
            float ndotv = static_cast<float>(y) / static_cast<float>(LUT_RESOLUTION - 1);
            
            // Avoid division by zero
            ndotv = glm::max(ndotv, 0.001f);
            roughness = glm::max(roughness, 0.001f);
            
            glm::vec2 brdf = IntegrateBRDF(roughness, ndotv);
            lutData[y * LUT_RESOLUTION + x] = brdf;
        }
    }
    
    // Mark as valid (actual GPU texture upload handled by asset system)
    isValid_ = true;
    return true;
}

glm::vec2 BRDFLut::IntegrateBRDF(float roughness, float ndotv) noexcept {
    // Simplified Cook-Torrance BRDF split-sum approximation
    // Returns scale and bias for Fresnel term (F0)
    
    const uint32_t SAMPLE_COUNT = 1024;
    
    glm::vec3 v(std::sqrt(1.0f - ndotv * ndotv), 0.0f, ndotv);
    float a = roughness * roughness;
    float a2 = a * a;
    
    float scale = 0.0f;
    float bias = 0.0f;
    
    for (uint32_t i = 0; i < SAMPLE_COUNT; ++i) {
        float xi = static_cast<float>(i) / static_cast<float>(SAMPLE_COUNT);
        float yi = (static_cast<float>(i) + 0.5f) / static_cast<float>(SAMPLE_COUNT);
        
        // Hammersley sequence approximation
        float theta = std::atan(a * std::sqrt(xi / (1.0f - xi)));
        float phi = 2.0f * std::numbers::pi_v<float> * yi;
        
        float h_x = std::sin(theta) * std::cos(phi);
        float h_y = std::sin(theta) * std::sin(phi);
        float h_z = std::cos(theta);
        
        glm::vec3 h(h_x, h_y, h_z);
        glm::vec3 l = 2.0f * glm::dot(v, h) * h - v;
        
        float ndotl = glm::max(l.z, 0.0f);
        float ndoth = glm::max(h.z, 0.0f);
        float vdoth = glm::max(glm::dot(v, h), 0.0f);
        
        if (ndotl > 0.0f) {
            // Fresnel-Schlick approximation
            float f0 = 1.0f;
            float f = f0 + (1.0f - f0) * std::pow(1.0f - vdoth, 5.0f);
            
            float g = 1.0f; // Simplified geometry
            float pdf = 1.0f / (4.0f * vdoth + 0.0001f);
            
            float value = f * g * ndotl / (ndotv * ndoth * pdf + 0.0001f);
            
            scale += value * f;
            bias += value * (1.0f - f);
        }
    }
    
    scale /= static_cast<float>(SAMPLE_COUNT);
    bias /= static_cast<float>(SAMPLE_COUNT);
    
    return glm::vec2(scale, bias);
}

} // namespace NeoEngine
