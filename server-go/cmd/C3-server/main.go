package main

import (
	"github.com/gin-gonic/gin"
	"github.com/yuzujr/C3/config"
	"github.com/yuzujr/C3/pkg/log"
	"github.com/yuzujr/C3/routes"
)

func main() {
	log.Infof("Starting C3 server...")

	cfg := config.Get()
	gin.SetMode(gin.ReleaseMode)
	e := gin.New()

	// 中间件
	e.Use(gin.Recovery())

	// 注册路由
	clientGroup := e.Group(cfg.Server.BasePath + "client") // 例如 /c3
	routes.RegisterClientRoutes(clientGroup)

	// 启动
	addr := cfg.Server.Host + ":" + cfg.Server.Port
	log.Infof("Listening on %s", addr)
	if err := e.Run(addr); err != nil {
		log.Errorf("Failed to start server: %v", err)
	}
}
