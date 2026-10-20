#!/system/bin/sh
# Magisk service：为 gadget 配置监听端口
MODDIR=${0%/*}
GADGET_CFG="$MODDIR/lib64/libgadget.config.so"

# 若不存在则生成默认配置（listen 模式，端口 27043）
if [ ! -f "$GADGET_CFG" ]; then
    cat > "$GADGET_CFG" <<'EOF'
{
  "interaction": {
    "type": "listen",
    "address": "127.0.0.1",
    "port": 27043,
    "on_port_conflict": "fail",
    "on_load": "wait"
  }
}
EOF
    chmod 644 "$GADGET_CFG"
fi

log -t DFZygisk "service.sh: gadget config ready at $GADGET_CFG"
