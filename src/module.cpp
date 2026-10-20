// Zygisk 模块：为 com.garena.game.df 早期注入 frida-gadget
//
// 时机：postAppSpecialize —— 进程已 fork、应用代码尚未执行
// 动作：dlopen frida-gadget，让它在本进程内启动 frida server
//
// SELinux 要点：app 进程只能读自己的 data 目录和少数公共目录，
// 所以 gadget 由 post-fs-data.sh 复制到 /data/data/com.garena.game.df/ 下。

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

// app 私有目录优先（app 有完整权限），/data/local/tmp 作为备用
constexpr const char* kPaths[] = {
    "/data/data/com.garena.game.df/files/libgadget.so",
    "/data/data/com.garena.game.df/libgadget.so",
    "/data/user/0/com.garena.game.df/files/libgadget.so",
    "/data/local/tmp/dflua/libgadget.so",
};

bool g_isTarget = false;
bool g_loaded = false;

bool loadGadget() {
    if (g_loaded) return true;
    for (const char* path : kPaths) {
        struct stat st {};
        if (stat(path, &st) != 0) continue;
        LOGI("found gadget: %s (%ld bytes)", path, (long) st.st_size);

        // 读权限检查
        if (access(path, R_OK) != 0) {
            LOGE("no read access: %s (%s)", path, strerror(errno));
            continue;
        }
        void* h = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
        if (h == nullptr) {
            LOGE("dlopen %s failed: %s", path, dlerror());
            continue;
        }
        LOGI(">>> GADGET LOADED at %p from %s", h, path);
        g_loaded = true;
        return true;
    }
    LOGE(">>> no gadget could be loaded");
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
