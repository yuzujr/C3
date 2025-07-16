package websocket

import "github.com/yuzujr/C3/internal/logger"

type hub struct {
	Clients    map[string]*client
	Register   chan *client
	Unregister chan *client
}

var HubInstance = newHub()

func newHub() *hub {
	return &hub{
		Clients:    make(map[string]*client),
		Register:   make(chan *client),
		Unregister: make(chan *client),
	}
}

func (h *hub) Run() {
	logger.Infof("WebSocket hub started")
	for {
		select {
		case client := <-h.Register:
			h.Clients[client.ID] = client
			logger.Infof("Client %s connected", client.ID)
		case client := <-h.Unregister:
			delete(h.Clients, client.ID)
			close(client.Send)
			logger.Infof("Client %s disconnected", client.ID)
		}
	}
}
