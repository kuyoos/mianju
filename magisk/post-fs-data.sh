#!/system/bin/sh
# Magisk post-fs-data
# 目标：把 gadget 放到 app 进程可读、可 mmap 的位置
#
# SELinux 现实：
#   /data/adb/modules/*     → app 进程完全无权
#   /data/local/tmp/*       → app 能 stat 但不能 mmap（execute 被拒）
#   /data/data/<pkg>/*      → app 自己的目录，完全可用  ✔

MODDIR=${0%/*}
PKG=com.garena.game.df
DATA=/data/data/$PKG

log -t DFZygisk "post-fs-data start"

mkdir -p /data/local/tmp/dflua

# 1) 复制到 app 私有目录（首选）
if [ -d "$DATA" ]; then
    mkdir -p "$DATA/files"
    if [ -f "$MODDIR/lib64/libgadget.so" ]; then
        cp -f "$MODDIR/lib64/libgadget.so" "$DATA/files/libgadget.so"
        chmod 755 "$DATA/files/libgadget.so"
        # 归属该 app（从 packages.list 取 uid）
        APP_UID=$(stat -c %u "$DATA" 2>/dev/null)
        if [ -n "$APP_UID" ]; then
            chown "$APP_UID:$APP_UID" "$DATA/files/libgadget.so" 2>/dev/null
        fi
        chcon u:object_r:app_data_file:s0 "$DATA/files/libgadget.so" 2>/dev/null
        log -t DFZygisk "installed gadget -> $DATA/files/libgadget.so (uid=$APP_UID)"
    fi
    if [ -f "$MODDIR/lib64/libgadget.config.so" ]; then
        cp -f "$MODDIR/lib64/libgadget.config.so" "$DATA/files/libgadget.config.so"
        chmod 755 "$DATA/files/libgadget.config.so"
        chcon u:object_r:app_data_file:s0 "$DATA/files/libgadget.config.so" 2>/dev/null
        log -t DFZygisk "installed config"
    fi
else
    log -t DFZygisk "WARN: $DATA not found (app not installed yet)"
fi

# 2) 同时放一份到 /data/local/tmp 作为备份
if [ -f "$MODDIR/lib64/libgadget.so" ]; then
    cp -f "$MODDIR/lib64/libgadget.so" /data/local/tmp/dflua/libgadget.so
    chmod 755 /data/local/tmp/dflua/libgadget.so
fi

ls -la "$DATA/files/" 2>/dev/null | grep gadget
log -t DFZygisk "post-fs-data done"
