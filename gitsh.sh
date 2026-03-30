#!/bin/bash

# 检查是否在 git 仓库中
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    echo "Error: Not a git repository"
    exit 1
fi

# 获取当前分支
branch=$(git branch --show-current)

git add .
git status

# 检查是否有变更要提交
if git diff --cached --quiet; then
    echo "No changes to commit"
    exit 0
fi

git commit -m "update"
git status
git push origin "$branch"