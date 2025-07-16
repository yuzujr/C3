package websocket

import (
	"github.com/gorilla/websocket"
	"github.com/yuzujr/C3/internal/logger"
)

type client struct {
	Conn *websocket.Conn
	Send chan []byte
	ID   string
}

func (c *client) readPump() {
	defer func() {
		HubInstance.Unregister <- c
		c.Conn.Close()
	}()
	for {
		_, message, err := c.Conn.ReadMessage()
		if err != nil {
			break
		}
		handleClientResponse(c.ID, message)
	}
}

func (c *client) writePump() {
	for msg := range c.Send {
		err := c.Conn.WriteMessage(websocket.TextMessage, msg)
		if err != nil {
			break
		}
	}
}

func handleClientResponse(clientID string, message []byte) {
	// 处理客户端响应逻辑
	// 此处应该转发给web websocket，根据requestId找到web的websocket连接
	logger.Infof("Received message from client %s: %s", clientID, message)
}
