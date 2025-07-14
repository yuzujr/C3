package routes

import (
	"net/http"

	"github.com/gin-gonic/gin"
	"github.com/yuzujr/C3/internal/logger"
)

// RegisterClientRoutes 注册客户端相关接口
func RegisterClientRoutes(r *gin.RouterGroup) {
	r.POST("/upload_screenshot", uploadScreenshot)
	r.POST("/upload_client_config", uploadClientConfig)
}

func uploadScreenshot(c *gin.Context) {
	logger.Infof("[UPLOAD] Upload screenshot endpoint hit")
	c.JSON(http.StatusCreated, gin.H{"message": "Screenshot uploaded"})
}

func uploadClientConfig(c *gin.Context) {
	logger.Infof("[CONFIG] Upload client config endpoint hit")
	c.JSON(http.StatusOK, gin.H{"message": "Client config received"})
}
