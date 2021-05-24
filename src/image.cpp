#include "image.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

ImageUPtr Image::Load(const std::string& filepath,bool flipVertical) {
    auto image = ImageUPtr(new Image());
    if (!image->LoadWithStb(filepath,flipVertical))
        return nullptr;
    return std::move(image);
}


ImageUPtr Image::Create(int width, int height, int channelCount) {
    auto image = ImageUPtr(new Image());
    if (!image->Allocate(width, height, channelCount))
        return nullptr;
    return std::move(image);
}

bool Image::Allocate(int width, int height, int channelCount) {
    m_width = width;
    m_height = height;
    m_channelCount = channelCount;
    m_data = (uint8_t*)malloc(m_width * m_height * m_channelCount);
    return m_data ? true : false;
}


Image::~Image() {
    if (m_data) {
        stbi_image_free(m_data);
    }
}

bool Image::LoadWithStb(const std::string& filepath,bool flipVertical) {
    stbi_set_flip_vertically_on_load(flipVertical);
    m_data = stbi_load(filepath.c_str(), &m_width, &m_height, &m_channelCount, 0);
    if (!m_data) {
        SPDLOG_ERROR("failed to load image: {}", filepath);
        return false;
    }
    return true;
}

ImageUPtr Image::CreateSingleColorImage(int width, int height, const glm::vec4 &color){   
    glm::vec4 clamped = glm::clamp(color * 255.0f, 0.0f, 255.0f);
    uint8_t rgba[4] = {
        (uint8_t)clamped.r,
        (uint8_t)clamped.g,
        (uint8_t)clamped.b,
        (uint8_t)clamped.a,
    };
    auto image = Create(width, height, 4);
    for (int i = 0; i < width * height; i++){
        memcpy(image->m_data + 4 * i, rgba, 4);
    }
    return std::move(image);
}
