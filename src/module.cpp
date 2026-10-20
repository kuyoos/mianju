// Zygisk 模块：为 com.garena.game.df 早期注入 frida-gadget
//
// 原理：
// - Zygisk 的 postAppSpecialize 在进程 fork 后、应用代码执行前调用
// - 此时 dlopen frida-gadget，gadget 会在进程内启动 frida server（监听端口）
// - 外部用 frida 连接该端口，即可在 libUE4.so 加载前就建立控制
//
// gadget 配置：从模块目录读取，指定 listen 端口与脚本

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <android/log.h>

#include "zygisk.hpp"

#define LOG_TAG "DFZygisk"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace {

constexpr const char* kTargetPkg = "com.garena.game.df";

// Magisk 模块目录（gadget + config 放在这里）
constexpr const char* kModDir = "/data/adb/modules/dfzygisk";
constexpr const char* kGadget64 = "/data/adb/modules/dfzygisk/lib64/libgadget.so";
constexpr const char* kGadget32 = "/data/adb/modules/dfzygisk/lib/libgadget.so";

bool g_isTarget = false;
bool g_gadgetLoaded = false;

// 检查进程是否 64 位
bool is64() {
    return sizeof(void*) == 8;
}

// 加载 gadget
bool loadGadget() {
    if (g_gadgetLoaded) {
        LOGI("gadget already loaded");
        return true;
    }

    const char* path = is64() ? kGadget64 : kGadget32;
    struct stat st {};
    if (stat(path, &st) != 0) {
        LOGE("gadget not found: %s (%s)", path, strerror(errno));
        return false;
    }
    LOGI("loading gadget: %s (%ld bytes)", path, (long) st.st_size);

    // RTLD_NOW | RTLD_GLOBAL：立即解析符号并全局可见
    void* h = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (!h) {
        LOGE("dlopen gadget failed: %s", dlerror());
        return false;
    }
    LOGI("gadget loaded at %p", h);
    g_gadgetLoaded = true;
    return true;
}

}  // namespace

class DFModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override {
        LOGI("onLoad api=%p (ptr size %zu)", api, sizeof(void*));
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        if (!args || !args->nice_name) return;
        const char* name = env_->GetStringUTFChars(args->nice_name, nullptr);
        if (name) {
            g_isTarget = (strcmp(name, kTargetPkg) == 0);
            if (g_isTarget) LOGI(">>> target process detected: %s", name);
            env_->ReleaseStringUTFChars(args->nice_name, name);
        }
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs* args) override {
        if (!g_isTarget) return;
        LOGI(">>> postAppSpecialize: injecting gadget into %s", kTargetPkg);

        if (loadGadget()) {
            LOGI(">>> gadget injection SUCCESS");
        } else {
            LOGE(">>> gadget injection FAILED");
        }
    }

private:
    JNIEnv* env_ = nullptr;
};

REGISTER_ZYGISK_MODULE(DFModule)
