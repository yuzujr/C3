package config

import (
	"log"
	"os"
	"sync"

	"github.com/joho/godotenv"
)

var (
	cfg  *Config
	once sync.Once
)

func Get() *Config {
	once.Do(func() {
		load()
	})
	return cfg
}

func load() {
	err := godotenv.Load()
	if err != nil {
		log.Fatalf("Error loading .env file: %v", err)
	}

	cfg = &Config{
		Server: ServerConfig{
			BasePath: os.Getenv("BASE_PATH"),
			Host:     os.Getenv("HOST"),
			Port:     os.Getenv("PORT"),
		},
		Database: DatabaseConfig{
			Host:     os.Getenv("DB_HOST"),
			Port:     os.Getenv("DB_PORT"),
			Name:     os.Getenv("DB_NAME"),
			User:     os.Getenv("DB_USER"),
			Password: os.Getenv("DB_PASSWORD"),
		},
		Auth: AuthConfig{
			Enabled:            os.Getenv("AUTH_ENABLED") == "true",
			Username:           os.Getenv("AUTH_USERNAME"),
			Password:           os.Getenv("AUTH_PASSWORD"),
			SessionSecret:      os.Getenv("SESSION_SECRET"),
			SessionExpireHours: os.Getenv("SESSION_EXPIRE_HOURS"),
		},
		Upload: UploadConfig{
			Directory: os.Getenv("UPLOAD_DIR"),
		},
		Log: LogConfig{
			Directory: os.Getenv("LOG_DIR"),
			Level:    os.Getenv("LOG_LEVEL"),
		},
	}
}

type (
	Config struct {
		Server   ServerConfig
		Database DatabaseConfig
		Auth     AuthConfig
		Upload   UploadConfig
		Log      LogConfig
	}

	ServerConfig struct {
		BasePath string
		Host     string
		Port     string
	}

	DatabaseConfig struct {
		Host     string
		Port     string
		Name     string
		User     string
		Password string
	}

	AuthConfig struct {
		Enabled            bool
		Username           string
		Password           string
		SessionSecret      string
		SessionExpireHours string
	}

	UploadConfig struct {
		Directory string
	}

	LogConfig struct {
		Directory string
		Level string
	}
)
