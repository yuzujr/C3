# 使用官方 Node.js 运行时作为基础镜像
FROM node:18-alpine

# 设置工作目录
WORKDIR /usr/src/app

# 创建 uploads 目录并设置权限
RUN mkdir -p uploads logs && \
    addgroup -g 1001 nodegroup && \
    adduser -u 1001 -G nodegroup -s /bin/sh -D nodeuser && \
    chown -R nodeuser:nodegroup /usr/src/app

# 复制 package.json 和 package-lock.json (如果存在)
COPY package*.json ./

# 安装项目依赖
RUN npm install

# 复制项目文件到工作目录
COPY . .

# 确保上传目录和日志目录存在并设置正确权限
RUN chown -R nodeuser:nodegroup /usr/src/app && \
    chmod -R 755 /usr/src/app && \
    chmod -R 777 uploads logs

# 切换到非特权用户
USER nodeuser

# 暴露端口
EXPOSE 3000

# 健康检查
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD node -e "require('http').get('http://localhost:3000', (res) => { process.exit(res.statusCode === 200 ? 0 : 1) }).on('error', () => { process.exit(1) })"

# 启动应用
CMD ["npm", "start"]
