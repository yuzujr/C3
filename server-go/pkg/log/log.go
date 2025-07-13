package log

import (
	"fmt"
	"os"
	"path/filepath"
	"sync"
	"time"

	"github.com/yuzujr/C3/config"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var (
	logger *zap.Logger
	once   sync.Once
)

// 获取全局 logger（延迟初始化）
func GetLogger() *zap.Logger {
	once.Do(initLogger)
	return logger
}

// 对外封装的便捷日志方法
func Infof(format string, args ...any)  { GetLogger().Sugar().Infof(format, args...) }
func Errorf(format string, args ...any) { GetLogger().Sugar().Errorf(format, args...) }
func Debugf(format string, args ...any) { GetLogger().Sugar().Debugf(format, args...) }
func Warnf(format string, args ...any)  { GetLogger().Sugar().Warnf(format, args...) }

// 初始化 zap 日志系统
func initLogger() {
	cfg := config.Get()
	logDir := cfg.Log.Directory
	level := parseLevel(cfg.Log.Level)

	_ = os.MkdirAll(logDir, os.ModePerm)

	consoleEncoder := zapcore.NewConsoleEncoder(getEncoderConfig())
	fileEncoder := zapcore.NewJSONEncoder(getEncoderConfig())

	logFile := getHourlyLogFile(logDir)

	core := zapcore.NewTee(
		zapcore.NewCore(consoleEncoder, zapcore.AddSync(os.Stdout), level),
		zapcore.NewCore(fileEncoder, zapcore.AddSync(logFile), level),
	)

	logger = zap.New(core, zap.AddCaller(), zap.AddCallerSkip(1))
}

// 日志等级字符串转换
func parseLevel(lvl string) zapcore.Level {
	switch lvl {
	case "debug":
		return zapcore.DebugLevel
	case "info":
		return zapcore.InfoLevel
	case "warn":
		return zapcore.WarnLevel
	case "error":
		return zapcore.ErrorLevel
	default:
		return zapcore.InfoLevel
	}
}

// 每小时一个日志文件
func getHourlyLogFile(dir string) *os.File {
	now := time.Now().UTC().Add(8 * time.Hour) // 转北京时间
	filename := fmt.Sprintf("%d-%02d-%02d-%02d.log",
		now.Year(), now.Month(), now.Day(), now.Hour())

	path := filepath.Join(dir, filename)
	f, err := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		panic(fmt.Sprintf("cannot open log file: %v", err))
	}
	return f
}

// 自定义 encoder 配置
func getEncoderConfig() zapcore.EncoderConfig {
	return zapcore.EncoderConfig{
		TimeKey:        "T",
		LevelKey:       "L",
		CallerKey:      "C",
		MessageKey:     "M",
		EncodeLevel:    zapcore.CapitalColorLevelEncoder, // 彩色输出
		EncodeTime:     beijingTimeEncoder,
		EncodeCaller:   zapcore.ShortCallerEncoder,
		EncodeDuration: zapcore.StringDurationEncoder,
	}
}

// 使用北京时间作为日志时间
func beijingTimeEncoder(t time.Time, enc zapcore.PrimitiveArrayEncoder) {
	bt := t.UTC().Add(8 * time.Hour)
	enc.AppendString(bt.Format("2006-01-02 15:04:05"))
}
