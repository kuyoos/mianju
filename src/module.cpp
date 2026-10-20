// Zygisk 模块：为 com.garena.game.df 早期注入 frida-gadget
//
// 时机：postAppSpecialize —— 进程已 fork、应用代码尚未执行
// 动作：dlopen frida-gadget，让它在本进程内启动 frida server
// 效果：外部可在 libUE4.so 加载前连上并 hook

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
constexpr const char* kGadget64 = "/data/adb/modules/dfzygisk/lib64/libgadget.so";
constexpr const char* kGadget32 = "/data/adb/modules/dfzygisk/lib/libgadget.so";

bool g_isTarget = false;
bool g_loaded = false;

bool loadGadget() {
    if (g_loaded) return true;
    const char* path = (sizeof(void*) == 8) ? kGadget64 : kGadget32;

    struct stat st {};
    if (stat(path, &st) != 0) {
        LOGE("gadget missing: %s (%s)", path, strerror(errno));
        return false;
    }
    LOGI("loading %s (%ld bytes)", path, (long) st.st_size);

    void* h = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (h == nullptr) {
        LOGE("dlopen failed: %s", dlerror());
        return false;
    }
    LOGI("gadget loaded at %p", h);
    g_loaded = true;
    return true;
}

}  // namespace

class DFModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override {
        env_ = env;
        LOGI("onLoad api=%p env=%p", api, env);
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        if (env_ == nullptr || args == nullptr || args->nice_name == nullptr) {
            return;
        }
        const char* name = env_->GetStringUTFChars(args->nice_name, nullptr);
        if (name != nullptr) {
            g_isTarget = (strcmp(name, kTargetPkg) == 0);
            if (g_isTarget) {
                LOGI(">>> target: %s", name);
            }
            env_->ReleaseStringUTFChars(args->nice_name, name);
        }
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs* args) override {
        if (!g_isTarget) return;
        LOGI(">>> postAppSpecialize, injecting gadget");
        if (loadGadget()) {
            LOGI(">>> gadget injection OK");
        } else {
            LOGE(">>> gadget injection FAILED");
        }
    }

private:
    JNIEnv* env_ = nullptr;
};

REGISTER_ZYGISK_MODULE(DFModule)
