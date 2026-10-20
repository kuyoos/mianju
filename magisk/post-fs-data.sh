#!/system/bin/sh
# Magisk post-fs-data
# 目标：把 gadget 复制到 app 进程可读的位置（绕开 SELinux 对 /data/adb 的限制）

MODDIR=${0%/*}
DEST=/data/local/tmp/dflua

mkdir -p "$DEST"
chmod 755 "$DEST"

# 复制 gadget（64 位）
if [ -f "$MODDIR/lib64/libgadget.so" ]; then
    cp -f "$MODDIR/lib64/libgadget.so" "$DEST/libgadget.so"
    chmod 755 "$DEST/libgadget.so"
    chown shell:shell "$DEST/libgadget.so" 2>/dev/null
    # 设置 SELinux 上下文为 app 可读
    chcon u:object_r:shell_data_file:s0 "$DEST/libgadget.so" 2>/dev/null || \
    chcon u:object_r:app_data_file:s0 "$DEST/libgadget.so" 2>/dev/null || true
    log -t DFZygisk "copied gadget to $DEST/libgadget.so"
fi

# 复制配置
if [ -f "$MODDIR/lib64/libgadget.config.so" ]; then
    cp -f "$MODDIR/lib64/libgadget.config.so" "$DEST/libgadget.config.so"
    chmod 755 "$DEST/libgadget.config.so"
    chcon u:object_r:shell_data_file:s0 "$DEST/libgadget.config.so" 2>/dev/null || true
    log -t DFZygisk "copied config"
fi

# 复制 zygisk 库到模块的 zygisk/ 目录（如未做）
mkdir -p "$MODDIR/zygisk"
if [ -f "$MODDIR/lib64/libzygisk.so" ] && [ ! -f "$MODDIR/zygisk/arm64-v8a.so" ]; then
    cp -f "$MODDIR/lib64/libzygisk.so" "$MODDIR/zygisk/arm64-v8a.so"
fi

ls -la "$DEST" 2>/dev/null
log -t DFZygisk "post-fs-data done"
