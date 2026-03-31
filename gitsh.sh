#!/bin/bash

# Git 推送脚本模板
# 用法: ./gitsh.sh [提交信息]

# 设置提交信息（默认为 "update"）
COMMIT_MSG="${1:-update}"

echo "========================================"
echo "开始 Git 推送流程"
echo "提交信息: $COMMIT_MSG"
echo "========================================"

# 1. 添加所有更改
echo ">>> 添加更改到暂存区..."
git add .

# 2. 查看暂存区状态
echo ">>> 当前暂存区状态:"
git status

# 3. 提交更改
echo ">>> 提交更改..."
git commit -m "$COMMIT_MSG"

# 4. 查看提交后状态
echo ">>> 提交后状态:"
git status

# 5. 推送到远程 main 分支
echo ">>> 推送到远程 main 分支..."
git push origin main

echo "========================================"
echo "推送完成！"
echo "========================================"