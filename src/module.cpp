// Zygisk 模块：为 com.garena.game.df 早期注入 frida-gadget
//
// 时机：postAppSpecialize —— 进程已 fork、应用代码尚未执行
// 动作：dlopen frida-gadget，让它在本进程内启动 frida server
// 效果：外部可在 libUE4.so 加载前连上并 hook
//
// 注意：SELinux 阻止 app 进程读 /data/adb/modules，
//       所以 gadget 从 /data/local/tmp/dflua/ 读取（post-fs-data.sh 负责复制并设上下文）

#include <cstring>
#include <cstdio>
#include <cerrno>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <android/log.h>

#include "zygisk.hpp"

#define LOG_TAG "DFZygisk"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

constexpr const char* kTargetPkg = "com.garena.game.df";

// 应用可访问的路径（post-fs-data.sh 从这里准备）
constexpr const char* kGadget64 = "/data/local/tmp/dflua/libgadget.so";
constexpr const char* kGadget32 = "/data/local/tmp/dflua/libgadget32.so";

// 备用：模块目录（部分 ROM 允许）
constexpr const char* kGadgetAlt = "/data/adb/modules/dfzygisk/lib64/libgadget.so";

bool g_isTarget = false;
bool g_loaded = false;

bool tryLoad(const char* path) {
    struct stat st {};
    if (stat(path, &st) != 0) {
        LOGE("stat %s: %s", path, strerror(errno));
        return false;
    }
    LOGI("loading %s (%ld bytes)", path, (long) st.st_size);
    void* h = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (h == nullptr) {
        LOGE("dlopen %s failed: %s", path, dlerror());
        return false;
    }
    LOGI(">>> gadget OK at %p", h);
    g_loaded = true;
    return true;
}

bool loadGadget() {
    if (g_loaded) return true;
    const char* primary = (sizeof(void*) == 8) ? kGadget64 : kGadget32;
    if (tryLoad(primary)) return true;
    if (tryLoad(kGadgetAlt)) return true;
    LOGE(">>> all gadget paths failed");
    return false;
}

}  // namespace

class DFModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override {
        env_ = env;
        LOGI("onLoad api=%p env=%p", api, env);
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        if (env_ == nullptr || args == nullptr || args->nice_name == nullptr) return;
        const char* name = env_->GetStringUTFChars(args->nice_name, nullptr);
        if (name != nullptr) {
            g_isTarget = (strcmp(name, kTargetPkg) == 0);
            if (g_isTarget) LOGI(">>> target: %s", name);
            env_->ReleaseStringUTFChars(args->nice_name, name);
        }
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs* args) override {
        if (!g_isTarget) return;
        LOGI(">>> postAppSpecialize, injecting gadget");
        loadGadget();
    }

private:
    JNIEnv* env_ = nullptr;
};

REGISTER_ZYGISK_MODULE(DFModule)
