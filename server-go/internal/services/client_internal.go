package services

import (
	"errors"

	"github.com/yuzujr/C3/internal/models"
)

func validateApiConfig(config models.ApiConfig) error {
	if config.Hostname == "" {
		return errors.New("hostname is required")
	}
	if config.Port <= 0 || config.Port > 65535 {
		return errors.New("port must be between 1 and 65535")
	}
	if config.IntervalSeconds <= 0 {
		return errors.New("interval_seconds must be greater than 0")
	}
	if config.RetryDelayMilliseconds <= 0 {
		return errors.New("retry_delay_ms must be greater than 0")
	}
	return nil
}
