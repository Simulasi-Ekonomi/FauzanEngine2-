#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>
#include <vector>

class AndroidFS {
public:
    static std::vector<char> ReadFile(AAssetManager* assetManager, const std::string& filename) {
        AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), AASSET_MODE_BUFFER);
        if (!asset) return {};

        size_t size = AAsset_getLength(asset);
        std::vector<char> buffer(size);
        AAsset_read(asset, buffer.data(), size);
        AAsset_close(asset);
        return buffer;
    }
};
